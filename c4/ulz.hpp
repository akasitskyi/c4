//MIT License
//
//Copyright(c) 2019 Alex Kasitskyi
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files(the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions :
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.

// Based on ULZ by Ilya Muravyov
// https://github.com/encode84/ulz


#pragma once

#include <cstdio>
#include <vector>
#include <string>
#include <cassert>
#include <algorithm>
#include <istream>
#include <ostream>

#include <fstream>
#include <iterator>

#include "exception.hpp"
#include "file_utils.hpp"

namespace c4 {
    namespace detail {
        inline uint16_t load16(const void* p) {
            return *reinterpret_cast<const uint16_t*>(p);
        }

        inline uint32_t load32(const void* p) {
            return *reinterpret_cast<const uint32_t*>(p);
        }

        inline void store16(void* p, uint16_t x) {
            *reinterpret_cast<uint16_t*>(p) = x;
        }

        inline void encode(uint8_t*& p, uint32_t x) {
            while (x >= 128) {
                x -= 128;
                *p++ = 128 + (x & 127);
                x >>= 7;
            }
            *p++ = x;
        }

        inline uint32_t decode(const uint8_t*& p) {
            uint32_t x = 0;
            for (int i = 0; i <= 21; i += 7) {
                const uint32_t c = *p++;
                x += c << i;
                if (c < 128)
                    break;
            }
            return x;
        }

        inline void store_uncompressed(const uint8_t* in, uint8_t*& op, int p, int anchor, int token) {
            const int run = p - anchor;
            if (run >= 7) {
                *op++ = (7 << 5) + token;
                encode(op, run - 7);
            } else
                *op++ = (run << 5) + token;

            memcpy(op, in + anchor, run);
            op += run;
        }

        inline void stream_copy64(uint8_t* dst, const uint8_t* src, int n) {
            for (int i = 0; i < n; i += 8)
                *(uint64_t*)(dst + i) = *(const uint64_t*)(src + i);
        }

        inline bool memequal32(const uint8_t* a, const uint8_t* b, int n) {
            assert(n >= 4);

            for (int i = 0; i + 4 < n; i += 4)
                if (*(const uint32_t*)(a + i) != *(const uint32_t*)(b + i))
                    return false;

            return *(const uint32_t*)(a + (n - 4)) == *(const uint32_t*)(b + (n - 4));
        }
    };

    class ultra_lz {
        static constexpr int WINDOW_BITS = 17;
        static constexpr int WINDOW_SIZE = 1 << WINDOW_BITS;
        static constexpr int WINDOW_MASK = WINDOW_SIZE - 1;

        static constexpr int MIN_MATCH = 4;

        static constexpr int HASH_BITS = 19;
        static constexpr int HASH_SIZE = 1 << HASH_BITS;

        inline uint32_t Hash32(const void* p) {
            return (detail::load32(p) * 0x9E3779B9) >> (32 - HASH_BITS);
        }

        std::vector<int> hash_table;
        std::vector<int> prev;

    public:
        static constexpr int EXCESS = 16;


        ultra_lz() : hash_table(HASH_SIZE), prev(WINDOW_SIZE) {}

        int compress_fast(const uint8_t* in, int in_len, uint8_t* out) {
            using namespace detail;

            std::fill(hash_table.begin(), hash_table.end(), -1);

            uint8_t* op = out;
            int anchor = 0;

            int p = 0;
            while (p < in_len) {
                int best_len = 0;
                int dist = 0;

                const int max_match = in_len - p;
                if (max_match >= MIN_MATCH) {
                    const int limit = std::max(p - WINDOW_SIZE, -1);

                    const uint32_t h = Hash32(&in[p]);
                    int s = hash_table[h];
                    hash_table[h] = p;

                    if (s > limit && load32(&in[s]) == load32(&in[p])) {
                        int len = MIN_MATCH;
                        while (len < max_match && in[s + len] == in[p + len])
                            ++len;

                        best_len = len;
                        dist = p - s;
                    }
                }

                if (best_len == MIN_MATCH && (p - anchor) >= (7 + 128))
                    best_len = 0;

                if (best_len >= MIN_MATCH) {
                    const int len = best_len - MIN_MATCH;
                    const int token = ((dist >> 12) & 16) + std::min(len, 15);

                    if (anchor != p) {
                        store_uncompressed(in, op, p, anchor, token);
                    } else
                        *op++ = token;

                    if (len >= 15)
                        encode(op, len - 15);

                    store16(op, dist);
                    op += 2;

                    anchor = p + best_len;
                    ++p;
                    hash_table[Hash32(&in[p])] = p++;
                    hash_table[Hash32(&in[p])] = p++;
                    hash_table[Hash32(&in[p])] = p++;

                    p = anchor;
                } else
                    ++p;
            }

            if (anchor != p) {
                store_uncompressed(in, op, p, anchor, 0);
            }

            return int(op - out);
        }

        int compress(const uint8_t* in, int in_len, uint8_t* out, int level) {
            if (level == 1)
                return compress_fast(in, in_len, out);

            using namespace detail;

            const int max_chain = (level < 9) ? 1 << level : 1 << 13;

            std::fill(hash_table.begin(), hash_table.end(), -1);

            uint8_t* op = out;
            int anchor = 0;

            int p = 0;
            while (p < in_len)
            {
                int best_len = 3;
                int dist = 0;

                const int max_match = in_len - p;
                if (max_match >= MIN_MATCH) {
                    const int limit = std::max(p - WINDOW_SIZE, -1);
                    int chain_len = max_chain;

                    int s = hash_table[Hash32(&in[p])];
                    while (s > limit) {
                        if (load32(&in[s + best_len - 3]) == load32(&in[p + best_len - 3]) && load32(&in[s]) == load32(&in[p])) {
                            int len = MIN_MATCH;

                            while (len < max_match && in[s + len] == in[p + len])
                                ++len;

                            if (len > best_len) {
                                best_len = len;
                                dist = p - s;

                                if (len == max_match)
                                    break;
                            }
                        }

                        if (--chain_len == 0)
                            break;

                        s = prev[s&WINDOW_MASK];
                    }
                }

                if (best_len == MIN_MATCH && (p - anchor) >= (7 + 128))
                    best_len = 0;

                if (level >= 5 && best_len >= MIN_MATCH && best_len < max_match && (p - anchor) != 6) {
                    const int x = p + 1;
                    const int target_len = best_len + 1;

                    const int limit = std::max(x - WINDOW_SIZE, -1);
                    int chain_len = max_chain;

                    const uint32_t suffix = load32(&in[x + best_len - 3]);

                    int s = hash_table[Hash32(&in[x])];
                    while (s > limit) {
                        if (load32(&in[s + best_len - 3]) == suffix && memequal32(in + s, in + x, best_len &~3)) {
                            best_len = 0;
                            break;
                        }

                        if (--chain_len == 0)
                            break;

                        s = prev[s&WINDOW_MASK];
                    }
                }

                if (best_len >= MIN_MATCH) {
                    const int len = best_len - MIN_MATCH;
                    const int token = ((dist >> 12) & 16) + std::min(len, 15);

                    if (anchor != p) {
                        store_uncompressed(in, op, p, anchor, token);
                    } else
                        *op++ = token;

                    if (len >= 15)
                        encode(op, len - 15);

                    store16(op, dist);
                    op += 2;

                    while (best_len-- != 0) {
                        const uint32_t h = Hash32(&in[p]);
                        prev[p&WINDOW_MASK] = hash_table[h];
                        hash_table[h] = p++;
                    }
                    anchor = p;
                } else {
                    const uint32_t h = Hash32(&in[p]);
                    prev[p&WINDOW_MASK] = hash_table[h];
                    hash_table[h] = p++;
                }
            }

            if (anchor != p) {
                store_uncompressed(in, op, p, anchor, 0);
            }

            return int(op - out);
        }

        static int decompress(const uint8_t* in, int in_len, uint8_t* out, int out_len) {
            using namespace detail;

            uint8_t* op = out;
            const uint8_t* ip = in;
            const uint8_t* ip_end = ip + in_len;
            const uint8_t* op_end = op + out_len;

            while (ip < ip_end) {
                const int token = *ip++;

                if (token >= 32) {
                    int run = token >> 5;
                    if (run == 7)
                        run += decode(ip);
                    if ((op_end - op) < run || (ip_end - ip) < run) // Overrun check
                        return -1;

                    memcpy(op, ip, run);
                    op += run;
                    ip += run;
                    if (ip >= ip_end)
                        break;
                }

                int len = (token & 15) + MIN_MATCH;
                if (len == (15 + MIN_MATCH))
                    len += decode(ip);
                if ((op_end - op) < len) // Overrun check
                    return -1;

                const int dist = ((token & 16) << 12) + load16(ip);
                ip += 2;
                uint8_t* cp = op - dist;
                if ((op - out) < dist) // Range check
                    return -1;

                if (dist >= 8) {
                    stream_copy64(op, cp, len);
                    op += len;
                } else {
                    while (len--)
                        *op++ = *cp++;
                }
            }

            return (ip == ip_end) ? int(op - out) : -1;
        }
    };

    namespace detail {
        constexpr int ULZ_MAGIC = 0x215A4C55; // "ULZ!"

        constexpr int BLOCK_SIZE = 1 << 24;

        class iulzbuf : public std::streambuf {
            std::istream* in;
            std::vector<char> in_buf;
            std::vector<char> out_buf;
        protected:
            iulzbuf(std::istream* in) : in(in), in_buf(BLOCK_SIZE + ultra_lz::EXCESS), out_buf(BLOCK_SIZE + ultra_lz::EXCESS) {
                uint32_t magic;
                in->read((char*)&magic, sizeof(magic));
                assert(magic == ULZ_MAGIC);

                underflow();
            }

            virtual std::streambuf::int_type underflow() override {
                if (in->eof()) {
                    return traits_type::eof();
                }

                uint32_t comp_len;
                in->read((char*)&comp_len, sizeof(comp_len));

                if (in->eof()) {
                    return traits_type::eof();
                }

                assert(comp_len <= in_buf.size());

                in->read(in_buf.data(), comp_len);
                assert(in->gcount() == comp_len);

                const int out_len = ultra_lz::decompress((const uint8_t*)in_buf.data(), comp_len, (uint8_t*)out_buf.data(), BLOCK_SIZE);

                setg(out_buf.data(), out_buf.data(), out_buf.data() + out_len);

                return traits_type::to_int_type(*gptr());
            }
        };

        class oulzbuf : public std::streambuf {
            std::ostream* out;
            const int level;
            std::vector<char> in_buf;
            std::vector<char> out_buf;
            ultra_lz ulz;
        protected:
            oulzbuf(std::ostream* out, int level) : out(out), level(level), in_buf(BLOCK_SIZE + ultra_lz::EXCESS), out_buf(BLOCK_SIZE + ultra_lz::EXCESS) {
                out->write((const char*)&ULZ_MAGIC, sizeof(ULZ_MAGIC));

                setp(in_buf.data(), in_buf.data(), in_buf.data() + BLOCK_SIZE);
            }

            void compress_buffer() {
                int num_bytes = (int)std::distance(pbase(), pptr());

                const int comp_len = ulz.compress((uint8_t*)in_buf.data(), num_bytes, (uint8_t*)out_buf.data(), level);

                out->write((const char*)&comp_len, sizeof(comp_len));
                out->write(out_buf.data(), comp_len);

                setp(in_buf.data(), in_buf.data(), in_buf.data() + BLOCK_SIZE);
            }

            virtual std::streambuf::int_type overflow(std::streambuf::int_type ch) override {
                if (ch != traits_type::eof()) {
                    compress_buffer();

                    *pptr() = traits_type::to_char_type(ch);
                    pbump(1);
                }

                return ch;
            }

            virtual int sync() override {
                compress_buffer();
                return 0;
            }

            virtual ~oulzbuf() override {
                compress_buffer();
            }
        };
    };

    class iulzstream : private detail::iulzbuf, public std::istream {
    public:
        iulzstream(std::istream* in)
            : iulzbuf(in)
            , std::istream(static_cast<std::streambuf*>(this))
        {}
    };

    class oulzstream : private detail::oulzbuf, public std::ostream {
    public:
        oulzstream(std::ostream* out, int level)
            : oulzbuf(out, level)
            , std::ostream(static_cast<std::streambuf*>(this))
        {}
    };

    void compress_file(std::string input_filepath, std::string output_filepath, int level) {
        file_ptr in(input_filepath.c_str(), "rb");
        file_ptr out(output_filepath.c_str(), "wb");

        ultra_lz ulz;

        std::vector<uint8_t> in_buf(detail::BLOCK_SIZE + ultra_lz::EXCESS);
        std::vector<uint8_t> out_buf(detail::BLOCK_SIZE + ultra_lz::EXCESS);

        fwrite(&detail::ULZ_MAGIC, 1, sizeof(detail::ULZ_MAGIC), out);

        int raw_len;
        while ((raw_len = (int)fread(in_buf.data(), 1, detail::BLOCK_SIZE, in)) > 0) {
            const int comp_len = ulz.compress(in_buf.data(), raw_len, out_buf.data(), level);

            fwrite(&comp_len, 1, sizeof(comp_len), out);
            fwrite(out_buf.data(), 1, comp_len, out);
        }
    }

    void copy_stream(std::istream& in, std::ostream& out) {
        c4::scoped_timer t("copy_stream");
        char buf[1024];
        while (in) {
            in.read(buf, sizeof(buf));
            out.write(buf, in.gcount());
        }
    }

    void compress_stream(std::string input_filepath, std::string output_filepath, int level) {
        std::ifstream fin(input_filepath, std::ifstream::binary);
        std::ofstream fout(output_filepath, std::ofstream::binary);

        oulzstream ulz_out(&fout, level);

        copy_stream(fin, ulz_out);
    }

    int decompress_file(std::string input_filepath, std::string output_filepath) {
        file_ptr in(input_filepath.c_str(), "rb");
        file_ptr out(output_filepath.c_str(), "wb");

        std::vector<uint8_t> in_buf(detail::BLOCK_SIZE + ultra_lz::EXCESS);
        std::vector<uint8_t> out_buf(detail::BLOCK_SIZE + ultra_lz::EXCESS);

        int magic;
        fread(&magic, 1, sizeof(magic), in);
        ASSERT_EQUAL(magic, detail::ULZ_MAGIC);

        int comp_len;
        while (fread(&comp_len, 1, sizeof(comp_len), in) > 0) {
            if (comp_len < 2 || comp_len >(detail::BLOCK_SIZE + ultra_lz::EXCESS) || fread(in_buf.data(), 1, comp_len, in) != comp_len)
                return -1;

            const int out_len = ultra_lz::decompress(in_buf.data(), comp_len, out_buf.data(), detail::BLOCK_SIZE);
            ASSERT_TRUE(out_len > 0);

            fwrite(out_buf.data(), 1, out_len, out);
        }

        return 0;
    }

    void decompress_stream(std::string input_filepath, std::string output_filepath) {
        std::ifstream fin(input_filepath, std::ifstream::binary);
        std::ofstream fout(output_filepath, std::ofstream::binary);

        iulzstream ulz_in(&fin);

        copy_stream(ulz_in, fout);
    }
};

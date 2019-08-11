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
#include <algorithm>

#include "exception.hpp"

namespace detail {
    inline uint16_t load16(void* p) {
        return *reinterpret_cast<const uint16_t*>(p);
    }

    inline uint32_t load32(void* p) {
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

    inline uint32_t decode(uint8_t*& p) {
        uint32_t x = 0;
        for (int i = 0; i <= 21; i += 7) {
            const uint32_t c = *p++;
            x += c << i;
            if (c < 128)
                break;
        }
        return x;
    }
};

class ULZ {
public:
    static const int EXCESS = 16;

    static const int WINDOW_BITS = 17; // Hard-coded
    static const int WINDOW_SIZE = 1 << WINDOW_BITS;
    static const int WINDOW_MASK = WINDOW_SIZE - 1;

    static const int MIN_MATCH = 4;

    static const int HASH_BITS = 19;
    static const int HASH_SIZE = 1 << HASH_BITS;
    static const int NIL = -1;

    inline uint32_t Hash32(void* p) {
        return (detail::load32(p) * 0x9E3779B9) >> (32 - HASH_BITS);
    }

    int HashTable[HASH_SIZE];
    int Prev[WINDOW_SIZE];

    // LZ77

    int CompressFast(uint8_t* in, int in_len, uint8_t* out) {
        using namespace detail;

        for (int i = 0; i < HASH_SIZE; ++i)
            HashTable[i] = NIL;

        uint8_t* op = out;
        int anchor = 0;

        int p = 0;
        while (p < in_len) {
            int best_len = 0;
            int dist = 0;

            const int max_match = in_len - p;
            if (max_match >= MIN_MATCH) {
                const int limit = std::max(p - WINDOW_SIZE, NIL);

                const uint32_t h = Hash32(&in[p]);
                int s = HashTable[h];
                HashTable[h] = p;

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
                    const int run = p - anchor;
                    if (run >= 7) {
                        *op++ = (7 << 5) + token;
                        encode(op, run - 7);
                    } else
                        *op++ = (run << 5) + token;

                    memcpy(op, &in[anchor], run);
                    op += run;
                } else
                    *op++ = token;

                if (len >= 15)
                    encode(op, len - 15);

                store16(op, dist);
                op += 2;

                anchor = p + best_len;
                ++p;
                HashTable[Hash32(&in[p])] = p++;
                HashTable[Hash32(&in[p])] = p++;
                HashTable[Hash32(&in[p])] = p++;
                p = anchor;
            } else
                ++p;
        }

        if (anchor != p) {
            const int run = p - anchor;
            if (run >= 7)
            {
                *op++ = 7 << 5;
                encode(op, run - 7);
            } else
                *op++ = run << 5;

            memcpy(op, &in[anchor], run);
            op += run;
        }

        return op - out;
    }

    int Compress(uint8_t* in, int in_len, uint8_t* out, int level) {
        if (level == 1)
            return CompressFast(in, in_len, out);

        using namespace detail;

        if (level < 1 || level>9)
            return -1;
        const int max_chain = (level < 9) ? 1 << level : 1 << 13;

        for (int i = 0; i < HASH_SIZE; ++i)
            HashTable[i] = NIL;

        uint8_t* op = out;
        int anchor = 0;

        int p = 0;
        while (p < in_len)
        {
            int best_len = 0;
            int dist = 0;

            const int max_match = in_len - p;
            if (max_match >= MIN_MATCH)
            {
                const int limit = std::max(p - WINDOW_SIZE, NIL);
                int chain_len = max_chain;

                int s = HashTable[Hash32(&in[p])];
                while (s > limit)
                {
                    if (in[s + best_len] == in[p + best_len] && load32(&in[s]) == load32(&in[p]))
                    {
                        int len = MIN_MATCH;
                        while (len < max_match && in[s + len] == in[p + len])
                            ++len;

                        if (len > best_len)
                        {
                            best_len = len;
                            dist = p - s;

                            if (len == max_match)
                                break;
                        }
                    }

                    if (--chain_len == 0)
                        break;

                    s = Prev[s&WINDOW_MASK];
                }
            }

            if (best_len == MIN_MATCH && (p - anchor) >= (7 + 128))
                best_len = 0;

            if (level >= 5 && best_len >= MIN_MATCH && best_len < max_match && (p - anchor) != 6) {
                const int x = p + 1;
                const int target_len = best_len + 1;

                const int limit = std::max(x - WINDOW_SIZE, NIL);
                int chain_len = max_chain;

                int s = HashTable[Hash32(&in[x])];
                while (s > limit) {
                    if (in[s + best_len] == in[x + best_len] && load32(&in[s]) == load32(&in[x])) {
                        int len = MIN_MATCH;
                        while (len < target_len && in[s + len] == in[x + len])
                            ++len;

                        if (len == target_len) {
                            best_len = 0;
                            break;
                        }
                    }

                    if (--chain_len == 0)
                        break;

                    s = Prev[s&WINDOW_MASK];
                }
            }

            if (best_len >= MIN_MATCH) {
                const int len = best_len - MIN_MATCH;
                const int token = ((dist >> 12) & 16) + std::min(len, 15);

                if (anchor != p) {
                    const int run = p - anchor;
                    if (run >= 7) {
                        *op++ = (7 << 5) + token;
                        encode(op, run - 7);
                    } else
                        *op++ = (run << 5) + token;

                    memcpy(op, &in[anchor], run);
                    op += run;
                } else
                    *op++ = token;

                if (len >= 15)
                    encode(op, len - 15);

                store16(op, dist);
                op += 2;

                while (best_len-- != 0) {
                    const uint32_t h = Hash32(&in[p]);
                    Prev[p&WINDOW_MASK] = HashTable[h];
                    HashTable[h] = p++;
                }
                anchor = p;
            } else {
                const uint32_t h = Hash32(&in[p]);
                Prev[p&WINDOW_MASK] = HashTable[h];
                HashTable[h] = p++;
            }
        }

        if (anchor != p) {
            const int run = p - anchor;
            if (run >= 7) {
                *op++ = 7 << 5;
                encode(op, run - 7);
            } else
                *op++ = run << 5;

            memcpy(op, &in[anchor], run);
            op += run;
        }

        return op - out;
    }

    static int Decompress(uint8_t* in, int in_len, uint8_t* out, int out_len) {
        using namespace detail;

        uint8_t* op = out;
        uint8_t* ip = in;
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
                memcpy(op, cp, len);
                op += len;
            } else {
                *op++ = *cp++;
                *op++ = *cp++;
                *op++ = *cp++;
                *op++ = *cp++;
                while (len-- != 4)
                    *op++ = *cp++;
            }
        }

        return (ip == ip_end) ? op - out : -1;
    }
};

constexpr int ULZ_MAGIC = 0x215A4C55; // "ULZ!"

constexpr int BLOCK_SIZE = 1 << 24;


void compress(FILE* in, FILE* out, int level) {
    ULZ* ulz = new ULZ;

    std::vector<uint8_t> in_buf(BLOCK_SIZE + ULZ::EXCESS);
    std::vector<uint8_t> out_buf(BLOCK_SIZE + ULZ::EXCESS);

    int raw_len;
    while ((raw_len = fread(in_buf.data(), 1, BLOCK_SIZE, in)) > 0)
    {
        const int comp_len = ulz->Compress(in_buf.data(), raw_len, out_buf.data(), level);

        fwrite(&comp_len, 1, sizeof(comp_len), out);
        fwrite(out_buf.data(), 1, comp_len, out);

        fprintf(stderr, "%lld -> %lld\n", _ftelli64(in), _ftelli64(out));

        std::cerr << _ftelli64(out) * 100 / _ftelli64(in) << "%" << std::endl;
    }

    delete ulz;
}

int decompress(FILE* in, FILE* out)
{
    ULZ* ulz = new ULZ;

    std::vector<uint8_t> in_buf(BLOCK_SIZE + ULZ::EXCESS);
    std::vector<uint8_t> out_buf(BLOCK_SIZE + ULZ::EXCESS);

    int comp_len;
    while (fread(&comp_len, 1, sizeof(comp_len), in) > 0)
    {
        if (comp_len<2 || comp_len>(BLOCK_SIZE + ULZ::EXCESS) || fread(in_buf.data(), 1, comp_len, in) != comp_len)
            return -1;

        const int out_len = ulz->Decompress(in_buf.data(), comp_len, out_buf.data(), BLOCK_SIZE);
        if (out_len < 0)
            return -1;

        if (fwrite(out_buf.data(), 1, out_len, out) != out_len) {
            THROW_EXCEPTION("Fwrite() failed");
        }

        fprintf(stderr, "%lld -> %lld\r", _ftelli64(in), _ftelli64(out));
    }

    delete ulz;

    return 0;
}


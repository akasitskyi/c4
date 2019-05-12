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

#pragma once

// DOCUMENTATION
//
// Based on https://github.com/nothings/stb/blob/master/stb_image.h
//
// Limitations:
//    - no 12-bit-per-channel JPEG
//    - no JPEGs with arithmetic coding

#include <istream>
#include <fstream>
#include <vector>

#include "byte_stream.hpp"
#include "exception.hpp"
#include "matrix.hpp"
#include "math.hpp"

#include <cstdint>

#include <climits>
#include <cassert>

namespace c4 {
    namespace detail {
        inline uint32_t lrot(uint32_t x, int y) {
#ifdef _MSC_VER
            return _lrotl(x, y);
#else
            return (x << y) | (x >> (32 - y));
#endif
        }

        inline uint8_t jpeg_compute_y(int r, int g, int b) {
            return (uint8_t)(((r * 77) + (g * 150) + (29 * b)) >> 8);
        }

        inline uint8_t jpeg_div4(int x) {
            return uint8_t(x >> 2);
        }

        inline uint8_t jpeg_div16(int x) {
            return uint8_t(x >> 4);
        }

        static constexpr int FAST_BITS = 9;

        struct huffman {
            uint8_t  fast[1 << FAST_BITS];
            uint16_t code[256];
            uint8_t  values[256];
            uint8_t  size[257];
            unsigned int maxcode[18];
            int    delta[17];   // old 'firstsymbol' - old 'firstcode'


            void build_huffman(int *count) {
                unsigned int code;
                // build size list for each symbol (from JPEG spec)
                int k = 0;
                for (int i = 0; i < 16; ++i)
                    for (int j = 0; j < count[i]; ++j)
                        size[k++] = (uint8_t)(i + 1);
                size[k] = 0;

                // compute actual symbols (from jpeg spec)
                code = 0;
                k = 0;
                int j = 1;
                for (; j <= 16; ++j) {
                    // compute delta to add to code to compute symbol id
                    delta[j] = k - code;
                    if (size[k] == j) {
                        while (size[k] == j)
                            this->code[k++] = (uint16_t)(code++);
                        if (code - 1 >= (1u << j)) THROW_EXCEPTION("Corrupt JPEG: bad code lengths");
                    }
                    // compute largest code + 1 for this size, preshifted as needed later
                    maxcode[j] = code << (16 - j);
                    code <<= 1;
                }
                maxcode[j] = 0xffffffff;

                // build non-spec acceleration table; 255 is flag for not-accelerated
                memset(fast, 255, 1 << FAST_BITS);
                for (int i = 0; i < k; ++i) {
                    int s = size[i];
                    if (s <= FAST_BITS) {
                        int c = this->code[i] << (FAST_BITS - s);
                        int m = 1 << (FAST_BITS - s);
                        for (j = 0; j < m; ++j) {
                            fast[c + j] = (uint8_t)i;
                        }
                    }
                }
            }

            // build a table that decodes both magnitude and value of small ACs in
            // one go.
            void build_fast_ac(int16_t *fast_ac) {
                for (int i = 0; i < (1 << FAST_BITS); ++i) {
                    uint8_t fast = this->fast[i];
                    fast_ac[i] = 0;
                    if (fast < 255) {
                        int rs = values[fast];
                        int run = (rs >> 4) & 15;
                        int magbits = rs & 15;
                        int len = size[fast];

                        if (magbits && len + magbits <= FAST_BITS) {
                            // magnitude code followed by receive_extend code
                            int k = ((i << len) & ((1 << FAST_BITS) - 1)) >> (FAST_BITS - magbits);
                            int m = 1 << (magbits - 1);
                            if (k < m) k += (~0U << magbits) + 1;
                            // if the result is small enough, we can fit it in fast_ac table
                            if (k >= -128 && k <= 127)
                                fast_ac[i] = (int16_t)((k * 256) + (run * 16) + (len + magbits));
                        }
                    }
                }
            }
        };

        // (1 << n) - 1
        static constexpr uint32_t bmask[17] = { 0,1,3,7,15,31,63,127,255,511,1023,2047,4095,8191,16383,32767,65535 };
        // bias[n] = (-1<<n) + 1
        static constexpr int jbias[16] = { 0,-1,-3,-7,-15,-31,-63,-127,-255,-511,-1023,-2047,-4095,-8191,-16383,-32767 };


        struct jpeg_decoder {
            struct {
                uint32_t img_x, img_y;
                int img_n, img_out_n;
            }os;

            huffman huff_dc[4];
            huffman huff_ac[4];
            uint16_t dequant[4][64];
            int16_t fast_ac[4][1 << FAST_BITS];

            // sizes for components, interleaved MCUs
            int img_h_max, img_v_max;
            int img_mcu_x, img_mcu_y;
            int img_mcu_w, img_mcu_h;

            // definition of jpeg image component
            struct {
                int id;
                int h, v;
                int tq;
                int hd, ha;
                int dc_pred;

                int x, y, w2, h2;
                std::vector<uint8_t> data;
                std::vector<uint8_t> linebuf;
                std::vector<short>   coeff;   // progressive only
                int      coeff_w, coeff_h; // number of 8x8 coefficient blocks
            } img_comp[4];

            uint32_t   code_buffer; // jpeg entropy-coded buffer
            int            code_bits;   // number of valid bits
            unsigned char  marker;      // marker seen while filling entropy buffer
            int            nomore;      // flag if we saw a marker so must stop

            int            progressive;
            int            spec_start;
            int            spec_end;
            int            succ_high;
            int            succ_low;
            int            eob_run;
            int            jfif;
            int            app14_color_transform; // Adobe APP14 tag
            int            rgb;

            int scan_n, order[4];
            int restart_interval, todo;

            template<class T>
            void grow_buffer_unsafe(c4::byte_input_stream<T>& in) {
                do {
                    unsigned int b = nomore ? 0 : in.get8();
                    if (b == 0xff) {
                        int c = in.get8();
                        while (c == 0xff) c = in.get8(); // consume fill bytes
                        if (c != 0) {
                            marker = (unsigned char)c;
                            nomore = 1;
                            return;
                        }
                    }
                    code_buffer |= b << (24 - code_bits);
                    code_bits += 8;
                } while (code_bits <= 24);
            }

            // combined JPEG 'receive' and JPEG 'extend', since baseline
            // always extends everything it receives.
            template<class T>
            inline int extend_receive(c4::byte_input_stream<T>& in, int n) {
                if (code_bits < n) grow_buffer_unsafe(in);

                int sgn = (int32_t)code_buffer >> 31; // sign bit is always in MSB
                unsigned int k = lrot(code_buffer, n);
                assert(n >= 0 && n < (int)(sizeof(bmask) / sizeof(*bmask)));
                code_buffer = k & ~bmask[n];
                k &= bmask[n];
                code_bits -= n;
                return k + (jbias[n] & ~sgn);
            }

            // decode a jpeg huffman value from the bitstream
            template<class T>
            inline int jpeg_huff_decode(c4::byte_input_stream<T>& in, const huffman& h) {
                if (code_bits < 16) grow_buffer_unsafe(in);

                // look at the top FAST_BITS and determine what symbol ID it is,
                // if the code is <= FAST_BITS
                int c = (code_buffer >> (32 - FAST_BITS)) & ((1 << FAST_BITS) - 1);
                int k = h.fast[c];
                if (k < 255) {
                    int s = h.size[k];
                    if (s > code_bits)
                        return -1;
                    code_buffer <<= s;
                    code_bits -= s;
                    return h.values[k];
                }

                // naive test is to shift the code_buffer down so k bits are
                // valid, then test against maxcode. To speed this up, we've
                // preshifted maxcode left so that it has (16-k) 0s at the
                // end; in other words, regardless of the number of bits, it
                // wants to be compared against something shifted to have 16;
                // that way we don't need to shift inside the loop.
                unsigned int temp = code_buffer >> 16;
                for (k = FAST_BITS + 1; ; ++k)
                    if (temp < h.maxcode[k])
                        break;
                if (k == 17) {
                    // error! code not found
                    code_bits -= 16;
                    return -1;
                }

                if (k > code_bits)
                    return -1;

                // convert the huffman code to the symbol id
                c = ((code_buffer >> (32 - k)) & bmask[k]) + h.delta[k];
                assert((((code_buffer) >> (32 - h.size[c])) & bmask[h.size[c]]) == h.code[c]);

                // convert the id to a symbol
                code_bits -= k;
                code_buffer <<= k;
                return h.values[c];
            }
        };

        // get some unsigned bits
        template<class T>
        inline static int jpeg_get_bits(c4::byte_input_stream<T>& in, jpeg_decoder& j, int n) {
            if (j.code_bits < n) j.grow_buffer_unsafe(in);
            unsigned int k = lrot(j.code_buffer, n);
            j.code_buffer = k & ~bmask[n];
            k &= bmask[n];
            j.code_bits -= n;
            return k;
        }

        template<class T>
        inline static int jpeg_get_bit(c4::byte_input_stream<T>& in, jpeg_decoder& j) {
            if (j.code_bits < 1) j.grow_buffer_unsafe(in);
            unsigned int k = j.code_buffer;
            j.code_buffer <<= 1;
            --j.code_bits;
            return k & 0x80000000;
        }

        // given a value that's at position X in the zigzag stream,
        // where does it appear in the 8x8 matrix coded as row-major?
        static constexpr uint8_t jpeg_dezigzag[64 + 15] = {
            0,  1,  8, 16,  9,  2,  3, 10,
           17, 24, 32, 25, 18, 11,  4,  5,
           12, 19, 26, 33, 40, 48, 41, 34,
           27, 20, 13,  6,  7, 14, 21, 28,
           35, 42, 49, 56, 57, 50, 43, 36,
           29, 22, 15, 23, 30, 37, 44, 51,
           58, 59, 52, 45, 38, 31, 39, 46,
           53, 60, 61, 54, 47, 55, 62, 63,
           // let corrupt input sample past end
           63, 63, 63, 63, 63, 63, 63, 63,
           63, 63, 63, 63, 63, 63, 63
        };

        // decode one 64-entry block--
        template<class T>
        static void jpeg_decode_block(c4::byte_input_stream<T>& in, jpeg_decoder& j, short data[64], huffman& hdc, huffman& hac, int16_t *fac, int b, uint16_t *dequant)
        {
            int diff, dc, k;
            int t;

            if (j.code_bits < 16) j.grow_buffer_unsafe(in);
            t = j.jpeg_huff_decode(in, hdc);
            if (t < 0) THROW_EXCEPTION("Corrupt JPEG: bad huffman code");

            // 0 all the ac values now so we can do it 32-bits at a time
            memset(data, 0, 64 * sizeof(data[0]));

            diff = t ? j.extend_receive(in, t) : 0;
            dc = j.img_comp[b].dc_pred + diff;
            j.img_comp[b].dc_pred = dc;
            data[0] = (short)(dc * dequant[0]);

            // decode AC components, see JPEG spec
            k = 1;
            do {
                unsigned int zig;
                int c, r, s;
                if (j.code_bits < 16) j.grow_buffer_unsafe(in);
                c = (j.code_buffer >> (32 - FAST_BITS)) & ((1 << FAST_BITS) - 1);
                r = fac[c];
                if (r) { // fast-AC path
                    k += (r >> 4) & 15; // run
                    s = r & 15; // combined length
                    j.code_buffer <<= s;
                    j.code_bits -= s;
                    // decode into unzigzag'd location
                    zig = jpeg_dezigzag[k++];
                    data[zig] = (short)((r >> 8) * dequant[zig]);
                }
                else {
                    int rs = j.jpeg_huff_decode(in, hac);
                    if (rs < 0) THROW_EXCEPTION("Corrupt JPEG: bad huffman code");
                    s = rs & 15;
                    r = rs >> 4;
                    if (s == 0) {
                        if (rs != 0xf0) break; // end block
                        k += 16;
                    }
                    else {
                        k += r;
                        // decode into unzigzag'd location
                        zig = jpeg_dezigzag[k++];
                        data[zig] = (short)(j.extend_receive(in, s) * dequant[zig]);
                    }
                }
            } while (k < 64);
        }

        template<class T>
        static void jpeg_decode_block_prog_dc(c4::byte_input_stream<T>& in, jpeg_decoder& j, short data[64], huffman& hdc, int b) {
            int diff, dc;
            int t;
            if (j.spec_end != 0) THROW_EXCEPTION("Corrupt JPEG: can't merge dc and ac");

            if (j.code_bits < 16) j.grow_buffer_unsafe(in);

            if (j.succ_high == 0) {
                // first scan for DC coefficient, must be first
                memset(data, 0, 64 * sizeof(data[0])); // 0 all the ac values now
                t = j.jpeg_huff_decode(in, hdc);
                diff = t ? j.extend_receive(in, t) : 0;

                dc = j.img_comp[b].dc_pred + diff;
                j.img_comp[b].dc_pred = dc;
                data[0] = (short)(dc << j.succ_low);
            }
            else {
                // refinement scan for DC coefficient
                if (jpeg_get_bit(in, j))
                    data[0] += (short)(1 << j.succ_low);
            }
        }

        // @OPTIMIZE: store non-zigzagged during the decode passes,
        // and only de-zigzag when dequantizing
        template<class T>
        static void jpeg_decode_block_prog_ac(c4::byte_input_stream<T>& in, jpeg_decoder& j, short data[64], huffman& hac, int16_t *fac) {
            int k;
            if (j.spec_start == 0) THROW_EXCEPTION("Corrupt JPEG: can't merge dc and ac");

            if (j.succ_high == 0) {
                int shift = j.succ_low;

                if (j.eob_run) {
                    --j.eob_run;
                    return;
                }

                k = j.spec_start;
                do {
                    unsigned int zig;
                    int c, r, s;
                    if (j.code_bits < 16) j.grow_buffer_unsafe(in);
                    c = (j.code_buffer >> (32 - FAST_BITS)) & ((1 << FAST_BITS) - 1);
                    r = fac[c];
                    if (r) { // fast-AC path
                        k += (r >> 4) & 15; // run
                        s = r & 15; // combined length
                        j.code_buffer <<= s;
                        j.code_bits -= s;
                        zig = jpeg_dezigzag[k++];
                        data[zig] = (short)((r >> 8) << shift);
                    }
                    else {
                        int rs = j.jpeg_huff_decode(in, hac);
                        if (rs < 0) THROW_EXCEPTION("Corrupt JPEG: bad huffman code");
                        s = rs & 15;
                        r = rs >> 4;
                        if (s == 0) {
                            if (r < 15) {
                                j.eob_run = (1 << r);
                                if (r)
                                    j.eob_run += jpeg_get_bits(in, j, r);
                                --j.eob_run;
                                break;
                            }
                            k += 16;
                        }
                        else {
                            k += r;
                            zig = jpeg_dezigzag[k++];
                            data[zig] = (short)(j.extend_receive(in, s) << shift);
                        }
                    }
                } while (k <= j.spec_end);
            }
            else {
                // refinement scan for these AC coefficients

                short bit = (short)(1 << j.succ_low);

                if (j.eob_run) {
                    --j.eob_run;
                    for (k = j.spec_start; k <= j.spec_end; ++k) {
                        short *p = &data[jpeg_dezigzag[k]];
                        if (*p != 0)
                            if (jpeg_get_bit(in, j))
                                if ((*p & bit) == 0) {
                                    if (*p > 0)
                                        *p += bit;
                                    else
                                        *p -= bit;
                                }
                    }
                }
                else {
                    k = j.spec_start;
                    do {
                        int r, s;
                        int rs = j.jpeg_huff_decode(in, hac); // @OPTIMIZE see if we can use the fast path here, advance-by-r is so slow, eh
                        if (rs < 0) THROW_EXCEPTION("Corrupt JPEG: bad huffman code");
                        s = rs & 15;
                        r = rs >> 4;
                        if (s == 0) {
                            if (r < 15) {
                                j.eob_run = (1 << r) - 1;
                                if (r)
                                    j.eob_run += jpeg_get_bits(in, j, r);
                                r = 64; // force end of block
                            }
                            else {
                                // r=15 s=0 should write 16 0s, so we just do
                                // a run of 15 0s and then write s (which is 0),
                                // so we don't have to do anything special here
                            }
                        }
                        else {
                            if (s != 1) THROW_EXCEPTION("Corrupt JPEG: bad huffman code");
                            // sign bit
                            if (jpeg_get_bit(in, j))
                                s = bit;
                            else
                                s = -bit;
                        }

                        // advance by r
                        while (k <= j.spec_end) {
                            short *p = &data[jpeg_dezigzag[k++]];
                            if (*p != 0) {
                                if (jpeg_get_bit(in, j))
                                    if ((*p & bit) == 0) {
                                        if (*p > 0)
                                            *p += bit;
                                        else
                                            *p -= bit;
                                    }
                            }
                            else {
                                if (r == 0) {
                                    *p = (short)s;
                                    break;
                                }
                                --r;
                            }
                        }
                    } while (k <= j.spec_end);
                }
            }
        }

        inline int f2f(float x) {
            return int(x * 4096 + 0.5);
        }

        inline int fsh(int x) {
            return x * 4096;
        }

        // derived from jidctint -- DCT_ISLOW
        struct STBI__IDCT_1D {
            int t0, t1, t2, t3, x0, x1, x2, x3;

            STBI__IDCT_1D(int s0, int s1, int s2, int s3, int s4, int s5, int s6, int s7) {
                int p2 = s2;
                int p3 = s6;
                int p1 = (p2 + p3) * f2f(0.5411961f);
                t2 = p1 + p3 * f2f(-1.847759065f);
                t3 = p1 + p2 * f2f(0.765366865f);
                p2 = s0;
                p3 = s4;
                t0 = fsh(p2 + p3);
                t1 = fsh(p2 - p3);
                x0 = t0 + t3;
                x3 = t0 - t3;
                x1 = t1 + t2;
                x2 = t1 - t2;
                t0 = s7;
                t1 = s5;
                t2 = s3;
                t3 = s1;
                p3 = t0 + t2;
                int p4 = t1 + t3;
                p1 = t0 + t3;
                p2 = t1 + t2;
                int p5 = (p3 + p4)*f2f(1.175875602f);
                t0 = t0 * f2f(0.298631336f);
                t1 = t1 * f2f(2.053119869f);
                t2 = t2 * f2f(3.072711026f);
                t3 = t3 * f2f(1.501321110f);
                p1 = p5 + p1 * f2f(-0.899976223f);
                p2 = p5 + p2 * f2f(-2.562915447f);
                p3 = p3 * f2f(-1.961570560f);
                p4 = p4 * f2f(-0.390180644f);
                t3 += p1 + p4;
                t2 += p2 + p3;
                t1 += p2 + p4;
                t0 += p1 + p3;
            }
        };

        static void idct_block(uint8_t *out, int out_stride, short data[64]) {
            int i, val[64], *v = val;
            uint8_t *o;
            short *d = data;

            // columns
            for (i = 0; i < 8; ++i, ++d, ++v) {
                // if all zeroes, shortcut -- this avoids dequantizing 0s and IDCTing
                if (d[8] == 0 && d[16] == 0 && d[24] == 0 && d[32] == 0
                    && d[40] == 0 && d[48] == 0 && d[56] == 0) {
                    //    no shortcut                 0     seconds
                    //    (1|2|3|4|5|6|7)==0          0     seconds
                    //    all separate               -0.047 seconds
                    //    1 && 2|3 && 4|5 && 6|7:    -0.047 seconds
                    int dcterm = d[0] * 4;
                    v[0] = v[8] = v[16] = v[24] = v[32] = v[40] = v[48] = v[56] = dcterm;
                }
                else {
                    STBI__IDCT_1D idct(d[0], d[8], d[16], d[24], d[32], d[40], d[48], d[56]);
                    // constants scaled things up by 1<<12; let's bring them back
                    // down, but keep 2 extra bits of precision
                    idct.x0 += 512; idct.x1 += 512; idct.x2 += 512; idct.x3 += 512;
                    v[0] = (idct.x0 + idct.t3) >> 10;
                    v[56] = (idct.x0 - idct.t3) >> 10;
                    v[8] = (idct.x1 + idct.t2) >> 10;
                    v[48] = (idct.x1 - idct.t2) >> 10;
                    v[16] = (idct.x2 + idct.t1) >> 10;
                    v[40] = (idct.x2 - idct.t1) >> 10;
                    v[24] = (idct.x3 + idct.t0) >> 10;
                    v[32] = (idct.x3 - idct.t0) >> 10;
                }
            }

            for (i = 0, v = val, o = out; i < 8; ++i, v += 8, o += out_stride) {
                // no fast case since the first 1D IDCT spread components out
                STBI__IDCT_1D idct(v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]);
                // constants scaled things up by 1<<12, plus we had 1<<2 from first
                // loop, plus horizontal and vertical each scale by sqrt(8) so together
                // we've got an extra 1<<3, so 1<<17 total we need to remove.
                // so we want to round that, which means adding 0.5 * 1<<17,
                // aka 65536. Also, we'll end up with -128 to 127 that we want
                // to encode as 0..255 by adding 128, so we'll add that before the shift
                idct.x0 += 65536 + (128 << 17);
                idct.x1 += 65536 + (128 << 17);
                idct.x2 += 65536 + (128 << 17);
                idct.x3 += 65536 + (128 << 17);
                // tried computing the shifts into temps, or'ing the temps to see
                // if any were out of range, but that was slower
                o[0] = c4::clamp<uint8_t>((idct.x0 + idct.t3) >> 17);
                o[7] = c4::clamp<uint8_t>((idct.x0 - idct.t3) >> 17);
                o[1] = c4::clamp<uint8_t>((idct.x1 + idct.t2) >> 17);
                o[6] = c4::clamp<uint8_t>((idct.x1 - idct.t2) >> 17);
                o[2] = c4::clamp<uint8_t>((idct.x2 + idct.t1) >> 17);
                o[5] = c4::clamp<uint8_t>((idct.x2 - idct.t1) >> 17);
                o[3] = c4::clamp<uint8_t>((idct.x3 + idct.t0) >> 17);
                o[4] = c4::clamp<uint8_t>((idct.x3 - idct.t0) >> 17);
            }
        }


        static constexpr int STBI__MARKER_none = 0xff;
        // if there's a pending marker from the entropy stream, return that
        // otherwise, fetch from the stream and get a marker. if there's no
        // marker, return 0xff, which is never a valid marker value
        template<class T>
        static uint8_t get_marker(c4::byte_input_stream<T>& in, jpeg_decoder& j) {
            uint8_t x;
            if (j.marker != STBI__MARKER_none) { x = j.marker; j.marker = STBI__MARKER_none; return x; }
            x = in.get8();
            if (x != 0xff) return STBI__MARKER_none;
            while (x == 0xff)
                x = in.get8(); // consume repeated 0xff fill bytes
            return x;
        }

        // in each scan, we'll have scan_n components, and the order
        // of the components is specified by order[]
        inline bool STBI__RESTART(uint8_t x) {
            return x >= 0xd0 && x <= 0xd7;
        }

        // after a restart interval, jpeg_reset the entropy decoder and
        // the dc prediction
        static void jpeg_reset(jpeg_decoder& j) {
            j.code_bits = 0;
            j.code_buffer = 0;
            j.nomore = 0;
            j.img_comp[0].dc_pred = j.img_comp[1].dc_pred = j.img_comp[2].dc_pred = j.img_comp[3].dc_pred = 0;
            j.marker = STBI__MARKER_none;
            j.todo = j.restart_interval ? j.restart_interval : 0x7fffffff;
            j.eob_run = 0;
            // no more than 1<<31 MCUs if no restart_interal? that's plenty safe,
            // since we don't even allow 1<<30 pixels
        }

        template<class T>
        static void parse_entropy_coded_data(c4::byte_input_stream<T>& in, jpeg_decoder& z) {
            jpeg_reset(z);
            if (!z.progressive) {
                if (z.scan_n == 1) {
                    int i, j;
                    short data[64];
                    int n = z.order[0];
                    // non-interleaved data, we just need to process one block at a time,
                    // in trivial scanline order
                    // number of blocks to do just depends on how many actual "pixels" this
                    // component has, independent of interleaved MCU blocking and such
                    int w = (z.img_comp[n].x + 7) >> 3;
                    int h = (z.img_comp[n].y + 7) >> 3;
                    for (j = 0; j < h; ++j) {
                        for (i = 0; i < w; ++i) {
                            int ha = z.img_comp[n].ha;
                            jpeg_decode_block(in, z, data, z.huff_dc[z.img_comp[n].hd], z.huff_ac[ha], z.fast_ac[ha], n, z.dequant[z.img_comp[n].tq]);
                            idct_block(z.img_comp[n].data.data() + z.img_comp[n].w2*j * 8 + i * 8, z.img_comp[n].w2, data);
                            // every data block is an MCU, so countdown the restart interval
                            if (--z.todo <= 0) {
                                if (z.code_bits < 24) z.grow_buffer_unsafe(in);
                                // if it's NOT a restart, then just bail, so we get corrupt data
                                // rather than no data
                                if (!STBI__RESTART(z.marker)) return;
                                jpeg_reset(z);
                            }
                        }
                    }
                    return;
                }
                else { // interleaved
                    int i, j, k, x, y;
                    short data[64];
                    for (j = 0; j < z.img_mcu_y; ++j) {
                        for (i = 0; i < z.img_mcu_x; ++i) {
                            // scan an interleaved mcu... process scan_n components in order
                            for (k = 0; k < z.scan_n; ++k) {
                                int n = z.order[k];
                                // scan out an mcu's worth of this component; that's just determined
                                // by the basic H and V specified for the component
                                for (y = 0; y < z.img_comp[n].v; ++y) {
                                    for (x = 0; x < z.img_comp[n].h; ++x) {
                                        int x2 = (i*z.img_comp[n].h + x) * 8;
                                        int y2 = (j*z.img_comp[n].v + y) * 8;
                                        int ha = z.img_comp[n].ha;
                                        jpeg_decode_block(in, z, data, z.huff_dc[z.img_comp[n].hd], z.huff_ac[ha], z.fast_ac[ha], n, z.dequant[z.img_comp[n].tq]);
                                        idct_block(z.img_comp[n].data.data() + z.img_comp[n].w2*y2 + x2, z.img_comp[n].w2, data);
                                    }
                                }
                            }
                            // after all interleaved components, that's an interleaved MCU,
                            // so now count down the restart interval
                            if (--z.todo <= 0) {
                                if (z.code_bits < 24) z.grow_buffer_unsafe(in);
                                if (!STBI__RESTART(z.marker)) return;
                                jpeg_reset(z);
                            }
                        }
                    }
                    return;
                }
            }
            else {
                if (z.scan_n == 1) {
                    int i, j;
                    int n = z.order[0];
                    // non-interleaved data, we just need to process one block at a time,
                    // in trivial scanline order
                    // number of blocks to do just depends on how many actual "pixels" this
                    // component has, independent of interleaved MCU blocking and such
                    int w = (z.img_comp[n].x + 7) >> 3;
                    int h = (z.img_comp[n].y + 7) >> 3;
                    for (j = 0; j < h; ++j) {
                        for (i = 0; i < w; ++i) {
                            short *data = z.img_comp[n].coeff.data() + 64 * (i + j * z.img_comp[n].coeff_w);
                            if (z.spec_start == 0) {
                                jpeg_decode_block_prog_dc(in, z, data, z.huff_dc[z.img_comp[n].hd], n);
                            }
                            else {
                                int ha = z.img_comp[n].ha;
                                jpeg_decode_block_prog_ac(in, z, data, z.huff_ac[ha], z.fast_ac[ha]);
                            }
                            // every data block is an MCU, so countdown the restart interval
                            if (--z.todo <= 0) {
                                if (z.code_bits < 24) z.grow_buffer_unsafe(in);
                                if (!STBI__RESTART(z.marker)) return;
                                jpeg_reset(z);
                            }
                        }
                    }
                    return;
                }
                else { // interleaved
                    int i, j, k, x, y;
                    for (j = 0; j < z.img_mcu_y; ++j) {
                        for (i = 0; i < z.img_mcu_x; ++i) {
                            // scan an interleaved mcu... process scan_n components in order
                            for (k = 0; k < z.scan_n; ++k) {
                                int n = z.order[k];
                                // scan out an mcu's worth of this component; that's just determined
                                // by the basic H and V specified for the component
                                for (y = 0; y < z.img_comp[n].v; ++y) {
                                    for (x = 0; x < z.img_comp[n].h; ++x) {
                                        int x2 = (i*z.img_comp[n].h + x);
                                        int y2 = (j*z.img_comp[n].v + y);
                                        short *data = z.img_comp[n].coeff.data() + 64 * (x2 + y2 * z.img_comp[n].coeff_w);
                                        jpeg_decode_block_prog_dc(in, z, data, z.huff_dc[z.img_comp[n].hd], n);
                                    }
                                }
                            }
                            // after all interleaved components, that's an interleaved MCU,
                            // so now count down the restart interval
                            if (--z.todo <= 0) {
                                if (z.code_bits < 24) z.grow_buffer_unsafe(in);
                                if (!STBI__RESTART(z.marker)) return;
                                jpeg_reset(z);
                            }
                        }
                    }
                    return;
                }
            }
        }

        static void jpeg_dequantize(short *data, uint16_t *dequant) {
            for (int i = 0; i < 64; ++i)
                data[i] *= dequant[i];
        }

        static void jpeg_finish(jpeg_decoder& z) {
            if (z.progressive) {
                // dequantize and idct the data
                int i, j, n;
                for (n = 0; n < z.os.img_n; ++n) {
                    int w = (z.img_comp[n].x + 7) >> 3;
                    int h = (z.img_comp[n].y + 7) >> 3;
                    for (j = 0; j < h; ++j) {
                        for (i = 0; i < w; ++i) {
                            short *data = z.img_comp[n].coeff.data() + 64 * (i + j * z.img_comp[n].coeff_w);
                            jpeg_dequantize(data, z.dequant[z.img_comp[n].tq]);
                            idct_block(z.img_comp[n].data.data() + z.img_comp[n].w2*j * 8 + i * 8, z.img_comp[n].w2, data);
                        }
                    }
                }
            }
        }

        template<class T>
        static void process_marker(c4::byte_input_stream<T>& in, jpeg_decoder& z, int m) {
            int L;
            switch (m) {
            case STBI__MARKER_none: // no marker found
                THROW_EXCEPTION("Corrupt JPEG: expected marker");

            case 0xDD: // DRI - specify restart interval
                if (in.get16be() != 4) THROW_EXCEPTION("Corrupt JPEG: bad DRI len");
                z.restart_interval = in.get16be();
                return;

            case 0xDB: // DQT - define quantization table
                L = in.get16be() - 2;
                while (L > 0) {
                    int q = in.get8();
                    int p = q >> 4, sixteen = (p != 0);
                    int t = q & 15, i;
                    if (p != 0 && p != 1) THROW_EXCEPTION("Corrupt JPEG: bad DQT type");
                    if (t > 3) THROW_EXCEPTION("Corrupt JPEG: bad DQT table");

                    for (i = 0; i < 64; ++i)
                        z.dequant[t][jpeg_dezigzag[i]] = (uint16_t)(sixteen ? in.get16be() : in.get8());
                    L -= (sixteen ? 129 : 65);
                }
                ASSERT_EQUAL(L, 0);
                return;

            case 0xC4: // DHT - define huffman table
                L = in.get16be() - 2;
                while (L > 0) {
                    uint8_t *v;
                    int sizes[16], i, n = 0;
                    int q = in.get8();
                    int tc = q >> 4;
                    int th = q & 15;
                    if (tc > 1 || th > 3) THROW_EXCEPTION("Corrupt JPEG: bad DHT header");
                    for (i = 0; i < 16; ++i) {
                        sizes[i] = in.get8();
                        n += sizes[i];
                    }
                    L -= 17;
                    if (tc == 0) {
                        z.huff_dc[th].build_huffman(sizes);
                        v = z.huff_dc[th].values;
                    }
                    else {
                        z.huff_ac[th].build_huffman(sizes);
                        v = z.huff_ac[th].values;
                    }
                    for (i = 0; i < n; ++i)
                        v[i] = in.get8();
                    if (tc != 0)
                        z.huff_ac[th].build_fast_ac(z.fast_ac[th]);
                    L -= n;
                }
                ASSERT_EQUAL(L, 0);
                return;
            }

            // check for comment block or APP blocks
            if ((m >= 0xE0 && m <= 0xEF) || m == 0xFE) {
                L = in.get16be();
                if (L < 2) {
                    if (m == 0xFE)
                        THROW_EXCEPTION("Corrupt JPEG: bad COM len");
                    else
                        THROW_EXCEPTION("Corrupt JPEG: bad APP len");
                }
                L -= 2;

                if (m == 0xE0 && L >= 5) { // JFIF APP0 segment
                    static const unsigned char tag[5] = { 'J','F','I','F','\0' };
                    int ok = 1;
                    int i;
                    for (i = 0; i < 5; ++i)
                        if (in.get8() != tag[i])
                            ok = 0;
                    L -= 5;
                    if (ok)
                        z.jfif = 1;
                }
                else if (m == 0xEE && L >= 12) { // Adobe APP14 segment
                    static const unsigned char tag[6] = { 'A','d','o','b','e','\0' };
                    int ok = 1;
                    int i;
                    for (i = 0; i < 6; ++i)
                        if (in.get8() != tag[i])
                            ok = 0;
                    L -= 6;
                    if (ok) {
                        in.get8(); // version
                        in.get16be(); // flags0
                        in.get16be(); // flags1
                        z.app14_color_transform = in.get8(); // color transform
                        L -= 6;
                    }
                }

                in.skip(L);
            }
            else {
                THROW_EXCEPTION("Corrupt JPEG: unknown marker");
            }
        }

        // after we see SOS
        template<class T>
        static void process_scan_header(c4::byte_input_stream<T>& in, jpeg_decoder& z) {
            int i;
            int Ls = in.get16be();
            z.scan_n = in.get8();
            if (z.scan_n < 1 || z.scan_n > 4 || z.scan_n > (int)z.os.img_n) THROW_EXCEPTION("Corrupt JPEG: bad SOS component count");
            if (Ls != 6 + 2 * z.scan_n) THROW_EXCEPTION("Corrupt JPEG: bad SOS len");
            for (i = 0; i < z.scan_n; ++i) {
                int id = in.get8(), which;
                int q = in.get8();
                for (which = 0; which < z.os.img_n; ++which)
                    if (z.img_comp[which].id == id)
                        break;
                ASSERT_TRUE(which < z.os.img_n);

                z.img_comp[which].hd = q >> 4;   if (z.img_comp[which].hd > 3) THROW_EXCEPTION("Corrupt JPEG: bad DC huff");
                z.img_comp[which].ha = q & 15;   if (z.img_comp[which].ha > 3) THROW_EXCEPTION("Corrupt JPEG: bad AC huff");
                z.order[i] = which;
            }

            {
                z.spec_start = in.get8();
                z.spec_end = in.get8(); // should be 63, but might be 0
                int aa = in.get8();
                z.succ_high = (aa >> 4);
                z.succ_low = (aa & 15);
                if (z.progressive) {
                    if (z.spec_start > 63 || z.spec_end > 63 || z.spec_start > z.spec_end || z.succ_high > 13 || z.succ_low > 13)
                        THROW_EXCEPTION("Corrupt JPEG: bad SOS");
                }
                else {
                    if (z.spec_start != 0 || z.succ_high != 0 || z.succ_low != 0) THROW_EXCEPTION("Corrupt JPEG: bad SOS");
                    z.spec_end = 63;
                }
            }
        }

        template<class T>
        static void process_frame_header(c4::byte_input_stream<T>& in, jpeg_decoder& z)
        {
            auto *os = &z.os;
            int Lf, p, i, q, h_max = 1, v_max = 1, c;
            Lf = in.get16be();         if (Lf < 11) THROW_EXCEPTION("Corrupt JPEG: bad SOF len"); // JPEG
            p = in.get8();            if (p != 8) THROW_EXCEPTION("JPEG format not supported: 8-bit only"); // JPEG baseline
            os->img_y = in.get16be();   if (os->img_y == 0) THROW_EXCEPTION("JPEG format not supported: delayed height"); // Legal, but we don't handle it--but neither does IJG
            os->img_x = in.get16be();   if (os->img_x == 0) THROW_EXCEPTION("Corrupt JPEG: 0 width"); // JPEG requires
            c = in.get8();
            if (c != 3 && c != 1 && c != 4) THROW_EXCEPTION("Corrupt JPEG: bad component count");
            os->img_n = c;

            if (Lf != 8 + 3 * os->img_n) THROW_EXCEPTION("Corrupt JPEG: bad SOF len");

            z.rgb = 0;
            for (i = 0; i < os->img_n; ++i) {
                static const unsigned char rgb[3] = { 'R', 'G', 'B' };
                z.img_comp[i].id = in.get8();
                if (os->img_n == 3 && z.img_comp[i].id == rgb[i])
                    ++z.rgb;
                q = in.get8();
                z.img_comp[i].h = (q >> 4);  if (!z.img_comp[i].h || z.img_comp[i].h > 4) THROW_EXCEPTION("Corrupt JPEG: bad H");
                z.img_comp[i].v = q & 15;    if (!z.img_comp[i].v || z.img_comp[i].v > 4) THROW_EXCEPTION("Corrupt JPEG: bad V");
                z.img_comp[i].tq = in.get8();  if (z.img_comp[i].tq > 3) THROW_EXCEPTION("Corrupt JPEG: bad TQ");
            }

            for (i = 0; i < os->img_n; ++i) {
                if (z.img_comp[i].h > h_max) h_max = z.img_comp[i].h;
                if (z.img_comp[i].v > v_max) v_max = z.img_comp[i].v;
            }

            // compute interleaved mcu info
            z.img_h_max = h_max;
            z.img_v_max = v_max;
            z.img_mcu_w = h_max * 8;
            z.img_mcu_h = v_max * 8;
            // these sizes can't be more than 17 bits
            z.img_mcu_x = (os->img_x + z.img_mcu_w - 1) / z.img_mcu_w;
            z.img_mcu_y = (os->img_y + z.img_mcu_h - 1) / z.img_mcu_h;

            for (i = 0; i < os->img_n; ++i) {
                // number of effective pixels (e.g. for non-interleaved MCU)
                z.img_comp[i].x = (os->img_x * z.img_comp[i].h + h_max - 1) / h_max;
                z.img_comp[i].y = (os->img_y * z.img_comp[i].v + v_max - 1) / v_max;
                // to simplify generation, we'll allocate enough memory to decode
                // the bogus oversized data from using interleaved MCUs and their
                // big blocks (e.g. a 16x16 iMCU on an image of width 33); we won't
                // discard the extra data until colorspace conversion
                //
                // img_mcu_x, img_mcu_y: <=17 bits; comp[i].h and .v are <=4 (checked earlier)
                // so these muls can't overflow with 32-bit ints (which we require)
                z.img_comp[i].w2 = z.img_mcu_x * z.img_comp[i].h * 8;
                z.img_comp[i].h2 = z.img_mcu_y * z.img_comp[i].v * 8;
                z.img_comp[i].data.resize(z.img_comp[i].w2 * z.img_comp[i].h2);
                if (z.progressive) {
                    // w2, h2 are multiples of 8 (see above)
                    z.img_comp[i].coeff_w = z.img_comp[i].w2 / 8;
                    z.img_comp[i].coeff_h = z.img_comp[i].h2 / 8;
                    z.img_comp[i].coeff.resize(z.img_comp[i].w2 * z.img_comp[i].h2);
                }
            }
        }

        // use comparisons since in some cases we handle more than one case (e.g. SOF)
        inline bool DNL(int x) {
            return x == 0xdc;
        }
        inline bool SOI(int x) {
            return x == 0xd8;
        }
        inline bool EOI(int x) {
            return x == 0xd9;
        }
        inline bool SOF(int x) {
            return x == 0xc0 || x == 0xc1 || x == 0xc2;
        }
        inline bool SOS(int x) {
            return x == 0xda;
        }
        inline bool SOF_progressive(int x) {
            return x == 0xc2;
        }

        template<class T>
        static void decode_jpeg_header(c4::byte_input_stream<T>& in, jpeg_decoder& z)
        {
            z.jfif = 0;
            z.app14_color_transform = -1; // valid values are 0,1,2
            z.marker = STBI__MARKER_none; // initialize cached marker to empty
            int m = get_marker(in, z);
            if (!SOI(m)) THROW_EXCEPTION("Corrupt JPEG: no SOI");
            m = get_marker(in, z);
            while (!SOF(m)) {
                process_marker(in, z, m);
                m = get_marker(in, z);
                while (m == STBI__MARKER_none) {
                    // some files have extra padding after their blocks, so ok, we'll scan
                    if (in.eof())THROW_EXCEPTION("Corrupt JPEG: no SOF");
                    m = get_marker(in, z);
                }
            }
            z.progressive = SOF_progressive(m);
            process_frame_header(in, z);
        }

        // decode image to YCbCr format
        template<class T>
        static void decode_jpeg_image(c4::byte_input_stream<T>& in, jpeg_decoder& j) {
            j.restart_interval = 0;
            decode_jpeg_header(in, j);
            int m = get_marker(in, j);
            while (!EOI(m)) {
                if (SOS(m)) {
                    process_scan_header(in, j);
                    parse_entropy_coded_data(in, j);
                    if (j.marker == STBI__MARKER_none) {
                        // handle 0s at the end of image data from IP Kamera 9060
                        while (!in.eof()) {
                            int x = in.get8();
                            if (x == 255) {
                                j.marker = in.get8();
                                break;
                            }
                        }
                        // if we reach eof without hitting a marker, get_marker() below will fail and we'll eventually return 0
                    }
                }
                else if (DNL(m)) {
                    int Ld = in.get16be();
                    uint32_t NL = in.get16be();
                    if (Ld != 4) THROW_EXCEPTION("Corrupt JPEG: bad DNL len");
                    if (NL != j.os.img_y) THROW_EXCEPTION("Corrupt JPEG: bad DNL height");
                }
                else {
                    process_marker(in, j, m);
                }
                m = get_marker(in, j);
            }
            if (j.progressive)
                jpeg_finish(j);
        }

        // static jfif-centered resampling (across block boundaries)

        typedef uint8_t *(*resample_row_func)(uint8_t *out, uint8_t *in0, uint8_t *in1,
            int w, int hs);

        static uint8_t *resample_row_1(uint8_t* /*out*/, uint8_t* in_near, uint8_t* /*in_far*/, int /*w*/, int /*hs*/) {
            return in_near;
        }

        static uint8_t* resample_row_v_2(uint8_t *out, uint8_t *in_near, uint8_t *in_far, int w, int /*hs*/) {
            // need to generate two samples vertically for every one in input
            for (int i = 0; i < w; ++i)
                out[i] = jpeg_div4(3 * in_near[i] + in_far[i] + 2);
            return out;
        }

        static uint8_t*  resample_row_h_2(uint8_t *out, uint8_t *in_near, uint8_t* /*in_far*/, int w, int /*hs*/)
        {
            // need to generate two samples horizontally for every one in input
            uint8_t *input = in_near;

            if (w == 1) {
                // if only one sample, can't do any interpolation
                out[0] = out[1] = input[0];
                return out;
            }

            out[0] = input[0];
            out[1] = jpeg_div4(input[0] * 3 + input[1] + 2);

            int i;
            for (i = 1; i < w - 1; ++i) {
                int n = 3 * input[i] + 2;
                out[i * 2 + 0] = jpeg_div4(n + input[i - 1]);
                out[i * 2 + 1] = jpeg_div4(n + input[i + 1]);
            }
            out[i * 2 + 0] = jpeg_div4(input[w - 2] * 3 + input[w - 1] + 2);
            out[i * 2 + 1] = input[w - 1];

            return out;
        }

        static uint8_t *resample_row_hv_2(uint8_t *out, uint8_t *in_near, uint8_t *in_far, int w, int /*hs*/)
        {
            // need to generate 2x2 samples for every one in input
            if (w == 1) {
                out[0] = out[1] = jpeg_div4(3 * in_near[0] + in_far[0] + 2);
                return out;
            }

            int t1 = 3 * in_near[0] + in_far[0];
            out[0] = jpeg_div4(t1 + 2);
            for (int i = 1; i < w; ++i) {
                int t0 = t1;
                t1 = 3 * in_near[i] + in_far[i];
                out[i * 2 - 1] = jpeg_div16(3 * t0 + t1 + 8);
                out[i * 2] = jpeg_div16(3 * t1 + t0 + 8);
            }
            out[w * 2 - 1] = jpeg_div4(t1 + 2);

            return out;
        }

        static uint8_t *resample_row_generic(uint8_t *out, uint8_t *in_near, uint8_t* /*in_far*/, int w, int hs) {
            // resample with nearest-neighbor
            for (int i = 0; i < w; ++i)
                for (int j = 0; j < hs; ++j)
                    out[i*hs + j] = in_near[i];
            return out;
        }

        inline int float2fixed(float x) {
            return int(x * 4096.0f + 0.5f) << 8;
        }

        static void YCbCr_to_RGB_row(uint8_t *out, const uint8_t *y, const uint8_t *pcb, const uint8_t *pcr, int count, int step) {
            for (int i = 0; i < count; ++i) {
                int y_fixed = (y[i] << 20) + (1 << 19); // rounding
                int r, g, b;
                int cr = pcr[i] - 128;
                int cb = pcb[i] - 128;
                r = y_fixed + cr * float2fixed(1.40200f);
                g = y_fixed + (cr*-float2fixed(0.71414f)) + ((cb*-float2fixed(0.34414f)) & 0xffff0000);
                b = y_fixed + cb * float2fixed(1.77200f);
                r >>= 20;
                g >>= 20;
                b >>= 20;
                if ((unsigned)r > 255) { if (r < 0) r = 0; else r = 255; }
                if ((unsigned)g > 255) { if (g < 0) g = 0; else g = 255; }
                if ((unsigned)b > 255) { if (b < 0) b = 0; else b = 255; }
                out[0] = (uint8_t)r;
                out[1] = (uint8_t)g;
                out[2] = (uint8_t)b;
                out[3] = 255;
                out += step;
            }
        }

        struct jpeg_resample {
            resample_row_func resample;
            uint8_t *line0, *line1;
            int hs, vs;   // expansion factor in each axis
            int w_lores; // horizontal pixels pre-expansion
            int ystep;   // how far through vertical expansion we are
            int ypos;    // which pre-expansion row we're on
        };

        // fast 0..255 * 0..255 => 0..255 rounded multiplication
        static uint8_t blinn_8x8(uint8_t x, uint8_t y) {
            unsigned int t = x * y + 128;
            return (uint8_t)((t + (t >> 8)) >> 8);
        }

        template<class InputT, typename T>
        static void load_jpeg_image(c4::byte_input_stream<InputT>& in, c4::matrix<T>& img) {
            jpeg_decoder z;

            z.os.img_n = 0; // make cleanup_jpeg safe

            // load a jpeg image from whichever source, but leave in YCbCr format
            decode_jpeg_image(in, z);

            // determine actual number of components to generate
            const int n = sizeof(T) / sizeof(uint8_t);

            const int is_rgb = z.os.img_n == 3 && (z.rgb == 3 || (z.app14_color_transform == 0 && !z.jfif));

            const int decode_n = (z.os.img_n == 3 && n < 3 && !is_rgb) ? 1 : z.os.img_n;

            // resample and color-convert
            {
                jpeg_resample res_comp[4];

                for (int k = 0; k < decode_n; ++k) {
                    auto *r = &res_comp[k];

                    // allocate line buffer big enough for upsampling off the edges
                    // with upsample factor of 4
                    z.img_comp[k].linebuf.resize(z.os.img_x + 3);

                    r->hs = z.img_h_max / z.img_comp[k].h;
                    r->vs = z.img_v_max / z.img_comp[k].v;
                    r->ystep = r->vs >> 1;
                    r->w_lores = (z.os.img_x + r->hs - 1) / r->hs;
                    r->ypos = 0;
                    r->line0 = r->line1 = z.img_comp[k].data.data();

                    if (r->hs == 1 && r->vs == 1) r->resample = resample_row_1;
                    else if (r->hs == 1 && r->vs == 2) r->resample = resample_row_v_2;
                    else if (r->hs == 2 && r->vs == 1) r->resample = resample_row_h_2;
                    else if (r->hs == 2 && r->vs == 2) r->resample = resample_row_hv_2;
                    else                               r->resample = resample_row_generic;
                }

                // can't error after this so, this is safe
                img.resize(z.os.img_y, z.os.img_x);
                uint8_t *output = (uint8_t *)img.data();

                uint8_t *coutput[4] = { NULL, NULL, NULL, NULL };

                // now go ahead and resample
                for (uint32_t j = 0; j < z.os.img_y; ++j) {
                    uint8_t *out = output + n * z.os.img_x * j;
                    for (int k = 0; k < decode_n; ++k) {
                        auto *r = &res_comp[k];
                        int y_bot = r->ystep >= (r->vs >> 1);
                        coutput[k] = r->resample(z.img_comp[k].linebuf.data(),
                            y_bot ? r->line1 : r->line0,
                            y_bot ? r->line0 : r->line1,
                            r->w_lores, r->hs);
                        if (++r->ystep >= r->vs) {
                            r->ystep = 0;
                            r->line0 = r->line1;
                            if (++r->ypos < z.img_comp[k].y)
                                r->line1 += z.img_comp[k].w2;
                        }
                    }
                    if (n >= 3) {
                        uint8_t *y = coutput[0];
                        if (z.os.img_n == 3) {
                            if (is_rgb) {
                                for (uint32_t i = 0; i < z.os.img_x; ++i) {
                                    out[0] = y[i];
                                    out[1] = coutput[1][i];
                                    out[2] = coutput[2][i];
                                    out[3] = 255;
                                    out += n;
                                }
                            }
                            else {
                                YCbCr_to_RGB_row(out, y, coutput[1], coutput[2], z.os.img_x, n);
                            }
                        }
                        else if (z.os.img_n == 4) {
                            if (z.app14_color_transform == 0) { // CMYK
                                for (uint32_t i = 0; i < z.os.img_x; ++i) {
                                    uint8_t m = coutput[3][i];
                                    out[0] = blinn_8x8(coutput[0][i], m);
                                    out[1] = blinn_8x8(coutput[1][i], m);
                                    out[2] = blinn_8x8(coutput[2][i], m);
                                    out[3] = 255;
                                    out += n;
                                }
                            }
                            else if (z.app14_color_transform == 2) { // YCCK
                                YCbCr_to_RGB_row(out, y, coutput[1], coutput[2], z.os.img_x, n);
                                for (uint32_t i = 0; i < z.os.img_x; ++i) {
                                    uint8_t m = coutput[3][i];
                                    out[0] = blinn_8x8(255 - out[0], m);
                                    out[1] = blinn_8x8(255 - out[1], m);
                                    out[2] = blinn_8x8(255 - out[2], m);
                                    out += n;
                                }
                            }
                            else { // YCbCr + alpha?  Ignore the fourth channel for now
                                YCbCr_to_RGB_row(out, y, coutput[1], coutput[2], z.os.img_x, n);
                            }
                        }
                        else
                            for (uint32_t i = 0; i < z.os.img_x; ++i) {
                                out[0] = out[1] = out[2] = y[i];
                                out[3] = 255; // not used if n==3
                                out += n;
                            }
                    }
                    else {
                        if (is_rgb) {
                            if (n == 1)
                                for (uint32_t i = 0; i < z.os.img_x; ++i)
                                    *out++ = jpeg_compute_y(coutput[0][i], coutput[1][i], coutput[2][i]);
                            else {
                                for (uint32_t i = 0; i < z.os.img_x; ++i, out += 2) {
                                    out[0] = jpeg_compute_y(coutput[0][i], coutput[1][i], coutput[2][i]);
                                    out[1] = 255;
                                }
                            }
                        }
                        else if (z.os.img_n == 4 && z.app14_color_transform == 0) {
                            for (uint32_t i = 0; i < z.os.img_x; ++i) {
                                uint8_t m = coutput[3][i];
                                uint8_t r = blinn_8x8(coutput[0][i], m);
                                uint8_t g = blinn_8x8(coutput[1][i], m);
                                uint8_t b = blinn_8x8(coutput[2][i], m);
                                out[0] = jpeg_compute_y(r, g, b);
                                out[1] = 255;
                                out += n;
                            }
                        }
                        else if (z.os.img_n == 4 && z.app14_color_transform == 2) {
                            for (uint32_t i = 0; i < z.os.img_x; ++i) {
                                out[0] = blinn_8x8(255 - coutput[0][i], coutput[3][i]);
                                out[1] = 255;
                                out += n;
                            }
                        }
                        else {
                            uint8_t *y = coutput[0];
                            if (n == 1)
                                for (uint32_t i = 0; i < z.os.img_x; ++i) out[i] = y[i];
                            else
                                for (uint32_t i = 0; i < z.os.img_x; ++i) { *out++ = y[i]; *out++ = 255; }
                        }
                    }
                }
            }
        }
    }; // namespace detail

    template<typename T>
    static void read_jpeg(const std::string& filename, c4::matrix<T>& img) {
        c4::file_byte_input_stream fin(filename);
        try {
            detail::load_jpeg_image(fin, img);
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string(e.what()) + ", while reading " + filename);
        }
    }

    template<typename T>
    static void read_jpeg(const uint8_t* ptr, size_t size, c4::matrix<T>& img) {
        c4::memory_byte_input_stream fin(ptr, size);
        detail::load_jpeg_image(fin, img);
    }
}; // namespace c4

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

// Based on
// https://github.com/nothings/stb/blob/master/stb_image.h
// https://github.com/nothings/stb/blob/master/stb_image_write.h
//
// Limitations:
//    - no 12-bit-per-channel JPEG
//    - no JPEGs with arithmetic coding

#include <vector>
#include <istream>
#include <fstream>
#include <cstring>

#include "byte_stream.hpp"
#include "mstream.hpp"
#include "exception.hpp"
#include "matrix.hpp"
#include "math.hpp"
#include "simd.hpp"

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
            struct img_comp {
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

                img_comp() : id(0), h(0), v(0), tq(0), hd(0), ha(0), dc_pred(0), x(0), y(0), w2(0), h2(0), coeff_w(0), coeff_h(0) {}
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

            void grow_buffer_unsafe(std::istream& in) {
                do {
                    unsigned int b = nomore ? 0 : get8(in);
                    if (b == 0xff) {
                        int c = get8(in);
                        while (c == 0xff) c = get8(in); // consume fill bytes
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
            inline int extend_receive(std::istream& in, int n) {
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
            inline int jpeg_huff_decode(std::istream& in, const huffman& h) {
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
        inline static int jpeg_get_bits(std::istream& in, jpeg_decoder& j, int n) {
            if (j.code_bits < n) j.grow_buffer_unsafe(in);
            unsigned int k = lrot(j.code_buffer, n);
            j.code_buffer = k & ~bmask[n];
            k &= bmask[n];
            j.code_bits -= n;
            return k;
        }

        inline static int jpeg_get_bit(std::istream& in, jpeg_decoder& j) {
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
        static void jpeg_decode_block(std::istream& in, jpeg_decoder& j, short data[64], huffman& hdc, huffman& hac, int16_t *fac, int b, uint16_t *dequant)
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

        static void jpeg_decode_block_prog_dc(std::istream& in, jpeg_decoder& j, short data[64], huffman& hdc, int b) {
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
        static void jpeg_decode_block_prog_ac(std::istream& in, jpeg_decoder& j, short data[64], huffman& hac, int16_t *fac) {
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
            int val[64], *v = val;
            const short *d = data;

            // columns
            for (int i = 0; i < 8; ++i, ++d, ++v) {
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

            int t[8];
            v = val;
            for (int i = 0; i < 8; ++i, v += 8, out += out_stride) {
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
                t[0] = idct.x0 + idct.t3;
                t[7] = idct.x0 - idct.t3;
                t[1] = idct.x1 + idct.t2;
                t[6] = idct.x1 - idct.t2;
                t[2] = idct.x2 + idct.t1;
                t[5] = idct.x2 - idct.t1;
                t[3] = idct.x3 + idct.t0;
                t[4] = idct.x3 - idct.t0;
#ifdef __C4_SIMD__
                simd::int32x4x2 t32 = simd::load_tuple<2>(t);
                t32 = simd::shift_right<17>(t32);
                simd::int16x8 t16 = simd::narrow_saturate(t32);
                simd::half<simd::uint8x16> t8 = simd::narrow_unsigned_saturate(t16);
                simd::store(out, t8);
#else
                for (int k = 0; k < 8; k++) {
                    out[k] = c4::clamp<uint8_t>(t[k] >> 17);
                }
#endif
            }
        }


        static constexpr int STBI__MARKER_none = 0xff;
        // if there's a pending marker from the entropy stream, return that
        // otherwise, fetch from the stream and get a marker. if there's no
        // marker, return 0xff, which is never a valid marker value
        static uint8_t get_marker(std::istream& in, jpeg_decoder& j) {
            uint8_t x;
            if (j.marker != STBI__MARKER_none) { x = j.marker; j.marker = STBI__MARKER_none; return x; }
            x = get8(in);
            if (x != 0xff) return STBI__MARKER_none;
            while (x == 0xff)
                x = get8(in); // consume repeated 0xff fill bytes
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

        static void parse_entropy_coded_data(std::istream& in, jpeg_decoder& z) {
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

        static void process_marker(std::istream& in, jpeg_decoder& z, int m) {
            int L;
            switch (m) {
            case STBI__MARKER_none: // no marker found
                THROW_EXCEPTION("Corrupt JPEG: expected marker");

            case 0xDD: // DRI - specify restart interval
                if (get16be(in) != 4) THROW_EXCEPTION("Corrupt JPEG: bad DRI len");
                z.restart_interval = get16be(in);
                return;

            case 0xDB: // DQT - define quantization table
                L = get16be(in) - 2;
                while (L > 0) {
                    int q = get8(in);
                    int p = q >> 4, sixteen = (p != 0);
                    int t = q & 15, i;
                    if (p != 0 && p != 1) THROW_EXCEPTION("Corrupt JPEG: bad DQT type");
                    if (t > 3) THROW_EXCEPTION("Corrupt JPEG: bad DQT table");

                    for (i = 0; i < 64; ++i)
                        z.dequant[t][jpeg_dezigzag[i]] = (uint16_t)(sixteen ? get16be(in) : get8(in));
                    L -= (sixteen ? 129 : 65);
                }
                ASSERT_EQUAL(L, 0);
                return;

            case 0xC4: // DHT - define huffman table
                L = get16be(in) - 2;
                while (L > 0) {
                    uint8_t *v;
                    int sizes[16], i, n = 0;
                    int q = get8(in);
                    int tc = q >> 4;
                    int th = q & 15;
                    if (tc > 1 || th > 3) THROW_EXCEPTION("Corrupt JPEG: bad DHT header");
                    for (i = 0; i < 16; ++i) {
                        sizes[i] = get8(in);
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
                        v[i] = get8(in);
                    if (tc != 0)
                        z.huff_ac[th].build_fast_ac(z.fast_ac[th]);
                    L -= n;
                }
                ASSERT_EQUAL(L, 0);
                return;
            }

            // check for comment block or APP blocks
            if ((m >= 0xE0 && m <= 0xEF) || m == 0xFE) {
                L = get16be(in);
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
                        if (get8(in) != tag[i])
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
                        if (get8(in) != tag[i])
                            ok = 0;
                    L -= 6;
                    if (ok) {
                        get8(in); // version
                        get16be(in); // flags0
                        get16be(in); // flags1
                        z.app14_color_transform = get8(in); // color transform
                        L -= 6;
                    }
                }

                skip(in, L);
            }
            else {
                THROW_EXCEPTION("Corrupt JPEG: unknown marker");
            }
        }

        // after we see SOS
        static void process_scan_header(std::istream& in, jpeg_decoder& z) {
            int i;
            int Ls = get16be(in);
            z.scan_n = get8(in);
            if (z.scan_n < 1 || z.scan_n > 4 || z.scan_n > (int)z.os.img_n) THROW_EXCEPTION("Corrupt JPEG: bad SOS component count");
            if (Ls != 6 + 2 * z.scan_n) THROW_EXCEPTION("Corrupt JPEG: bad SOS len");
            for (i = 0; i < z.scan_n; ++i) {
                int id = get8(in), which;
                int q = get8(in);
                for (which = 0; which < z.os.img_n; ++which)
                    if (z.img_comp[which].id == id)
                        break;
                ASSERT_TRUE(which < z.os.img_n);

                z.img_comp[which].hd = q >> 4;   if (z.img_comp[which].hd > 3) THROW_EXCEPTION("Corrupt JPEG: bad DC huff");
                z.img_comp[which].ha = q & 15;   if (z.img_comp[which].ha > 3) THROW_EXCEPTION("Corrupt JPEG: bad AC huff");
                z.order[i] = which;
            }

            {
                z.spec_start = get8(in);
                z.spec_end = get8(in); // should be 63, but might be 0
                int aa = get8(in);
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

        static void process_frame_header(std::istream& in, jpeg_decoder& z)
        {
            auto *os = &z.os;
            int Lf, p, i, q, h_max = 1, v_max = 1, c;
            Lf = get16be(in);         if (Lf < 11) THROW_EXCEPTION("Corrupt JPEG: bad SOF len"); // JPEG
            p = get8(in);            if (p != 8) THROW_EXCEPTION("JPEG format not supported: 8-bit only"); // JPEG baseline
            os->img_y = get16be(in);   if (os->img_y == 0) THROW_EXCEPTION("JPEG format not supported: delayed height"); // Legal, but we don't handle it--but neither does IJG
            os->img_x = get16be(in);   if (os->img_x == 0) THROW_EXCEPTION("Corrupt JPEG: 0 width"); // JPEG requires
            c = get8(in);
            if (c != 3 && c != 1 && c != 4) THROW_EXCEPTION("Corrupt JPEG: bad component count");
            os->img_n = c;

            if (Lf != 8 + 3 * os->img_n) THROW_EXCEPTION("Corrupt JPEG: bad SOF len");

            z.rgb = 0;
            for (i = 0; i < os->img_n; ++i) {
                static const unsigned char rgb[3] = { 'R', 'G', 'B' };
                z.img_comp[i].id = get8(in);
                if (os->img_n == 3 && z.img_comp[i].id == rgb[i])
                    ++z.rgb;
                q = get8(in);
                z.img_comp[i].h = (q >> 4);  if (!z.img_comp[i].h || z.img_comp[i].h > 4) THROW_EXCEPTION("Corrupt JPEG: bad H");
                z.img_comp[i].v = q & 15;    if (!z.img_comp[i].v || z.img_comp[i].v > 4) THROW_EXCEPTION("Corrupt JPEG: bad V");
                z.img_comp[i].tq = get8(in);  if (z.img_comp[i].tq > 3) THROW_EXCEPTION("Corrupt JPEG: bad TQ");
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

        static void decode_jpeg_header(std::istream& in, jpeg_decoder& z)
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
        static void decode_jpeg_image(std::istream& in, jpeg_decoder& j) {
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
                            int x = get8(in);
                            if (x == 255) {
                                j.marker = get8(in);
                                break;
                            }
                        }
                        // if we reach eof without hitting a marker, get_marker() below will fail and we'll eventually return 0
                    }
                }
                else if (DNL(m)) {
                    int Ld = get16be(in);
                    uint32_t NL = get16be(in);
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

        static const unsigned char jpg_ZigZag[] = { 0,1,5,6,14,15,27,28,2,4,7,13,16,26,29,42,3,8,12,17,25,30,41,43,9,11,18,
            24,31,40,44,53,10,19,23,32,39,45,52,54,20,22,33,38,46,51,55,60,21,34,37,47,50,56,59,61,35,36,48,49,57,58,62,63 };

        static void jpg_writeBits(std::ostream& out, int *bitBufP, int *bitCntP, const unsigned short *bs) {
            int bitBuf = *bitBufP, bitCnt = *bitCntP;
            bitCnt += bs[1];
            bitBuf |= bs[0] << (24 - bitCnt);
            while (bitCnt >= 8) {
                unsigned char c = (bitBuf >> 16) & 255;

                out.put(c);
                if (c == 255) {
                    out.put(0);
                }
                bitBuf <<= 8;
                bitCnt -= 8;
            }
            *bitBufP = bitBuf;
            *bitCntP = bitCnt;
        }

        static void jpg_DCT(float *d0p, float *d1p, float *d2p, float *d3p, float *d4p, float *d5p, float *d6p, float *d7p) {
            float d0 = *d0p, d1 = *d1p, d2 = *d2p, d3 = *d3p, d4 = *d4p, d5 = *d5p, d6 = *d6p, d7 = *d7p;
            float z1, z2, z3, z4, z5, z11, z13;

            float tmp0 = d0 + d7;
            float tmp7 = d0 - d7;
            float tmp1 = d1 + d6;
            float tmp6 = d1 - d6;
            float tmp2 = d2 + d5;
            float tmp5 = d2 - d5;
            float tmp3 = d3 + d4;
            float tmp4 = d3 - d4;

            // Even part
            float tmp10 = tmp0 + tmp3;   // phase 2
            float tmp13 = tmp0 - tmp3;
            float tmp11 = tmp1 + tmp2;
            float tmp12 = tmp1 - tmp2;

            d0 = tmp10 + tmp11;       // phase 3
            d4 = tmp10 - tmp11;

            z1 = (tmp12 + tmp13) * 0.707106781f; // c4
            d2 = tmp13 + z1;       // phase 5
            d6 = tmp13 - z1;

            // Odd part
            tmp10 = tmp4 + tmp5;       // phase 2
            tmp11 = tmp5 + tmp6;
            tmp12 = tmp6 + tmp7;

            // The rotator is modified from fig 4-8 to avoid extra negations.
            z5 = (tmp10 - tmp12) * 0.382683433f; // c6
            z2 = tmp10 * 0.541196100f + z5; // c2-c6
            z4 = tmp12 * 1.306562965f + z5; // c2+c6
            z3 = tmp11 * 0.707106781f; // c4

            z11 = tmp7 + z3;      // phase 5
            z13 = tmp7 - z3;

            *d5p = z13 + z2;         // phase 6
            *d3p = z13 - z2;
            *d1p = z11 + z4;
            *d7p = z11 - z4;

            *d0p = d0;  *d2p = d2;  *d4p = d4;  *d6p = d6;
        }

        static void jpg_calcBits(int val, unsigned short bits[2]) {
            int tmp1 = val < 0 ? -val : val;
            val = val < 0 ? val - 1 : val;
            bits[1] = 1;
            while (tmp1 >>= 1) {
                ++bits[1];
            }
            bits[0] = val & ((1 << bits[1]) - 1);
        }

        static int jpg_processDU(std::ostream& out, int *bitBuf, int *bitCnt, float *CDU, float *fdtbl, int DC, const unsigned short HTDC[256][2], const unsigned short HTAC[256][2]) {
            const unsigned short EOB[2] = { HTAC[0x00][0], HTAC[0x00][1] };
            const unsigned short M16zeroes[2] = { HTAC[0xF0][0], HTAC[0xF0][1] };
            int dataOff, i, diff, end0pos;
            int DU[64];

            // DCT rows
            for (dataOff = 0; dataOff < 64; dataOff += 8) {
                jpg_DCT(&CDU[dataOff], &CDU[dataOff + 1], &CDU[dataOff + 2], &CDU[dataOff + 3], &CDU[dataOff + 4], &CDU[dataOff + 5], &CDU[dataOff + 6], &CDU[dataOff + 7]);
            }
            // DCT columns
            for (dataOff = 0; dataOff < 8; ++dataOff) {
                jpg_DCT(&CDU[dataOff], &CDU[dataOff + 8], &CDU[dataOff + 16], &CDU[dataOff + 24], &CDU[dataOff + 32], &CDU[dataOff + 40], &CDU[dataOff + 48], &CDU[dataOff + 56]);
            }
            // Quantize/descale/zigzag the coefficients
            for (i = 0; i < 64; ++i) {
                float v = CDU[i] * fdtbl[i];
                // DU[stbiw__jpg_ZigZag[i]] = (int)(v < 0 ? ceilf(v - 0.5f) : floorf(v + 0.5f));
                // ceilf() and floorf() are C99, not C89, but I /think/ they're not needed here anyway?
                DU[jpg_ZigZag[i]] = (int)(v < 0 ? v - 0.5f : v + 0.5f);
            }

            // Encode DC
            diff = DU[0] - DC;
            if (diff == 0) {
                jpg_writeBits(out, bitBuf, bitCnt, HTDC[0]);
            } else {
                unsigned short bits[2];
                jpg_calcBits(diff, bits);
                jpg_writeBits(out, bitBuf, bitCnt, HTDC[bits[1]]);
                jpg_writeBits(out, bitBuf, bitCnt, bits);
            }
            // Encode ACs
            end0pos = 63;
            for (; (end0pos > 0) && (DU[end0pos] == 0); --end0pos) {
            }
            // end0pos = first element in reverse order !=0
            if (end0pos == 0) {
                jpg_writeBits(out, bitBuf, bitCnt, EOB);
                return DU[0];
            }
            for (i = 1; i <= end0pos; ++i) {
                int startpos = i;
                int nrzeroes;
                unsigned short bits[2];
                for (; DU[i] == 0 && i <= end0pos; ++i) {
                }
                nrzeroes = i - startpos;
                if (nrzeroes >= 16) {
                    int lng = nrzeroes >> 4;
                    int nrmarker;
                    for (nrmarker = 1; nrmarker <= lng; ++nrmarker)
                        jpg_writeBits(out, bitBuf, bitCnt, M16zeroes);
                    nrzeroes &= 15;
                }
                jpg_calcBits(DU[i], bits);
                jpg_writeBits(out, bitBuf, bitCnt, HTAC[(nrzeroes << 4) + bits[1]]);
                jpg_writeBits(out, bitBuf, bitCnt, bits);
            }
            if (end0pos != 63) {
                jpg_writeBits(out, bitBuf, bitCnt, EOB);
            }
            return DU[0];
        }

        static int write_jpg(std::ostream& out, int width, int height, int comp, const void* data, int quality, bool flip_vertically) {
            // Constants that don't pollute global namespace
            static const unsigned char std_dc_luminance_nrcodes[] = { 0,0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0 };
            static const unsigned char std_dc_luminance_values[] = { 0,1,2,3,4,5,6,7,8,9,10,11 };
            static const unsigned char std_ac_luminance_nrcodes[] = { 0,0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7d };
            static const unsigned char std_ac_luminance_values[] = {
                0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,0x22,0x71,0x14,0x32,0x81,0x91,0xa1,0x08,
                0x23,0x42,0xb1,0xc1,0x15,0x52,0xd1,0xf0,0x24,0x33,0x62,0x72,0x82,0x09,0x0a,0x16,0x17,0x18,0x19,0x1a,0x25,0x26,0x27,0x28,
                0x29,0x2a,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,
                0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
                0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,
                0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe1,0xe2,
                0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa
            };
            static const unsigned char std_dc_chrominance_nrcodes[] = { 0,0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0 };
            static const unsigned char std_dc_chrominance_values[] = { 0,1,2,3,4,5,6,7,8,9,10,11 };
            static const unsigned char std_ac_chrominance_nrcodes[] = { 0,0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,0x77 };
            static const unsigned char std_ac_chrominance_values[] = {
                0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,
                0xa1,0xb1,0xc1,0x09,0x23,0x33,0x52,0xf0,0x15,0x62,0x72,0xd1,0x0a,0x16,0x24,0x34,0xe1,0x25,0xf1,0x17,0x18,0x19,0x1a,0x26,
                0x27,0x28,0x29,0x2a,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,
                0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x82,0x83,0x84,0x85,0x86,0x87,
                0x88,0x89,0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,
                0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,
                0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa
            };
            // Huffman tables
            static const unsigned short YDC_HT[256][2] = { { 0,2 },{ 2,3 },{ 3,3 },{ 4,3 },{ 5,3 },{ 6,3 },{ 14,4 },{ 30,5 },{ 62,6 },{ 126,7 },{ 254,8 },{ 510,9 } };
            static const unsigned short UVDC_HT[256][2] = { { 0,2 },{ 1,2 },{ 2,2 },{ 6,3 },{ 14,4 },{ 30,5 },{ 62,6 },{ 126,7 },{ 254,8 },{ 510,9 },{ 1022,10 },{ 2046,11 } };
            static const unsigned short YAC_HT[256][2] = {
                { 10,4 },{ 0,2 },{ 1,2 },{ 4,3 },{ 11,4 },{ 26,5 },{ 120,7 },{ 248,8 },{ 1014,10 },{ 65410,16 },{ 65411,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 12,4 },{ 27,5 },{ 121,7 },{ 502,9 },{ 2038,11 },{ 65412,16 },{ 65413,16 },{ 65414,16 },{ 65415,16 },{ 65416,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 28,5 },{ 249,8 },{ 1015,10 },{ 4084,12 },{ 65417,16 },{ 65418,16 },{ 65419,16 },{ 65420,16 },{ 65421,16 },{ 65422,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 58,6 },{ 503,9 },{ 4085,12 },{ 65423,16 },{ 65424,16 },{ 65425,16 },{ 65426,16 },{ 65427,16 },{ 65428,16 },{ 65429,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 59,6 },{ 1016,10 },{ 65430,16 },{ 65431,16 },{ 65432,16 },{ 65433,16 },{ 65434,16 },{ 65435,16 },{ 65436,16 },{ 65437,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 122,7 },{ 2039,11 },{ 65438,16 },{ 65439,16 },{ 65440,16 },{ 65441,16 },{ 65442,16 },{ 65443,16 },{ 65444,16 },{ 65445,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 123,7 },{ 4086,12 },{ 65446,16 },{ 65447,16 },{ 65448,16 },{ 65449,16 },{ 65450,16 },{ 65451,16 },{ 65452,16 },{ 65453,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 250,8 },{ 4087,12 },{ 65454,16 },{ 65455,16 },{ 65456,16 },{ 65457,16 },{ 65458,16 },{ 65459,16 },{ 65460,16 },{ 65461,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 504,9 },{ 32704,15 },{ 65462,16 },{ 65463,16 },{ 65464,16 },{ 65465,16 },{ 65466,16 },{ 65467,16 },{ 65468,16 },{ 65469,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 505,9 },{ 65470,16 },{ 65471,16 },{ 65472,16 },{ 65473,16 },{ 65474,16 },{ 65475,16 },{ 65476,16 },{ 65477,16 },{ 65478,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 506,9 },{ 65479,16 },{ 65480,16 },{ 65481,16 },{ 65482,16 },{ 65483,16 },{ 65484,16 },{ 65485,16 },{ 65486,16 },{ 65487,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 1017,10 },{ 65488,16 },{ 65489,16 },{ 65490,16 },{ 65491,16 },{ 65492,16 },{ 65493,16 },{ 65494,16 },{ 65495,16 },{ 65496,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 1018,10 },{ 65497,16 },{ 65498,16 },{ 65499,16 },{ 65500,16 },{ 65501,16 },{ 65502,16 },{ 65503,16 },{ 65504,16 },{ 65505,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 2040,11 },{ 65506,16 },{ 65507,16 },{ 65508,16 },{ 65509,16 },{ 65510,16 },{ 65511,16 },{ 65512,16 },{ 65513,16 },{ 65514,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 65515,16 },{ 65516,16 },{ 65517,16 },{ 65518,16 },{ 65519,16 },{ 65520,16 },{ 65521,16 },{ 65522,16 },{ 65523,16 },{ 65524,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 2041,11 },{ 65525,16 },{ 65526,16 },{ 65527,16 },{ 65528,16 },{ 65529,16 },{ 65530,16 },{ 65531,16 },{ 65532,16 },{ 65533,16 },{ 65534,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 }
            };
            static const unsigned short UVAC_HT[256][2] = {
                { 0,2 },{ 1,2 },{ 4,3 },{ 10,4 },{ 24,5 },{ 25,5 },{ 56,6 },{ 120,7 },{ 500,9 },{ 1014,10 },{ 4084,12 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 11,4 },{ 57,6 },{ 246,8 },{ 501,9 },{ 2038,11 },{ 4085,12 },{ 65416,16 },{ 65417,16 },{ 65418,16 },{ 65419,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 26,5 },{ 247,8 },{ 1015,10 },{ 4086,12 },{ 32706,15 },{ 65420,16 },{ 65421,16 },{ 65422,16 },{ 65423,16 },{ 65424,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 27,5 },{ 248,8 },{ 1016,10 },{ 4087,12 },{ 65425,16 },{ 65426,16 },{ 65427,16 },{ 65428,16 },{ 65429,16 },{ 65430,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 58,6 },{ 502,9 },{ 65431,16 },{ 65432,16 },{ 65433,16 },{ 65434,16 },{ 65435,16 },{ 65436,16 },{ 65437,16 },{ 65438,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 59,6 },{ 1017,10 },{ 65439,16 },{ 65440,16 },{ 65441,16 },{ 65442,16 },{ 65443,16 },{ 65444,16 },{ 65445,16 },{ 65446,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 121,7 },{ 2039,11 },{ 65447,16 },{ 65448,16 },{ 65449,16 },{ 65450,16 },{ 65451,16 },{ 65452,16 },{ 65453,16 },{ 65454,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 122,7 },{ 2040,11 },{ 65455,16 },{ 65456,16 },{ 65457,16 },{ 65458,16 },{ 65459,16 },{ 65460,16 },{ 65461,16 },{ 65462,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 249,8 },{ 65463,16 },{ 65464,16 },{ 65465,16 },{ 65466,16 },{ 65467,16 },{ 65468,16 },{ 65469,16 },{ 65470,16 },{ 65471,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 503,9 },{ 65472,16 },{ 65473,16 },{ 65474,16 },{ 65475,16 },{ 65476,16 },{ 65477,16 },{ 65478,16 },{ 65479,16 },{ 65480,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 504,9 },{ 65481,16 },{ 65482,16 },{ 65483,16 },{ 65484,16 },{ 65485,16 },{ 65486,16 },{ 65487,16 },{ 65488,16 },{ 65489,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 505,9 },{ 65490,16 },{ 65491,16 },{ 65492,16 },{ 65493,16 },{ 65494,16 },{ 65495,16 },{ 65496,16 },{ 65497,16 },{ 65498,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 506,9 },{ 65499,16 },{ 65500,16 },{ 65501,16 },{ 65502,16 },{ 65503,16 },{ 65504,16 },{ 65505,16 },{ 65506,16 },{ 65507,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 2041,11 },{ 65508,16 },{ 65509,16 },{ 65510,16 },{ 65511,16 },{ 65512,16 },{ 65513,16 },{ 65514,16 },{ 65515,16 },{ 65516,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 16352,14 },{ 65517,16 },{ 65518,16 },{ 65519,16 },{ 65520,16 },{ 65521,16 },{ 65522,16 },{ 65523,16 },{ 65524,16 },{ 65525,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
            { 1018,10 },{ 32707,15 },{ 65526,16 },{ 65527,16 },{ 65528,16 },{ 65529,16 },{ 65530,16 },{ 65531,16 },{ 65532,16 },{ 65533,16 },{ 65534,16 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 }
            };
            static const int YQT[] = { 16,11,10,16,24,40,51,61,12,12,14,19,26,58,60,55,14,13,16,24,40,57,69,56,14,17,22,29,51,87,80,62,18,22,
                37,56,68,109,103,77,24,35,55,64,81,104,113,92,49,64,78,87,103,121,120,101,72,92,95,98,112,100,103,99 };
            static const int UVQT[] = { 17,18,24,47,99,99,99,99,18,21,26,66,99,99,99,99,24,26,56,99,99,99,99,99,47,66,99,99,99,99,99,99,
                99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99 };
            static const float aasf[] = { 1.0f * 2.828427125f, 1.387039845f * 2.828427125f, 1.306562965f * 2.828427125f, 1.175875602f * 2.828427125f,
                1.0f * 2.828427125f, 0.785694958f * 2.828427125f, 0.541196100f * 2.828427125f, 0.275899379f * 2.828427125f };

            int row, col, i, k;
            float fdtbl_Y[64], fdtbl_UV[64];
            unsigned char YTable[64], UVTable[64];

            ASSERT_TRUE(data);
            ASSERT_TRUE(width > 0 && height > 0);
            ASSERT_TRUE(comp <= 4 && comp >= 1);

            quality = quality ? quality : 90;
            quality = quality < 1 ? 1 : quality > 100 ? 100 : quality;
            quality = quality < 50 ? 5000 / quality : 200 - quality * 2;

            for (i = 0; i < 64; ++i) {
                int uvti, yti = (YQT[i] * quality + 50) / 100;
                YTable[jpg_ZigZag[i]] = (unsigned char)(yti < 1 ? 1 : yti > 255 ? 255 : yti);
                uvti = (UVQT[i] * quality + 50) / 100;
                UVTable[jpg_ZigZag[i]] = (unsigned char)(uvti < 1 ? 1 : uvti > 255 ? 255 : uvti);
            }

            for (row = 0, k = 0; row < 8; ++row) {
                for (col = 0; col < 8; ++col, ++k) {
                    fdtbl_Y[k] = 1 / (YTable[jpg_ZigZag[k]] * aasf[row] * aasf[col]);
                    fdtbl_UV[k] = 1 / (UVTable[jpg_ZigZag[k]] * aasf[row] * aasf[col]);
                }
            }

            // Write Headers
            {
                static const unsigned char head0[] = { 0xFF,0xD8,0xFF,0xE0,0,0x10,'J','F','I','F',0,1,1,0,0,1,0,1,0,0,0xFF,0xDB,0,0x84,0 };
                static const unsigned char head2[] = { 0xFF,0xDA,0,0xC,3,1,0,2,0x11,3,0x11,0,0x3F,0 };
                const unsigned char head1[] = { 0xFF,0xC0,0,0x11,8,uint8_t(height >> 8),uint8_t(height),uint8_t(width >> 8),uint8_t(width),
                    3,1,0x11,0,2,0x11,1,3,0x11,1,0xFF,0xC4,0x01,0xA2,0 };

                out.write((const char*)head0, sizeof(head0));
                out.write((const char*)YTable, sizeof(YTable));
                out.put(1);
                out.write((const char*)UVTable, sizeof(UVTable));
                out.write((const char*)head1, sizeof(head1));
                out.write((const char*)(std_dc_luminance_nrcodes + 1), sizeof(std_dc_luminance_nrcodes) - 1);
                out.write((const char*)std_dc_luminance_values, sizeof(std_dc_luminance_values));
                out.put(0x10); // HTYACinfo
                out.write((const char*)(std_ac_luminance_nrcodes + 1), sizeof(std_ac_luminance_nrcodes) - 1);
                out.write((const char*)std_ac_luminance_values, sizeof(std_ac_luminance_values));
                out.put(1); // HTUDCinfo
                out.write((const char*)(std_dc_chrominance_nrcodes + 1), sizeof(std_dc_chrominance_nrcodes) - 1);
                out.write((const char*)std_dc_chrominance_values, sizeof(std_dc_chrominance_values));
                out.put(0x11); // HTUACinfo
                out.write((const char*)(std_ac_chrominance_nrcodes + 1), sizeof(std_ac_chrominance_nrcodes) - 1);
                out.write((const char*)std_ac_chrominance_values, sizeof(std_ac_chrominance_values));
                out.write((const char*)head2, sizeof(head2));
            }

            // Encode 8x8 macroblocks
            {
                static const unsigned short fillBits[] = { 0x7F, 7 };
                const unsigned char *imageData = (const unsigned char *)data;
                int DCY = 0, DCU = 0, DCV = 0;
                int bitBuf = 0, bitCnt = 0;
                // comp == 2 is grey+alpha (alpha is ignored)
                int ofsG = comp > 2 ? 1 : 0, ofsB = comp > 2 ? 2 : 0;
                int x, y, pos;
                for (y = 0; y < height; y += 8) {
                    for (x = 0; x < width; x += 8) {
                        float YDU[64], UDU[64], VDU[64];
                        for (row = y, pos = 0; row < y + 8; ++row) {
                            // row >= height => use last input row
                            int clamped_row = (row < height) ? row : height - 1;
                            int base_p = (flip_vertically ? (height - 1 - clamped_row) : clamped_row)*width*comp;
                            for (col = x; col < x + 8; ++col, ++pos) {
                                float r, g, b;
                                // if col >= width => use pixel from last input column
                                int p = base_p + ((col < width) ? col : (width - 1))*comp;

                                r = imageData[p + 0];
                                g = imageData[p + ofsG];
                                b = imageData[p + ofsB];
                                YDU[pos] = +0.29900f*r + 0.58700f*g + 0.11400f*b - 128;
                                UDU[pos] = -0.16874f*r - 0.33126f*g + 0.50000f*b;
                                VDU[pos] = +0.50000f*r - 0.41869f*g - 0.08131f*b;
                            }
                        }

                        DCY = jpg_processDU(out, &bitBuf, &bitCnt, YDU, fdtbl_Y, DCY, YDC_HT, YAC_HT);
                        DCU = jpg_processDU(out, &bitBuf, &bitCnt, UDU, fdtbl_UV, DCU, UVDC_HT, UVAC_HT);
                        DCV = jpg_processDU(out, &bitBuf, &bitCnt, VDU, fdtbl_UV, DCV, UVDC_HT, UVAC_HT);
                    }
                }

                // Do the bit alignment of the EOI marker
                jpg_writeBits(out, &bitBuf, &bitCnt, fillBits);
            }

            // EOI
            out.put((char)0xFF);
            out.put((char)0xD9);

            return 1;
        }

    }; // namespace detail

    template<typename T>
    static void load_jpeg_image(std::istream& in, c4::matrix<T>& img) {
        using namespace detail;

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

    template<typename T>
    static void read_jpeg(const std::string& filename, c4::matrix<T>& img) {
        std::ifstream fin(filename, std::ifstream::binary);
        try {
            load_jpeg_image(fin, img);
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string(e.what()) + ", while reading " + filename);
        }
    }

    template<typename T>
    static void read_jpeg(const uint8_t* ptr, size_t size, c4::matrix<T>& img) {
        imstream in(ptr, size);
        load_jpeg_image(in, img);
    }

    template<typename T>
    inline void write_jpeg(std::ostream& out, const matrix_ref<T>& img, int quality = 75, bool flip_vertically = false) {
        detail::write_jpg(out, img.width(), img.height(), sizeof(T) / sizeof(uint8_t), img.data(), quality, flip_vertically);
    }

    template<typename T>
    inline void write_jpeg(const std::string& filepath, const matrix_ref<T>& img, int quality = 75, bool flip_vertically = false) {
        std::ofstream fout(filepath, std::ofstream::binary);
        write_jpeg(fout, img, quality, flip_vertically);
    }
}; // namespace c4

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
// Limitations:
//    - no 12-bit-per-channel JPEG
//    - no JPEGs with arithmetic coding
//
// Basic usage:
//    int x,y,n;
//    unsigned char *data = stbi_load(filename, &x, &y, &n, 0);
//    // ... process data if not NULL ...
//    // ... x = width, y = height, n = # 8-bit components per pixel ...
//    // ... replace '0' with '1'..'4' to force that many components per pixel
//    // ... but 'n' will always be the number that it would have been if you said 0
//    free(data)
//
// Standard parameters:
//    int *x                 -- outputs image width in pixels
//    int *y                 -- outputs image height in pixels
//    int *channels_in_file  -- outputs # of image components in image file
//    int desired_channels   -- if non-zero, # of image components requested in result
//
// The return value from an image loader is an 'unsigned char *' which points
// to the pixel data, or NULL on an allocation failure or if the image is
// corrupt or invalid. The pixel data consists of *y scanlines of *x pixels,
// with each pixel consisting of N interleaved 8-bit components; the first
// pixel pointed to is top-left-most in the image. There is no padding between
// image scanlines or between pixels, regardless of format. The number of
// components N is 'desired_channels' if desired_channels is non-zero, or
// *channels_in_file otherwise. If desired_channels is non-zero,
// *channels_in_file has the number of components that _would_ have been
// output otherwise. E.g. if you set desired_channels to 4, you will always
// get RGBA output, but you can check *channels_in_file to see if it's trivially
// opaque because e.g. there were only 3 channels in the source image.
//
// An output image with N components has the following components interleaved
// in this order in each pixel:
//
//     N=#comp     components
//       1           grey
//       2           grey, alpha
//       3           red, green, blue
//       4           red, green, blue, alpha
//
// If image loading fails for any reason, the return value will be NULL,
// and *x, *y, *channels_in_file will be unchanged. The function
// stbi_failure_reason() can be queried for an extremely brief, end-user
// unfriendly explanation of why the load failed. Define STBI_NO_FAILURE_STRINGS
// to avoid compiling these strings at all, and STBI_FAILURE_USERMSG to get slightly
// more user-friendly ones.
//

#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <istream>
#include <fstream>
#include <vector>

#include "exception.hpp"
#include "math.hpp"

#include <cstdlib>
#include <cstdint>

#include <cstdlib>
#include <cstring>
#include <climits>
#include <cstdio>
#include <cassert>

#ifdef _MSC_VER
#define STBI_NOTUSED(v)  (void)(v)
#else
#define STBI_NOTUSED(v)  (void)sizeof(v)
#endif

#ifdef _MSC_VER
   #define stbi_lrot(x,y)  _lrotl(x,y)
#else
   #define stbi_lrot(x,y)  (((x) << (y)) | ((x) >> (32 - (y))))
#endif

inline uint8_t get8(std::istream& in) {
    uint8_t r;
    in.read((char*)&r, sizeof(r));
    return r;
}

inline int get16be(std::istream& in) {
    return (get8(in) << 8) + get8(in);
}

inline uint32_t get32be(std::istream& in) {
    return (get16be(in) << 16) + get16be(in);
}

inline void skip(std::istream& in, int n) {
    in.seekg(in.tellg() + std::streamoff(n));
}


enum {
   STBI__SCAN_load=0,
   STBI__SCAN_type,
   STBI__SCAN_header
};

static uint8_t stbi__compute_y(int r, int g, int b) {
   return (uint8_t) (((r*77) + (g*150) +  (29*b)) >> 8);
}

//////////////////////////////////////////////////////////////////////////////
//
//  "baseline" JPEG/JFIF decoder
//
//    simple implementation
//      - doesn't support delayed output of y-dimension
//      - simple interface (only one output format: 8-bit interleaved RGB)
//      - doesn't try to recover corrupt jpegs
//      - doesn't allow partial loading, loading multiple at once
//      - still fast on x86 (copying globals into locals doesn't help x86)
//      - allocates lots of intermediate memory (full size of all components)
//        - non-interleaved case requires this anyway
//        - allows good upsampling (see next)
//    high-quality
//      - upsampled channels are bilinearly interpolated, even across blocks
//      - quality integer IDCT derived from IJG's 'slow'
//    performance
//      - fast huffman; reasonable integer IDCT
//      - uses a lot of intermediate memory, could cache poorly

// huffman decoding acceleration
#define FAST_BITS   9  // larger handles more cases; smaller stomps less cache

struct stbi__huffman {
   uint8_t  fast[1 << FAST_BITS];
   // weirdly, repacking this into AoS is a 10% speed loss, instead of a win
   uint16_t code[256];
   uint8_t  values[256];
   uint8_t  size[257];
   unsigned int maxcode[18];
   int    delta[17];   // old 'firstsymbol' - old 'firstcode'
};

struct stbi__jpeg {
    struct {
        uint32_t img_x, img_y;
        int img_n, img_out_n;
    }os;

   stbi__huffman huff_dc[4];
   stbi__huffman huff_ac[4];
   uint16_t dequant[4][64];
   int16_t fast_ac[4][1 << FAST_BITS];

// sizes for components, interleaved MCUs
   int img_h_max, img_v_max;
   int img_mcu_x, img_mcu_y;
   int img_mcu_w, img_mcu_h;

// definition of jpeg image component
   struct
   {
      int id;
      int h,v;
      int tq;
      int hd,ha;
      int dc_pred;

      int x,y,w2,h2;
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
} ;

static int stbi__build_huffman(stbi__huffman *h, int *count) {
   int i,j,k=0;
   unsigned int code;
   // build size list for each symbol (from JPEG spec)
   for (i=0; i < 16; ++i)
      for (j=0; j < count[i]; ++j)
         h->size[k++] = (uint8_t) (i+1);
   h->size[k] = 0;

   // compute actual symbols (from jpeg spec)
   code = 0;
   k = 0;
   for(j=1; j <= 16; ++j) {
      // compute delta to add to code to compute symbol id
      h->delta[j] = k - code;
      if (h->size[k] == j) {
         while (h->size[k] == j)
            h->code[k++] = (uint16_t) (code++);
         if (code-1 >= (1u << j)) THROW_EXCEPTION("Corrupt JPEG: bad code lengths");
      }
      // compute largest code + 1 for this size, preshifted as needed later
      h->maxcode[j] = code << (16-j);
      code <<= 1;
   }
   h->maxcode[j] = 0xffffffff;

   // build non-spec acceleration table; 255 is flag for not-accelerated
   memset(h->fast, 255, 1 << FAST_BITS);
   for (i=0; i < k; ++i) {
      int s = h->size[i];
      if (s <= FAST_BITS) {
         int c = h->code[i] << (FAST_BITS-s);
         int m = 1 << (FAST_BITS-s);
         for (j=0; j < m; ++j) {
            h->fast[c+j] = (uint8_t) i;
         }
      }
   }
   return 1;
}

// build a table that decodes both magnitude and value of small ACs in
// one go.
static void stbi__build_fast_ac(int16_t *fast_ac, stbi__huffman *h) {
   for (int i=0; i < (1 << FAST_BITS); ++i) {
      uint8_t fast = h->fast[i];
      fast_ac[i] = 0;
      if (fast < 255) {
         int rs = h->values[fast];
         int run = (rs >> 4) & 15;
         int magbits = rs & 15;
         int len = h->size[fast];

         if (magbits && len + magbits <= FAST_BITS) {
            // magnitude code followed by receive_extend code
            int k = ((i << len) & ((1 << FAST_BITS) - 1)) >> (FAST_BITS - magbits);
            int m = 1 << (magbits - 1);
            if (k < m) k += (~0U << magbits) + 1;
            // if the result is small enough, we can fit it in fast_ac table
            if (k >= -128 && k <= 127)
               fast_ac[i] = (int16_t) ((k * 256) + (run * 16) + (len + magbits));
         }
      }
   }
}

static void stbi__grow_buffer_unsafe(std::istream& in, stbi__jpeg& j) {
   do {
      unsigned int b = j.nomore ? 0 : get8(in);
      if (b == 0xff) {
         int c = get8(in);
         while (c == 0xff) c = get8(in); // consume fill bytes
         if (c != 0) {
            j.marker = (unsigned char) c;
            j.nomore = 1;
            return;
         }
      }
      j.code_buffer |= b << (24 - j.code_bits);
      j.code_bits += 8;
   } while (j.code_bits <= 24);
}

// (1 << n) - 1
static const uint32_t stbi__bmask[17]={0,1,3,7,15,31,63,127,255,511,1023,2047,4095,8191,16383,32767,65535};

// decode a jpeg huffman value from the bitstream
inline static int stbi__jpeg_huff_decode(std::istream& in, stbi__jpeg& j, stbi__huffman *h)
{
   unsigned int temp;
   int c,k;

   if (j.code_bits < 16) stbi__grow_buffer_unsafe(in, j);

   // look at the top FAST_BITS and determine what symbol ID it is,
   // if the code is <= FAST_BITS
   c = (j.code_buffer >> (32 - FAST_BITS)) & ((1 << FAST_BITS)-1);
   k = h->fast[c];
   if (k < 255) {
      int s = h->size[k];
      if (s > j.code_bits)
         return -1;
      j.code_buffer <<= s;
      j.code_bits -= s;
      return h->values[k];
   }

   // naive test is to shift the code_buffer down so k bits are
   // valid, then test against maxcode. To speed this up, we've
   // preshifted maxcode left so that it has (16-k) 0s at the
   // end; in other words, regardless of the number of bits, it
   // wants to be compared against something shifted to have 16;
   // that way we don't need to shift inside the loop.
   temp = j.code_buffer >> 16;
   for (k=FAST_BITS+1 ; ; ++k)
      if (temp < h->maxcode[k])
         break;
   if (k == 17) {
      // error! code not found
      j.code_bits -= 16;
      return -1;
   }

   if (k > j.code_bits)
      return -1;

   // convert the huffman code to the symbol id
   c = ((j.code_buffer >> (32 - k)) & stbi__bmask[k]) + h->delta[k];
   assert((((j.code_buffer) >> (32 - h->size[c])) & stbi__bmask[h->size[c]]) == h->code[c]);

   // convert the id to a symbol
   j.code_bits -= k;
   j.code_buffer <<= k;
   return h->values[c];
}

// bias[n] = (-1<<n) + 1
static const int stbi__jbias[16] = {0,-1,-3,-7,-15,-31,-63,-127,-255,-511,-1023,-2047,-4095,-8191,-16383,-32767};

// combined JPEG 'receive' and JPEG 'extend', since baseline
// always extends everything it receives.
inline static int stbi__extend_receive(std::istream& in, stbi__jpeg& j, int n)
{
   unsigned int k;
   int sgn;
   if (j.code_bits < n) stbi__grow_buffer_unsafe(in, j);

   sgn = (int32_t)j.code_buffer >> 31; // sign bit is always in MSB
   k = stbi_lrot(j.code_buffer, n);
   assert(n >= 0 && n < (int) (sizeof(stbi__bmask)/sizeof(*stbi__bmask)));
   j.code_buffer = k & ~stbi__bmask[n];
   k &= stbi__bmask[n];
   j.code_bits -= n;
   return k + (stbi__jbias[n] & ~sgn);
}

// get some unsigned bits
inline static int stbi__jpeg_get_bits(std::istream& in, stbi__jpeg& j, int n)
{
   unsigned int k;
   if (j.code_bits < n) stbi__grow_buffer_unsafe(in, j);
   k = stbi_lrot(j.code_buffer, n);
   j.code_buffer = k & ~stbi__bmask[n];
   k &= stbi__bmask[n];
   j.code_bits -= n;
   return k;
}

inline static int stbi__jpeg_get_bit(std::istream& in, stbi__jpeg& j)
{
   unsigned int k;
   if (j.code_bits < 1) stbi__grow_buffer_unsafe(in, j);
   k = j.code_buffer;
   j.code_buffer <<= 1;
   --j.code_bits;
   return k & 0x80000000;
}

// given a value that's at position X in the zigzag stream,
// where does it appear in the 8x8 matrix coded as row-major?
static const uint8_t stbi__jpeg_dezigzag[64+15] =
{
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
static int stbi__jpeg_decode_block(std::istream& in, stbi__jpeg& j, short data[64], stbi__huffman *hdc, stbi__huffman *hac, int16_t *fac, int b, uint16_t *dequant)
{
   int diff,dc,k;
   int t;

   if (j.code_bits < 16) stbi__grow_buffer_unsafe(in, j);
   t = stbi__jpeg_huff_decode(in, j, hdc);
   if (t < 0) THROW_EXCEPTION("Corrupt JPEG: bad huffman code");

   // 0 all the ac values now so we can do it 32-bits at a time
   memset(data,0,64*sizeof(data[0]));

   diff = t ? stbi__extend_receive(in, j, t) : 0;
   dc = j.img_comp[b].dc_pred + diff;
   j.img_comp[b].dc_pred = dc;
   data[0] = (short) (dc * dequant[0]);

   // decode AC components, see JPEG spec
   k = 1;
   do {
      unsigned int zig;
      int c,r,s;
      if (j.code_bits < 16) stbi__grow_buffer_unsafe(in, j);
      c = (j.code_buffer >> (32 - FAST_BITS)) & ((1 << FAST_BITS)-1);
      r = fac[c];
      if (r) { // fast-AC path
         k += (r >> 4) & 15; // run
         s = r & 15; // combined length
         j.code_buffer <<= s;
         j.code_bits -= s;
         // decode into unzigzag'd location
         zig = stbi__jpeg_dezigzag[k++];
         data[zig] = (short) ((r >> 8) * dequant[zig]);
      } else {
         int rs = stbi__jpeg_huff_decode(in, j, hac);
         if (rs < 0) THROW_EXCEPTION("Corrupt JPEG: bad huffman code");
         s = rs & 15;
         r = rs >> 4;
         if (s == 0) {
            if (rs != 0xf0) break; // end block
            k += 16;
         } else {
            k += r;
            // decode into unzigzag'd location
            zig = stbi__jpeg_dezigzag[k++];
            data[zig] = (short) (stbi__extend_receive(in, j,s) * dequant[zig]);
         }
      }
   } while (k < 64);
   return 1;
}

static int stbi__jpeg_decode_block_prog_dc(std::istream& in, stbi__jpeg& j, short data[64], stbi__huffman *hdc, int b)
{
   int diff,dc;
   int t;
   if (j.spec_end != 0) THROW_EXCEPTION("Corrupt JPEG: can't merge dc and ac");

   if (j.code_bits < 16) stbi__grow_buffer_unsafe(in, j);

   if (j.succ_high == 0) {
      // first scan for DC coefficient, must be first
      memset(data,0,64*sizeof(data[0])); // 0 all the ac values now
      t = stbi__jpeg_huff_decode(in, j, hdc);
      diff = t ? stbi__extend_receive(in, j, t) : 0;

      dc = j.img_comp[b].dc_pred + diff;
      j.img_comp[b].dc_pred = dc;
      data[0] = (short) (dc << j.succ_low);
   } else {
      // refinement scan for DC coefficient
      if (stbi__jpeg_get_bit(in, j))
         data[0] += (short) (1 << j.succ_low);
   }
   return 1;
}

// @OPTIMIZE: store non-zigzagged during the decode passes,
// and only de-zigzag when dequantizing
static int stbi__jpeg_decode_block_prog_ac(std::istream& in, stbi__jpeg& j, short data[64], stbi__huffman *hac, int16_t *fac)
{
   int k;
   if (j.spec_start == 0) THROW_EXCEPTION("Corrupt JPEG: can't merge dc and ac");

   if (j.succ_high == 0) {
      int shift = j.succ_low;

      if (j.eob_run) {
         --j.eob_run;
         return 1;
      }

      k = j.spec_start;
      do {
         unsigned int zig;
         int c,r,s;
         if (j.code_bits < 16) stbi__grow_buffer_unsafe(in, j);
         c = (j.code_buffer >> (32 - FAST_BITS)) & ((1 << FAST_BITS)-1);
         r = fac[c];
         if (r) { // fast-AC path
            k += (r >> 4) & 15; // run
            s = r & 15; // combined length
            j.code_buffer <<= s;
            j.code_bits -= s;
            zig = stbi__jpeg_dezigzag[k++];
            data[zig] = (short) ((r >> 8) << shift);
         } else {
            int rs = stbi__jpeg_huff_decode(in, j, hac);
            if (rs < 0) THROW_EXCEPTION("Corrupt JPEG: bad huffman code");
            s = rs & 15;
            r = rs >> 4;
            if (s == 0) {
               if (r < 15) {
                  j.eob_run = (1 << r);
                  if (r)
                     j.eob_run += stbi__jpeg_get_bits(in, j, r);
                  --j.eob_run;
                  break;
               }
               k += 16;
            } else {
               k += r;
               zig = stbi__jpeg_dezigzag[k++];
               data[zig] = (short) (stbi__extend_receive(in, j,s) << shift);
            }
         }
      } while (k <= j.spec_end);
   } else {
      // refinement scan for these AC coefficients

      short bit = (short) (1 << j.succ_low);

      if (j.eob_run) {
         --j.eob_run;
         for (k = j.spec_start; k <= j.spec_end; ++k) {
            short *p = &data[stbi__jpeg_dezigzag[k]];
            if (*p != 0)
               if (stbi__jpeg_get_bit(in, j))
                  if ((*p & bit)==0) {
                     if (*p > 0)
                        *p += bit;
                     else
                        *p -= bit;
                  }
         }
      } else {
         k = j.spec_start;
         do {
            int r,s;
            int rs = stbi__jpeg_huff_decode(in, j, hac); // @OPTIMIZE see if we can use the fast path here, advance-by-r is so slow, eh
            if (rs < 0) THROW_EXCEPTION("Corrupt JPEG: bad huffman code");
            s = rs & 15;
            r = rs >> 4;
            if (s == 0) {
               if (r < 15) {
                  j.eob_run = (1 << r) - 1;
                  if (r)
                     j.eob_run += stbi__jpeg_get_bits(in, j, r);
                  r = 64; // force end of block
               } else {
                  // r=15 s=0 should write 16 0s, so we just do
                  // a run of 15 0s and then write s (which is 0),
                  // so we don't have to do anything special here
               }
            } else {
               if (s != 1) THROW_EXCEPTION("Corrupt JPEG: bad huffman code");
               // sign bit
               if (stbi__jpeg_get_bit(in, j))
                  s = bit;
               else
                  s = -bit;
            }

            // advance by r
            while (k <= j.spec_end) {
               short *p = &data[stbi__jpeg_dezigzag[k++]];
               if (*p != 0) {
                  if (stbi__jpeg_get_bit(in, j))
                     if ((*p & bit)==0) {
                        if (*p > 0)
                           *p += bit;
                        else
                           *p -= bit;
                     }
               } else {
                  if (r == 0) {
                     *p = (short) s;
                     break;
                  }
                  --r;
               }
            }
         } while (k <= j.spec_end);
      }
   }
   return 1;
}

#define stbi__f2f(x)  ((int) (((x) * 4096 + 0.5)))
#define stbi__fsh(x)  ((x) * 4096)

// derived from jidctint -- DCT_ISLOW
#define STBI__IDCT_1D(s0,s1,s2,s3,s4,s5,s6,s7) \
   int t0,t1,t2,t3,p1,p2,p3,p4,p5,x0,x1,x2,x3; \
   p2 = s2;                                    \
   p3 = s6;                                    \
   p1 = (p2+p3) * stbi__f2f(0.5411961f);       \
   t2 = p1 + p3*stbi__f2f(-1.847759065f);      \
   t3 = p1 + p2*stbi__f2f( 0.765366865f);      \
   p2 = s0;                                    \
   p3 = s4;                                    \
   t0 = stbi__fsh(p2+p3);                      \
   t1 = stbi__fsh(p2-p3);                      \
   x0 = t0+t3;                                 \
   x3 = t0-t3;                                 \
   x1 = t1+t2;                                 \
   x2 = t1-t2;                                 \
   t0 = s7;                                    \
   t1 = s5;                                    \
   t2 = s3;                                    \
   t3 = s1;                                    \
   p3 = t0+t2;                                 \
   p4 = t1+t3;                                 \
   p1 = t0+t3;                                 \
   p2 = t1+t2;                                 \
   p5 = (p3+p4)*stbi__f2f( 1.175875602f);      \
   t0 = t0*stbi__f2f( 0.298631336f);           \
   t1 = t1*stbi__f2f( 2.053119869f);           \
   t2 = t2*stbi__f2f( 3.072711026f);           \
   t3 = t3*stbi__f2f( 1.501321110f);           \
   p1 = p5 + p1*stbi__f2f(-0.899976223f);      \
   p2 = p5 + p2*stbi__f2f(-2.562915447f);      \
   p3 = p3*stbi__f2f(-1.961570560f);           \
   p4 = p4*stbi__f2f(-0.390180644f);           \
   t3 += p1+p4;                                \
   t2 += p2+p3;                                \
   t1 += p2+p4;                                \
   t0 += p1+p3;

static void stbi__idct_block(uint8_t *out, int out_stride, short data[64]) {
   int i,val[64],*v=val;
   uint8_t *o;
   short *d = data;

   // columns
   for (i=0; i < 8; ++i,++d, ++v) {
      // if all zeroes, shortcut -- this avoids dequantizing 0s and IDCTing
      if (d[ 8]==0 && d[16]==0 && d[24]==0 && d[32]==0
           && d[40]==0 && d[48]==0 && d[56]==0) {
         //    no shortcut                 0     seconds
         //    (1|2|3|4|5|6|7)==0          0     seconds
         //    all separate               -0.047 seconds
         //    1 && 2|3 && 4|5 && 6|7:    -0.047 seconds
         int dcterm = d[0]*4;
         v[0] = v[8] = v[16] = v[24] = v[32] = v[40] = v[48] = v[56] = dcterm;
      } else {
         STBI__IDCT_1D(d[ 0],d[ 8],d[16],d[24],d[32],d[40],d[48],d[56])
         // constants scaled things up by 1<<12; let's bring them back
         // down, but keep 2 extra bits of precision
         x0 += 512; x1 += 512; x2 += 512; x3 += 512;
         v[ 0] = (x0+t3) >> 10;
         v[56] = (x0-t3) >> 10;
         v[ 8] = (x1+t2) >> 10;
         v[48] = (x1-t2) >> 10;
         v[16] = (x2+t1) >> 10;
         v[40] = (x2-t1) >> 10;
         v[24] = (x3+t0) >> 10;
         v[32] = (x3-t0) >> 10;
      }
   }

   for (i=0, v=val, o=out; i < 8; ++i,v+=8,o+=out_stride) {
      // no fast case since the first 1D IDCT spread components out
      STBI__IDCT_1D(v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7])
      // constants scaled things up by 1<<12, plus we had 1<<2 from first
      // loop, plus horizontal and vertical each scale by sqrt(8) so together
      // we've got an extra 1<<3, so 1<<17 total we need to remove.
      // so we want to round that, which means adding 0.5 * 1<<17,
      // aka 65536. Also, we'll end up with -128 to 127 that we want
      // to encode as 0..255 by adding 128, so we'll add that before the shift
      x0 += 65536 + (128<<17);
      x1 += 65536 + (128<<17);
      x2 += 65536 + (128<<17);
      x3 += 65536 + (128<<17);
      // tried computing the shifts into temps, or'ing the temps to see
      // if any were out of range, but that was slower
      o[0] = c4::clamp<uint8_t>((x0+t3) >> 17);
      o[7] = c4::clamp<uint8_t>((x0-t3) >> 17);
      o[1] = c4::clamp<uint8_t>((x1+t2) >> 17);
      o[6] = c4::clamp<uint8_t>((x1-t2) >> 17);
      o[2] = c4::clamp<uint8_t>((x2+t1) >> 17);
      o[5] = c4::clamp<uint8_t>((x2-t1) >> 17);
      o[3] = c4::clamp<uint8_t>((x3+t0) >> 17);
      o[4] = c4::clamp<uint8_t>((x3-t0) >> 17);
   }
}


#define STBI__MARKER_none  0xff
// if there's a pending marker from the entropy stream, return that
// otherwise, fetch from the stream and get a marker. if there's no
// marker, return 0xff, which is never a valid marker value
static uint8_t stbi__get_marker(std::istream& in, stbi__jpeg& j) {
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
#define STBI__RESTART(x)     ((x) >= 0xd0 && (x) <= 0xd7)

// after a restart interval, stbi__jpeg_reset the entropy decoder and
// the dc prediction
static void stbi__jpeg_reset(stbi__jpeg& j) {
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

static int stbi__parse_entropy_coded_data(std::istream& in, stbi__jpeg& z) {
   stbi__jpeg_reset(z);
   if (!z.progressive) {
      if (z.scan_n == 1) {
         int i,j;
         short data[64];
         int n = z.order[0];
         // non-interleaved data, we just need to process one block at a time,
         // in trivial scanline order
         // number of blocks to do just depends on how many actual "pixels" this
         // component has, independent of interleaved MCU blocking and such
         int w = (z.img_comp[n].x+7) >> 3;
         int h = (z.img_comp[n].y+7) >> 3;
         for (j=0; j < h; ++j) {
            for (i=0; i < w; ++i) {
               int ha = z.img_comp[n].ha;
               if (!stbi__jpeg_decode_block(in, z, data, z.huff_dc+z.img_comp[n].hd, z.huff_ac+ha, z.fast_ac[ha], n, z.dequant[z.img_comp[n].tq])) return 0;
               stbi__idct_block(z.img_comp[n].data.data()+z.img_comp[n].w2*j*8+i*8, z.img_comp[n].w2, data);
               // every data block is an MCU, so countdown the restart interval
               if (--z.todo <= 0) {
                  if (z.code_bits < 24) stbi__grow_buffer_unsafe(in, z);
                  // if it's NOT a restart, then just bail, so we get corrupt data
                  // rather than no data
                  if (!STBI__RESTART(z.marker)) return 1;
                  stbi__jpeg_reset(z);
               }
            }
         }
         return 1;
      } else { // interleaved
         int i,j,k,x,y;
         short data[64];
         for (j=0; j < z.img_mcu_y; ++j) {
            for (i=0; i < z.img_mcu_x; ++i) {
               // scan an interleaved mcu... process scan_n components in order
               for (k=0; k < z.scan_n; ++k) {
                  int n = z.order[k];
                  // scan out an mcu's worth of this component; that's just determined
                  // by the basic H and V specified for the component
                  for (y=0; y < z.img_comp[n].v; ++y) {
                     for (x=0; x < z.img_comp[n].h; ++x) {
                        int x2 = (i*z.img_comp[n].h + x)*8;
                        int y2 = (j*z.img_comp[n].v + y)*8;
                        int ha = z.img_comp[n].ha;
                        if (!stbi__jpeg_decode_block(in, z, data, z.huff_dc+z.img_comp[n].hd, z.huff_ac+ha, z.fast_ac[ha], n, z.dequant[z.img_comp[n].tq])) return 0;
                        stbi__idct_block(z.img_comp[n].data.data() +z.img_comp[n].w2*y2+x2, z.img_comp[n].w2, data);
                     }
                  }
               }
               // after all interleaved components, that's an interleaved MCU,
               // so now count down the restart interval
               if (--z.todo <= 0) {
                  if (z.code_bits < 24) stbi__grow_buffer_unsafe(in, z);
                  if (!STBI__RESTART(z.marker)) return 1;
                  stbi__jpeg_reset(z);
               }
            }
         }
         return 1;
      }
   } else {
      if (z.scan_n == 1) {
         int i,j;
         int n = z.order[0];
         // non-interleaved data, we just need to process one block at a time,
         // in trivial scanline order
         // number of blocks to do just depends on how many actual "pixels" this
         // component has, independent of interleaved MCU blocking and such
         int w = (z.img_comp[n].x+7) >> 3;
         int h = (z.img_comp[n].y+7) >> 3;
         for (j=0; j < h; ++j) {
            for (i=0; i < w; ++i) {
               short *data = z.img_comp[n].coeff.data() + 64 * (i + j * z.img_comp[n].coeff_w);
               if (z.spec_start == 0) {
                  if (!stbi__jpeg_decode_block_prog_dc(in, z, data, &z.huff_dc[z.img_comp[n].hd], n))
                     return 0;
               } else {
                  int ha = z.img_comp[n].ha;
                  if (!stbi__jpeg_decode_block_prog_ac(in, z, data, &z.huff_ac[ha], z.fast_ac[ha]))
                     return 0;
               }
               // every data block is an MCU, so countdown the restart interval
               if (--z.todo <= 0) {
                  if (z.code_bits < 24) stbi__grow_buffer_unsafe(in, z);
                  if (!STBI__RESTART(z.marker)) return 1;
                  stbi__jpeg_reset(z);
               }
            }
         }
         return 1;
      } else { // interleaved
         int i,j,k,x,y;
         for (j=0; j < z.img_mcu_y; ++j) {
            for (i=0; i < z.img_mcu_x; ++i) {
               // scan an interleaved mcu... process scan_n components in order
               for (k=0; k < z.scan_n; ++k) {
                  int n = z.order[k];
                  // scan out an mcu's worth of this component; that's just determined
                  // by the basic H and V specified for the component
                  for (y=0; y < z.img_comp[n].v; ++y) {
                     for (x=0; x < z.img_comp[n].h; ++x) {
                        int x2 = (i*z.img_comp[n].h + x);
                        int y2 = (j*z.img_comp[n].v + y);
                        short *data = z.img_comp[n].coeff.data() + 64 * (x2 + y2 * z.img_comp[n].coeff_w);
                        if (!stbi__jpeg_decode_block_prog_dc(in, z, data, &z.huff_dc[z.img_comp[n].hd], n))
                           return 0;
                     }
                  }
               }
               // after all interleaved components, that's an interleaved MCU,
               // so now count down the restart interval
               if (--z.todo <= 0) {
                  if (z.code_bits < 24) stbi__grow_buffer_unsafe(in, z);
                  if (!STBI__RESTART(z.marker)) return 1;
                  stbi__jpeg_reset(z);
               }
            }
         }
         return 1;
      }
   }
}

static void stbi__jpeg_dequantize(short *data, uint16_t *dequant) {
   for (int i=0; i < 64; ++i)
      data[i] *= dequant[i];
}

static void stbi__jpeg_finish(stbi__jpeg& z) {
   if (z.progressive) {
      // dequantize and idct the data
      int i,j,n;
      for (n=0; n < z.os.img_n; ++n) {
         int w = (z.img_comp[n].x+7) >> 3;
         int h = (z.img_comp[n].y+7) >> 3;
         for (j=0; j < h; ++j) {
            for (i=0; i < w; ++i) {
               short *data = z.img_comp[n].coeff.data() + 64 * (i + j * z.img_comp[n].coeff_w);
               stbi__jpeg_dequantize(data, z.dequant[z.img_comp[n].tq]);
               stbi__idct_block(z.img_comp[n].data.data() +z.img_comp[n].w2*j*8+i*8, z.img_comp[n].w2, data);
            }
         }
      }
   }
}

static int stbi__process_marker(std::istream& in, stbi__jpeg& z, int m) {
   int L;
   switch (m) {
      case STBI__MARKER_none: // no marker found
          THROW_EXCEPTION("Corrupt JPEG: expected marker");

      case 0xDD: // DRI - specify restart interval
         if (get16be(in) != 4) THROW_EXCEPTION("Corrupt JPEG: bad DRI len");
         z.restart_interval = get16be(in);
         return 1;

      case 0xDB: // DQT - define quantization table
         L = get16be(in)-2;
         while (L > 0) {
            int q = get8(in);
            int p = q >> 4, sixteen = (p != 0);
            int t = q & 15,i;
            if (p != 0 && p != 1) THROW_EXCEPTION("Corrupt JPEG: bad DQT type");
            if (t > 3) THROW_EXCEPTION("Corrupt JPEG: bad DQT table");

            for (i=0; i < 64; ++i)
               z.dequant[t][stbi__jpeg_dezigzag[i]] = (uint16_t)(sixteen ? get16be(in) : get8(in));
            L -= (sixteen ? 129 : 65);
         }
         return L==0;

      case 0xC4: // DHT - define huffman table
         L = get16be(in)-2;
         while (L > 0) {
            uint8_t *v;
            int sizes[16],i,n=0;
            int q = get8(in);
            int tc = q >> 4;
            int th = q & 15;
            if (tc > 1 || th > 3) THROW_EXCEPTION("Corrupt JPEG: bad DHT header");
            for (i=0; i < 16; ++i) {
               sizes[i] = get8(in);
               n += sizes[i];
            }
            L -= 17;
            if (tc == 0) {
               if (!stbi__build_huffman(z.huff_dc+th, sizes)) return 0;
               v = z.huff_dc[th].values;
            } else {
               if (!stbi__build_huffman(z.huff_ac+th, sizes)) return 0;
               v = z.huff_ac[th].values;
            }
            for (i=0; i < n; ++i)
               v[i] = get8(in);
            if (tc != 0)
               stbi__build_fast_ac(z.fast_ac[th], z.huff_ac + th);
            L -= n;
         }
         return L==0;
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
         static const unsigned char tag[5] = {'J','F','I','F','\0'};
         int ok = 1;
         int i;
         for (i=0; i < 5; ++i)
            if (get8(in) != tag[i])
               ok = 0;
         L -= 5;
         if (ok)
            z.jfif = 1;
      } else if (m == 0xEE && L >= 12) { // Adobe APP14 segment
         static const unsigned char tag[6] = {'A','d','o','b','e','\0'};
         int ok = 1;
         int i;
         for (i=0; i < 6; ++i)
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
      return 1;
   }

   THROW_EXCEPTION("Corrupt JPEG: unknown marker");
}

// after we see SOS
static int stbi__process_scan_header(std::istream& in, stbi__jpeg& z) {
   int i;
   int Ls = get16be(in);
   z.scan_n = get8(in);
   if (z.scan_n < 1 || z.scan_n > 4 || z.scan_n > (int) z.os.img_n) THROW_EXCEPTION("Corrupt JPEG: bad SOS component count");
   if (Ls != 6+2*z.scan_n) THROW_EXCEPTION("Corrupt JPEG: bad SOS len");
   for (i=0; i < z.scan_n; ++i) {
      int id = get8(in), which;
      int q = get8(in);
      for (which = 0; which < z.os.img_n; ++which)
         if (z.img_comp[which].id == id)
            break;
      if (which == z.os.img_n) return 0; // no match
      z.img_comp[which].hd = q >> 4;   if (z.img_comp[which].hd > 3) THROW_EXCEPTION("Corrupt JPEG: bad DC huff");
      z.img_comp[which].ha = q & 15;   if (z.img_comp[which].ha > 3) THROW_EXCEPTION("Corrupt JPEG: bad AC huff");
      z.order[i] = which;
   }

   {
      z.spec_start = get8(in);
      z.spec_end   = get8(in); // should be 63, but might be 0
      int aa = get8(in);
      z.succ_high = (aa >> 4);
      z.succ_low  = (aa & 15);
      if (z.progressive) {
         if (z.spec_start > 63 || z.spec_end > 63  || z.spec_start > z.spec_end || z.succ_high > 13 || z.succ_low > 13)
             THROW_EXCEPTION("Corrupt JPEG: bad SOS");
      } else {
         if (z.spec_start != 0 || z.succ_high != 0 || z.succ_low != 0) THROW_EXCEPTION("Corrupt JPEG: bad SOS");
         z.spec_end = 63;
      }
   }

   return 1;
}

static int stbi__process_frame_header(std::istream& in, stbi__jpeg& z, int scan)
{
   auto *os = &z.os;
   int Lf,p,i,q, h_max=1,v_max=1,c;
   Lf = get16be(in);         if (Lf < 11) THROW_EXCEPTION("Corrupt JPEG: bad SOF len"); // JPEG
   p  = get8(in);            if (p != 8) THROW_EXCEPTION("JPEG format not supported: 8-bit only"); // JPEG baseline
   os->img_y = get16be(in);   if (os->img_y == 0) THROW_EXCEPTION("JPEG format not supported: delayed height"); // Legal, but we don't handle it--but neither does IJG
   os->img_x = get16be(in);   if (os->img_x == 0) THROW_EXCEPTION("Corrupt JPEG: 0 width"); // JPEG requires
   c = get8(in);
   if (c != 3 && c != 1 && c != 4) THROW_EXCEPTION("Corrupt JPEG: bad component count");
   os->img_n = c;
   for (i=0; i < c; ++i) {
      z.img_comp[i].data.clear();
      z.img_comp[i].linebuf.clear();
   }

   if (Lf != 8+3*os->img_n) THROW_EXCEPTION("Corrupt JPEG: bad SOF len");

   z.rgb = 0;
   for (i=0; i < os->img_n; ++i) {
      static const unsigned char rgb[3] = { 'R', 'G', 'B' };
      z.img_comp[i].id = get8(in);
      if (os->img_n == 3 && z.img_comp[i].id == rgb[i])
         ++z.rgb;
      q = get8(in);
      z.img_comp[i].h = (q >> 4);  if (!z.img_comp[i].h || z.img_comp[i].h > 4) THROW_EXCEPTION("Corrupt JPEG: bad H");
      z.img_comp[i].v = q & 15;    if (!z.img_comp[i].v || z.img_comp[i].v > 4) THROW_EXCEPTION("Corrupt JPEG: bad V");
      z.img_comp[i].tq = get8(in);  if (z.img_comp[i].tq > 3) THROW_EXCEPTION("Corrupt JPEG: bad TQ");
   }

   if (scan != STBI__SCAN_load) return 1;

   for (i=0; i < os->img_n; ++i) {
      if (z.img_comp[i].h > h_max) h_max = z.img_comp[i].h;
      if (z.img_comp[i].v > v_max) v_max = z.img_comp[i].v;
   }

   // compute interleaved mcu info
   z.img_h_max = h_max;
   z.img_v_max = v_max;
   z.img_mcu_w = h_max * 8;
   z.img_mcu_h = v_max * 8;
   // these sizes can't be more than 17 bits
   z.img_mcu_x = (os->img_x + z.img_mcu_w-1) / z.img_mcu_w;
   z.img_mcu_y = (os->img_y + z.img_mcu_h-1) / z.img_mcu_h;

   for (i=0; i < os->img_n; ++i) {
      // number of effective pixels (e.g. for non-interleaved MCU)
      z.img_comp[i].x = (os->img_x * z.img_comp[i].h + h_max-1) / h_max;
      z.img_comp[i].y = (os->img_y * z.img_comp[i].v + v_max-1) / v_max;
      // to simplify generation, we'll allocate enough memory to decode
      // the bogus oversized data from using interleaved MCUs and their
      // big blocks (e.g. a 16x16 iMCU on an image of width 33); we won't
      // discard the extra data until colorspace conversion
      //
      // img_mcu_x, img_mcu_y: <=17 bits; comp[i].h and .v are <=4 (checked earlier)
      // so these muls can't overflow with 32-bit ints (which we require)
      z.img_comp[i].w2 = z.img_mcu_x * z.img_comp[i].h * 8;
      z.img_comp[i].h2 = z.img_mcu_y * z.img_comp[i].v * 8;
      z.img_comp[i].coeff.clear();
      z.img_comp[i].linebuf.clear();
      z.img_comp[i].data.resize(z.img_comp[i].w2 * z.img_comp[i].h2);
      if (z.progressive) {
         // w2, h2 are multiples of 8 (see above)
         z.img_comp[i].coeff_w = z.img_comp[i].w2 / 8;
         z.img_comp[i].coeff_h = z.img_comp[i].h2 / 8;
         z.img_comp[i].coeff.resize(z.img_comp[i].w2 * z.img_comp[i].h2);
      }
   }

   return 1;
}

// use comparisons since in some cases we handle more than one case (e.g. SOF)
#define stbi__DNL(x)         ((x) == 0xdc)
#define stbi__SOI(x)         ((x) == 0xd8)
#define stbi__EOI(x)         ((x) == 0xd9)
#define stbi__SOF(x)         ((x) == 0xc0 || (x) == 0xc1 || (x) == 0xc2)
#define stbi__SOS(x)         ((x) == 0xda)

#define stbi__SOF_progressive(x)   ((x) == 0xc2)

static int stbi__decode_jpeg_header(std::istream& in, stbi__jpeg& z, int scan)
{
   z.jfif = 0;
   z.app14_color_transform = -1; // valid values are 0,1,2
   z.marker = STBI__MARKER_none; // initialize cached marker to empty
   int m = stbi__get_marker(in, z);
   if (!stbi__SOI(m)) THROW_EXCEPTION("Corrupt JPEG: no SOI");
   if (scan == STBI__SCAN_type) return 1;
   m = stbi__get_marker(in, z);
   while (!stbi__SOF(m)) {
      if (!stbi__process_marker(in, z,m)) return 0;
      m = stbi__get_marker(in, z);
      while (m == STBI__MARKER_none) {
         // some files have extra padding after their blocks, so ok, we'll scan
         if (in.eof())THROW_EXCEPTION("Corrupt JPEG: no SOF");
         m = stbi__get_marker(in, z);
      }
   }
   z.progressive = stbi__SOF_progressive(m);
   if (!stbi__process_frame_header(in, z, scan)) return 0;
   return 1;
}

// decode image to YCbCr format
static int stbi__decode_jpeg_image(std::istream& in, stbi__jpeg& j)
{
   int m;
   for (m = 0; m < 4; m++) {
      j.img_comp[m].data.clear();
      j.img_comp[m].coeff.clear();
   }
   j.restart_interval = 0;
   if (!stbi__decode_jpeg_header(in, j, STBI__SCAN_load)) return 0;
   m = stbi__get_marker(in, j);
   while (!stbi__EOI(m)) {
      if (stbi__SOS(m)) {
         if (!stbi__process_scan_header(in, j)) return 0;
         if (!stbi__parse_entropy_coded_data(in, j)) return 0;
         if (j.marker == STBI__MARKER_none ) {
            // handle 0s at the end of image data from IP Kamera 9060
            while (!in.eof()) {
               int x = get8(in);
               if (x == 255) {
                  j.marker = get8(in);
                  break;
               }
            }
            // if we reach eof without hitting a marker, stbi__get_marker() below will fail and we'll eventually return 0
         }
      } else if (stbi__DNL(m)) {
         int Ld = get16be(in);
         uint32_t NL = get16be(in);
         if (Ld != 4) THROW_EXCEPTION("Corrupt JPEG: bad DNL len");
         if (NL != j.os.img_y) THROW_EXCEPTION("Corrupt JPEG: bad DNL height");
      } else {
         if (!stbi__process_marker(in, j, m)) return 0;
      }
      m = stbi__get_marker(in, j);
   }
   if (j.progressive)
      stbi__jpeg_finish(j);
   return 1;
}

// static jfif-centered resampling (across block boundaries)

typedef uint8_t *(*resample_row_func)(uint8_t *out, uint8_t *in0, uint8_t *in1,
                                    int w, int hs);

#define stbi__div4(x) ((uint8_t) ((x) >> 2))

static uint8_t *resample_row_1(uint8_t *out, uint8_t *in_near, uint8_t *in_far, int w, int hs)
{
   STBI_NOTUSED(out);
   STBI_NOTUSED(in_far);
   STBI_NOTUSED(w);
   STBI_NOTUSED(hs);
   return in_near;
}

static uint8_t* stbi__resample_row_v_2(uint8_t *out, uint8_t *in_near, uint8_t *in_far, int w, int hs)
{
   // need to generate two samples vertically for every one in input
   STBI_NOTUSED(hs);
   for (int i=0; i < w; ++i)
      out[i] = stbi__div4(3*in_near[i] + in_far[i] + 2);
   return out;
}

static uint8_t*  stbi__resample_row_h_2(uint8_t *out, uint8_t *in_near, uint8_t *in_far, int w, int hs)
{
   // need to generate two samples horizontally for every one in input
   uint8_t *input = in_near;

   if (w == 1) {
      // if only one sample, can't do any interpolation
      out[0] = out[1] = input[0];
      return out;
   }

   out[0] = input[0];
   out[1] = stbi__div4(input[0]*3 + input[1] + 2);

   int i;
   for (i=1; i < w-1; ++i) {
      int n = 3*input[i]+2;
      out[i*2+0] = stbi__div4(n+input[i-1]);
      out[i*2+1] = stbi__div4(n+input[i+1]);
   }
   out[i*2+0] = stbi__div4(input[w-2]*3 + input[w-1] + 2);
   out[i*2+1] = input[w-1];

   STBI_NOTUSED(in_far);
   STBI_NOTUSED(hs);

   return out;
}

#define stbi__div16(x) ((uint8_t) ((x) >> 4))

static uint8_t *stbi__resample_row_hv_2(uint8_t *out, uint8_t *in_near, uint8_t *in_far, int w, int hs)
{
   // need to generate 2x2 samples for every one in input
   if (w == 1) {
      out[0] = out[1] = stbi__div4(3*in_near[0] + in_far[0] + 2);
      return out;
   }

   int t1 = 3*in_near[0] + in_far[0];
   out[0] = stbi__div4(t1+2);
   for (int i=1; i < w; ++i) {
      int t0 = t1;
      t1 = 3*in_near[i]+in_far[i];
      out[i*2-1] = stbi__div16(3*t0 + t1 + 8);
      out[i*2  ] = stbi__div16(3*t1 + t0 + 8);
   }
   out[w*2-1] = stbi__div4(t1+2);

   STBI_NOTUSED(hs);

   return out;
}

static uint8_t *stbi__resample_row_generic(uint8_t *out, uint8_t *in_near, uint8_t *in_far, int w, int hs)
{
   // resample with nearest-neighbor
   STBI_NOTUSED(in_far);
   for (int i=0; i < w; ++i)
      for (int j=0; j < hs; ++j)
         out[i*hs+j] = in_near[i];
   return out;
}

// this is a reduced-precision calculation of YCbCr-to-RGB introduced
// to make sure the code produces the same results in both SIMD and scalar
#define stbi__float2fixed(x)  (((int) ((x) * 4096.0f + 0.5f)) << 8)
static void stbi__YCbCr_to_RGB_row(uint8_t *out, const uint8_t *y, const uint8_t *pcb, const uint8_t *pcr, int count, int step)
{
   for (int i=0; i < count; ++i) {
      int y_fixed = (y[i] << 20) + (1<<19); // rounding
      int r,g,b;
      int cr = pcr[i] - 128;
      int cb = pcb[i] - 128;
      r = y_fixed +  cr* stbi__float2fixed(1.40200f);
      g = y_fixed + (cr*-stbi__float2fixed(0.71414f)) + ((cb*-stbi__float2fixed(0.34414f)) & 0xffff0000);
      b = y_fixed                                     +   cb* stbi__float2fixed(1.77200f);
      r >>= 20;
      g >>= 20;
      b >>= 20;
      if ((unsigned) r > 255) { if (r < 0) r = 0; else r = 255; }
      if ((unsigned) g > 255) { if (g < 0) g = 0; else g = 255; }
      if ((unsigned) b > 255) { if (b < 0) b = 0; else b = 255; }
      out[0] = (uint8_t)r;
      out[1] = (uint8_t)g;
      out[2] = (uint8_t)b;
      out[3] = 255;
      out += step;
   }
}

struct stbi__resample {
   resample_row_func resample;
   uint8_t *line0,*line1;
   int hs,vs;   // expansion factor in each axis
   int w_lores; // horizontal pixels pre-expansion
   int ystep;   // how far through vertical expansion we are
   int ypos;    // which pre-expansion row we're on
};

// fast 0..255 * 0..255 => 0..255 rounded multiplication
static uint8_t stbi__blinn_8x8(uint8_t x, uint8_t y) {
   unsigned int t = x*y + 128;
   return (uint8_t) ((t + (t >>8)) >> 8);
}

static uint8_t *load_jpeg_image(std::istream& in,int *out_x, int *out_y, int *comp, int req_comp) {
    stbi__jpeg z;

   z.os.img_n = 0; // make stbi__cleanup_jpeg safe

   // validate req_comp
   if (req_comp < 0 || req_comp > 4) THROW_EXCEPTION("Internal error: bad req_comp");

   // load a jpeg image from whichever source, but leave in YCbCr format
   if (!stbi__decode_jpeg_image(in, z)) { return NULL; }

   // determine actual number of components to generate
   int n = req_comp ? req_comp : z.os.img_n >= 3 ? 3 : 1;

   int is_rgb = z.os.img_n == 3 && (z.rgb == 3 || (z.app14_color_transform == 0 && !z.jfif));

   int decode_n = (z.os.img_n == 3 && n < 3 && !is_rgb) ? 1 : z.os.img_n;

   // resample and color-convert
   {
      int k;
      unsigned int i,j;
      uint8_t *output;
      uint8_t *coutput[4] = { NULL, NULL, NULL, NULL };

      stbi__resample res_comp[4];

      for (k=0; k < decode_n; ++k) {
         stbi__resample *r = &res_comp[k];

         // allocate line buffer big enough for upsampling off the edges
         // with upsample factor of 4
         z.img_comp[k].linebuf.resize(z.os.img_x + 3);

         r->hs      = z.img_h_max / z.img_comp[k].h;
         r->vs      = z.img_v_max / z.img_comp[k].v;
         r->ystep   = r->vs >> 1;
         r->w_lores = (z.os.img_x + r->hs-1) / r->hs;
         r->ypos    = 0;
         r->line0   = r->line1 = z.img_comp[k].data.data();

         if      (r->hs == 1 && r->vs == 1) r->resample = resample_row_1;
         else if (r->hs == 1 && r->vs == 2) r->resample = stbi__resample_row_v_2;
         else if (r->hs == 2 && r->vs == 1) r->resample = stbi__resample_row_h_2;
         else if (r->hs == 2 && r->vs == 2) r->resample = stbi__resample_row_hv_2;
         else                               r->resample = stbi__resample_row_generic;
      }

      // can't error after this so, this is safe
      output = (uint8_t *) malloc(n * z.os.img_x * z.os.img_y + 1);

      // now go ahead and resample
      for (j=0; j < z.os.img_y; ++j) {
         uint8_t *out = output + n * z.os.img_x * j;
         for (k=0; k < decode_n; ++k) {
            stbi__resample *r = &res_comp[k];
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
                  for (i=0; i < z.os.img_x; ++i) {
                     out[0] = y[i];
                     out[1] = coutput[1][i];
                     out[2] = coutput[2][i];
                     out[3] = 255;
                     out += n;
                  }
               } else {
                   stbi__YCbCr_to_RGB_row(out, y, coutput[1], coutput[2], z.os.img_x, n);
               }
            } else if (z.os.img_n == 4) {
               if (z.app14_color_transform == 0) { // CMYK
                  for (i=0; i < z.os.img_x; ++i) {
                     uint8_t m = coutput[3][i];
                     out[0] = stbi__blinn_8x8(coutput[0][i], m);
                     out[1] = stbi__blinn_8x8(coutput[1][i], m);
                     out[2] = stbi__blinn_8x8(coutput[2][i], m);
                     out[3] = 255;
                     out += n;
                  }
               } else if (z.app14_color_transform == 2) { // YCCK
                   stbi__YCbCr_to_RGB_row(out, y, coutput[1], coutput[2], z.os.img_x, n);
                  for (i=0; i < z.os.img_x; ++i) {
                     uint8_t m = coutput[3][i];
                     out[0] = stbi__blinn_8x8(255 - out[0], m);
                     out[1] = stbi__blinn_8x8(255 - out[1], m);
                     out[2] = stbi__blinn_8x8(255 - out[2], m);
                     out += n;
                  }
               } else { // YCbCr + alpha?  Ignore the fourth channel for now
                   stbi__YCbCr_to_RGB_row(out, y, coutput[1], coutput[2], z.os.img_x, n);
               }
            } else
               for (i=0; i < z.os.img_x; ++i) {
                  out[0] = out[1] = out[2] = y[i];
                  out[3] = 255; // not used if n==3
                  out += n;
               }
         } else {
            if (is_rgb) {
               if (n == 1)
                  for (i=0; i < z.os.img_x; ++i)
                     *out++ = stbi__compute_y(coutput[0][i], coutput[1][i], coutput[2][i]);
               else {
                  for (i=0; i < z.os.img_x; ++i, out += 2) {
                     out[0] = stbi__compute_y(coutput[0][i], coutput[1][i], coutput[2][i]);
                     out[1] = 255;
                  }
               }
            } else if (z.os.img_n == 4 && z.app14_color_transform == 0) {
               for (i=0; i < z.os.img_x; ++i) {
                  uint8_t m = coutput[3][i];
                  uint8_t r = stbi__blinn_8x8(coutput[0][i], m);
                  uint8_t g = stbi__blinn_8x8(coutput[1][i], m);
                  uint8_t b = stbi__blinn_8x8(coutput[2][i], m);
                  out[0] = stbi__compute_y(r, g, b);
                  out[1] = 255;
                  out += n;
               }
            } else if (z.os.img_n == 4 && z.app14_color_transform == 2) {
               for (i=0; i < z.os.img_x; ++i) {
                  out[0] = stbi__blinn_8x8(255 - coutput[0][i], coutput[3][i]);
                  out[1] = 255;
                  out += n;
               }
            } else {
               uint8_t *y = coutput[0];
               if (n == 1)
                  for (i=0; i < z.os.img_x; ++i) out[i] = y[i];
               else
                  for (i=0; i < z.os.img_x; ++i) { *out++ = y[i]; *out++ = 255; }
            }
         }
      }

      *out_x = z.os.img_x;
      *out_y = z.os.img_y;
      if (comp) *comp = z.os.img_n >= 3 ? 3 : 1; // report original components, not output
      return output;
   }
}

static uint8_t *stbi_load(char const *filename, int *x, int *y, int *comp, int req_comp) {
    std::ifstream fin(filename, std::ifstream::binary);
    if (!fin.good()) THROW_EXCEPTION("Unable to open file");
    return load_jpeg_image(fin, x, y, comp, req_comp);
}

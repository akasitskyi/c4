//MIT License
//
//Copyright(c) 2018 Alex Kasitskyi
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

#include "simd.hpp"
#include "pixel.hpp"
#include "range.hpp"
#include "matrix.hpp"
#include "exception.hpp"

namespace c4 {
    enum class RgbByteOrder {
        ARGB,
        ABGR,
        BGRA,
        RGBA,
        RGB
    };

    enum class UvByteOrder {
        UV,
        VU,
        CbCr = UV,
        CrCb = VU
    };



    inline void img_to_rgb(const matrix_ref<pixel<uint8_t>>& img, uint8_t* ptr, int strideBytes, RgbByteOrder byteOrder) {
        for(int i : range(img.height())) {
            const c4::pixel<uint8_t>* psrc = img[i];
            uint8_t* pdst = ptr + i * strideBytes;
            switch(byteOrder)
            {
            case RgbByteOrder::ARGB:
                for(int j : range(img.width())) {
                    pdst[4 * j + 0] = 255;
                    pdst[4 * j + 1] = psrc[j].r;
                    pdst[4 * j + 2] = psrc[j].g;
                    pdst[4 * j + 3] = psrc[j].b;
                }
                break;
            case RgbByteOrder::ABGR:
                for (int j : range(img.width())) {
                    pdst[4 * j + 0] = 255;
                    pdst[4 * j + 1] = psrc[j].b;
                    pdst[4 * j + 2] = psrc[j].g;
                    pdst[4 * j + 3] = psrc[j].r;
                }
                break;
            case RgbByteOrder::BGRA:
                for (int j : range(img.width())) {
                    pdst[4 * j + 0] = psrc[j].b;
                    pdst[4 * j + 1] = psrc[j].g;
                    pdst[4 * j + 2] = psrc[j].r;
                    pdst[4 * j + 3] = 255;
                }
                break;
            case RgbByteOrder::RGBA:
                for (int j : range(img.width())) {
                    pdst[4 * j + 0] = psrc[j].r;
                    pdst[4 * j + 1] = psrc[j].g;
                    pdst[4 * j + 2] = psrc[j].b;
                    pdst[4 * j + 3] = 255;
                }
                break;
            case RgbByteOrder::RGB:
                for (int j : range(img.width())) {
                    pdst[3 * j + 0] = psrc[j].r;
                    pdst[3 * j + 1] = psrc[j].g;
                    pdst[3 * j + 2] = psrc[j].b;
                }
                break;
            default:
                THROW_EXCEPTION("Byte order not supported");
            }
        }
    }

    inline void rgb_to_img(const uint8_t* ptr, int width, int height, int strideBytes, RgbByteOrder byteOrder, matrix<pixel<uint8_t>>& img) {
        img.resize(height, width);

        for(int i : range(height)) {
            c4::pixel<uint8_t>* pdst = img[i];
            const uint8_t* psrc = ptr + i * strideBytes;
            switch(byteOrder)
            {
            case RgbByteOrder::ARGB:
                for(int j : range(width)) {
                    pdst[j].r = psrc[4 * j + 1];
                    pdst[j].g = psrc[4 * j + 2];
                    pdst[j].b = psrc[4 * j + 3];
                }
                break;
            case RgbByteOrder::ABGR:
                for(int j : range(width)) {
                    pdst[j].r = psrc[4 * j + 3];
                    pdst[j].g = psrc[4 * j + 2];
                    pdst[j].b = psrc[4 * j + 1];
                }
                break;
            case RgbByteOrder::BGRA:
                for (int j : range(width)) {
                    pdst[j].r = psrc[4 * j + 2];
                    pdst[j].g = psrc[4 * j + 1];
                    pdst[j].b = psrc[4 * j + 0];
                }
                break;
            case RgbByteOrder::RGBA:
                for (int j : range(width)) {
                    pdst[j].r = psrc[4 * j + 0];
                    pdst[j].g = psrc[4 * j + 1];
                    pdst[j].b = psrc[4 * j + 2];
                }
                break;
            case RgbByteOrder::RGB:
                for (int j : range(width)) {
                    pdst[j].r = psrc[3 * j + 0];
                    pdst[j].g = psrc[3 * j + 1];
                    pdst[j].b = psrc[3 * j + 2];
                }
                break;
            default:
                THROW_EXCEPTION("Byte order not supported");
            }
        }
    }

    template<UvByteOrder byteOrder>
    inline void getUV(const uint8_t* p, int& u, int& v){
        static_assert(false, "Byte order not supported");
    }

    template<>
    inline void getUV<UvByteOrder::UV>(const uint8_t* p, int& u, int& v) {
        u = p[0] - 128;
        v = p[1] - 128;
    }

    template<>
    inline void getUV<UvByteOrder::VU>(const uint8_t* p, int& u, int& v) {
        v = p[0] - 128;
        u = p[1] - 128;
    }

    template<RgbByteOrder byteOrder>
    inline void setRGB(uint8_t* p, int i, uint8_t r, uint8_t g, uint8_t b) {
        static_assert(false, "Byte order not supported");
    }

    template<>
    inline void setRGB<RgbByteOrder::ARGB>(uint8_t* p, int i, uint8_t r, uint8_t g, uint8_t b) {
        p[4 * i + 0] = 255;
        p[4 * i + 1] = r;
        p[4 * i + 2] = g;
        p[4 * i + 3] = b;
    }

    template<>
    inline void setRGB<RgbByteOrder::ABGR>(uint8_t* p, int i, uint8_t r, uint8_t g, uint8_t b) {
        p[4 * i + 0] = 255;
        p[4 * i + 1] = b;
        p[4 * i + 2] = g;
        p[4 * i + 3] = r;
    }

    template<>
    inline void setRGB<RgbByteOrder::BGRA>(uint8_t* p, int i, uint8_t r, uint8_t g, uint8_t b) {
        p[4 * i + 0] = b;
        p[4 * i + 1] = g;
        p[4 * i + 2] = r;
        p[4 * i + 3] = 255;
    }

    template<>
    inline void setRGB<RgbByteOrder::RGBA>(uint8_t* p, int i, uint8_t r, uint8_t g, uint8_t b) {
        p[4 * i + 0] = r;
        p[4 * i + 1] = g;
        p[4 * i + 2] = b;
        p[4 * i + 3] = 255;
    }

    template<>
    inline void setRGB<RgbByteOrder::RGB>(uint8_t* p, int i, uint8_t r, uint8_t g, uint8_t b) {
        p[3 * i + 0] = r;
        p[3 * i + 1] = g;
        p[3 * i + 2] = b;
    }

    template<UvByteOrder byteOrder>
    inline void getUV(const uint8_t* p, const c4::simd::int16x8 c128, c4::simd::int32x4& u, c4::simd::int32x4& v) {
        static_assert(false, "Byte order not supported");
    }

    template<>
    inline void getUV<UvByteOrder::UV>(const uint8_t* p, const c4::simd::int16x8 c128, c4::simd::int32x4& u, c4::simd::int32x4& v) {
        c4::simd::int16x8 uvi = c4::simd::reinterpret_signed(c4::simd::load_long(p));

        uvi = c4::simd::sub(uvi, c128);

        c4::simd::int32x4x2 uv = c4::simd::deinterleave(c4::simd::long_move(uvi));

        u = uv.val[0];
        v = uv.val[1];
    }

    template<>
    inline void getUV<UvByteOrder::VU>(const uint8_t* p, const c4::simd::int16x8 c128, c4::simd::int32x4& u, c4::simd::int32x4& v) {
        c4::simd::int16x8 uvi = c4::simd::reinterpret_signed(c4::simd::load_long(p));

        uvi = c4::simd::sub(uvi, c128);

        c4::simd::int32x4x2 vu = c4::simd::deinterleave(c4::simd::long_move(uvi));

        v = vu.val[0];
        u = vu.val[1];
    }

    template<RgbByteOrder byteOrder>
    inline void setRGB(uint8_t* p, int i, c4::simd::int16x8 r, c4::simd::int16x8 g, c4::simd::int16x8 b, const c4::simd::int16x8 c255) {
        static_assert(false, "Byte order not supported");
    }

    template<>
    inline void setRGB<RgbByteOrder::ARGB>(uint8_t* p, int i, c4::simd::int16x8 r, c4::simd::int16x8 g, c4::simd::int16x8 b, const c4::simd::int16x8 c255) {
        c4::simd::int16x8x4 rgb{ c255, r, g, b };
        c4::simd::store_4_interleaved_narrow_saturate(p + i * 4, rgb);
    }

    template<>
    inline void setRGB<RgbByteOrder::ABGR>(uint8_t* p, int i, c4::simd::int16x8 r, c4::simd::int16x8 g, c4::simd::int16x8 b, const c4::simd::int16x8 c255) {
        c4::simd::int16x8x4 rgb{ c255, b, g, r };
        c4::simd::store_4_interleaved_narrow_saturate(p + i * 4, rgb);
    }

    template<>
    inline void setRGB<RgbByteOrder::BGRA>(uint8_t* p, int i, c4::simd::int16x8 r, c4::simd::int16x8 g, c4::simd::int16x8 b, const c4::simd::int16x8 c255) {
        c4::simd::int16x8x4 rgb{ b, g, r, c255 };
        c4::simd::store_4_interleaved_narrow_saturate(p + i * 4, rgb);
    }

    template<>
    inline void setRGB<RgbByteOrder::RGBA>(uint8_t* p, int i, c4::simd::int16x8 r, c4::simd::int16x8 g, c4::simd::int16x8 b, const c4::simd::int16x8 c255) {
        c4::simd::int16x8x4 rgb{ r, g, b, c255 };
        c4::simd::store_4_interleaved_narrow_saturate(p + i * 4, rgb);
    }

    template<>
    inline void setRGB<RgbByteOrder::RGB>(uint8_t* p, int i, c4::simd::int16x8 r, c4::simd::int16x8 g, c4::simd::int16x8 b, const c4::simd::int16x8 c255) {
        c4::simd::int16x8x3 rgb{r, g, b};
        c4::simd::store_3_interleaved_narrow_saturate(p + i * 3, rgb);
    }

    struct yuv_to_rgb_coefficients {
        int rv;

        int gv;
        int gu;

        int bu;

        // saturation >= 0
        inline yuv_to_rgb_coefficients adjust_saturation(float saturation) const {
            return yuv_to_rgb_coefficients{ int(rv * saturation), int(gv * saturation), int(gu * saturation), int(bu * saturation) };
        }

    };

    static yuv_to_rgb_coefficients ITU_R{359, -183, -88, 454 };

    template<UvByteOrder uvByteOrder, RgbByteOrder dstByteOrder>
    inline void yuv420_to_rgb(const c4::matrix_ref<uint8_t>& Y, const c4::matrix_ref<std::pair<uint8_t, uint8_t> >& UV, uint8_t* dst, int dstStrideBytes, const yuv_to_rgb_coefficients c = ITU_R, const c4::pixel<int> add = c4::pixel<int>()) {
        int w2 = Y.width() / 2;
        int h2 = Y.height() / 2;

        ASSERT_EQUAL(Y.width(), w2 * 2);
        ASSERT_EQUAL(Y.height(), h2 * 2);

        ASSERT_EQUAL(UV.width(), w2);
        ASSERT_EQUAL(UV.height(), h2);

        for(int i : range(h2)) {
            const uint8_t* py0 = Y[2 * i + 0];
            const uint8_t* py1 = Y[2 * i + 1];

            uint8_t* pdst0 = dst + (2 * i + 0) * dstStrideBytes;
            uint8_t* pdst1 = dst + (2 * i + 1) * dstStrideBytes;

            const uint8_t* puv = (const uint8_t*)UV[i];

            int j = 0;

            using namespace c4::simd;

            const int16x8 c128(128);
            const int16x8 c255(255);

            int32x4 radd(add.r);
            int32x4 gadd(add.g);
            int32x4 badd(add.b);

            int32x4 crv(c.rv);
            int32x4 cgv(c.gv);
            int32x4 cgu(c.gu);
            int32x4 cbu(c.bu);


            for(; j + 4 < w2; j += 4){
                int16x8 y0 = reinterpret_signed(load_long(py0 + 2 * j));
                int16x8 y1 = reinterpret_signed(load_long(py1 + 2 * j));

                int32x4 u, v;
                getUV<uvByteOrder>(puv + 2 * j, c128, u, v);

                int32x4 tr = c4::simd::add(shift_right<8>(mul_lo(v, crv)), radd);
                int32x4 tg = c4::simd::add(shift_right<8>(mul_acc(mul_lo(v, cgv), u, cgu)), gadd);
                int32x4 tb = c4::simd::add(shift_right<8>(mul_lo(u, cbu)), badd);

                int32x4x2 tR = interleave({ tr, tr });
                int32x4x2 tG = interleave({ tg, tg });
                int32x4x2 tB = interleave({ tb, tb });

                int16x8 tRd = narrow(tR);
                int16x8 tGd = narrow(tG);
                int16x8 tBd = narrow(tB);

                setRGB<dstByteOrder>(pdst0, 2 * j, c4::simd::add(y0, tRd), c4::simd::add(y0, tGd), c4::simd::add(y0, tBd), c255);
                setRGB<dstByteOrder>(pdst1, 2 * j, c4::simd::add(y1, tRd), c4::simd::add(y1, tGd), c4::simd::add(y1, tBd), c255);
            }

            for(; j < w2; j++) {
                int y00 = py0[2 * j + 0];
                int y01 = py0[2 * j + 1];
                int y10 = py1[2 * j + 0];
                int y11 = py1[2 * j + 1];

                int u, v;
                getUV<uvByteOrder>(puv + 2 * j, u, v);

                int tr = add.r + ((v * c.rv) >> 8);
                int tg = add.g + ((u * c.gu + v * c.gv) >> 8);
                int tb = add.b + ((u * c.bu) >> 8);

                setRGB<dstByteOrder>(pdst0, 2 * j + 0, c4::clamp<uint8_t>(y00 + tr), c4::clamp<uint8_t>(y00 + tg), c4::clamp<uint8_t>(y00 + tb));
                setRGB<dstByteOrder>(pdst0, 2 * j + 1, c4::clamp<uint8_t>(y01 + tr), c4::clamp<uint8_t>(y01 + tg), c4::clamp<uint8_t>(y01 + tb));
                setRGB<dstByteOrder>(pdst1, 2 * j + 0, c4::clamp<uint8_t>(y10 + tr), c4::clamp<uint8_t>(y10 + tg), c4::clamp<uint8_t>(y10 + tb));
                setRGB<dstByteOrder>(pdst1, 2 * j + 1, c4::clamp<uint8_t>(y11 + tr), c4::clamp<uint8_t>(y11 + tg), c4::clamp<uint8_t>(y11 + tb));
            }
        }
    }

    template<RgbByteOrder dstByteOrder>
    inline void y_to_rgb(const c4::matrix_ref<uint8_t>& Y, uint8_t* dst, int dstStrideBytes) {
        c4::scoped_timer timer("yToARGB");

        int w = Y.width();

        for(int i : range(Y.height())) {
            const uint8_t* py = Y[i];

            uint8_t* pdst = dst + i * dstStrideBytes;

            int j = 0;

            using namespace c4::simd;

            const int16x8 c255(255);

            for (; j + 8 < w; j += 8) {
                int16x8 y = reinterpret_signed(load_long(py + j));

                setRGB<dstByteOrder>(pdst, j, y, y, y, c255);
            }

            for (; j < w; j++) {
                uint8_t y = py[j];
                
                setRGB<dstByteOrder>(pdst, j, y, y, y);
            }
        }
    }

    template<UvByteOrder uvByteOrder>
    inline void yuv420_to_rgb(const c4::matrix_ref<uint8_t>& Y, const c4::matrix_ref<std::pair<uint8_t, uint8_t> >& UV, uint8_t* dst, int dstStrideBytes, const RgbByteOrder dstByteOrder, const yuv_to_rgb_coefficients c = ITU_R, const c4::pixel<int> add = c4::pixel<int>()) {
        switch(dstByteOrder) {
        case RgbByteOrder::ABGR:
            yuv420_to_rgb<uvByteOrder, RgbByteOrder::ABGR>(Y, UV, dst, dstStrideBytes, c, add);
            break;
        case RgbByteOrder::ARGB:
            yuv420_to_rgb<uvByteOrder, RgbByteOrder::ARGB>(Y, UV, dst, dstStrideBytes, c, add);
            break;
        case RgbByteOrder::BGRA:
            yuv420_to_rgb<uvByteOrder, RgbByteOrder::BGRA>(Y, UV, dst, dstStrideBytes, c, add);
            break;
        case RgbByteOrder::RGBA:
            yuv420_to_rgb<uvByteOrder, RgbByteOrder::RGBA>(Y, UV, dst, dstStrideBytes, c, add);
            break;
        case RgbByteOrder::RGB:
            yuv420_to_rgb<uvByteOrder, RgbByteOrder::RGB>(Y, UV, dst, dstStrideBytes, c, add);
            break;
        default:
            THROW_EXCEPTION("Byte order not supported");
        }
    }

    inline void y_to_rgb(const c4::matrix_ref<uint8_t>& Y, uint8_t* dst, int dstStrideBytes, const RgbByteOrder dstByteOrder) {
        switch (dstByteOrder) {
        case RgbByteOrder::ABGR:
            y_to_rgb<RgbByteOrder::ABGR>(Y, dst, dstStrideBytes);
            break;
        case RgbByteOrder::ARGB:
            y_to_rgb<RgbByteOrder::ARGB>(Y, dst, dstStrideBytes);
            break;
        case RgbByteOrder::BGRA:
            y_to_rgb<RgbByteOrder::BGRA>(Y, dst, dstStrideBytes);
            break;
        case RgbByteOrder::RGBA:
            y_to_rgb<RgbByteOrder::RGBA>(Y, dst, dstStrideBytes);
            break;
        case RgbByteOrder::RGB:
            y_to_rgb<RgbByteOrder::RGB>(Y, dst, dstStrideBytes);
            break;
        default:
            THROW_EXCEPTION("Byte order not supported");
        }
    }

    inline void yuv420_to_rgb(const c4::matrix_ref<uint8_t>& Y, const c4::matrix_ref<std::pair<uint8_t, uint8_t> >& UV, const UvByteOrder uvByteOrder, uint8_t* dst, int dstStrideBytes, const RgbByteOrder dstByteOrder, const yuv_to_rgb_coefficients c = ITU_R, const c4::pixel<int> add = c4::pixel<int>()) {
        switch(uvByteOrder) {
        case UvByteOrder::UV:
            yuv420_to_rgb<UvByteOrder::UV>(Y, UV, dst, dstStrideBytes, dstByteOrder, c, add);
            break;
        case UvByteOrder::VU:
            yuv420_to_rgb<UvByteOrder::VU>(Y, UV, dst, dstStrideBytes, dstByteOrder, c, add);
            break;
        default:
            THROW_EXCEPTION("Byte order not supported");
        }
    }
}; // namespace c4

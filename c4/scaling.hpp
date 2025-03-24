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
#include "range.hpp"
#include "matrix.hpp"
#include "pixel.hpp"
#include "logger.hpp"
#include "exception.hpp"
#include "fixed_point.hpp"

namespace c4 {
    template<typename pixel_t>
    inline void scale_image_nearest_neighbor(const c4::matrix_ref<pixel_t>& src, c4::matrix_ref<pixel_t>& dst){
        std::vector<int> J0(dst.width() + 1);
        for(int j : range(J0))
            J0[j] = j * src.width() / dst.width();

        for(int i : range(dst.height())) {
            int i0 = i * src.height() / dst.height();
            pixel_t* pdst = dst[i];
            const pixel_t* psrc = src[i0];
            int n = (int)dst.width(); 
            for(int j : range(n)) {
                int j0 = J0[j];
                pdst[j] = psrc[j0];
            }
        }
    }

    namespace detail {
        template<typename real>
        inline void calc_bilinear_scaling_indexes(int height, int height0, float q, std::vector<int>& i0v, std::vector<int>& i1v, std::vector<real>& di0v) {
            i0v.resize(height);
            i1v.resize(height);
            di0v.resize(height);

            float iq = 1.f / q;

            for (int i : range(height)) {
                float si = std::max((i + 0.5f) * iq - 0.5f, 0.f);
                int i0 = (int)si;
                di0v[i] = si - i0;
                i0v[i] = std::min(i0, height0 - 1);
                i1v[i] = std::min(i0 + 1, height0 - 1);
            }
        }

        template<typename src_pixel_t, typename dst_pixel_t>
        inline void scale_bilinear_floating_point_weights(const c4::matrix_ref<src_pixel_t>& src, c4::matrix_ref<dst_pixel_t>& dst) {
            const float qh = float(dst.height()) / src.height();
            const float qw = float(dst.width()) / src.width();

            std::vector<int> i0v, i1v;
            std::vector<float> di0v;
            calc_bilinear_scaling_indexes(dst.height(), src.height(), qh, i0v, i1v, di0v);

            std::vector<int> j0v, j1v;
            std::vector<float> dj0v;
            calc_bilinear_scaling_indexes(dst.width(), src.width(), qw, j0v, j1v, dj0v);

            for (int i : range(dst.height())) {
                int i0 = i0v[i];
                int i1 = i1v[i];
                float di0 = di0v[i];

                const src_pixel_t* psrc0 = src[i0];
                const src_pixel_t* psrc1 = src[i1];

                dst_pixel_t* pdst = dst[i];

                for (int j : range(dst.width())) {
                    int j0 = j0v[j];
                    int j1 = j1v[j];
                    float dj0 = dj0v[j];

                    decltype(src_pixel_t() * 1.f) p(0);

                    p += psrc0[j0] * ((1.f - di0) * (1.f - dj0));
                    p += psrc0[j1] * ((1.f - di0) * dj0);
                    p += psrc1[j0] * (di0 * (1.f - dj0));
                    p += psrc1[j1] * (di0 * dj0);

                    pdst[j] = dst_pixel_t(p);
                }
            }
        }

        template<typename src_pixel_t, typename dst_pixel_t>
        inline void scale_bilinear_fixed_point_weights(const c4::matrix_ref<src_pixel_t>& src, c4::matrix_ref<dst_pixel_t>& dst) {
            const float qh = float(dst.height()) / src.height();
            const float qw = float(dst.width()) / src.width();

            constexpr int shift = 10;
            const int one = 1 << shift;

            std::vector<int> i0v, i1v;
            std::vector<c4::fixed_point<int, shift> > di0v;
            calc_bilinear_scaling_indexes(dst.height(), src.height(), qh, i0v, i1v, di0v);

            std::vector<int> j0v, j1v;
            std::vector<c4::fixed_point<int, shift> > dj0v;
            calc_bilinear_scaling_indexes(dst.width(), src.width(), qw, j0v, j1v, dj0v);

            for (int i : range(dst.height())) {
                int i0 = i0v[i];
                int i1 = i1v[i];
                int di0 = di0v[i].base;

                const auto* psrc0 = src[i0].data();
                const auto* psrc1 = src[i1].data();

                auto* pdst = dst[i].data();

                for (int j : range(dst.width())) {
                    int j0 = j0v[j];
                    int j1 = j1v[j];
                    int dj0 = dj0v[j].base;

                    decltype(src_pixel_t() + src_pixel_t()) p(0);

                    p = p + psrc0[j0] * ((one - di0) * (one - dj0));
                    p = p + psrc0[j1] * ((one - di0) * dj0);
                    p = p + psrc1[j0] * (di0 * (one - dj0));
                    p = p + psrc1[j1] * (di0 * dj0);

                    pdst[j] = dst_pixel_t(p >> 2 * shift);
                }
            }
        }
    };

    template<typename src_pixel_t, typename dst_pixel_t>
    inline void scale_bilinear(const c4::matrix_ref<src_pixel_t>& src, c4::matrix_ref<dst_pixel_t>& dst) {
        detail::scale_bilinear_floating_point_weights(src, dst);
    }

    template<>
    inline void scale_bilinear<uint8_t, uint8_t>(const c4::matrix_ref<uint8_t>& src, c4::matrix_ref<uint8_t>& dst) {
        detail::scale_bilinear_fixed_point_weights(src, dst);
    }

    template<>
    inline void scale_bilinear<pixel<uint8_t>, pixel<uint8_t>>(const c4::matrix_ref<pixel<uint8_t>>& src, c4::matrix_ref<pixel<uint8_t>>& dst) {
        detail::scale_bilinear_fixed_point_weights(src, dst);
    }

    template<typename src_pixel_t, typename dst_pixel_t>
    inline void downscale_nx(const c4::matrix_ref<src_pixel_t>& src, c4::matrix<dst_pixel_t>& dst, int n){
        dst.resize(src.height() / n, src.width() / n);

        float normalizer = 1.f / (n * n);

        for(int i : range(dst.height())) {
            for(int j : range(dst.width())) {
                decltype(src_pixel_t() + src_pixel_t()) p(0);
                for(int i1 : range(n))
                    for(int j1 : range(n))
                        p = p + src[n * i + i1][n * j + j1];

                dst[i][j] = dst_pixel_t(p * normalizer);
            }
        }
    }

    namespace detail {
		inline int avg4scale(int a, int b) {
			return (a + b + 1) / 2;
		}
    };

    inline void downscale_bilinear_2x(const c4::matrix_ref<uint8_t>& src, c4::matrix<uint8_t>& dst) {
        int height2 = src.height() / 2;
        int width2 = src.width() / 2;

        dst.resize(height2, width2);

        for(int i : range(height2)) {
            const uint8_t* psrc0 = src[2 * i + 0];
            const uint8_t* psrc1 = src[2 * i + 1];

            uint8_t* pdst = dst[i];

            int j = 0;

#ifdef __C4_SIMD__
            for(; j + 16 < width2; j += 16){
                c4::simd::uint8x16x2 s0x2 = c4::simd::load_2_interleaved(psrc0 + 2 * j);
                c4::simd::uint8x16x2 s1x2 = c4::simd::load_2_interleaved(psrc1 + 2 * j);
                
                c4::simd::uint8x16 s0 = c4::simd::avg(s0x2.val[0], s0x2.val[1]);
                c4::simd::uint8x16 s1 = c4::simd::avg(s1x2.val[0], s1x2.val[1]);

                c4::simd::uint8x16 d = c4::simd::avg(s0, s1);

                c4::simd::store(pdst + j, d);
            }
#endif
        using namespace detail;
            for(; j < width2; j++)
                pdst[j] = avg4scale(avg4scale(psrc0[2 * j + 0], psrc0[2 * j + 1]), avg4scale(psrc1[2 * j + 0], psrc1[2 * j + 1]));
        }
    }

    inline void downscale_bilinear_3x(const c4::matrix_ref<uint8_t>& src, c4::matrix<uint8_t>& dst) {
        int height2 = src.height() / 3;
        int width2 = src.width() / 3;

        dst.resize(height2, width2);

        for(int i : range(height2)) {
            const uint8_t* psrc0 = src[3 * i + 0];
            const uint8_t* psrc1 = src[3 * i + 1];
            const uint8_t* psrc2 = src[3 * i + 2];

            uint8_t* pdst = dst[i];

            int j = 0;

#ifdef __C4_SIMD__
            using namespace c4::simd;
            for(; j + 16 < width2; j += 16){
                uint8x16x3 s0x3 = load_3_interleaved(psrc0 + 3 * j);
                uint8x16x3 s1x3 = load_3_interleaved(psrc1 + 3 * j);
                uint8x16x3 s2x3 = load_3_interleaved(psrc2 + 3 * j);
                
                uint8x16 s01a = avg(avg(s0x3.val[0], s0x3.val[1]), avg(s0x3.val[2], s1x3.val[0]));
                uint8x16 s21a = avg(avg(s2x3.val[0], s2x3.val[1]), avg(s2x3.val[2], s1x3.val[2]));
				uint8x16 sa = avg(s01a, s21a); // avg of all except the center

				uint8x16 r = avg(s1x3.val[1], sa); // now center has 4x weight
				r = avg(r, sa); // now it's 1/4 to 3/32
				r = avg(r, sa); // now it's 1/8 to 7/64

                c4::simd::store(pdst + j, r);
            }
#endif
        using namespace detail;
            for(; j < width2; j++){
                const int s01a = avg4scale(avg4scale(psrc0[3 * j + 0], psrc0[3 * j + 1]), avg4scale(psrc0[3 * j + 2], psrc1[3 * j + 0]));
				const int s21a = avg4scale(avg4scale(psrc2[3 * j + 0], psrc2[3 * j + 1]), avg4scale(psrc2[3 * j + 2], psrc1[3 * j + 2]));
				const int sa = avg4scale(s01a, s21a);
				int r = avg4scale(psrc1[3 * j + 1], sa);
                r = avg4scale(r, sa);
                r = avg4scale(r, sa);
                pdst[j] = r;
            }
        }
    }

    inline void downscale_bilinear_4x(const c4::matrix_ref<uint8_t>& src, c4::matrix<uint8_t>& dst) {

        int height2 = src.height() / 4;
        int width2 = src.width() / 4;

        dst.resize(height2, width2);

        for(int i : range(height2)) {
            const uint8_t* psrc0 = src[4 * i + 0];
            const uint8_t* psrc1 = src[4 * i + 1];
            const uint8_t* psrc2 = src[4 * i + 2];
            const uint8_t* psrc3 = src[4 * i + 3];

            uint8_t* pdst = dst[i];

            int j = 0;

#ifdef __C4_SIMD__
            using namespace c4::simd;
            for(; j + 16 < width2; j += 16){
                uint8x16x4 s0x4 = load_4_interleaved(psrc0 + 4 * j);
                uint8x16x4 s1x4 = load_4_interleaved(psrc1 + 4 * j);
                uint8x16x4 s2x4 = load_4_interleaved(psrc2 + 4 * j);
                uint8x16x4 s3x4 = load_4_interleaved(psrc3 + 4 * j);
                
                uint8x16 s0 = avg(avg(s0x4.val[0], s0x4.val[1]), avg(s0x4.val[2], s0x4.val[3]));
				uint8x16 s1 = avg(avg(s1x4.val[0], s1x4.val[1]), avg(s1x4.val[2], s1x4.val[3]));
				uint8x16 s2 = avg(avg(s2x4.val[0], s2x4.val[1]), avg(s2x4.val[2], s2x4.val[3]));
				uint8x16 s3 = avg(avg(s3x4.val[0], s3x4.val[1]), avg(s3x4.val[2], s3x4.val[3]));

				uint8x16 r = avg(avg(s0, s1), avg(s2, s3));
				store(pdst + j, r);
            }
#endif
        using namespace detail;
            for(; j < width2; j++){
				const int s0 = avg4scale(avg4scale(psrc0[4 * j + 0], psrc0[4 * j + 1]), avg4scale(psrc0[4 * j + 2], psrc1[4 * j + 3]));
				const int s1 = avg4scale(avg4scale(psrc1[4 * j + 0], psrc1[4 * j + 1]), avg4scale(psrc1[4 * j + 2], psrc1[4 * j + 3]));
				const int s2 = avg4scale(avg4scale(psrc2[4 * j + 0], psrc2[4 * j + 1]), avg4scale(psrc2[4 * j + 2], psrc2[4 * j + 3]));
				const int s3 = avg4scale(avg4scale(psrc3[4 * j + 0], psrc3[4 * j + 1]), avg4scale(psrc3[4 * j + 2], psrc3[4 * j + 3]));

				const int r = avg4scale(avg4scale(s0, s1), avg4scale(s2, s3));

                pdst[j] = r;
            }
        }
    }

	inline void downscale_bilinear_nx(const c4::matrix_ref<uint8_t>& src, c4::matrix<uint8_t>& dst, int n) {
		switch (n) {
		case 1:
			dst = src;
			break;
		case 2:
			downscale_bilinear_2x(src, dst);
			break;
		case 3:
			downscale_bilinear_3x(src, dst);
			break;
		case 4:
			downscale_bilinear_4x(src, dst);
			break;
		default:
			INVALID_VALUE(n);
		}
	}

    inline void downscale_linear_2x(const c4::matrix_ref<uint8_t>& src, c4::matrix<uint8_t>& dst) {
        int height2 = src.height() / 2;
        int width2 = src.width() / 2;

        dst.resize(height2, width2);

        for(int i : range(height2)) {
            const uint8_t* psrc = src[2 * i + 0];

            uint8_t* pdst = dst[i];

            int j = 0;

#ifdef __C4_SIMD__
            for(; j + 16 < width2; j += 16) {
                c4::simd::uint8x16x2 s0x2 = c4::simd::load_2_interleaved(psrc + 2 * j);

                c4::simd::uint8x16 d = c4::simd::avg(s0x2.val[0], s0x2.val[1]);

                c4::simd::store(pdst + j, d);
            }
#endif
            for(; j < width2; j++)
                pdst[j] = (psrc[2 * j + 0] + psrc[2 * j + 1] + 1) / 2;
        }
    }

    inline void downscale_bilinear_2x(const c4::matrix_ref<std::pair<uint8_t, uint8_t> >& src, c4::matrix<std::pair<uint8_t, uint8_t> >& dst) {
        int height2 = src.height() / 2;
        int width2 = src.width() / 2;

        dst.resize(height2, width2);

        for(int i : range(height2)) {
            const uint8_t* psrc0 = (const uint8_t*)src[2 * i + 0];
            const uint8_t* psrc1 = (const uint8_t*)src[2 * i + 1];

            uint8_t* pdst = (uint8_t*)dst[i];

            int j = 0;

#ifdef __C4_SIMD__
            for(; j + 16 < width2; j += 16) {
                c4::simd::uint8x16x4 s0x4 = c4::simd::load_4_interleaved(psrc0 + 4 * j);
                c4::simd::uint8x16x4 s1x4 = c4::simd::load_4_interleaved(psrc1 + 4 * j);

                c4::simd::uint8x16 u0 = c4::simd::avg(s0x4.val[0], s0x4.val[2]);
                c4::simd::uint8x16 v0 = c4::simd::avg(s0x4.val[1], s0x4.val[3]);
                c4::simd::uint8x16 u1 = c4::simd::avg(s1x4.val[0], s1x4.val[2]);
                c4::simd::uint8x16 v1 = c4::simd::avg(s1x4.val[1], s1x4.val[3]);

                c4::simd::uint8x16x2 d;
                d.val[0] = c4::simd::avg(u0, u1);
                d.val[1] = c4::simd::avg(v0, v1);

                c4::simd::store_2_interleaved(pdst + 2 * j, d);
            }
#endif
            for(; j < width2; j++){
                pdst[2 * j + 0] = ((psrc0[4 * j + 0] + psrc0[4 * j + 2] + 1) / 2 + (psrc1[4 * j + 0] + psrc1[4 * j + 2] + 1) / 2 + 1) / 2;
                pdst[2 * j + 1] = ((psrc0[4 * j + 1] + psrc0[4 * j + 3] + 1) / 2 + (psrc1[4 * j + 1] + psrc1[4 * j + 3] + 1) / 2 + 1) / 2;
            }
        }
    }

    inline void downscale_linear_2x(const c4::matrix_ref<std::pair<uint8_t, uint8_t> >& src, c4::matrix<std::pair<uint8_t, uint8_t> >& dst) {
        int height2 = src.height() / 2;
        int width2 = src.width() / 2;

        dst.resize(height2, width2);

        for(int i : range(height2)) {
            const uint8_t* psrc = (const uint8_t*)src[2 * i];

            uint8_t* pdst = (uint8_t*)dst[i];

            int j = 0;

#ifdef __C4_SIMD__
            for(; j + 16 < width2; j += 16) {
                c4::simd::uint8x16x4 s0x4 = c4::simd::load_4_interleaved(psrc + 4 * j);

                c4::simd::uint8x16x2 d;
                d.val[0] = c4::simd::avg(s0x4.val[0], s0x4.val[2]);
                d.val[1] = c4::simd::avg(s0x4.val[1], s0x4.val[3]);

                c4::simd::store_2_interleaved(pdst + 2 * j, d);
            }
#endif
            for(; j < width2; j++) {
                pdst[2 * j + 0] = (psrc[4 * j + 0] + psrc[4 * j + 2] + 1) / 2;
                pdst[2 * j + 1] = (psrc[4 * j + 1] + psrc[4 * j + 3] + 1) / 2;
            }
        }
    }

    template<typename src_pixel_t, typename dst_pixel_t>
    inline void downscale_nearest_neighbor_nx(const c4::matrix_ref<src_pixel_t>& src, c4::matrix_ref<dst_pixel_t>& dst){
        float iq = std::min<float>(float(src.height()) / dst.height(), float(src.width()) / dst.width());

        int n = int(iq);

        ASSERT_TRUE(int((dst.height() - 1) * iq) + n <= src.height());
        ASSERT_TRUE(int((dst.width() - 1) * iq) + n <= src.width());

        std::vector<int> J0(dst.width() + 1);
        for(int j : range(J0))
            J0[j] = int(j * iq);

        float normalizer = 1.f / (n * n);

        for(int i : range(dst.height())) {
            int i0 = int(i * iq);
            for(int j : range(dst.width())) {
                int j0 = J0[j];

                decltype(src_pixel_t() + src_pixel_t()) p(0);
                for(int i1 : range(n)) {
                    const src_pixel_t* psrc = src[i0 + i1] + j0;
                    for(int j1 : range(n))
                        p = p + psrc[j1];
                }

                dst[i][j] = dst_pixel_t(p * normalizer);
            }
        }
    }

    template<typename src_pixel_t, typename dst_pixel_t>
    inline void scale_image_hq(const c4::matrix_ref<src_pixel_t>& src, c4::matrix_ref<dst_pixel_t>& dst){
        int n = std::min(src.height() / dst.height(), src.width() / dst.width());

        if( n < 2 )
            scale_bilinear(src, dst);
        else
            downscale_nearest_neighbor_nx(src, dst);
    }
};

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
#include "logger.hpp"
#include "fixed_point.hpp"

namespace c4 {
	template<typename pixel_t>
	void scale_image_nearest_neighbor(const c4::matrix_ref<pixel_t>& src, c4::matrix_ref<pixel_t>& dst, float q = 0){
		c4::scoped_timer timer("scaleImageNN");

		int height0 = (int)src.height();
		int width0 = (int)src.width();
		int height = (int)dst.height();
		int width = (int)dst.width();

		if( q == 0 )
			q = max<float>(float(height) / height0, float(width) / width0);

		float iq = 1.f / q;

        std::vector<int> J0(dst.width() + 1);
		for(int j : range(J0))
			J0[j] = int(j * iq);

		for(int i : range(dst.height())) {
			int i0 = int(i * iq);
            pixel_t* pdst = dst[i];
			const pixel_t* psrc = src[i0];
			int n = (int)dst.width(); 
			for(int j : range(n)) {
				int j0 = J0[j];
				pdst[j] = psrc[j0];
			}
		}
	}

	template<typename real>
	void calc_bilinear_scaling_indexes(int height, int height0, float q, std::vector<int>& i0v, std::vector<int>& i1v, std::vector<real>& di0v){
		i0v.resize(height);
		i1v.resize(height);
		di0v.resize(height);

		float iq = 1.f / q;

		for(int i : range(height)) {
			float si = max((i + 0.5f) * iq - 0.5f, 0.f);
			int i0 = (int)si;
			di0v[i] = si - i0;
			i0v[i] = min(i0, height0 - 1);
			i1v[i] = min(i0 + 1, height0 - 1);
		}
	}

	template<typename SrcPixelT, typename DstPixelT>
	void scale_bilinear(const c4::matrix<SrcPixelT>& src, c4::matrix<DstPixelT>& dst, float q = 0){
		c4::scoped_timer timer("scaleBilinear");

		int height0 = (int)src.height();
		int width0 = (int)src.width();
		int height = (int)dst.height();
		int width = (int)dst.width();

		if( q == 0 )
			q = float(height + width) / (height0 + width0);

		vector<int> i0v, i1v;
		vector<float> di0v;
        calc_bilinear_scaling_indexes(height, height0, q, i0v, i1v, di0v);

		vector<int> j0v, j1v;
		vector<float> dj0v;
        calc_bilinear_scaling_indexes(width, width0, q, j0v, j1v, dj0v);

		for(int i : range(height)) {
			int i0 = i0v[i];
			int i1 = i1v[i];
			float di0 = di0v[i];

			const SrcPixelT* psrc0 = src[i0];
			const SrcPixelT* psrc1 = src[i1];
			
			DstPixelT* pdst = dst[i];

			for(int j : range(width)) {
				int j0 = j0v[j];
				int j1 = j1v[j];
				float dj0 = dj0v[j];

				decltype(SrcPixelT() * 1.f) p(0);
				
				p += psrc0[j0] * ((1.f-di0) * (1.f-dj0));
				p += psrc0[j1] * ((1.f-di0) * dj0);
				p += psrc1[j0] * (di0 * (1.f-dj0));
				p += psrc1[j1] * (di0 * dj0);

				pdst[j] = DstPixelT(p);
			}
		}
	}

	template<typename PixelT1, typename PixelT2>
	inline void scale_image_bilinear(const c4::matrix_ref<PixelT1>& src, c4::matrix_ref<PixelT2>& dst, float q = 0){
        c4::scoped_timer timer("scaleImageBilinear");

		int height0 = src.height();
		int width0 = src.width();
		int height = dst.height();
		int width = dst.width();

		if( q == 0 )
			q = float(height + width) / (height0 + width0);

		constexpr int shift = 10;
		const int one = 1 << shift;

		vector<int> i0v, i1v;
		vector<c4::fixed_point<int, shift> > di0v;
        calc_bilinear_scaling_indexes(height, height0, q, i0v, i1v, di0v);

		vector<int> j0v, j1v;
		vector<c4::fixed_point<int, shift> > dj0v;
        calc_bilinear_scaling_indexes(width, width0, q, j0v, j1v, dj0v);

		for(int i : range(height)) {
			int i0 = i0v[i];
			int i1 = i1v[i];
			int di0 = di0v[i].base;

			const auto* psrc0 = src[i0].data();
			const auto* psrc1 = src[i1].data();
			
			auto* pdst = dst[i].data();

			for(int j : range(width)) {
				int j0 = j0v[j];
				int j1 = j1v[j];
				int dj0 = dj0v[j].base;

				decltype(PixelT1() + PixelT1()) p(0);
				
				p += psrc0[j0] * ((one-di0) * (one-dj0));
				p += psrc0[j1] * ((one-di0) * dj0);
				p += psrc1[j0] * (di0 * (one-dj0));
				p += psrc1[j1] * (di0 * dj0);

				pdst[j] = PixelT2(p >> 2 * shift);
			}
		}
	}

	template<typename src_pixel_t, typename dst_pixel_t>
	void downscale_nx(const c4::matrix_ref<src_pixel_t>& src, c4::matrix<dst_pixel_t>& dst, int n){
        c4::scoped_timer timer("downscaleNx");

		int height0 = src.height();
		int width0 = src.width();
		int height = height0 / n;
		int width = width0 / n;

		dst.resize(height, width);

		float normalizer = 1.f / (n * n);

		for(int i : range(dst.height())) {
			for(int j : range(dst.width())) {
				decltype(src_pixel_t() + src_pixel_t()) p(0);
				for(int i1 : range(n))
					for(int j1 : range(n))
						p += src[n * i + i1][n * j + j1];

				dst[i][j] = dst_pixel_t(p * normalizer);
			}
		}
	}

	static void downscale_bilinear_2x(const c4::matrix_ref<uint8_t>& src, c4::matrix<uint8_t>& dst) {
        c4::scoped_timer timer("downscale2x");

		int height2 = src.height() / 2;
		int width2 = src.width() / 2;

		dst.resize(height2, width2);

		for(int i : range(height2)) {
			const uint8_t* psrc0 = src[2 * i + 0];
			const uint8_t* psrc1 = src[2 * i + 1];

			uint8_t* pdst = dst[i];

			int j = 0;

			for(; j + 16 < width2; j += 16){
				c4::simd::uint8x16x2 s0x2 = c4::simd::load_2_interleaved(psrc0 + 2 * j);
                c4::simd::uint8x16x2 s1x2 = c4::simd::load_2_interleaved(psrc1 + 2 * j);
				
                c4::simd::uint8x16 s0 = c4::simd::avg(s0x2.val[0], s0x2.val[1]);
                c4::simd::uint8x16 s1 = c4::simd::avg(s1x2.val[0], s1x2.val[1]);

                c4::simd::uint8x16 d = c4::simd::avg(s0, s1);

                c4::simd::store(pdst + j, d);
			}

            for(; j < width2; j++)
				pdst[j] = ((psrc0[2 * j + 0] + psrc0[2 * j + 1] + 1) / 2 + (psrc1[2 * j + 0] + psrc1[2 * j + 1] + 1) / 2 + 1) / 2;
		}
	}

	static void downscale_linear_2x(const c4::matrix_ref<uint8_t>& src, c4::matrix<uint8_t>& dst) {
        c4::scoped_timer timer("downscale2xFast");

		int height2 = src.height() / 2;
		int width2 = src.width() / 2;

		dst.resize(height2, width2);

		for(int i : range(height2)) {
			const uint8_t* psrc = src[2 * i + 0];

			uint8_t* pdst = dst[i];

			int j = 0;

			for(; j + 16 < width2; j += 16) {
				c4::simd::uint8x16x2 s0x2 = c4::simd::load_2_interleaved(psrc + 2 * j);

                c4::simd::uint8x16 d = c4::simd::avg(s0x2.val[0], s0x2.val[1]);

                c4::simd::store(pdst + j, d);
			}

            for(; j < width2; j++)
				pdst[j] = (psrc[2 * j + 0] + psrc[2 * j + 1] + 1) / 2;
		}
	}

	static void downscale_bilinear_2x(const c4::matrix_ref<std::pair<uint8_t, uint8_t> >& src, c4::matrix<std::pair<uint8_t, uint8_t> >& dst) {
        c4::scoped_timer timer("downscale2x");

		int height2 = src.height() / 2;
		int width2 = src.width() / 2;

		dst.resize(height2, width2);

		for(int i : range(height2)) {
			const uint8_t* psrc0 = (const uint8_t*)src[2 * i + 0];
			const uint8_t* psrc1 = (const uint8_t*)src[2 * i + 1];

			uint8_t* pdst = (uint8_t*)dst[i];

			int j = 0;

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

            for(; j < width2; j++){
				pdst[2 * j + 0] = ((psrc0[4 * j + 0] + psrc0[4 * j + 2] + 1) / 2 + (psrc1[4 * j + 0] + psrc1[4 * j + 2] + 1) / 2 + 1) / 2;
				pdst[2 * j + 1] = ((psrc0[4 * j + 1] + psrc0[4 * j + 3] + 1) / 2 + (psrc1[4 * j + 1] + psrc1[4 * j + 3] + 1) / 2 + 1) / 2;
			}
		}
	}

	static void downscale_linear_2x(const c4::matrix_ref<std::pair<uint8_t, uint8_t> >& src, c4::matrix<std::pair<uint8_t, uint8_t> >& dst) {
        c4::scoped_timer timer("downscale2xFast");

		int height2 = src.height() / 2;
		int width2 = src.width() / 2;

		dst.resize(height2, width2);

		for(int i : range(height2)) {
			const uint8_t* psrc = (const uint8_t*)src[2 * i];

			uint8_t* pdst = (uint8_t*)dst[i];

			int j = 0;

			for(; j + 16 < width2; j += 16) {
                c4::simd::uint8x16x4 s0x4 = c4::simd::load_4_interleaved(psrc + 4 * j);

                c4::simd::uint8x16x2 d;
				d.val[0] = c4::simd::avg(s0x4.val[0], s0x4.val[2]);
				d.val[1] = c4::simd::avg(s0x4.val[1], s0x4.val[3]);

                c4::simd::store_2_interleaved(pdst + 2 * j, d);
			}

            for(; j < width2; j++) {
				pdst[2 * j + 0] = (psrc[4 * j + 0] + psrc[4 * j + 2] + 1) / 2;
				pdst[2 * j + 1] = (psrc[4 * j + 1] + psrc[4 * j + 3] + 1) / 2;
			}
		}
	}

	template<typename src_pixel_t, typename dst_pixel_t>
	inline void downscale_nearest_neighbor_nx(const c4::matrix_ref<src_pixel_t>& src, c4::matrix_ref<dst_pixel_t>& dst, float q){
		c4::scoped_timer timer("downscaleNnNx");

		float iq = 1.f / q;

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
						p += psrc[j1];
				}

				dst[i][j] = dst_pixel_t(p * normalizer);
			}
		}
	}

	template<typename SrcPixelT, typename DstPixelT>
	inline void scale_image_hq(const c4::matrix<SrcPixelT>& src, c4::matrix<DstPixelT>& dst, float q = 0){
        c4::scoped_timer timer("scaleImageHQ");

		int height0 = (int)src.height();
		int width0 = (int)src.width();
		int height = (int)dst.height();
		int width = (int)dst.width();

		if( q == 0 )
			q = max<float>(float(height) / height0, float(width) / width0);

		float iq = 1.f / q;

		int n = int(iq);

		if( n < 2 )
			scale_image_bilinear(src, dst, q);
		else
			downscale_nearest_neighbor_nx(src, dst, q);
	}
};

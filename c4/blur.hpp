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

#include <c4/simd.hpp>
#include <c4/range.hpp>
#include <c4/math.hpp>
#include <c4/matrix.hpp>

namespace c4 {
	template<typename PixelT>
	inline void box_blur_1d(const PixelT* first, const PixelT* last, PixelT* dst, int r){
		float div = 1.f / (2 * r + 1);

		decltype(PixelT() + PixelT()) v(0);

		for(int j : range(1, r + 1)){
			v = v + first[j];
		}

		v = v * 2 + *first; // 2 - mirror border

		for(int j : range(r)){
			dst[j] = PixelT(v * div);
			v = v + first[j + r + 1] - first[r - j];//mirror border
		}
		
		int end = int(last - first) - r - 1;
		
		for(int j : range(r, end)){
			dst[j] = PixelT(v * div);
			v = v + first[j + r + 1] - first[j - r];
		}

        for (int j : range(r + 1)) {
			dst[end + j] = PixelT(v * div);
			v = v + first[(end + r + 1) - j - 1] - first[end + j - r];
		}
	}

	template<typename PixelT>
	void box_blur_horizontal(matrix_ref<PixelT>& image, int r){
		std::vector<PixelT> tmp(image.width());

		for(int i : range(image.height())) {
            box_blur_1d(image[i].data(), image[i].data() + image.width(), tmp.data(), r);
			std::copy(tmp.begin(), tmp.end(), image[i].data());
		}
	}

	template<typename PixelT>
	void box_blur_horizontal(const matrix_ref<PixelT>& src, matrix_ref<PixelT>& dst, int r){
		for(int i : range(src))
            box_blur_1d(src[i], src[i] + src.width(), dst[i], r);
	}

	template<>
	void box_blur_horizontal(const matrix_ref<uint8_t>& src, matrix_ref<uint8_t>& dst, int r) {
		int height = src.height();
		int width = src.width();

		int i = 0;

        std::vector<float> row4(width * 4);
        std::vector<float> tmp4(width * 4);

		for(; i + 4 <= height; i += 4){
			float* const prow4 = row4.data();

			using namespace simd;

			{
				const uint8_t* psrc[4];
				for(int k : range(4))
					psrc[k] = src[i + k];

				int j = 0;
				for(; j + 8 <= width; j += 8){
					float32x4x4 vtl, vth;
                    for (int k : range(4)) {
						float32x4x2 v = to_float(long_move(reinterpret_signed(load_long(psrc[k] + j))));
						vtl.val[k] = v.val[0];
						vth.val[k] = v.val[1];
					}

                    store_4_interleaved(prow4 + 4 * j + 0, vtl);
                    store_4_interleaved(prow4 + 4 * j + 16, vth);
				}

                for (; j < width; j++) {
                    for (int k : range(4)) {
                        prow4[4 * j + k] = (float)psrc[k][j];
                    }
                }
			}
			{
                float32x4 div(1.f / (2 * r + 1));

				float32x4 v(0.f);

				for(int j : range(1, r + 1)) {
					float32x4 t = load(prow4 + j * 4);
					v = add(v, t);
				}

				v = add(add(v, v), load(prow4));

				float* ptmp4 = tmp4.data();

				for(int j : range(r)) {
					float32x4 d = mul(v, div);
					store(ptmp4, d);
					ptmp4 += 4;

					v = add(v, sub(load(prow4 + (j + r + 1) * 4), load(prow4 + (r - j) * 4)));
				}

				int end = width - r - 1;

				for(int j : range(r, end)) {
					float32x4 d = mul(v, div);
					store(ptmp4, d);
					ptmp4 += 4;

					v = add(v, sub(load(prow4 + (j + r + 1) * 4), load(prow4 + (j - r) * 4)));
				}

                for (int j : range(r + 1)) {
					float32x4 d = mul(v, div);
					store(ptmp4, d);
					ptmp4 += 4;

					v = add(v, sub(load(prow4 + ((end + r + 1) - j - 1) * 4), load(prow4 + (end + j - r) * 4)));
				}
			}
            {
                float* ptmp4 = tmp4.data();
                uint8_t* pdst[4];
                for (int k : range(4))
                    pdst[k] = dst[i + k];

                int j = 0;
                for (; j + 8 <= width; j += 8) {
                    float32x4x4 vtl, vth;
                    vtl = load_4_interleaved(ptmp4 + 4 * j + 0);
                    vth = load_4_interleaved(ptmp4 + 4 * j + 16);

                    for (int k : range(4)) {
                        float32x4x2 v{ vtl.val[k], vth.val[k] };
                        store(pdst[k] + j, narrow_unsigned_saturate(narrow(to_int(v))));
                    }
                }

                for (; j < width; j++){
                    for (int k : range(4)) {
                        pdst[k][j] = math::clamp<uint8_t>((int)ptmp4[4 * j + k]);
                    }
                }
			}
		}

		for(; i < height; i++)
            box_blur_1d(src[i].data(), src[i].data() + src.width(), dst[i].data(), r);
	}

	template<typename PixelT>
	void box_blur_vertical(matrix_ref<PixelT>& image, int r){
		int height = image.height();
		int width = image.width();
		int stride = image.stride();

        std::vector<PixelT> row(image.height());
        std::vector<PixelT> tmp(image.height());
		
		for(int j : range(width)){
			PixelT* ptr = image[0] + j;
			for(int i : range(height)){
				row[i] = *ptr;
				ptr += stride;
			}

			box_blur_1d(row.data(), row.data() + row.size(), tmp.data(), r);

			ptr = image[0] + j;
			for(int i : range(height)){
				*ptr = tmp[i];
				ptr += stride;
			}
		}
	}

	template<>
	void box_blur_vertical(matrix_ref<uint8_t>& image, int r) {
		int height = image.height();
		int width = image.width();
		int stride = image.stride();

		int j = 0;

        std::vector<uint8_t> row16(image.height() * 16);

		for(; j + 16 <= width; j += 16) {
			uint8_t* ptr = image[0] + j;
			uint8_t*const rptr = row16.data();
            
            using namespace simd;

            for(int i : range(height)) {
				uint8x16 v = load(ptr);
                store(rptr + i * 16, v);
				ptr += stride;
			}

            float32x4 div(1.f / (2 * r + 1));

            float32x4x4 v{ float32x4(0.f), float32x4(0.f), float32x4(0.f), float32x4(0.f) };

			for(int j : range(1, r + 1)) {
                float32x4x4 t = to_float(long_move(reinterpret_signed(long_move(load(rptr + j * 16)))));
				v = add(v, t);
			}
			
            float32x4x4 t = to_float(long_move(reinterpret_signed(long_move(load(rptr)))));
            v = add(add(v, v), t);

			ptr = image[0] + j;

			for(int j : range(r)) {
                float32x4x4 d = mul(v, div);
                uint8x16 u8 = narrow(reinterpret_unsigned(narrow(to_int(d))));
				store(ptr, u8);
				ptr += stride;

                int16x8x2 t1 = reinterpret_signed(long_move(load(rptr + (j + r + 1) * 16)));
                int16x8x2 t2 = reinterpret_signed(long_move(load(rptr + (r - j) * 16)));

                float32x4x4 dt = to_float(long_move(sub(t1, t2)));

				v = add(v, dt);
			}

			int end = height - r - 1;

			for(int j : range(r, end)) {
                float32x4x4 d = mul(v, div);
                uint8x16 u8 = narrow(reinterpret_unsigned(narrow(to_int(d))));
                store(ptr, u8);
				ptr += stride;

                int16x8x2 t1 = reinterpret_signed(long_move(load(rptr + (j + r + 1) * 16)));
                int16x8x2 t2 = reinterpret_signed(long_move(load(rptr + (j - r) * 16)));

                float32x4x4 dt = to_float(long_move(sub(t1, t2)));

                v = add(v, dt);
            }

			for(int j : range(r + 1)) {
                float32x4x4 d = mul(v, div);
                uint8x16 u8 = narrow(reinterpret_unsigned(narrow(to_int(d))));
                store(ptr, u8);
                ptr += stride;

                int16x8x2 t1 = reinterpret_signed(long_move(load(rptr + ((end + r + 1) - j - 1) * 16)));
                int16x8x2 t2 = reinterpret_signed(long_move(load(rptr + (end + j - r) * 16)));

                float32x4x4 dt = to_float(long_move(sub(t1, t2)));

                v = add(v, dt);
			}
		}

        std::vector<uint8_t> row(image.height());
        std::vector<uint8_t> tmp(image.height());

		for(; j < width; j++) {
			uint8_t* ptr = image[0] + j;
			for(int i : range(height)) {
				row[i] = *ptr;
				ptr += stride;
			}

			box_blur_1d(row.data(), row.data() + row.size(), tmp.data(), r);

			ptr = image[0] + j;
			for(int i : range(height)) {
				*ptr = tmp[i];
				ptr += stride;
			}
		}
	}

	template<typename PixelT>
	void box_blur(const matrix_ref<PixelT>& src, matrix<PixelT>& dst, int r){
		dst.resize(src);

		box_blur_horizontal(src, dst, r);
		box_blur_vertical(dst, r);
	}

	template<typename PixelT>
	void box_blur(matrix_ref<PixelT>& image, int r){
        box_blur_horizontal(image, r);
        box_blur_vertical(image, r);
	}

	template<typename PixelT>
	inline matrix<PixelT> box_blurred(const matrix_ref<PixelT>& src, int r){
		matrix<PixelT> res;

		box_blur(src, res, r);

		return res;
	}
};

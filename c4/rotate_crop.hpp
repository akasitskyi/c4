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

#include <c4/matrix.hpp>

namespace c4 {
	template<typename PixelT>
	void rotate_crop_nearest_neighbor(const c4::matrix_ref<PixelT>& src, float alpha, int left, int top, c4::matrix_ref<PixelT>& dst) {
		const float w2 = src.width() * 0.5f;
		const float h2 = src.height() * 0.5f;

		const float sn = sin(-alpha);
		const float cs = cos(-alpha);

		const float rw2 = abs(w2 * cs) + abs(h2 * sn);
		const float rh2 = abs(h2 * cs) + abs(w2 * sn);

        for (int i : c4::range(dst.height())) {
            for (int j : c4::range(dst.width())) {
				int i0 = top + i;
				int j0 = left + j;

				float di0 = i0 - rh2;
				float dj0 = j0 - rw2;

				float dir0f = sn * dj0 + cs * di0;
				float djr0f = cs * dj0 - sn * di0;

				int ir0 = int(h2 + dir0f + 0.5f);
				int jr0 = int(w2 + djr0f + 0.5f);

				dst[i][j] = src.clamp_get(ir0, jr0);
			}
		}
	}

	template<typename PixelT>
	void rotate_crop_bilinear(const c4::matrix_ref<PixelT>& src, float alpha, int left, int top, c4::matrix_ref<PixelT>& dst) {
		const float w2 = src.width() * 0.5f;
		const float h2 = src.height() * 0.5f;

		const float sn = sin(-alpha);
		const float cs = cos(-alpha);

		const float rw2 = abs(w2 * cs) + abs(h2 * sn);
		const float rh2 = abs(h2 * cs) + abs(w2 * sn);

        for (int i : c4::range(dst.height())) {
            for (int j : c4::range(dst.width())) {
				int i0 = top + i;
				int j0 = left + j;

				float di0 = i0 - rh2;
				float dj0 = j0 - rw2;

				float dir0f = sn * dj0 + cs * di0;
				float djr0f = cs * dj0 - sn * di0;

				float ir0f = h2 + dir0f;
				float jr0f = w2 + djr0f;

				int ir0 = int(ir0f);
				int jr0 = int(jr0f);

				float mir0 = ir0f - ir0;
				float mjr0 = jr0f - jr0;

				dst[i][j] = PixelT((src.clamp_get(ir0, jr0) * (1 - mir0) + src.clamp_get(ir0 + 1, jr0) * mir0) * (1 - mjr0)
								+ (src.clamp_get(ir0, jr0 + 1) * (1 - mir0) + src.clamp_get(ir0 + 1, jr0 + 1) * mir0) * mjr0);
			}
		}
	}
};

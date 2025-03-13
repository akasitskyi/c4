//MIT License
//
//Copyright(c) 2025 Alex Kasitskyi
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
#include "optimization.hpp"

#include <numeric>

namespace c4 {
	class MotionDetector {
	public:
		struct Motion {
			point<double> shift;
			double scale;
			double alpha;

			point<double> apply(const point<double>& p) const {
				return p.rotate(alpha) * scale + shift;
			}
		};

		struct Params {
			float dxMax = 0.1f;
			float dyMax = 0.1f;
			float scaleMax = 1.1f;
			float alphaMax = float(std::numbers::pi / 10);
		};

		Params params;
		
		MotionDetector() {
		}
		
		Motion detect(const matrix_ref<uint8_t>& prev, const matrix_ref<uint8_t>& frame) {
			c4::scoped_timer timer("MotionDetector::detect time");

			ASSERT_EQUAL(prev.height(), frame.height());
			ASSERT_EQUAL(prev.width(), frame.width());

			const int block = 16;
			const int halfBlock = block / 2;
			std::vector<point<double>> src, dst;
			std::vector<double> weights;

			for (int y = halfBlock; y + block + halfBlock <= frame.height(); y += block) {
				for (int x = halfBlock; x + block + halfBlock <= frame.width(); x += block) {
					point<int> shift(0, 0);
					const int diff0 = calcDiff(frame.submatrix(y, x, block, block), prev.submatrix(y, x, block, block));
					int minDiff = diff0;

					for (int dy = -halfBlock; dy <= halfBlock; dy++) {
						for (int dx = -halfBlock; dx <= halfBlock; dx++) {
							if (dx == 0 && dy == 0) {
								continue;
							}

							const double diff = calcDiff(frame.submatrix(y + dy, x + dx, block, block), prev.submatrix(y, x, block, block));

							if (diff < minDiff) {
								minDiff = diff;
								shift = { dx, dy };
							}
						}
					}
					src.emplace_back(x + halfBlock, y + halfBlock);
					dst.emplace_back(x + halfBlock + shift.x, y + halfBlock + shift.y);
					
					const double eps = 1E-5;
					weights.push_back(1. - (minDiff + eps) / (diff0 + eps));
				}
			}

			std::vector<double> v0{0., 0., 1., 0.};
			std::vector<double> l{-frame.width() * params.dxMax, -frame.height() * params.dyMax, 1. / params.scaleMax, -params.alphaMax};
			std::vector<double> h{frame.width() * params.dxMax, frame.height() * params.dyMax, params.scaleMax, params.alphaMax};

			auto errorF = [&](const std::vector<double>& v) {
				Motion motion{ {v[0], v[1]}, v[2], v[3] };

				double sum = 0;
				for (int i : range(src)) {
					const point<double> dst1 = motion.apply(src[i]);
					sum += weights[i] * dist_squared(dst[i], dst1);
				}
				return sum;
			};

			auto m = minimize(l, h, v0, errorF);

			PRINT_DEBUG(errorF(m));

			return { {m[0], m[1]}, m[2], m[3] };
		}

	private:
		int calcDiff(const matrix_ref<uint8_t>& A, const matrix_ref<uint8_t>& B) {
			ASSERT_EQUAL(A.width(), 16);

			simd::uint32x4 sum(0);

			for (int i : range(A.height())) {
				simd::uint8x16 a = simd::load(A[i].data());
				simd::uint8x16 b = simd::load(B[i].data());
				simd::uint32x4 diff = simd::sad(a, b);
				sum = simd::add(sum, diff);
			}

			uint32_t arr[4];
			simd::store(arr, sum);

			return arr[0] + arr[2];
		}
	};
};

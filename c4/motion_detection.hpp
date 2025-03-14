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
#include "drawing.hpp"
#include "scaling.hpp"
#include "image_dumper.hpp"
#include "optimization.hpp"

#include <numeric>

namespace c4 {
	class MotionDetector {
		//static constexpr int block = 32;
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
			double scaleMax = 1.;
			double alphaMax = std::numbers::pi * 0.1;
			int downscale = 2;
			int blockSize = 32;
		};

		MotionDetector() {
		}
		
		Motion detect(const matrix_ref<uint8_t>& prev, const matrix_ref<uint8_t>& frame, const Params& params, const std::vector<rectangle<int>> ignore = {}) {
			c4::scoped_timer timer("MotionDetector::detect");

			ASSERT_EQUAL(prev.height(), frame.height());
			ASSERT_EQUAL(prev.width(), frame.width());

			matrix<point<int>> shifts;
			matrix<double> weights;

			switch (params.downscale) {
			case 1:
				detect_local(prev, frame, shifts, weights, params.blockSize);
				break;
			case 2:
				downscale_bilinear_2x(prev, downPrev);
				downscale_bilinear_2x(frame, downFrame);
				detect_local(downPrev, downFrame, shifts, weights, params.blockSize);
				break;
			case 3:
				downscale_bilinear_3x(prev, downPrev);
				downscale_bilinear_3x(frame, downFrame);
				detect_local(downPrev, downFrame, shifts, weights, params.blockSize);
				break;
			case 4:
				downscale_bilinear_4x(prev, downPrev);
				downscale_bilinear_4x(frame, downFrame);
				detect_local(downPrev, downFrame, shifts, weights, params.blockSize);
				break;
			default:
				INVALID_VALUE(params.downscale);
			}

			if (params.downscale > 1){
				transform_inplace(shifts, [&params](const point<int>& p) { return p * params.downscale; });
			}

			const int block = params.blockSize * params.downscale;

			src.resize(shifts.dimensions());
			dst.resize(shifts.dimensions());

			for (int i : range(shifts.height())) {
				for (int j : range(shifts.width())) {
					src[i][j] = point<double>((j + 1) * block, (i + 1) * block);
					dst[i][j] = src[i][j] + point<double>(shifts[i][j]);

					for (const rectangle<int>& r : ignore) {
						if (r.contains(src[i][j])) {
							weights[i][j] = 0;
							break;
						}
					}
				}
			}

			if (image_dumper::getInstance().isEnabled()) {
				image4dump = frame;
				for (int i : range(shifts.height())) {
					for (int j : range(shifts.width())) {
						if (weights[i][j] > 0) {
							uint8_t color = 127 + 128 * weights[i][j];
							draw_line(image4dump, src[i][j], src[i][j] + point<double>(shifts[i][j]), color, 1);
							draw_point(image4dump, src[i][j] + point<double>(shifts[i][j]), color, 5);
						}
					}
				}
				dump_image(image4dump, "local");
			}

			std::vector<double> v0{0., 0., 1., 0.};
			std::vector<double> l{-1. * block, -1. * block, 1. / params.scaleMax, -params.alphaMax};
			std::vector<double> h{ 1. * block, 1. * block, params.scaleMax, params.alphaMax};

			auto errorF = [&](const std::vector<double>& v) {
				Motion motion{ {v[0], v[1]}, v[2], v[3] };

				double sum = 0;
				for (int i : range(src.height())) {
					for (int j : range(src.width())) {
						const point<double> dst1 = motion.apply(src[i][j]);
						sum += weights[i][j] * dist_squared(dst[i][j], dst1);
					}
				}

				return sum;
			};

			c4::scoped_timer timer1("MotionDetector::detect minimize");

			auto m = minimize(l, h, v0, errorF);

			PRINT_DEBUG(errorF(m));

			return { {m[0], m[1]}, m[2], m[3] };
		}

		static void detect_local(const matrix_ref<uint8_t>& prev, const matrix_ref<uint8_t>& frame, matrix<point<int>>& shifts, matrix<double>& weights, int blockSize) {
			switch (blockSize) {
			case 8:
				return detect_local_impl<8>(prev, frame, shifts, weights);
			case 16:
				return detect_local_impl<16>(prev, frame, shifts, weights);
			case 24:
				return detect_local_impl<24>(prev, frame, shifts, weights);
			case 32:
				return detect_local_impl<32>(prev, frame, shifts, weights);
			case 48:
				return detect_local_impl<48>(prev, frame, shifts, weights);
			case 64:
				return detect_local_impl<64>(prev, frame, shifts, weights);
			default:
				INVALID_VALUE(blockSize);
			}
		}
		
		template<int block>
		static void detect_local_impl(const matrix_ref<uint8_t>& prev, const matrix_ref<uint8_t>& frame, matrix<point<int>>& shifts, matrix<double>& weights) {
			c4::scoped_timer timer("MotionDetector::detect_local_impl");

			ASSERT_EQUAL(prev.height(), frame.height());
			ASSERT_EQUAL(prev.width(), frame.width());

			const int halfBlock = block / 2;

			ASSERT_LESS(block + 2 * halfBlock, frame.height());
			ASSERT_LESS(block + 2 * halfBlock, frame.width());

			const int bh = (frame.height() - 2 * halfBlock) / block;
			const int bw = (frame.width() - 2 * halfBlock) / block;

			shifts = matrix<point<int>>(bh, bw);
			weights.resize(bh, bw);
			
			for (int i : range(shifts.height())) {
				for (int j : range(shifts.width())) {
					point<int>& shift = shifts[i][j];

					const int x = j * block + halfBlock;
					const int y = i * block + halfBlock;

					int minDiff = std::numeric_limits<int>::max();
					int sumDiff = 0;
					int cntDiff = 0;

					for (int dy = -halfBlock; dy <= halfBlock; dy++) {
						for (int dx = -halfBlock; dx <= halfBlock; dx++) {
							// random offset to break ties, so that the scan order doesn't matter
							const int randomOffset = (((y * 3 + dy) * 5 + x) * 7 + dx) * 13 % 32;
							const int diff = calcDiff<block>(frame.submatrix(y + dy, x + dx, block, block), prev.submatrix(y, x, block, block)) + randomOffset;

							sumDiff += diff;
							cntDiff++;

							if (diff < minDiff) {
								minDiff = diff;
								shift = { dx, dy };
							}
						}
					}
					const double avgDiff = double(sumDiff) / cntDiff;

					const double noiseOffset = 0.5 * block * block;
					weights[i][j] = 1. - (minDiff + noiseOffset) / (avgDiff + noiseOffset);
				}
			}
		}

	private:
		matrix<uint8_t> downFrame;
		matrix<uint8_t> downPrev;
		
		matrix<point<double>> src;
		matrix<point<double>> dst;

		matrix<uint8_t> image4dump;

		template<int dim>
		static int inline calcDiff(const matrix_ref<uint8_t>& A, const matrix_ref<uint8_t>& B) = delete;

		template<>
		static int inline calcDiff<8>(const matrix_ref<uint8_t>& A, const matrix_ref<uint8_t>& B) {
			ASSERT_EQUAL(A.width(), 8);

			simd::uint32x4 sum(0);

			for (int i : range(A.height())) {
				simd::uint8x16 a1 = simd::load_half(A[i].data());
				simd::uint8x16 b1 = simd::load_half(B[i].data());
				sum = simd::add(sum, simd::sad(a1, b1));
			}

			uint32_t arr[4];
			simd::store(arr, sum);

			return arr[0] + arr[2];
		}

		template<>
		static int inline calcDiff<16>(const matrix_ref<uint8_t>& A, const matrix_ref<uint8_t>& B) {
			ASSERT_EQUAL(A.width(), 16);

			simd::uint32x4 sum(0);

			for (int i : range(A.height())) {
				simd::uint8x16 a1 = simd::load(A[i].data());
				simd::uint8x16 b1 = simd::load(B[i].data());
				sum = simd::add(sum, simd::sad(a1, b1));
			}

			uint32_t arr[4];
			simd::store(arr, sum);

			return arr[0] + arr[2];
		}
		
		template<>
		static int inline calcDiff<24>(const matrix_ref<uint8_t>& A, const matrix_ref<uint8_t>& B) {
			ASSERT_EQUAL(A.width(), 24);

			simd::uint32x4 sum(0);

			for (int i : range(A.height())) {
				simd::uint8x16 a1 = simd::load(A[i].data());
				simd::uint8x16 a2 = simd::load_half(A[i].data() + 16);
				simd::uint8x16 b1 = simd::load(B[i].data());
				simd::uint8x16 b2 = simd::load_half(B[i].data() + 16);
				sum = simd::add(sum, simd::add(simd::sad(a1, b1), simd::sad(a2, b2)));
			}

			uint32_t arr[4];
			simd::store(arr, sum);

			return arr[0] + arr[2];
		}

		template<>
		static int inline calcDiff<32>(const matrix_ref<uint8_t>& A, const matrix_ref<uint8_t>& B) {
			ASSERT_EQUAL(A.width(), 32);

			simd::uint32x4 sum(0);

			for (int i : range(A.height())) {
				simd::uint8x16 a1 = simd::load(A[i].data());
				simd::uint8x16 a2 = simd::load(A[i].data() + 16);
				simd::uint8x16 b1 = simd::load(B[i].data());
				simd::uint8x16 b2 = simd::load(B[i].data() + 16);
				sum = simd::add(sum, simd::add(simd::sad(a1, b1), simd::sad(a2, b2)));
			}

			uint32_t arr[4];
			simd::store(arr, sum);

			return arr[0] + arr[2];
		}

		template<>
		static int inline calcDiff<48>(const matrix_ref<uint8_t>& A, const matrix_ref<uint8_t>& B) {
			ASSERT_EQUAL(A.width(), 48);

			simd::uint32x4 sum(0);

			for (int i : range(A.height())) {
				simd::uint8x16 a1 = simd::load(A[i].data());
				simd::uint8x16 a2 = simd::load(A[i].data() + 16);
				simd::uint8x16 a3 = simd::load(A[i].data() + 32);
				simd::uint8x16 b1 = simd::load(B[i].data());
				simd::uint8x16 b2 = simd::load(B[i].data() + 16);
				simd::uint8x16 b3 = simd::load(B[i].data() + 32);
				sum = simd::add(sum, simd::add(simd::sad(a1, b1), simd::sad(a2, b2)));
				sum = simd::add(sum, simd::sad(a3, b3));
			}

			uint32_t arr[4];
			simd::store(arr, sum);

			return arr[0] + arr[2];
		}

		template<>
		static int inline calcDiff<64>(const matrix_ref<uint8_t>& A, const matrix_ref<uint8_t>& B) {
			ASSERT_EQUAL(A.width(), 64);

			simd::uint32x4 sum(0);

			for (int i : range(A.height())) {
				simd::uint8x16 a1 = simd::load(A[i].data());
				simd::uint8x16 a2 = simd::load(A[i].data() + 16);
				simd::uint8x16 a3 = simd::load(A[i].data() + 32);
				simd::uint8x16 a4 = simd::load(A[i].data() + 48);
				simd::uint8x16 b1 = simd::load(B[i].data());
				simd::uint8x16 b2 = simd::load(B[i].data() + 16);
				simd::uint8x16 b3 = simd::load(B[i].data() + 32);
				simd::uint8x16 b4 = simd::load(B[i].data() + 48);
				sum = simd::add(sum, simd::add(simd::sad(a1, b1), simd::sad(a2, b2)));
				sum = simd::add(sum, simd::add(simd::sad(a3, b3), simd::sad(a4, b4)));
			}

			uint32_t arr[4];
			simd::store(arr, sum);

			return arr[0] + arr[2];
		}
	};
};

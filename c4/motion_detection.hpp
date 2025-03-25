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
#include "parallel.hpp"
#include "image_dumper.hpp"
#include "optimization.hpp"

#include <numeric>
#include <algorithm>

namespace c4 {
	class MotionDetector {
	public:
		static inline point<double> center(const matrix_ref<uint8_t>& frame) {
			return point<double>((frame.width() - 1) * 0.5, (frame.height() - 1) * 0.5);
		}

		struct Motion {
			point<double> shift;
			double scale = 1.0;
			double alpha = 0.0;

			point<double> apply(const matrix_ref<uint8_t>& frame, const point<double>& p) const {
				point<double> C = center(frame);
				return C + (p-C).rotate(alpha) * scale + shift;
			}

			void apply(const matrix_ref<uint8_t>& src, matrix_ref<uint8_t>& dst) const {
				c4::scoped_timer timer2("Motion::apply");
				ASSERT_EQUAL(src.dimensions(), dst.dimensions());

				const double sns = std::sin(alpha) * scale;
				const double css = std::cos(alpha) * scale;

				const point<double> C = center(src);
				const point<double> Cps = C + shift;

				parallel_for(range(src.height()), [&src, &dst, C, css, sns, Cps](int y){
					for (int x : c4::range(src.width())) {
						c4::point<double> p = c4::point<double>(x, y) - C;
						c4::point<double> prs(css * p.x - sns * p.y, sns * p.x + css * p.y);
						c4::point<double> t = prs + Cps;

						dst[y][x] = src.get_interpolate(t);
					}
				});
			}

			// combine two motions: first apply this motion, then apply other motion
			Motion combine(const Motion& other) const {
				return { shift.rotate(other.alpha) * other.scale + other.shift, scale * other.scale, alpha + other.alpha };
			}
		};

		struct Params {
			double scaleMax = 1.05;
			double alphaMax = std::numbers::pi * 0.1;

			int blockSize = 32;
			int maxShift = 16;
		};

		MotionDetector() {
		}
		
		static Motion detect(const matrix_ref<uint8_t>& prev, const matrix_ref<uint8_t>& frame, const Params& params, const std::vector<rectangle<int>> ignore = {}) {
			c4::scoped_timer timer("MotionDetector::detect");

			ASSERT_EQUAL(prev.height(), frame.height());
			ASSERT_EQUAL(prev.width(), frame.width());

			matrix<point<int>> shifts;
			matrix<double> weights;

			detect_local(prev, frame, shifts, weights, params.blockSize, params.maxShift);

			matrix<point<double>> src(shifts.dimensions());

			for (int i : range(shifts.height())) {
				for (int j : range(shifts.width())) {
					src[i][j] = point<double>((j + 1) * params.blockSize, (i + 1) * params.blockSize);

					for (const rectangle<int>& r : ignore) {
						if (r.contains(src[i][j])) {
							weights[i][j] = 0;
							break;
						}
					}
				}
			}

			if (image_dumper::getInstance().isEnabled()) {
				matrix<uint8_t> image4dump = frame;
				for (int i : range(shifts.height())) {
					for (int j : range(shifts.width())) {
						if (weights[i][j] > 0) {
							const uint8_t color = 255;
							point<double> dst = src[i][j] + point<double>(shifts[i][j]);
							draw_line(image4dump, src[i][j], dst, color, 1, weights[i][j]);
							draw_point(image4dump, dst, color, 5, weights[i][j]);
						}
					}
				}
				dump_image(image4dump, "local");
			}

			return motion_from_local_mat(frame, src, shifts, weights, params);
		}

		static Motion motion_from_local_mat(const matrix_ref<uint8_t>& frame, const matrix<point<double>>& src, const matrix<point<int>>& shifts, const matrix<double>& weights, const Params& params) {
			point<double> sumShift(0, 0);
			double sumWeight = 0;

			for (int i : range(shifts.height())) {
				for (int j : range(shifts.width())) {
					sumShift += shifts[i][j] * weights[i][j];
					sumWeight += weights[i][j];
				}
			}
			const double EPS = 1E-6;
			if(sumWeight < EPS){
				return Motion{{0, 0}, 0, 0};
			}

			const point<double> rshift = sumShift * (1. / sumWeight);

			matrix<point<double>> dst(shifts.dimensions());
			transform(src, shifts, [rshift](const point<double>& s, const point<int>& shift) { return s + point<double>(shift) - rshift; }, dst);

			const point<double> C = center(frame);

			double sumScale = 0;
			sumWeight = 0;
			
			for (int i : range(shifts.height())) {
				for (int j : range(shifts.width())) {
					const double d0 = dist(C, src[i][j]);
					const double d1 = dist(C, dst[i][j]);
					if (d0 < EPS){
						continue;
					}
					const double scale = d1 / d0;
					const double weight = weights[i][j] * d0;
					sumScale += scale * weight;
					sumWeight += weight;
				}
			}

			const double rscale = std::clamp(sumScale / sumWeight, 1. / params.scaleMax, params.scaleMax);

			transform_inplace(dst, [rscale](const point<double>& p) { return p * rscale; });

			double sumSinAlpha = 0;
			sumWeight = 0;

			for (int i : range(shifts.height())) {
				for (int j : range(shifts.width())) {
					const point<double> A = src[i][j] - C;
					const point<double> B = dst[i][j] - C;

					const double lA = A.length();
					const double lB = A.length();
					if (lA < EPS || lB < EPS) {
						continue;
					}

					const double xp = A ^ B;
					const double sinAlpha = xp / (lA * lB);
					const double weight = weights[i][j] * lA;

					sumSinAlpha += sinAlpha * weight;
					sumWeight += weight;
				}
			}

			const double alpha = std::clamp(std::asin(sumSinAlpha / sumWeight), -params.alphaMax, params.alphaMax);

			return { rshift, rscale, alpha };
		}

		static void detect_local(const matrix_ref<uint8_t>& prev, const matrix_ref<uint8_t>& frame, matrix<point<int>>& shifts, matrix<double>& weights, int blockSize, int maxShift) {
			switch (blockSize) {
			case 16:
				return detect_local_impl<16>(prev, frame, shifts, weights, maxShift);
			case 32:
				return detect_local_impl<32>(prev, frame, shifts, weights, maxShift);
			case 48:
				return detect_local_impl<48>(prev, frame, shifts, weights, maxShift);
			case 64:
				return detect_local_impl<64>(prev, frame, shifts, weights, maxShift);
			default:
				INVALID_VALUE(blockSize);
			}
		}
		
		template<int block>
		static void detect_local_impl(const matrix_ref<uint8_t>& prev, const matrix_ref<uint8_t>& frame, matrix<point<int>>& shifts, matrix<double>& weights, const int maxShift) {
			c4::scoped_timer timer("MotionDetector::detect_local_impl");

			ASSERT_EQUAL(prev.height(), frame.height());
			ASSERT_EQUAL(prev.width(), frame.width());

			const int halfBlock = block / 2;

			ASSERT_LESS(block + 2 * halfBlock, frame.height());
			ASSERT_LESS(block + 2 * halfBlock, frame.width());
			ASSERT_LESS_EQUAL(maxShift, halfBlock);

			const int bh = (frame.height() - 2 * halfBlock) / block;
			const int bw = (frame.width() - 2 * halfBlock) / block;

			shifts.resize(bh, bw);
			weights.resize(bh, bw);

			ASSERT_EQUAL(block & (block - 1), 0);
			ASSERT_TRUE(block <= 64); // our 32-bit diff calculation can handle up to 64x64 blocks

			const uint32_t diffMul = 64 * 64;
			const uint32_t randMask = diffMul - 1;

			constexpr int block2 = block * block;

			enumerable_thread_specific<matrix<int>> diffs_v;
			for (auto& diffs : diffs_v){
				diffs.resize(2 * maxShift + 1, 2 * maxShift + 1);
			}

			parallel_for(range(shifts.height()), [maxShift, &diffs_v, &shifts, &prev, &frame, &weights](int i){
				auto& diffs = diffs_v.local();

				for (int j : range(shifts.width())) {
					point<int> shift(0, 0);

					const int x = j * block + halfBlock;
					const int y = i * block + halfBlock;

					uint32_t minDiff = std::numeric_limits<uint32_t>::max();

					const auto sma = prev.submatrix(y, x, block, block);

					const int avg_a = (accumulate<block>(sma) + block2 / 2) / block2;

					for (int dy = -maxShift; dy <= maxShift; dy++) {
						for (int dx = -maxShift; dx <= maxShift; dx++) {
							const auto smb = frame.submatrix(y + dy, x + dx, block, block);
							const int avg_b = (accumulate<block>(smb) + block2 / 2) / block2;
							
							const uint8_t da = std::max(avg_b - avg_a, 0);
							const uint8_t db = std::max(avg_a - avg_b, 0);
							
							uint32_t diff = calc_diff<block>(sma, smb, da, db);
							diffs[maxShift + dy][maxShift + dx] = diff;

							// random offset to break ties, so that the scan order doesn't matter
							const uint32_t randomOffset = ((((y + 10007) * 10009 + dy) * 10037 + x) * 10039 + dx) * 10061 & randMask;
							diff = diff * diffMul +  randomOffset;


							if (diff < minDiff) {
								minDiff = diff;
								shift = { dx, dy };
							}
						}
					}

					const double noiseOffset = block * block;
					minDiff /= diffMul;

					double maxQ = 0;

					for (int i = 0; i < diffs.height(); i++) {
						for (int j = 0; j < diffs.width(); j++) {
							const int d2 = dist_squared(shift, { j - maxShift, i - maxShift });
							const double qdiff = (minDiff + noiseOffset) / (diffs[i][j] + noiseOffset);
							const double qlength = std::min(0.05 * d2, 1.);
							const double q = qdiff * qlength;
							maxQ = std::max(maxQ, q);
						}
					}
					const double w = 1. - maxQ;

					shifts[i][j] = shift;
					weights[i][j] = w;
				}
			});
		}

	private:
		template<int dim>
		static uint32_t inline accumulate(const matrix_ref<uint8_t>& src) = delete;

		template<>
		static uint32_t inline accumulate<16>(const matrix_ref<uint8_t>& src) {
			assert(src.width() == 16);
			assert(src.height() == 16);

			simd::uint32x4 sum = simd::set_zero<simd::uint32x4>();

			for (int i = 0; i < 16; i++) {
				auto a = simd::load(src[i].data());
				sum = simd::add(sum, simd::sad(a, simd::set_zero<simd::uint8x16>()));
			}

			return simd::sum02(sum);
		}

		template<>
		static uint32_t inline accumulate<32>(const matrix_ref<uint8_t>& src) {
			assert(src.width() == 32);
			assert(src.height() == 32);

			simd::uint32x4 sum = simd::set_zero<simd::uint32x4>();

			for (int i = 0; i < 32; i++) {
				auto a = simd::load(src[i].data());
				auto b = simd::load(src[i].data() + 16);
				sum = simd::add(sum, simd::sad(a, simd::set_zero<simd::uint8x16>()));
				sum = simd::add(sum, simd::sad(b, simd::set_zero<simd::uint8x16>()));
			}

			return simd::sum02(sum);
		}

		template<>
		static uint32_t inline accumulate<48>(const matrix_ref<uint8_t>& src) {
			assert(src.width() == 48);
			assert(src.height() == 48);

			simd::uint32x4 sum = simd::set_zero<simd::uint32x4>();

			for (int i = 0; i < 48; i++) {
				auto a = simd::load(src[i].data());
				auto b = simd::load(src[i].data() + 16);
				auto c = simd::load(src[i].data() + 32);
				sum = simd::add(sum, simd::sad(a, simd::set_zero<simd::uint8x16>()));
				sum = simd::add(sum, simd::sad(b, simd::set_zero<simd::uint8x16>()));
				sum = simd::add(sum, simd::sad(c, simd::set_zero<simd::uint8x16>()));
			}

			return simd::sum02(sum);
		}

		template<>
		static uint32_t inline accumulate<64>(const matrix_ref<uint8_t>& src) {
			assert(src.width() == 64);
			assert(src.height() == 64);

			simd::uint32x4 sum = simd::set_zero<simd::uint32x4>();

			for (int i = 0; i < 64; i++) {
				auto a = simd::load(src[i].data());
				auto b = simd::load(src[i].data() + 16);
				auto c = simd::load(src[i].data() + 32);
				auto d = simd::load(src[i].data() + 48);

				sum = simd::add(sum, simd::sad(a, simd::set_zero<simd::uint8x16>()));
				sum = simd::add(sum, simd::sad(b, simd::set_zero<simd::uint8x16>()));
				sum = simd::add(sum, simd::sad(c, simd::set_zero<simd::uint8x16>()));
				sum = simd::add(sum, simd::sad(d, simd::set_zero<simd::uint8x16>()));
			}

			return simd::sum02(sum);
		}

		template<int dim>
		static uint32_t inline calc_diff(const matrix_ref<uint8_t>& A, const matrix_ref<uint8_t>& B, const uint8_t da, const uint8_t db) = delete;

		template<>
		static uint32_t inline calc_diff<16>(const matrix_ref<uint8_t>& A, const matrix_ref<uint8_t>& B, const uint8_t da, const uint8_t db) {
			assert(A.width() == 16);

			const simd::uint8x16 dav(da);
			const simd::uint8x16 dbv(db);

			simd::uint32x4 sum = simd::set_zero<simd::uint32x4>();

			for (int i = 0; i < 16; i++) {
				simd::uint8x16 a1 = simd::load(A[i].data());
				simd::uint8x16 b1 = simd::load(B[i].data());
				a1 = simd::add_saturate(a1, dav);
				b1 = simd::add_saturate(b1, dbv);
				sum = simd::add(sum, simd::sad(a1, b1));
			}

			return simd::sum02(sum);
		}
		
		template<>
		static uint32_t inline calc_diff<32>(const matrix_ref<uint8_t>& A, const matrix_ref<uint8_t>& B, const uint8_t da, const uint8_t db) {
			assert(A.width() == 32);

			const simd::uint8x16 dav(da);
			const simd::uint8x16 dbv(db);

			simd::uint32x4 sum = simd::set_zero<simd::uint32x4>();

			for (int i = 0; i < 32; i++) {
				simd::uint8x16 a1 = simd::load(A[i].data());
				simd::uint8x16 a2 = simd::load(A[i].data() + 16);
				simd::uint8x16 b1 = simd::load(B[i].data());
				simd::uint8x16 b2 = simd::load(B[i].data() + 16);
				a1 = simd::add_saturate(a1, dav);
				a2 = simd::add_saturate(a2, dav);
				b1 = simd::add_saturate(b1, dbv);
				b2 = simd::add_saturate(b2, dbv);

				sum = simd::add(sum, simd::add(simd::sad(a1, b1), simd::sad(a2, b2)));
			}

			return simd::sum02(sum);
		}

		template<>
		static uint32_t inline calc_diff<48>(const matrix_ref<uint8_t>& A, const matrix_ref<uint8_t>& B, const uint8_t da, const uint8_t db) {
			assert(A.width() == 48);

			const simd::uint8x16 dav(da);
			const simd::uint8x16 dbv(db);

			simd::uint32x4 sum = simd::set_zero<simd::uint32x4>();

			for (int i = 0; i < 48; i++) {
				simd::uint8x16 a1 = simd::load(A[i].data());
				simd::uint8x16 a2 = simd::load(A[i].data() + 16);
				simd::uint8x16 a3 = simd::load(A[i].data() + 32);
				simd::uint8x16 b1 = simd::load(B[i].data());
				simd::uint8x16 b2 = simd::load(B[i].data() + 16);
				simd::uint8x16 b3 = simd::load(B[i].data() + 32);
				a1 = simd::add_saturate(a1, dav);
				a2 = simd::add_saturate(a2, dav);
				a3 = simd::add_saturate(a3, dav);
				b1 = simd::add_saturate(b1, dbv);
				b2 = simd::add_saturate(b2, dbv);
				b3 = simd::add_saturate(b3, dbv);

				sum = simd::add(sum, simd::add(simd::sad(a1, b1), simd::sad(a2, b2)));
				sum = simd::add(sum, simd::sad(a3, b3));
			}

			return simd::sum02(sum);
		}

		template<>
		static uint32_t inline calc_diff<64>(const matrix_ref<uint8_t>& A, const matrix_ref<uint8_t>& B, const uint8_t da, const uint8_t db) {
			assert(A.width() == 64);

			const simd::uint8x16 dav(da);
			const simd::uint8x16 dbv(db);

			simd::uint32x4 sum = simd::set_zero<simd::uint32x4>();

			for (int i = 0; i < 64; i++) {
				simd::uint8x16 a1 = simd::load(A[i].data());
				simd::uint8x16 a2 = simd::load(A[i].data() + 16);
				simd::uint8x16 a3 = simd::load(A[i].data() + 32);
				simd::uint8x16 a4 = simd::load(A[i].data() + 48);
				simd::uint8x16 b1 = simd::load(B[i].data());
				simd::uint8x16 b2 = simd::load(B[i].data() + 16);
				simd::uint8x16 b3 = simd::load(B[i].data() + 32);
				simd::uint8x16 b4 = simd::load(B[i].data() + 48);

				a1 = simd::add_saturate(a1, dav);
				a2 = simd::add_saturate(a2, dav);
				a3 = simd::add_saturate(a3, dav);
				a4 = simd::add_saturate(a4, dav);
				b1 = simd::add_saturate(b1, dbv);
				b2 = simd::add_saturate(b2, dbv);
				b3 = simd::add_saturate(b3, dbv);
				b4 = simd::add_saturate(b4, dbv);

				sum = simd::add(sum, simd::add(simd::sad(a1, b1), simd::sad(a2, b2)));
				sum = simd::add(sum, simd::add(simd::sad(a3, b3), simd::sad(a4, b4)));
			}

			return simd::sum02(sum);
		}
	};
};

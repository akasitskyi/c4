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

#include "fixed_point.hpp"
#include "range.hpp"
#include "serialize.hpp"
#include "geometry.hpp"
#include "linear_algebra.hpp"

namespace c4 {
    namespace impl {
        inline c4::affine_trasform<float> unnormalizing_tform(const c4::rectangle<int>& rect) {
            c4::point<float> tl((float)rect.x, (float)rect.y);

            auto tform = c4::affine_trasform<float>::move_trasform(tl).combine(c4::affine_trasform<float>::scale_trasform((float)rect.w, (float)rect.h));
            return tform;
        }

        typedef std::vector<c4::point<float>> shape_t;

        struct split_feature{
			int8_t l1, l2;
			static const int ALPHA_DENOM = 3;
			int8_t alpha_num;

			float k;

			float d1, d2;

            inline float f(const c4::matrix_ref<uint8_t>& img, const shape_t& shape, const c4::affine_trasform<float>& tformToImg) const {
                const c4::point<float> p1 = shape[l1];
                const c4::point<float> p2 = shape[l2];
                
                const c4::point<float> v = p2 - p1;
                
                const c4::point<float> rp = p1 + v * (float(alpha_num) / split_feature::ALPHA_DENOM);

                int y1 = img.clamp_get(tformToImg(rp + d1 * v));
                int y2 = img.clamp_get(tformToImg(rp + d2 * v));

                return (y1 - y2) / std::max(float(y1 + y2), 1.f);
            }

            inline bool evaluate(const c4::matrix_ref<uint8_t>& img, const shape_t& shape, const c4::affine_trasform<float>& tformToImg) const {
                return f(img, shape, tformToImg) >= k;
            }

            template<class Archive>
			void load(Archive & ar) {
                ar(l1, l2, alpha_num, d1, d2, k);
            }

            template<class Archive>
            void save(Archive & ar) const {
                ar(l1, l2, alpha_num, d1, d2, k);
            }
        };

        inline uint32_t left_child(uint32_t idx) { return 2*idx + 1; }

		inline uint32_t right_child(uint32_t idx) { return 2*idx + 2; }

		struct regression_tree{
			static constexpr int FP_SHIFT = 19;
			typedef std::vector<c4::point<c4::fixed_point<int16_t, FP_SHIFT>>> leaf_value_t;

            std::vector<split_feature> splits;
            std::vector<leaf_value_t> leaf_values;

			inline const leaf_value_t operator()(const c4::matrix_ref<uint8_t>& img, const shape_t& shape, const c4::affine_trasform<float>& tformToImg) const{
                int mask = 0;
                for (const auto& s : splits) {
                    mask = 2 * mask + (s.evaluate(img, shape, tformToImg) ? 0 : 1);
                }

                return leaf_values[mask];
            }

			template<class Archive>
			void save(Archive& ar) const {
				ar(splits);
				
				uint16_t lvc = uint16_t(leaf_values.size());
				ASSERT_EQUAL(lvc, leaf_values.size());
				uint16_t vdim = uint16_t(leaf_values[0].size());
				ASSERT_EQUAL(vdim, leaf_values[0].size());

				ar(lvc);
				ar(vdim);

				for(const leaf_value_t& l : leaf_values)
					ar(c4::serialize::as_binary(l.data(), vdim));
			}

			template<class Archive>
			void load(Archive& ar) {
				ar(splits);
				uint16_t lvc, vdim;
				
				ar(lvc);
				ar(vdim);

				leaf_values.resize(lvc, leaf_value_t(vdim));
				for(leaf_value_t& l : leaf_values)
					ar(c4::serialize::as_binary(l.data(), vdim));
			}
		};
    }; // end namespace impl

	class shape_predictor {
        impl::shape_t initial_shape;
		std::vector<impl::regression_tree> forest;
	public:
        shape_predictor() {}

        shape_predictor(const impl::shape_t& initial_shape, const std::vector<impl::regression_tree>& forest) : initial_shape(initial_shape), forest(forest) {
        }

        int landmarks_count() const {
            return c4::isize(initial_shape);
        }

        c4::object_on_image operator()(const c4::matrix_ref<uint8_t>& img, const c4::rectangle<int>& rect) const {
            using namespace impl;
            std::vector<c4::point<float>> current_shape = initial_shape;
			const auto tform_to_img = unnormalizing_tform(rect);

			for(auto& tree : forest)
                current_shape += tree(img, current_shape, tform_to_img);

            c4::object_on_image o;
            o.rect = rect;
            o.landmarks.resize(current_shape.size());

            std::transform(current_shape.begin(), current_shape.end(), o.landmarks.begin(), tform_to_img);

            return o;
        }

		template<class Archive>
		void save(Archive& ar) const {
			ar((int)impl::split_feature::ALPHA_DENOM);
			ar((int)impl::regression_tree::FP_SHIFT);

			ar(initial_shape, forest);
		}

		template<class Archive, typename T>
		void check(Archive& ar, const T c) {
			T t;
			ar(t);
			ASSERT_EQUAL(t, c);
		}

		template<class Archive>
		void load(Archive& ar) {
			check(ar, impl::split_feature::ALPHA_DENOM);
			check(ar, impl::regression_tree::FP_SHIFT);

			ar(initial_shape, forest);
		}
    };
};// namespace zeroalex

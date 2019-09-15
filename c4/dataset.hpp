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

#include <fstream>
#include <string>
#include <vector>

#include "meta_data_set.hpp"
#include "jpeg.hpp"
#include "string.hpp"
#include "range.hpp"
#include "scaling.hpp"
#include "parallel.hpp"
#include "logger.hpp"
#include "matrix_regression.hpp"
#include "image_dumper.hpp"
#include "drawing.hpp"
#include "progress_indicator.hpp"

namespace c4 {
    template<class FeatureSpaceTransform>
    struct dataset {
        const matrix_dimensions sample_size;
        const bool dump_rects;

        matrix<std::vector<uint8_t>> rx;
        std::vector<float> y;

        dataset(const matrix_dimensions& sample_size, const bool dump_rects = false) : sample_size(sample_size), dump_rects(dump_rects), rx(FeatureSpaceTransform::calc_dimensions(sample_size)) {}

        static void generate_samples(const matrix_dimensions& sample_size, const matrix<uint8_t>& img, const std::vector<object_on_image>& objects, std::vector<matrix<uint8_t>>& pos, std::vector<matrix<uint8_t>>& neg, const float neg_to_pos_ratio, int k) {
            int positive = 0;

            matrix<uint8_t> sample(sample_size);

            for (int i : range(objects)) {
                const auto& rect = objects[i].rect;
                for (int dx = -k; dx <= k; dx++) {
                    for (int dy = -k; dy <= k; dy++) {
                        rectangle<int> rect1(rect.x + dx, rect.y + dy, rect.w, rect.h);

                        if (rect1.intersect(img.rect()) != rect1)
                            continue;

                        scale_image_hq(img.submatrix(rect1), sample);
                        pos.push_back(FeatureSpaceTransform::transform(sample));
                        positive++;
                    }
                }
            }

            if (img.height() - 1 <= sample_size.height || img.width() - 1 <= sample_size.width)
                return;

            fast_rand rnd;
            fast_rand_float_uniform rnd_float(1.f, std::min((img.height() - 1.f) / sample_size.height, (img.width() - 1.f) / sample_size.width));

            const int need_neg = int(std::ceil(positive * neg_to_pos_ratio));

            for(int negatives = 0, wasted = 0; wasted < 20 * sample_size.area() && negatives < need_neg;) {
                rectangle<int> r;
                const float scale = rnd_float();

                r.h = int(sample_size.height * scale);
                r.w = int(sample_size.width * scale);

                ASSERT_TRUE(r.h < img.height() && r.w < img.width());

                r.y = rnd() % (img.height() - r.h);
                r.x = rnd() % (img.width() - r.w);

                double iou_max = 0.;
                for (const auto& t : objects) {
                    iou_max = std::max(iou_max, intersection_over_union(r, t.rect));
                    wasted++;
                }
                if (iou_max < 0.3) {
                    scale_image_hq(img.submatrix(r), sample);
                    neg.push_back(FeatureSpaceTransform::transform(sample));
                    wasted = 0;
                    negatives++;
                }
            }
        }

        static void push_back_repack(const std::vector<matrix<uint8_t>>& x, matrix<std::vector<uint8_t>>& rx) {
            if (x.empty())
                return;

            ASSERT_TRUE(x[0].dimensions() == rx.dimensions());

            for (int i : range(rx.height())) {
                for (int j : range(rx.width())) {
                    rx[i][j].reserve(rx[i][j].size() + x.size());
                }
            }

            parallel_for(range(x[0].height()), [&](int i) {
                for (const auto& m : x) {
                    for (int j : range(m.width())) {
                        rx[i][j].push_back(m[i][j]);
                    }
                }
            });
        }

        void load(const meta_data_set& mds, const int k, const float neg_to_pos_ratio, const float adjusted_neg_to_pos_ratio) {
            scoped_timer t("dataset::load");

            enumerable_thread_specific<std::vector<matrix<uint8_t>>> ts_xp;
            enumerable_thread_specific<std::vector<matrix<uint8_t>>> ts_xn;

            progress_indicator progress((uint32_t)mds.data.size());

            parallel_for(range(mds.data), [&](int i) {
                const auto& file_meta = mds.data[i];

                auto& xp = ts_xp.local();
                auto& xn = ts_xn.local();

                matrix<uint8_t> img;
                read_jpeg(file_meta.filepath, img);

                generate_samples(sample_size, img, file_meta.objects, xp, xn, adjusted_neg_to_pos_ratio, k);

                progress.did_some(1);
            });

            auto counter = [](int a, const std::vector<matrix<uint8_t>>& v) { return a + isize(v); };
            
            for (int i : range(ts_xn)) {
                int num_p = isize(ts_xp[i]);
                int num_n = isize(ts_xn[i]);

                ts_xn[i].resize(std::min(num_n, int(num_p * neg_to_pos_ratio + 0.5f)));
                ts_xp[i].resize(std::min(num_p, int(num_n / neg_to_pos_ratio + 0.5f)));
            }

            const int capacity = std::accumulate(ts_xp.begin(), ts_xp.end(), 0, counter) + std::accumulate(ts_xn.begin(), ts_xn.end(), 0, counter);

            y.reserve(capacity);

            for (auto& v : rx) {
                v.reserve(capacity);
            }

            for (auto& t : ts_xp) {
                push_back_repack(t, rx);
                y.resize(y.size() + t.size(), 1.f);
                t.clear();
                t.shrink_to_fit();
            }

            for (auto& t : ts_xn) {
                push_back_repack(t, rx);
                y.resize(y.size() + t.size(), 0.f);
                t.clear();
                t.shrink_to_fit();
            }
        }
    };
}; // namespace c4

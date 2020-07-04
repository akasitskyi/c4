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

#include "matrix.hpp"
#include "exception.hpp"
#include "matrix_regression.hpp"
#include "scaling.hpp"
#include "serialize.hpp"

namespace c4 {
    struct detection {
        rectangle<float> rect;
        float conf;
    };

    static void merge_rects(std::vector<detection>& dets) {
        matrix<char> m(isize(dets), isize(dets));

        for (int i : range(dets)) {
            for (int j : range(dets)) {
                const float sa = dets[i].rect.area();
                const float sb = dets[j].rect.area();
                const float si = dets[i].rect.intersect(dets[j].rect).area();

                const bool same_size = std::max(sa, sb) < 4 * std::min(sa, sb);
                const bool overlap_enough = si > 0.8 * std::min(sa, sb);

                m[i][j] = same_size && overlap_enough;
            }
        }

        for (int k : range(dets)) {
            for (int i : range(dets)) {
                for (int j : range(dets)) {
                    m[i][j] |= m[i][k] & m[k][j];
                }
            }
        }

        std::vector<bool> erased(isize(dets), false);

        for (int i : range(dets)) {
            if (erased[i])
                continue;

            auto& di = dets[i];

            for (int j : range(i + 1, isize(dets))) {
                if (erased[j])
                    continue;

                auto& dj = dets[j];

                if (m[i][j]) {
                    di.rect.x = (di.rect.x * di.conf + dj.rect.x * dj.conf) / (di.conf + dj.conf);
                    di.rect.y = (di.rect.y * di.conf + dj.rect.y * dj.conf) / (di.conf + dj.conf);
                    di.rect.w = (di.rect.w * di.conf + dj.rect.w * dj.conf) / (di.conf + dj.conf);
                    di.rect.h = (di.rect.h * di.conf + dj.rect.h * dj.conf) / (di.conf + dj.conf);
                    di.conf = di.conf + dj.conf;
                    erased[j] = true;
                }
            }
        }

        int n = 0;
        for (int i : range(dets)) {
            if (!erased[i]) {
                dets[n++] = dets[i];
            }
        }

        dets.resize(n);
    }

    static void cleanup_rects(std::vector<detection>& dets) {
        std::sort(dets.begin(), dets.end(), [](const detection& a, const detection& b) {  return a.conf > b.conf; });

        // Clean up rects with simillar position
        for (int i = 0; i < isize(dets); i++) {
            const auto& di = dets[i];
            for (int j = i + 1; j < isize(dets); j++) {
                const auto& dj = dets[j];

                const auto ir0 = di.rect.scale_around_center(0.8);
                const rectangle<double> ir1(di.rect.x + di.rect.w / 4, di.rect.y, di.rect.w / 2, di.rect.h);

                const auto jr0 = dj.rect.scale_around_center(0.8);

                if (ir0.contains(jr0) || ir1.contains(jr0)) {
                    dets.erase(dets.begin() + j);
                    --j;
                }
            }
        }

        // Clean up rects with big confidence difference on the same frame
        for (int i = 0; i < isize(dets); i++) {
            const auto& di = dets[i];
            for (int j = i + 1; j < isize(dets); j++) {
                const auto& dj = dets[j];

                if (di.conf > dj.conf * 10) {
                    dets.erase(dets.begin() + j);
                    --j;
                }
            }
        }
    }

    template<class TForm, int dim>
    struct window_detector {
        matrix_regression<dim> mr;
        float threshold;

        window_detector() = default;
        window_detector(const matrix_regression<dim>& mr, float threshold) : mr(mr), threshold(threshold) {}

        std::vector<detection> detect(const matrix_ref<uint8_t>& img) const {
            c4::matrix<uint8_t> timg = TForm::transform(img);

            auto m = mr.predict_multi(timg, TForm::row_step);

            std::vector<detection> dets;

            const matrix_dimensions obj_dims = mr.dimensions();

            for (int i : range(m.height())) {
                for (int j : range(m.width())) {
                    // why squared? because we trained on MSE!
                    const float conf = sqr(m[i][j]);
                    if (conf > threshold) {
                        rectangle<int> r(j, i * TForm::row_step, obj_dims.width, obj_dims.height);
                        dets.push_back({ rectangle<float>(TForm::reverse_rect(r)), conf });
                    }
                }
            }

            return dets;
        }

        matrix_dimensions dimensions() const {
            return TForm::reverse_dimensions(mr.dimensions());
        }

        template <typename Archive>
        void load(Archive& ar) {
            ar(mr, threshold);
        }

        template <typename Archive>
        void save(Archive& ar) const {
            ar(mr, threshold);
        }
    };

    template<class TForm, int dim>
    struct scaling_detector {
        window_detector<TForm, dim> wd;
        float start_scale;
        float scale_step;
        float rect_size_scale = 1.f;

        scaling_detector() = default;
        scaling_detector(const window_detector<TForm, dim>& wd, float start_scale, float scale_step) : wd(wd), start_scale(start_scale), scale_step(scale_step) {}

        std::vector<detection> detect(const matrix_ref<uint8_t>& img, std::vector<detection>& candidates) const {
            std::vector<detection> dets;

            float scale = start_scale;
            
            auto scaleWeight = [](float scale) { return 1.f / sqr(scale); };

            if (scale == 1.f) {
                dets = wd.detect(img);
                for (auto& d : dets) {
                    d.conf *= scaleWeight(scale);
                }
                scale *= scale_step;
            }

            const matrix_dimensions min_dims = wd.dimensions();

            matrix<uint8_t> scaled;

            for (; int(img.height() * scale) >= min_dims.height && int(img.width() * scale) >= min_dims.width; scale *= scale_step) {
                scaled.resize(int(img.height() * scale), int(img.width() * scale));

                scale_image_hq(img, scaled);

                std::vector<detection> scale_dets = wd.detect(scaled);

                for (detection& d : scale_dets) {
                    d.rect = d.rect.scale_around_origin(1.f / scale);
                    d.conf *= scaleWeight(scale);
                    dets.push_back(d);
                }
            }
            candidates = dets;
            merge_rects(dets);
            cleanup_rects(dets);

            for (auto& d : dets) {
                d.rect = d.rect.scale_around_center(rect_size_scale);
            }

            return dets;
        }

        std::vector<detection> detect(const matrix_ref<uint8_t>& img) const {
            std::vector<detection> candidates;
            return detect(img, candidates);
        }

        float min_width() const {
            return wd.dimensions().width * start_scale * rect_size_scale;
        }

        float min_height() const {
            return wd.dimensions().height * start_scale * rect_size_scale;
        }

        template <typename Archive>
        void load(Archive& ar) {
            ar(wd, start_scale, scale_step);
        }

        template <typename Archive>
        void save(Archive& ar) const {
            ar(wd, start_scale, scale_step);
        }
    };
};

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
                m[i][j] = (intersection_over_union(dets[i].rect, dets[j].rect) > 0.7f);
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
        while (true) {
            bool changed = false;
            for (int i = 0; i < isize(dets); i++) {
                auto& di = dets[i];
                for (int j : range(i)) {
                    auto& dj = dets[j];
                    const float iom = di.rect.intersect(dj.rect).area() / std::min(dj.rect.area(), di.rect.area());

                    if (iom > 0.75f) {
                        if (dj.conf > di.conf) {
                            std::swap(di, dj);
                        }

                        dets.erase(dets.begin() + j);
                        changed = true;
                        break;
                    }
                }
            }

            if (!changed)
                break;
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

        scaling_detector() = default;
        scaling_detector(const window_detector<TForm, dim>& wd, float start_scale, float scale_step) : wd(wd), start_scale(start_scale), scale_step(scale_step) {}

        std::vector<detection> detect(const matrix_ref<uint8_t>& img) const {
            std::vector<detection> dets;

            float scale = start_scale;

            if (scale == 1.f) {
                dets = wd.detect(img);
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
                    dets.push_back(d);
                }
            }

            merge_rects(dets);
            cleanup_rects(dets);

            return dets;
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

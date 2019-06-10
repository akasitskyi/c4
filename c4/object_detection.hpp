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
#include "parallel.hpp"
#include "matrix_regression.hpp"
#include "scaling.hpp"
#include "lbp.hpp"
#include "serialize.hpp"

namespace c4 {
    struct detection {
        rectangle<float> rect;
        float weight;
    };

    static void merge_rects(std::vector<detection>& dets) {
        matrix<char> m(isize(dets), isize(dets));

        for (int i : range(dets)) {
            for (int j : range(dets)) {
                m[i][j] = (intersection_over_union(dets[i].rect, dets[i].rect) > 0.9f);
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
                    di.rect.x = (di.rect.x * di.weight + dj.rect.x * dj.weight) / (di.weight + dj.weight);
                    di.rect.y = (di.rect.y * di.weight + dj.rect.y * dj.weight) / (di.weight + dj.weight);
                    di.rect.w = (di.rect.w * di.weight + dj.rect.w * dj.weight) / (di.weight + dj.weight);
                    di.rect.h = (di.rect.h * di.weight + dj.rect.h * dj.weight) / (di.weight + dj.weight);
                    di.weight = di.weight + dj.weight;
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
                        if (dj.weight > di.weight) {
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

    template<class TForm, int dim = 256>
    struct window_detector {
        const matrix_regression<dim> mr;
        const int step;

        window_detector(const matrix_regression<dim>& mr, const int step) : mr(mr), step(step) {}

        std::vector<detection> detect(const matrix_ref<uint8_t>& img, double threshold) const {
            c4::matrix<uint8_t> timg = TForm::transform(img);

            auto m = mr.predict_multi(timg);

            std::vector<detection> dets;

            const matrix_dimensions obj_dims = mr.dimensions();

            for (int i : range(m.height())) {
                for (int j : range(m.width())) {
                    if (m[i][j] > threshold) {
                        rectangle<int> r(j, i, obj_dims.width, obj_dims.height);
                        dets.push_back({ rectangle<float>(TForm::reverse_rect(r)), m[i][j] });
                    }
                }
            }

            return dets;
        }

        matrix_dimensions dimensions() const {
            return TForm::reverse_dimensions(mr.dimensions());
        }

        template <typename Archive>
        void serialize(Archive& ar) {
            ar(mr);
        }
    };

    template<class TForm, int dim = 256>
    struct scaling_detector {
        const window_detector<TForm, dim> wd;
        const float start_scale;
        const float scale_step;

        scaling_detector(const window_detector<TForm, dim>& wd, float start_scale, float scale_step) : wd(wd), start_scale(start_scale), scale_step(scale_step) {}

        std::vector<detection> detect(const matrix_ref<uint8_t>& img, double threshold) const {
            std::vector<detection> dets;

            float scale = start_scale;

            if (scale == 1.f) {
                dets = wd.detect(img, threshold);
                scale *= scale_step;
            }

            const matrix_dimensions min_dims = wd.dimensions();

            matrix<uint8_t> scaled;

            for (; int(img.height() * scale) >= min_dims.height && int(img.width() * scale) >= min_dims.width; scale *= scale_step) {
                scaled.resize(int(img.height() * scale), int(img.width() * scale));

                scale_image_hq(img, scaled);

                std::vector<detection> scale_dets = wd.detect(scaled, threshold);

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
        void serialize(Archive& ar) {
            ar(wd, scale_step);
        }
    };

    static c4::scaling_detector<c4::LBP, 256> load_scaling_detector(const std::string& filepath, float min_scale) {
        std::ifstream fin(filepath, std::ifstream::binary);
        c4::serialize::input_archive in(fin);

        c4::matrix_regression<> mr;
        in(mr);

        c4::window_detector<c4::LBP, 256> wd(mr, 1);

        c4::scaling_detector<c4::LBP, 256> sd(wd, min_scale, 0.9f);

        return sd;
    }
};

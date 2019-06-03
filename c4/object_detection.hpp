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
#include <c4/exception.hpp>
#include <c4/parallel.hpp>
#include <c4/matrix_regression.hpp>
#include <c4/scaling.hpp>

namespace c4 {
    struct detection {
        rectangle<int> rect;
        double confidence;
    };

    static void merge_rects(std::vector<detection>& dets, const double iou_threshold) {
        while (true) {
            bool changed = false;
            for (int i = 0; i < isize(dets); i++) {
                for (int j : range(i)) {
                    if (intersection_over_union(dets[i].rect, dets[j].rect) > iou_threshold) {
                        dets[i].rect.x = (dets[i].rect.x + dets[j].rect.x + 1) / 2;
                        dets[i].rect.y = (dets[i].rect.y + dets[j].rect.y + 1) / 2;
                        dets[i].rect.w = (dets[i].rect.w + dets[j].rect.w + 1) / 2;
                        dets[i].rect.h = (dets[i].rect.h + dets[j].rect.h + 1) / 2;

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
    class window_detector {
        const matrix_regression<dim> mr;
    public:

        window_detector(const matrix_regression<dim>& mr) : mr(mr) {}

        std::vector<detection> detect(const matrix_ref<uint8_t>& img, double threshold) const {
            c4::matrix<uint8_t> timg = TForm::transform(img);

            std::vector<detection> dets;

            const matrix_dimensions obj_dims = mr.dimensions();

            for (int i : range(timg.height() - obj_dims.height + 1)) {
                for (int j : range(timg.width() - obj_dims.width + 1)) {
                    rectangle<int> r(j, i, obj_dims.width, obj_dims.height);
                    double conf = mr.predict(timg.submatrix(r));
                    if (conf > threshold) {
                        dets.push_back({ r, conf });
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
    class scaling_detector {
        const window_detector<TForm, dim> wd;
        const float scale_step;
    public:

        scaling_detector(const window_detector<TForm, dim>& wd, float scale_step) : wd(wd), scale_step(scale_step) {}

        std::vector<detection> detect(const matrix_ref<uint8_t>& img, double threshold) const {
            std::vector<detection> dets = wd.detect(img, threshold);

            const matrix_dimensions min_dims = wd.dimensions();

            matrix<uint8_t> scaled;

            for (float scale = scale_step; int(img.height() * scale) >= min_dims.height && int(img.width() * scale) >= min_dims.width; scale *= scale_step) {
                scaled.resize(int(img.height() * scale), int(img.width() * scale));

                scale_image_hq(img, scaled);

                std::vector<detection> scale_dets = wd.detect(scaled, threshold);

                for (detection& d : scale_dets) {
                    d.rect = rectangle<int>(d.rect.scale_around_origin(1.f / scale));
                    dets.push_back(d);
                }
            }

            merge_rects(dets, 0.7);

            return dets;
        }

        template <typename Archive>
        void serialize(Archive& ar) {
            ar(wd, scale_step);
        }
    };
};

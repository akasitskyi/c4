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

#include "range.hpp"
#include "matrix.hpp"
#include "meta_data_set.hpp"

namespace c4 {
    struct classification_metrics {
        int tp = 0;
        int fp = 0;
        int tn = 0;
        int fn = 0;

        double recall() const {
            return tp / double(tp + fn);
        }

        double precission() const {
            return tp / double(tp + fp);
        }
    };

    static classification_metrics evaluate_object_detection(const std::vector<object_on_image>& etalon, const std::vector<object_on_image>& test, const double min_iou) {
        classification_metrics m;

        std::vector<bool> erased(test.size(), false);

        for (int i : range(etalon)){
            double max_iou = min_iou;

            int j0 = -1;
            for (int j : range(test)) {
                if (erased[j])
                    continue;

                const double iou = intersection_over_union(etalon[i].rect, test[j].rect);

                if (iou > max_iou) {
                    j0 = j;
                    max_iou = iou;
                }
            }

            if (j0 != -1) {
                erased[j0] = true;
                m.tp++;
            } else {
                m.fn++;
            }
        }

        m.fp = (int)std::count(erased.begin(), erased.end(), false);

        return m;
    }

    static classification_metrics evaluate_object_detection(const std::vector<image_file_metadata>& etalon, const std::vector<image_file_metadata>& test, const double min_iou) {
        ASSERT_EQUAL(etalon.size(), test.size());

        classification_metrics res;

        for (int k : range(test)) {
            ASSERT_EQUAL(etalon[k].filepath, test[k].filepath);

            classification_metrics rk = evaluate_object_detection(etalon[k].objects, test[k].objects, min_iou);

            res.tp += rk.tp;
            res.fp += rk.fp;
            res.tn += rk.tn;
            res.fn += rk.fn;
        }

        return res;
    }
}; // namespace c4

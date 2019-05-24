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

#include "csv.hpp"
#include "lbp.hpp"
#include "json.hpp"
#include "jpeg.hpp"
#include "string.hpp"
#include "range.hpp"
#include "parallel.hpp"
#include "logger.hpp"
#include "matrix_regression.hpp"

namespace c4 {
    struct dataset {
        matrix<std::vector<uint8_t>> rx;

        std::vector<float> y;
        const matrix_dimensions sample_size;

        dataset(const matrix_dimensions& sample_size) : sample_size(sample_size), rx(LBP().calc_dimensions(sample_size)) {}

        template<class TForm>
        static void generate_samples(const matrix_dimensions& sample_size, const matrix<uint8_t>& img, const std::vector<rectangle<int>>& rects, std::vector<matrix<uint8_t>>& train_x, std::vector<float>& train_y, int k, TForm tform) {
            matrix<uint8_t> sample(sample_size);

            int positive = 0;

            for (const auto& r : rects) {
                for (int dx = -k; dx <= k; dx++) {
                    for (int dy = -k; dy <= k; dy++) {
                        rectangle<int> r1(r.x + dx, r.y + dy, r.w, r.h);

                        const auto cropped_rect = r1.intersect(img.rect());
                        scale_bilinear(img.submatrix(cropped_rect), sample);
                        train_x.push_back(tform(sample));
                        train_y.push_back(1.f);

                        positive++;
                    }
                }
            }

            if (img.height() <= sample.height() || img.width() <= sample.width())
                return;

            fast_rand rnd;
            int negatives = positive * 1;
            while (negatives--) {
                rectangle<int> r;
                r.y = rnd() % (img.height() - sample.height());
                r.x = rnd() % (img.width() - sample.width());
                r.h = sample_size.height;
                r.w = sample_size.width;

                double iou_max = 0.;
                for (const auto& t : rects) {
                    iou_max = std::max(iou_max, intersection_over_union(r, t));
                }
                if (iou_max < 0.1) {
                    train_x.push_back(tform(img.submatrix(r.y, r.x, r.h, r.w)));
                    train_y.push_back(0.f);
                }
            }
        }

        void load(const std::vector<std::pair<std::string, std::vector<rectangle<int>>>>& data, const int k = 0) {
            scoped_timer t("dataset::load");


            enumerable_thread_specific<std::vector<matrix<uint8_t>>> ts_x;
            enumerable_thread_specific<std::vector<float>> ts_y;

            parallel_for(data, [&](const std::pair<std::string, std::vector<rectangle<int>>>& r) {
                auto& xl = ts_x.local();
                auto& yl = ts_y.local();

                matrix<uint8_t> img;
                read_jpeg(r.first, img);

                generate_samples(sample_size, img, r.second, xl, yl, k, LBP());
            });

            const size_t capacity = std::accumulate(ts_y.begin(), ts_y.end(), ts_y.size(), [](size_t a, const std::vector<float>& v) { return a + v.size(); });
            PRINT_DEBUG(capacity);

            y.reserve(capacity);

            for (auto& row : rx) {
                for (auto& v : row) {
                    v.reserve(capacity);
                }
            }

            for (auto& t : ts_y) {
                y.insert(y.end(), t.begin(), t.end());
                t.clear();
                t.shrink_to_fit();
            }

            for (auto& t : ts_x) {
                matrix_regression<>::push_back_repack(t, rx);
                t.clear();
                t.shrink_to_fit();
            }
        }

        void load_dlib(const std::string& labels_filepath, const int k = 0, const int sample = 1) {
            scoped_timer t("dataset::load_dlib");

            json data_json;

            std::string root = "C:/portraits-data/ibug_300W_large_face_landmark_dataset/";
            std::ifstream train_data_fin(root + labels_filepath);

            train_data_fin >> data_json;

            std::vector<std::pair<std::string, std::vector<rectangle<int>>>> data;

            for (int i : range(data_json["images"].size())) {
                const auto& image_info = data_json["images"][i];

                const std::string filename = image_info["filename"];

                if (!ends_with(to_lower(filename), ".jpg") || i % sample)
                    continue;

                std::vector<rectangle<int>> rects;

                for (const auto& box : image_info["boxes"]) {
                    rectangle<int> r(box["left"], box["top"], box["width"], box["height"]);
                    rects.push_back(r);
                }

                data.emplace_back(root + filename, rects);
            }

            load(data, k);
        }

        void load_vggface2(const std::string& root, const std::string& labels_filepath, const int k = 0, const int sample = 1) {
            scoped_timer t("dataset::load_vggface2");

            std::vector<std::pair<std::string, std::vector<rectangle<int>>>> data;

            std::ifstream fin(labels_filepath);

            csv csv;
            csv.read(fin);
            for (int i : range(1, isize(csv.data))) {
                if (i % sample)
                    continue;

                const auto& row = csv.data[i];

                data.emplace_back(root + row[0] + ".jpg", std::vector<rectangle<int>>{ rectangle<int>(stoi(row[1]), stoi(row[2]), stoi(row[3]), stoi(row[4])) });
            }

            load(data, 0);
        }
    };
}; // namespace c4

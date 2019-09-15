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
#include <c4/progress_indicator.hpp>

namespace c4 {
    template<int dim>
    class matrix_regression {
        bool symmetry;
        matrix<std::array<float, dim>> weights;
    public:

        matrix_regression(bool symmetry = false) : symmetry(symmetry) {}

        matrix_dimensions dimensions() const {
            return weights.dimensions();
        }

        float predict(const matrix_ref<uint8_t>& img) const {
            ASSERT_TRUE(img.dimensions() == weights.dimensions());
            
            float sum = 0.f;

            for (int i : range(img.height())) {
                for (int j : range(img.width())) {
                    sum += weights[i][j][img[i][j]];
                }
            }

            return sum;
        }

        matrix<float> predict_multi(const matrix_ref<uint8_t>& img) const {
            ASSERT_TRUE(img.height() >= weights.height() && img.width() >= weights.width());

            matrix<float> sum(img.height() - weights.height() + 1, img.width() - weights.width() + 1);

            const int n = sum.width();

            for (int di : range(weights.height())) {
                for (int dj : range(weights.width())) {
                    const auto& w = weights[di][dj];
                    for (int i : range(sum.height())) {
                        float* psum = &sum[i][0];
                        const uint8_t* pimg = &img[i + di][dj];
                        for (int j = 0; j < n; j++) {
                            psum[j] += w[pimg[j]];
                        }
                    }
                }
            }

            return sum;
        }

        std::vector<double> predict(const std::vector<matrix<uint8_t>>& x) const {
            std::vector<double> f(x.size());
            parallel_for(range(x), [&](int i) {
                f[i] = predict(x[i]);
            });

            return f;
        }

        void predict(const matrix<std::vector<uint8_t>>& rx, std::vector<double>& f) const {
            enumerable_thread_specific<std::vector<double>> f_ts(std::vector<double>(rx[0][0].size()));

            parallel_for(range(weights.height()), [&](int i) {
                auto& f = f_ts.local();
                for (int j : range(weights.width())) {
                    const auto& w = weights[i][j];
                    const auto& rxv = rx[i][j];

                    for (int k : range(rxv)) {
                        f[k] += w[rxv[k]];
                    }
                }
            });

            parallel_for(range(f), [&](int k) {
                double s = 0.;
                for (const auto& fi : f_ts) {
                    s += fi[k];
                }
                f[k] = s;
            });
        }

        std::vector<double> predict(const matrix<std::vector<uint8_t>>& rx) const {
            std::vector<double> f(rx[0][0].size());
            predict(rx, f);
            return f;
        }

        void train(const matrix<std::vector<uint8_t>>& rx, const std::vector<float>& y, const matrix<std::vector<uint8_t>>& test_rx, const std::vector<float>& test_y, const int itc) {
            c4::scoped_timer t("matrix_regression::train()");

            if (weights.height() == 0) {
                weights.resize(rx.dimensions());
                for (auto& t : weights) {
                    std::fill(t.begin(), t.end(), 0.f);
                }
            }

            ASSERT_TRUE(weights.dimensions() == rx.dimensions());

            matrix<std::array<double, dim>> d(weights.dimensions());
            matrix<std::array<uint32_t, dim>> n_m(weights.dimensions());
            matrix<std::array<double, dim>> sy_m(weights.dimensions());

            parallel_for(range(weights.height()), [&](int i) {
                for (int j : range(weights.width())) {
                    auto& sy = sy_m[i][j];
                    auto& n = n_m[i][j];
                    auto& rx_ij = rx[i][j];

                    for (int k : range(rx_ij)) {
                        sy[rx_ij[k]] += y[k];
                        n[rx_ij[k]]++;
                    }
                }
            });
            
            std::vector<double> f = predict(rx);
            {
                const double train_mse = mean_squared_error(f, y);
                const double test_mse = mean_squared_error(predict(test_rx), test_y);

                LOGD << "initial train_mse: " << train_mse << ", test_mse: " << test_mse;
            }

            progress_indicator progress(itc);

            for (int it = 1; it <= itc; it++) {
                parallel_for(range(weights.height()), [&](int i) {
                    for (int j : range(weights.width())) {
                        std::vector<double> sf(dim, 0.);
                        const auto& rx_ij = rx[i][j];

                        for (int k : range(rx_ij)) {
                            sf[rx_ij[k]] += f[k];
                        }

                        const auto& sy = sy_m[i][j];
                        const auto& n = n_m[i][j];

                        for (int k : range(dim)) {
                            if (n[k] == 0)
                                continue;

                            d[i][j][k] = (sy[k] - sf[k]) / n[k];
                        }
                    }
                });

                for (int i : range(weights.height())) {
                    for (int j : range(weights.width())) {
                        for (int k : range(dim)) {
                            double add = (symmetry ? (d[i][j][k] + d[i][weights.width() - j - 1][k]) / 2 : d[i][j][k]) / weights.dimensions().area();
                            weights[i][j][k] = float(weights[i][j][k] + add);
                        }
                    }
                }

                predict(rx, f);

                if (it % 100 == 0) {
                    const double train_mse = mean_squared_error(f, y);
                    const double test_mse = mean_squared_error(predict(test_rx), test_y);

                    LOGD << "it " << it << ", train_mse: " << train_mse << ", test_mse: " << test_mse;
                }

                progress.did_some(1);
            }
        }

        template <typename Archive>
        void load(Archive& ar) {
            ar(symmetry);
            {
                int h, w, d;
                ar(h);
                ar(w);
                ar(d);
                weights.resize(h, w);
                ASSERT_EQUAL(d, dim);
            }

            for (auto& row : weights) {
                const int n = symmetry ? (isize(row) + 1) / 2 : isize(row);

                for (int j : range(n)) {
                    ar(row[j]);
                }

                if (symmetry) {
                    std::reverse_copy(row.begin(), row.begin() + isize(row) / 2, row.begin() + n);
                }
            }
        }

        template <typename Archive>
        void save(Archive& ar) const {
            ar(symmetry);
            ar(weights.height());
            ar(weights.width());
            ar(dim);

            for (const auto& row : weights) {
                const int n = symmetry ? (isize(row) + 1) / 2 : isize(row);
                
                for (int j : range(n)) {
                    ar(row[j]);
                }
            }
        }
    };
};

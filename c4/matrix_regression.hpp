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

namespace __c4 {
    using namespace c4;

    template<int dim = 256>
    class matrix_regression {
        matrix<std::array<double, dim>> weights;
    public:
        matrix_regression() {}

        double predict(const matrix_ref<uint8_t>& img) const {
            ASSERT_TRUE(img.dimensions() == weights.dimensions());
            
            double sum = 0;

            for (int i : range(img.height())) {
                for (int j : range(img.width())) {
                    sum += weights[i][j][img[i][j]];
                }
            }

            return sum;
        }

        std::vector<double> predict(const std::vector<matrix<uint8_t>>& x) const {
            std::vector<double> f(x.size());
            for (int i : range(x)) {
                f[i] = predict(x[i]);
            }

            return f;
        }

        void train(const std::vector<matrix<uint8_t>>& x, const std::vector<float>& y, const std::vector<matrix<uint8_t>>& test_x, const std::vector<float>& test_y) {
            weights.resize(x[0].dimensions());
            for (auto& v : weights) {
                for (auto& t : v) {
                    std::fill(t.begin(), t.end(), 0.f);
                }
            }

            for (int it = 0; it < 1000; it++) {
                std::vector<double> f = predict(x);

                matrix<std::array<double, dim>> d(weights.dimensions());

                for (int i : range(weights.height())) {
                    for (int j : range(weights.width())) {
                        std::vector<double> sf(dim, 0.);
                        std::vector<double> sy(dim, 0.);
                        std::vector<size_t> n(dim, 0);

                        for (int k : range(x)) {
                            sf[x[k][i][j]] += f[k];
                            sy[x[k][i][j]] += y[k];
                            n[x[k][i][j]]++;
                        }

                        for (int k : range(dim)) {
                            if (n[k] == 0)
                                continue;

                            d[i][j][k] = (sy[k] - sf[k]) / n[k];
                        }
                    }
                }

                for (int i : range(weights.height())) {
                    for (int j : range(weights.width())) {
                        for (int k : range(dim)) {
                            weights[i][j][k] += d[i][j][k] / weights.dimensions().area();
                        }
                    }
                }

                const double train_mse = mean_squared_error(predict(x), y);
                const double test_mse = mean_squared_error(predict(test_x), test_y);

                LOGD << "it " << it << ", train_mse: " << train_mse << ", test_mse: " << test_mse;
            }
        }
    };
};

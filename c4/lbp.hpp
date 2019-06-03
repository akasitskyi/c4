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

namespace c4 {
    struct LBP {
        static c4::matrix<uint8_t> transform(const c4::matrix<uint8_t>& img) {
            c4::matrix<uint8_t> lbp(calc_dimensions(img.dimensions()));

            for (int i : c4::range(lbp.height())) {
                for (int j : c4::range(lbp.width())) {
                    const c4::matrix<uint8_t> m = img.submatrix(i, j, 3, 3);
                    const uint8_t c = m[1][1];

                    int v = 0;

                    v = (v << 1) | (m[0][0] > c);
                    v = (v << 1) | (m[0][1] > c);
                    v = (v << 1) | (m[0][2] > c);
                    v = (v << 1) | (m[1][0] > c);
                    v = (v << 1) | (m[1][2] > c);
                    v = (v << 1) | (m[2][0] > c);
                    v = (v << 1) | (m[2][1] > c);
                    v = (v << 1) | (m[2][2] > c);

                    lbp[i][j] = uint8_t(v);
                }
            }

            return lbp;
        }

        static c4::matrix_dimensions calc_dimensions(const c4::matrix_dimensions& md) {
            return { md.height - 2, md.width - 2 };
        }

        static c4::matrix_dimensions reverse_dimensions(const c4::matrix_dimensions& md) {
            return { md.height + 2, md.width + 2 };
        }
    };
};

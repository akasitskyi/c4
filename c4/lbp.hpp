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
    template<int mask = 0xf8>
    struct lbp {
        static c4::matrix<uint8_t> transform(const c4::matrix<uint8_t>& img) {
            c4::matrix<uint8_t> lbp(calc_dimensions(img.dimensions()));

            for (int i : c4::range(lbp.height())) {
                const uint8_t* m0 = &img[i + 0][0];
                const uint8_t* m1 = &img[i + 1][0];
                const uint8_t* m2 = &img[i + 2][0];

                for (int j : c4::range(lbp.width())) {
                    const int c = m1[j + 1] & mask;

                    int v = 0;

                    v = (v << 1) | ((m0[j + 0] & mask) > c);
                    v = (v << 1) | ((m0[j + 1] & mask) > c);
                    v = (v << 1) | ((m0[j + 2] & mask) > c);
                    v = (v << 1) | ((m1[j + 0] & mask) > c);
                    v = (v << 1) | ((m1[j + 2] & mask) > c);
                    v = (v << 1) | ((m2[j + 0] & mask) > c);
                    v = (v << 1) | ((m2[j + 1] & mask) > c);
                    v = (v << 1) | ((m2[j + 2] & mask) > c);

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

        static c4::rectangle<int> reverse_rect(c4::rectangle<int> r) {
            r.h += 2;
            r.w += 2;
            return r;
        }
    };
};

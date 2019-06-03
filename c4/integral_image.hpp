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
    static void calc_integral_image(const matrix_ref<uint8_t>& src, matrix<uint32_t>& dst) {
        dst.resize(src.dimensions());
        uint32_t row_sum = 0;
        for (int j : range(src.width())) {
            row_sum += src[0][j];
            dst[0][j] = row_sum;
        }

        for (int i : range(1, src.height())) {
            row_sum = 0;
            for (int j : range(src.width())) {
                row_sum += src[i][j];
                dst[i][j] = dst[i - 1][j] + row_sum;
            }
        }
    }

    // TODO: add more stuff
};

//MIT License
//
//Copyright(c) 2018 Alex Kasitskyi
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
#include "pixel.hpp"

#include <cstdint>
#include <ostream>
#include <vector>

namespace c4 {
    namespace detail {
        template<class T>
        inline void write_binary(std::ostream& out, const T& t) {
            out.write((const char*)&t, sizeof(t));
        }
    }

    inline void write_bmp(std::ostream& out, matrix_ref<pixel<uint8_t>>& img) {
        using namespace c4::detail;

        const int stride_bytes = (img.width() * 3 + 3) & ~3;

        // file header
        write_binary(out, 'B');
        write_binary(out, 'M');
        write_binary(out, uint32_t(14 + 40 + stride_bytes * img.height()));
        write_binary(out, uint16_t(0));
        write_binary(out, uint16_t(0));
        write_binary(out, uint32_t(14 + 40));

        // bitmap header
        write_binary(out, uint32_t(40));
        write_binary(out, uint32_t(img.width()));
        write_binary(out, uint32_t(img.height()));
        write_binary(out, uint16_t(1));
        write_binary(out, uint16_t(24));
        write_binary(out, uint32_t(0));
        write_binary(out, uint32_t(0));
        write_binary(out, uint32_t(0));
        write_binary(out, uint32_t(0));
        write_binary(out, uint32_t(0));
        write_binary(out, uint32_t(0));

        std::vector<char> row(stride_bytes);

        for (int j = img.height() - 1; j >= 0; j--) {
            const pixel<uint8_t>* src_row = img[j].data();

            // rgb -> bgr
            for (int i = 0; i < img.width(); i++) {
                row[3 * i + 0] = src_row[i].b;
                row[3 * i + 1] = src_row[i].g;
                row[3 * i + 2] = src_row[i].r;
            }

            out.write(row.data(), row.size());
        }
    }
};

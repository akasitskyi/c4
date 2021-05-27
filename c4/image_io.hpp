//MIT License
//
//Copyright(c) 2021 Alex Kasitskyi
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

#include <string>

#include "jpeg.hpp"
#include "bmp24.hpp"
#include "pbm.hpp"

#include "matrix.hpp"
#include "pixel.hpp"

namespace c4 {
    static void read_image(const std::string& filepath, matrix<pixel<uint8_t>>& out) {
        auto i = filepath.rfind('.');
        const std::string extension = c4::to_lower(i != std::string::npos ? filepath.substr(i) : "");
        
        if (extension == ".jpg" || extension == ".jpeg") {
            c4::read_jpeg(filepath, out);
        } else
        if (extension == ".bmp") {
            c4::read_bmp24(filepath, out);
        } else
        if (extension == ".ppm") {
            c4::read_ppm(filepath, out);
        } else {
            THROW_EXCEPTION("read_image not implemented for '" + extension + "' extension");
        }
    }
};

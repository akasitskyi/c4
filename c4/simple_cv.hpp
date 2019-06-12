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

#include "lbp.hpp"
#include "dataset.hpp"
#include "serialize.hpp"
#include "matrix_regression.hpp"
#include "object_detection.hpp"

namespace c4 {
    typedef dataset<lbp<>> lbp_dataset;
    typedef matrix_regression<256> byte_matrix_regression;
    typedef window_detector<lbp<>, 256> window_lbp_detector;
    typedef scaling_detector<lbp<>, 256> scaling_lbp_detector;

    static scaling_lbp_detector load_scaling_lbp_detector(const std::string& filepath) {
        std::ifstream fin(filepath, std::ifstream::binary);
        serialize::input_archive in(fin);

        scaling_lbp_detector sd;

        in(sd);

        return sd;
    }
}; // namespace c4

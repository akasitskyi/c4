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

#include <vector>
#include <cstdint>
#include <ostream>
#include <istream>
#include <fstream>

#include "byte_stream.hpp"
#include "matrix.hpp"
#include "pixel.hpp"

// TODO: implement full spec
namespace c4 {
    namespace detail {
        inline void get_pbm_line(std::istream& in, std::string& line) {
            do {
                if (!std::getline(in, line)) {
                    THROW_EXCEPTION("PbmReader EOF");
                }
                line.erase(std::find(line.begin(), line.end(), '#'), line.end());
            } while (line.empty());
        }
    };

    static void read_ppm(std::istream& in, matrix<pixel<uint8_t>>& out) {
        using namespace detail;

        std::string line;
        get_pbm_line(in, line);
        ASSERT_EQUAL(line, "P6");

        get_pbm_line(in, line);
        std::stringstream ss(line);
        int w = -1;
        int h = -1;
        int maxval = -1;
        ss >> w >> h >> maxval;
        ASSERT_EQUAL(maxval, 255);

        out.resize(h, w);
        for(int i : range(h)) {
            in.read((char*)out[i], w * 3);
        }
    }

    static void read_pgm(std::istream& in, matrix<uint8_t>& out) {
        using namespace detail;

        std::string line;
        get_pbm_line(in, line);
        ASSERT_EQUAL(line, "P5");

        get_pbm_line(in, line);
        std::stringstream ss(line);
        int w = -1;
        int h = -1;
        int maxval = -1;
        ss >> w >> h >> maxval;
        ASSERT_EQUAL(maxval, 255);

        out.resize(h, w);
        for (int i : range(h)) {
            in.read((char*)out[i], w);
        }
    }

    static void read_ppm(const std::string& filename, matrix<pixel<uint8_t>>& img) {
        std::ifstream fin(filename, std::ifstream::binary);
        try {
            read_ppm(fin, img);
        }
        catch (const std::exception& e) {
            throw std::runtime_error(std::string(e.what()) + ", while reading " + filename);
        }
    }

    static void read_pgm(const std::string& filename, matrix<uint8_t>& img) {
        std::ifstream fin(filename, std::ifstream::binary);
        try {
            read_pgm(fin, img);
        }
        catch (const std::exception& e) {
            throw std::runtime_error(std::string(e.what()) + ", while reading " + filename);
        }
    }
};

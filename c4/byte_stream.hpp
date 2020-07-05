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

#include <string>
#include <cassert>
#include <fstream>
#include "exception.hpp"

namespace c4 {
    inline uint8_t get8(std::istream& in) {
        uint8_t r;
        in.read((char*)&r, sizeof(r));
        return r;
    }

    inline uint16_t get16le(std::istream& in) {
        return uint16_t(get8(in) | (get8(in) << 8));
    }

    inline uint16_t get16be(std::istream& in) {
        return uint16_t((get8(in) << 8) | get8(in));
    }

    inline uint32_t get32le(std::istream& in) {
        return uint32_t(get16le(in) | (get16le(in) << 16));
    }

    inline uint32_t get32be(std::istream& in) {
        return uint32_t((get16be(in) << 16) | get16be(in));
    }

    inline void skip(std::istream& in, std::streamoff n) {
        in.seekg(in.tellg() + n);
    }

    template<class T>
    inline void write_le(std::ostream& out, T t) {
        uint8_t v[sizeof(T)];

        for (size_t i = 0; i < sizeof(T); i++) {
            v[i] = uint8_t(t & 0xff);
            t >>= 8;
        }

        out.write((const char*)v, sizeof(T));
    }

    inline void write_le(std::ostream& out, uint8_t t) {
        out.write((const char*)&t, sizeof(t));
    }
};

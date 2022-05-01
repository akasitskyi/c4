//MIT License
//
//Copyright(c) 2022 Alex Kasitskyi
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

namespace c4 {
    template<class T>
    class ring_buffer {
        std::vector<T> v;
        size_t start = 0;

    public:
        ring_buffer(size_t size) : v(size) {}

        T push(T t) {
            std::swap(v[start], t);
            start = (start + 1) % v.size();

            return t;
        }

        const T& peek() const {
            return v[start];
        }

        T& operator[](size_t i) {
            return v[(start + i) % v.size()];
        }

        void fill(const T& t) {
            std::fill(v.begin(), v.end(), t);
        }
    };
};

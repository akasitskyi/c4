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

#include "matrix.hpp"

namespace c4 {
    template<typename T>
    void operator+=(std::vector<T>& a, const std::vector<T>& b) {
        ASSERT_EQUAL(isize(a), isize(b));
        for (int i : range(a)) {
            a[i] += b[i];
        }
    }

    template<typename T>
    auto operator+(const std::vector<T>& a, const std::vector<T>& b) {
        std::vector<T> r(a);

        r += b;

        return r;
    }

    template<typename T>
    void operator-=(std::vector<T>& a, const std::vector<T>& b) {
        ASSERT_EQUAL(isize(a), isize(b));
        for (int i : range(a)) {
            a[i] -= b[i];
        }
    }

    template<typename T>
    auto operator-(const std::vector<T>& a, const std::vector<T>& b) {
        std::vector<T> r(a);

        r -= b;

        return r;
    }

    template<typename T>
    double operator*(const std::vector<T>& a, const std::vector<T>& b) {
        ASSERT_EQUAL(isize(a), isize(b));

        double r = 0.;

        for (int i : range(a)) {
            r += a[i] * b[i];
        }

        return r;
    }

    template<typename T>
    auto operator*(const float a, const std::vector<T>& b) {
        std::vector<T> r(b.size());

        for (int i : range(r)) {
            r[i] = a * b[i];
        }

        return r;
    }

    template<typename T>
    auto operator*(const std::vector<T>& a, float b) {
        return b * a;
    }

    template<typename T>
    auto operator/(const std::vector<T>& b, float a) {
        return (1.f / a) * b;
    }
}; // namespace c4

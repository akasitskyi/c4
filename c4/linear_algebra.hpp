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
#include "simd.hpp"

// TODO: this can be way better

namespace c4 {
    inline void increment(float* pa, const float* pb, const int n) {
        int i = 0;
        for (; i + 4 <= n; i += 4) {
            simd::float32x4 a = simd::load(pa + i);
            simd::float32x4 b = simd::load(pb + i);

            a = a + b;

            simd::store(pa + i, a);
        }

        for (; i < n; i++) {
            pa[i] += pb[i];
        }
    }

    inline void decrement(float* pa, const float* pb, const int n) {
        int i = 0;
        for (; i + 4 <= n; i += 4) {
            simd::float32x4 a = simd::load(pa + i);
            simd::float32x4 b = simd::load(pb + i);

            a = a - b;

            simd::store(pa + i, a);
        }

        for (; i < n; i++) {
            pa[i] -= pb[i];
        }
    }

    inline float dot_product(const float* pa, const float* pb, const int n) {
        simd::float32x4 s = simd::set_zero<simd::float32x4>();

        int i = 0;
        for (; i + 4 <= n; i += 4) {
            simd::float32x4 a = simd::load(pa + i);
            simd::float32x4 b = simd::load(pb + i);

            s = s + a * b;
        }

        float sv[4];
        simd::store(sv, s);
        
        float r = std::accumulate(sv, sv + 4, 0.f);

        for (; i < n; i++) {
            r += pa[i] * pb[i];
        }

        return r;
    }

    template<typename T>
    void operator+=(std::vector<T>& a, const std::vector<T>& b) {
        assert(isize(a) == isize(b));

        for (int i : range(a)) {
            a[i] += b[i];
        }
    }

    template<>
    void operator+=<point<float>>(std::vector<point<float>>& a, const std::vector<point<float>>& b) {
        assert(isize(a) == isize(b));

        increment((float*)a.data(), (const float*)b.data(), 2 * isize(a));
    }

    template<typename T>
    auto operator+(const std::vector<T>& a, const std::vector<T>& b) {
        std::vector<T> r(a);

        r += b;

        return r;
    }

    template<typename T>
    void operator-=(std::vector<T>& a, const std::vector<T>& b) {
        assert(isize(a) == isize(b));

        for (int i : range(a)) {
            a[i] -= b[i];
        }
    }

    template<>
    void operator-=<point<float>>(std::vector<point<float>>& a, const std::vector<point<float>>& b) {
        assert(isize(a) == isize(b));

        decrement((float*)a.data(), (const float*)b.data(), 2 * isize(a));
    }

    template<typename T>
    auto operator-(const std::vector<T>& a, const std::vector<T>& b) {
        std::vector<T> r(a);

        r -= b;

        return r;
    }

    float dot_product(const std::vector<point<float>>& a, const std::vector<point<float>>& b) {
        assert(isize(a) == isize(b));

        return dot_product((const float*)a.data(), (const float*)b.data(), 2 * isize(a));
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

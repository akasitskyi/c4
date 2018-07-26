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

#include <cmath>
#include <cstdint>

namespace c4 {
    namespace math {
        template<typename T>
        inline T pi() {
            static const T __pi = 2 * acos((T)0);
            return __pi;
        }

        template<typename T>
        inline T abs_or_rel_error(T x, T x0) {
            T abs_err = std::abs(x - x0);
            return abs_err / std::max(std::abs(x0), (T)1);
        }

        template<typename T>
        inline bool almost_equal(T x, T x0, T eps) {
            T r = abs_or_rel_error(x, x0);

            return r < eps;
        }

        template<typename T>
        inline auto sqr(T q) -> decltype(q * q) {
            return q * q;
        }

        template<typename T>
        inline T signed_sqrt(const T& x) {
            return x >= 0 ? sqrt(x) : -sqrt(-x);
        }

        inline float logistic_function(float k, float x0, float x) {
            return 1.f / (1.f + std::exp(-k * (x - x0)));
        }

        template<class dst_t, class src_t>
        inline typename std::enable_if<std::is_arithmetic<src_t>::value && std::is_arithmetic<dst_t>::value, dst_t>::type clamp(src_t x, dst_t x0, dst_t x1) {
            if (x < x0)
                return x0;
            if (x > x1)
                return x1;

            return (dst_t)x;
        }

        template<class dst_t, class src_t>
        typename std::enable_if<std::is_same<src_t, dst_t>::value, dst_t>::type clamp(src_t x) {
            return x;
        }

        template<class dst_t, class src_t>
        typename std::enable_if<!std::is_same<src_t, dst_t>::value && std::is_floating_point<dst_t>::value, dst_t>::type clamp(src_t x) {
            return (dst_t)x;
        }

        template<class dst_t, class src_t>
        inline typename std::enable_if<!std::is_same<src_t, dst_t>::value && std::is_integral<src_t>::value && std::is_same<dst_t, uint8_t>::value, uint8_t>::type clamp(src_t x) {
            return uint8_t((x & ~255) == 0 ? x : (x < 0 ? 0 : 255));
        }

        template<class dst_t, class src_t>
        inline typename std::enable_if<!std::is_same<src_t, dst_t>::value && std::is_integral<src_t>::value && std::is_same<dst_t, int8_t>::value, int8_t>::type clamp(src_t x) {
            return int8_t(clamp<uint8_t>(x + 128) - 128);
        }

        template<class dst_t, class src_t>
        inline typename std::enable_if<!std::is_same<src_t, dst_t>::value && std::is_floating_point<src_t>::value && (std::is_same<dst_t, uint8_t>::value || std::is_same<dst_t, int8_t>::value), dst_t>::type clamp(src_t x) {
            return clamp<dst_t>((int)x);
        }

        template<class dst_t, class src_t>
        inline typename std::enable_if<!std::is_same<src_t, dst_t>::value && std::is_integral<dst_t>::value && !std::is_same<dst_t, uint8_t>::value && !std::is_same<dst_t, int8_t>::value, dst_t>::type clamp(src_t x) {
            return clamp(x, std::numeric_limits<dst_t>::min(), std::numeric_limits<dst_t>::max());
        }

        template<class dst_t, class src_t>
        typename std::enable_if<!std::is_same<src_t, dst_t>::value && !std::is_arithmetic<dst_t>::value, dst_t>::type clamp(src_t x) {
            return dst_t(x);
        }

        template<class T>
        int clz(T x) {
            constexpr int n = 8 * sizeof(T);
            constexpr T mask = T(1ll << (n - 1));

            int r = 0;
            
            for (; r < n && !(x & mask); r++) {
                x <<= 1;
            }

            return r;
        }

    }; // namespace math
}; // namespace c4
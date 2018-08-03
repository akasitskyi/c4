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

#include "math.hpp"

namespace c4 {
    template<class T>
    struct point{
        T x;
        T y;

        point() : x(0), y(0) {}
        point(T x, T y) : x(x),y(y) {}
    
        auto polar_angle() const -> decltype(atan2(y, x)) {
            return atan2(y, x);
        }

        auto length_squared() const -> decltype(x * x + y * y){
            return x * x + y * y;
        }

        auto length() const -> decltype(std::sqrt(length_squared())) {
            return std::sqrt(length_squared());
        }

        template<class cfloat, class = std::enable_if<std::is_floating_point<cfloat>::value>::type>
        point rotate(cfloat sn, cfloat cs) {
            return point(cs * x - sn * y, sn * x + cs * y);
        }

        template<class cfloat, class = std::enable_if<std::is_floating_point<cfloat>::value>::type>
        point rotate(cfloat alpha) {
            return rotate(sin(alpha), cos(alpha));
        }

        inline bool inside_triangle(const point& a, const point& b, const point& c) const ;
    };

    template<class T>
    inline ostream& operator<<(ostream &out, const point<T> &t ) {
        out << "( " <<  t.x << " , " << t.y << " )";
        return out;
    }

    template<class T>
    inline point<T> operator+(const point<T>& a, const point<T>& b) {
        return { a.x + b.x, a.y + b.y };
    }

    template<class T>
    inline point<T> operator-(const point<T>& a, const point<T>& b) {
        return { a.x - b.x, a.y - b.y };
    }

    template<class T>
    inline auto operator*(const point<T>& a, const point<T>& b) -> decltype(a.x*b.x + a.y*b.y) {
        return a.x*b.x + a.y*b.y;
    }

    template<class T>
    inline auto operator^(const point<T>& a, const point<T>& b) -> decltype(a.x*b.x - a.y*b.y) {
        return a.x*b.y - a.y*b.x;
    }

    template<class T1, class T2>
    inline point<decltype(T1() * T2())> operator*(const T1& a, const point<T2>& b) {
        return { a * b.x, a * b.y };
    }

    template<class T1, class T2>
    inline point<decltype(T1() * T2())> operator*(const point<T1>& b, const T2& a) {
        return { a * b.x, a * b.y };
    }

    template<class T>
    inline point<T>& operator+=(point<T>& a, const point<T>& b) {
        a.x += b.x;
        a.y += b.y;

        return a;
    }

    template<class T>
    inline point<T>& operator-=(point<T>& a, const point<T>& b) {
        a.x -= b.x;
        a.y -= b.y;

        return a;
    }

    template<class T1, class T2>
    inline point<T>& operator*=(point<T1>& a, const T2& b) {
        a.x *= b;
        a.y *= b;

        return a;
    }

    template<class T>
    inline auto dist_squared(const point<T>& a, const point<T>& b) -> decltype((a - b).length_squared()) {
        return (a-b).length_squared();
    }

    template<class T>
    inline auto dist(const point<T>& a, const point<T>& b) -> decltype((a - b).length()) {
        return (a-b).length();
    }

    template<class T>
    inline bool point<T>::inside_triangle(const point<T>& a, const point<T>& b, const point<T>& c) const {
        const auto& p = *this;
        return sign((b - a) ^ (p - a)) != sign((c - a) ^ (p - a)) && sign((b - c) ^ (p - c)) != sign((a - c) ^ (p - c));
    }
};

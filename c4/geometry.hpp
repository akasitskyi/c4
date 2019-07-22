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
    template<typename T>
    struct point{
        T x;
        T y;

        point() : x(0), y(0) {}
        point(T x, T y) : x(x),y(y) {}
        
        template<typename T1>
        point(const point<T1>& p) : x((T)p.x), y((T)p.y) {}
    
        auto polar_angle() const {
            return atan2(y, x);
        }

        auto length_squared() const {
            return x * x + y * y;
        }

        auto length() const {
            return std::sqrt(length_squared());
        }

        template<class cfloat, class = typename std::enable_if<std::is_floating_point<cfloat>::value>::type>
        point rotate(cfloat sn, cfloat cs) {
            return point(cs * x - sn * y, sn * x + cs * y);
        }

        template<class cfloat, class = typename std::enable_if<std::is_floating_point<cfloat>::value>::type>
        point rotate(cfloat alpha) {
            return rotate(sin(alpha), cos(alpha));
        }

        inline bool inside_triangle(const point& a, const point& b, const point& c) const ;

        template <typename Archive>
        void save(Archive& archive) const {
            archive(x, y);
        }

        template <typename Archive>
        void load(Archive& archive) {
            archive(x, y);
        }
    };

    template<class T>
    inline std::ostream& operator<<(std::ostream &out, const point<T> &t ) {
        out << "( " <<  t.x << " , " << t.y << " )";
        return out;
    }

    template<class T>
    inline point<T> operator+(const point<T>& a, const point<T>& b) {
        return { a.x + b.x, a.y + b.y };
    }

    template<class T>
    inline point<T> operator-(const point<T>& a) {
        return { -a.x, -a.y };
    }

    template<class T>
    inline point<T> operator-(const point<T>& a, const point<T>& b) {
        return { a.x - b.x, a.y - b.y };
    }

    template<class T>
    inline auto operator*(const point<T>& a, const point<T>& b) {
        return a.x*b.x + a.y*b.y;
    }

    template<class T>
    inline auto operator^(const point<T>& a, const point<T>& b) {
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

    template<class T1, class T2>
    inline point<T1>& operator+=(point<T1>& a, const point<T2>& b) {
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

    template<class T1>
    inline point<T1>& operator*=(point<T1>& a, const T1& b) {
        a.x *= b;
        a.y *= b;

        return a;
    }

    template<class T>
    inline auto dist_squared(const point<T>& a, const point<T>& b) {
        return (a-b).length_squared();
    }

    template<class T>
    inline auto dist(const point<T>& a, const point<T>& b) {
        return (a-b).length();
    }

    template<class T>
    inline bool point<T>::inside_triangle(const point<T>& a, const point<T>& b, const point<T>& c) const {
        const auto& p = *this;
        return sign((b - a) ^ (p - a)) != sign((c - a) ^ (p - a)) && sign((b - c) ^ (p - c)) != sign((a - c) ^ (p - c));
    }

    template<typename Float>
    class affine_trasform {
        Float M[2][2];
        point<Float> v;
    public:
        affine_trasform() {
            M[0][0] = 1;
            M[0][1] = 0;
            M[1][0] = 0;
            M[1][1] = 1;
        }

        inline point<Float> operator()(const point<Float>& p) const {
            Float x = M[0][0] * p.x + M[0][1] * p.y + v.x;
            Float y = M[1][0] * p.x + M[1][1] * p.y + v.y;

            return point<Float>(x, y);
        }

        inline affine_trasform inverse() const {
            affine_trasform r;
            const auto d = M[0][0] * M[1][1] - M[0][1] * M[1][0];
            const auto id = 1 / d;

            r.M[0][0] = M[1][1] * id;
            r.M[0][1] = -M[0][1] * id;
            r.M[1][0] = -M[1][0] * id;
            r.M[1][1] = M[0][0] * id;

            r.v = -r(v);

            return r;
        }

        static affine_trasform move_trasform(const point<Float>& p) {
            affine_trasform r;
            r.v = p;

            return r;
        }

        static affine_trasform scale_trasform(Float scale_x, Float scale_y) {
            affine_trasform r;
            r.M[0][0] = scale_x;
            r.M[1][1] = scale_y;

            return r;
        }

        static affine_trasform rotate_trasform(Float alpha) {
            affine_trasform r;

            const Float cs = std::cos(alpha);
            const Float sn = std::sin(alpha);

            r.M[0][0] = cs;
            r.M[0][1] = sn;
            r.M[1][0] = -sn;
            r.M[1][1] = cs;
        }

        affine_trasform combine(const affine_trasform& o) const {
            const auto& N = o.M;
            affine_trasform r;
            r.M[0][0] = M[0][0] * N[0][0] + M[0][1] * N[1][0];
            r.M[0][1] = M[0][0] * N[0][1] + M[0][1] * N[1][1];
            r.M[1][0] = M[1][0] * N[0][0] + M[1][1] * N[1][0];
            r.M[1][1] = M[1][0] * N[0][1] + M[1][1] * N[1][1];

            r.v.x = M[0][0] * o.v.x + M[0][1] * o.v.y + v.x;
            r.v.y = M[1][0] * o.v.x + M[1][1] * o.v.y + v.y;

            return r;
        }
    };

    template<typename T>
    struct rectangle {
        T x, y, w, h;

        rectangle(T x, T y, T w, T h) : x(x), y(y), w(w), h(h) {}
        rectangle() : x(0), y(0), w(0), h(0) {}

        template<class T2>
        explicit rectangle(const rectangle<T2>& r) : x(c4::round<T>(r.x)), y(c4::round<T>(r.y)), w(c4::round<T>(r.w)), h(c4::round<T>(r.h)) {
        }

        auto area() const {
            return w * h;
        }

        template<typename T1>
        auto scale_around_origin(T1 sx, T1 sy) const {
            return rectangle<decltype(T() * T1())>(x * sx, y * sy, w * sx, h * sy);
        }

        template<typename T1>
        auto scale_around_origin(T1 s) const {
            return scale_around_origin(s, s);
        }

        template<typename T1>
        auto scale_around_center(T1 sx, T1 sy) const {
            auto cx2 = 2 * x + w;
            auto cy2 = 2 * y + w;
            auto w1 = w * sx;
            auto h1 = h * sy;

            auto x1 = (cx2 - w1) / 2;
            auto y1 = (cy2 - h1) / 2;

            return rectangle<decltype(T() * T1())>(x1, y1, w1, h1);
        }

        template<typename T1>
        auto scale_around_center(T1 s) const {
            return scale_around_center(s, s);
        }

        inline bool operator==(const rectangle<T>& other) const {
            return x == other.x && y == other.y && h == other.h && w == other.w;
        }

        inline bool operator!=(const rectangle<T>& other) const {
            return !operator==(other);
        }

        rectangle<T> intersect(const rectangle<T>& other) const {
            rectangle<T> r;
            
            r.x = std::max(x, other.x);
            r.y = std::max(y, other.y);
            
            const T x1 = std::min(x + w, other.x + other.w);
            const T y1 = std::min(y + h, other.y + other.h);
            
            if (x1 < r.x || y1 < r.y)
                return rectangle<T>();
            
            r.w = x1 - r.x;
            r.h = y1 - r.y;

            return r;
        }

        template<typename T1>
        inline bool contains(const point<T1>& p) const {
            return x <= p.x && p.x < x + w && y <= p.y && p.y < y + h;
        }
    };

    template<typename T>
    inline double intersection_over_union(const rectangle<T>& a, const rectangle<T>& b) {
        double sa = a.area();
        double sb = b.area();
        double si = a.intersect(b).area();

        return si / (sa + sb - si);
    }

    struct object_on_image {
        rectangle<int> rect;
        std::vector<point<float>> landmarks;
    };
};

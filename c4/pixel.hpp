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

#include <ostream>

#include "math.hpp"

namespace c4 {
    template<class T>
    struct pixel{
        T r, g, b;
        
        pixel() : r(0), g(0), b(0) {}


        template<class T1, class T2, class T3>
        pixel(T1 r, T2 g, T3 b) : r((T)r), g((T)g), b((T)b) {
            assert(fits_within<T>(r) && fits_within<T>(g) && fits_within<T>(b));
        }

        template<class T1>
        explicit pixel(T1 y) : r((T)y), g((T)y), b((T)y) {
            assert(fits_within<T>(y));
        }

        template<class T2>
        explicit pixel(const pixel<T2>& p) : r((T)p.r), g((T)p.g), b((T)p.b) {
            assert(fits_within<T>(p.r) && fits_within<T>(p.g) && fits_within<T>(p.b));
        }

        // return int for integral types and float for float types
        auto getY() const -> decltype(T() + T());

        float getCb() const;

        float getCr() const;

        void getHSV(float &h, float &s, float &v) const {
            const float eps = 1e-7f;

            float min, max, delta;
            min = std::min( r, std::min(g, b ));
            max = std::max( r, std::max(g, b ));
            v = max;                    // v
            delta = max - min;
            if( std::abs(delta) > eps)
                s = delta / max;        // s
            else {
                // r = g = b = 0        // s = 0, h is undefined
                s = 0;
                h = -1;
                return;
            }

            if( r == max )
                h = ( g - b ) / delta;      // between yellow & magenta
            else if( g == max )
                h = 2 + ( b - r ) / delta;  // between cyan & yellow
            else
                h = 4 + ( r - g ) / delta;  // between magenta & cyan
            h *= 60;                        // degrees
            if( h < 0 )
                h += 360;
        }

        static pixel<T> black() {
            return { 0, 0, 0 };
        }

        static pixel<T> red() {
            return { 255, 0, 0 };
        }

        static pixel<T> green() {
            return { 0, 255, 0 };
        }

        static pixel<T> blue() {
            return { 0, 0, 255 };
        }

        static pixel<T> gray() {
            return { 128, 128, 128 };
        }

        static pixel<T> white() {
            return { 255, 255, 255 };
        }
    };

    template<class dst_t, class src_t>
    pixel<dst_t> clamp(const pixel<src_t>& p) {
        return { clamp<dst_t>(p.r), clamp<dst_t>(p.g), clamp<dst_t>(p.b) };
    }

    struct rgb_weights{
    private:
        float wr;
        float wg;
        float wb;

        int iwr;
        int iwg;
        int iwb;

        rgb_weights(float wr, float wg, float wb) : wr(wr), wg(wg), wb(wb), iwr(int(wr * 256)), iwg(int(wg * 256)), iwb(int(wb * 256)) {}

    public:

        template<class T, class = typename std::enable_if<std::is_integral<T>::value>::type >
        auto combine(const pixel<T>& p) const -> decltype((p.r * iwr + p.g * iwg + p.b * iwb) >> 8) {
            return (p.r * iwr + p.g * iwg + p.b * iwb) >> 8;
        }

        template<class T, class = typename std::enable_if<std::is_floating_point<T>::value>::type >
        auto combine(const pixel<T>& p) const -> decltype(p.r * wr + p.g * wg + p.b * wb) {
            return p.r * wr + p.g * wg + p.b * wb;
        }

        static rgb_weights fromRG(float wR, float wG){
            return rgb_weights(wR, wG, 1.f - wR - wG);
        }

        static rgb_weights fromRB(float wR, float wB){
            return rgb_weights(wR, 1.f - wR - wB, wB);
        }

        static rgb_weights fromGB(float wG, float wB){
            return rgb_weights(1.f - wG - wB, wG, wB);
        }

        static rgb_weights fromR(float wR){
            return fromRG(wR, bt601().wG() * (1.f - wR) / (1.f - bt601().wR()));
        }

        static rgb_weights fromG(float wG){
            return fromRG(bt601().wR() * (1.f - wG) / (1.f - bt601().wG()), wG);
        }

        static rgb_weights fromB(float wB){
            return fromRB(bt601().wR() * (1.f - wB) / (1.f - bt601().wB()), wB);
        }

        static rgb_weights red(){
            return rgb_weights(1.f, 0.f, 0.f);
        }

        static rgb_weights green(){
            return rgb_weights(0.f, 1.f, 0.f);
        }

        static rgb_weights blue(){
            return rgb_weights(0.f, 0.f, 1.f);
        }

        static rgb_weights bt601(){
            return fromRB(0.299f, 0.114f);
        }

        float wR()const{return wr;}
        float wG()const{return wg;}
        float wB()const{return wb;}

    };

    template<class T>
    auto pixel<T>::getY() const -> decltype(T() + T()) {
        return rgb_weights::bt601().combine(*this);
    }

    template<class T>
    float pixel<T>::getCb() const {
        return 0.5f * (b - getY()) / (1 - rgb_weights::bt601().wB());
    }

    template<class T>
    float pixel<T>::getCr() const {
        return 0.5f * (r - getY()) / (1 - rgb_weights::bt601().wR());
    }

    inline std::ostream& operator<<(std::ostream& out,const rgb_weights& p ){
        out << "( r: " <<  +p.wR() << " , g: " << +p.wG() << " , b: " << +p.wB() << " )";
        return out;
    }

    template<class T1, class T2>
    inline auto operator+(const pixel<T1>& lhs, const pixel<T2>& rhs)->pixel<decltype(T1() + T2())> {
        return pixel<decltype(T1() + T2())>(lhs.r + rhs.r, lhs.g + rhs.g, lhs.b + rhs.b);
    }

    template<class T1, class T2>
    inline auto operator-(const pixel<T1>& lhs, const pixel<T2>& rhs)->pixel<decltype(T1() - T2())> {
        return pixel<decltype(T1() - T2())>(lhs.r - rhs.r, lhs.g - rhs.g, lhs.b - rhs.b);
    }

    template<class T1, class T2>
    inline pixel<decltype(T1() * T2())> operator*(const pixel<T1>& p, T2 alpha) {
        return pixel<decltype(T1() * T2())>(p.r * alpha, p.g * alpha, p.b * alpha);
    }

    template<class T1, class T2>
    inline pixel<decltype(T1() * T2())> operator*(T1 alpha, const pixel<T2>& p) {
        return p * alpha;
    }

    template<class T1, class T2>
    inline pixel<decltype(T1() * T2())> operator*(const pixel<T1>& p1, const pixel<T2>& p2) {
        return pixel<decltype(T1() * T2())>(p1.r * p2.r, p1.g * p2.g, p1.b * p2.b);
    }

    template<class T1, class T2>
    inline pixel<decltype(T1() * 1.f / T2())> operator/(const pixel<T1> p, T2 alpha) {
        return p * (1.f / alpha);
    }

    template<class T>
    inline pixel<T> operator >> (const pixel<T>& p, const int s) {
        return pixel<T>(p.r >> s, p.g >> s, p.b >> s);
    }

    template<class T>
    inline std::ostream& operator<<(std::ostream& out, const pixel<T>& p){
        out << "( r: " <<  +p.r << " , g: " << +p.g << " , b: " << +p.b << " )";
        return out;
    }

    template<class T1, class T2>
    inline float dist(const pixel<T1>& p1, const pixel<T2>& p2, rgb_weights w = rgb_weights::bt601() ){
        pixel<float> d((float)std::abs(p1.r - p2.r), (float)std::abs(p1.g - p2.g), (float)std::abs(p1.b - p2.b));

        return w.combine(d);
    }
};

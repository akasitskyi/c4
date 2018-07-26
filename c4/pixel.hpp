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

        template<class T2>
		explicit pixel(const pixel<T2>& p) {
            r = c4::math::clamp<T>(p.r);
            g = c4::math::clamp<T>(p.g);
            b = c4::math::clamp<T>(p.b);
        }

		template<class r_t, class g_t, class b_t>
		pixel(r_t r, g_t g, b_t b){
            this->r = c4::math::clamp<T>(r);
            this->g = c4::math::clamp<T>(g);
            this->b = c4::math::clamp<T>(b);
        }

		template<class T2>
		explicit pixel(T2 rgb){
			T crgb = c4::math::clamp<T>(rgb);
			r = crgb;
			g = crgb;
			b = crgb;
		}

		float getY() const;

		float getCb() const;

		float getCr() const;

		void getHSV(float &h, float &s, float &v) const {
            const float eps = 1e-7f;

			float min, max, delta;
			min = std::min( r, std::min(g, b ));
			max = std::max( r, std::max(g, b ));
			v = max;				    // v
			delta = max - min;
			if( std::abs(delta) > eps)
				s = delta / max;		// s
			else {
				// r = g = b = 0		// s = 0, h is undefined
				s = 0;
				h = -1;
				return;
			}

			if( r == max )
				h = ( g - b ) / delta;		// between yellow & magenta
			else if( g == max )
				h = 2 + ( b - r ) / delta;	// between cyan & yellow
			else
				h = 4 + ( r - g ) / delta;	// between magenta & cyan
			h *= 60;				        // degrees
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

	struct rgb_weights{
		template<class T, class = enable_if<is_integral<T>::value>::type >
		int combine(const pixel<T>& p) const {
			return (p.r * iwr + p.g * iwg + p.b * iwb) >> 8;
		}

		template<class T, class = enable_if<is_floating_point<T>::value>::type >
		float combine(const pixel<T>& p) const {
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

	private:
		float wr;
		float wg;
		float wb;

		int iwr;
		int iwg;
		int iwb;

		rgb_weights(float wr, float wg, float wb) : wr(wr), wg(wg), wb(wb), iwr(int(wr * 256)), iwg(int(wg * 256)), iwb(int(wb * 256)) {}
	};

	template<class T>
	float pixel<T>::getY() const {
		return (float)rgb_weights::bt601().combine(*this);
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
	auto operator+(const pixel<T1>& lhs, const pixel<T2>& rhs)->pixel<decltype(T1() + T2())> {
		return pixel<decltype(T1() + T2())>(lhs.r + rhs.r, lhs.g + rhs.g, lhs.b + rhs.b);
	}

	template<class T1, class T2>
	auto operator-(const pixel<T1>& lhs, const pixel<T2>& rhs)->pixel<decltype(T1() - T2())> {
		return pixel<decltype(T1() - T2())>(lhs.r - rhs.r, lhs.g - rhs.g, lhs.b - rhs.b);
	}

	template<class T1, class T2>
	pixel<decltype(T1() * T2())> operator*(const pixel<T1>& p, T2 alpha) {
		return pixel<decltype(T1() * T2())>(p.r * alpha, p.g * alpha, p.b * alpha);
	}

	template<class T1, class T2>
    pixel<decltype(T1() * T2())> operator*(T1 alpha, const pixel<T2>& p) {
		return p * alpha;
	}

	template<class T1, class T2>
    pixel<decltype(T1() * T2())> operator*(const pixel<T1>& p1, const pixel<T2>& p2) {
		return pixel<decltype(T1() * T2())>(p1.r * p2.r, p1.g * p2.g, p1.b * p2.b);
	}

	template<class T1, class T2>
	inline pixel<decltype(T1() * 1.f / T2())> operator/(const pixel<T1> p, T2 alpha) {
		return p * (1.f / alpha);
	}

	template<class T1, class T2>
	void operator*=(pixel<T1>& p, T2 alpha){
		p = pixel<T1>(p * alpha);
	}

	template<class T1, class T2>
	inline void operator/=(pixel<T1>& p, T2 alpha){
		p = pixel<T1>(p / alpha);
	}

	template<class T1, class T2>
	inline void operator+=(pixel<T1>& lhs, const pixel<T2>& rhs) {
        lhs.r = c4::math::clamp<T1>(lhs.r + rhs.r);
        lhs.g = c4::math::clamp<T1>(lhs.g + rhs.g);
        lhs.b = c4::math::clamp<T1>(lhs.b + rhs.b);
    }

	template<class T1, class T2>
	inline void operator-=(pixel<T1>& lhs, const pixel<T2>& rhs) {
        lhs.r = c4::math::clamp<T1>(lhs.r - rhs.r);
        lhs.g = c4::math::clamp<T1>(lhs.g - rhs.g);
        lhs.b = c4::math::clamp<T1>(lhs.b - rhs.b);
	}

	template<class T>
	inline pixel<T> operator >> (const pixel<T>& p, const int s) {
		return pixel<T>(p.r >> s, p.g >> s, p.b >> s);
	}

	template<class T>
	inline ostream& operator<<(ostream& out, const pixel<T>& p){
		out << "( r: " <<  +p.r << " , g: " << +p.g << " , b: " << +p.b << " )";
		return out;
	}

	template<class T1, class T2>
	float dist(const pixel<T1>& p1, const pixel<T2>& p2, rgb_weights w = rgb_weights::bt601() ){
        pixel<float> d((float)std::abs(p1.r - p2.r), (float)std::abs(p1.g - p2.g), (float)std::abs(p1.b - p2.b));

        return w.combine(d);
	}
};

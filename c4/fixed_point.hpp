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

#include <limits>
#include "math.hpp"

namespace c4 {
	template<class T, int shift>
	struct fixed_point {
        typedef T base_t;

        base_t base;

        fixed_point() : base(0) {}

        fixed_point(float v) {
			v = round(v * (1 << shift));
            base = clamp<base_t>(v);
		}

		operator float() const {
			float v = base * (1.f / (1 << shift));
			return v;
		}

		static float min() {
			return std::numeric_limits<base_t>::min() * (1.f / (1 << shift));
		}

		static float max() {
			return std::numeric_limits<base_t>::max() * (1.f / (1 << shift));
		}
	};

    template<class T, int shift>
    fixed_point<T, shift> operator+(fixed_point<T, shift> a, fixed_point<T, shift> b) {
        fixed_point<T, shift> r;
        r.base = a.base + b.base;
        return r;
    }

    template<class T, int shift>
    fixed_point<T, shift> operator-(fixed_point<T, shift> a, fixed_point<T, shift> b) {
        fixed_point<T, shift> r;
        r.base = a.base - b.base;
        return r;
    }

    template<class T, int shift>
    fixed_point<T, 2 * shift> operator*(fixed_point<T, shift> a, fixed_point<T, shift> b) {
        fixed_point<T, 2 * shift> r;
        r.base = a.base * b.base;
        return r;
    }
};

#ifndef __0ALEX_FIXED_POINT_HPP__
#define __0ALEX_FIXED_POINT_HPP__

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

#endif //__0ALEX_FIXED_POINT_HPP__

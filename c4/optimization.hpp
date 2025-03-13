//MIT License
//
//Copyright(c) 2025 Alex Kasitskyi
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
#include <c4/range.hpp>
#include <c4/linear_algebra.hpp>

namespace c4 {
    template<class F>
    static std::vector<double> minimize(const std::vector<double> l, const std::vector<double> h, std::vector<double> v0, F&& f, const int iterations = 10) {
		std::vector<double> d = h - l;

		double sum0 = f(v0);

		for (int it : range(iterations)) {
			for(int i : range(v0)){
				double adjust = 0.5;
				for(int sign : {-1, 1}){
					std::vector<double> v = v0;
					v[i] = clamp(v0[i] + sign * d[i], l[i], h[i]);
					double sum = f(v);
					if (sum < sum0) {
						sum0 = sum;
						v0 = v;
						adjust = 1.;
					}
				}
			}
        }

		return v0;
    }
};

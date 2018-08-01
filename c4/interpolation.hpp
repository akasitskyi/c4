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

#include <vector>
#include "range.hpp"
#include "exception.hpp"
#include "geometry.hpp"

namespace c4 {
	class cubic_spline{
		struct spline{
			double a;
			double b;
			double c;
			double d;
			double x;
		};

		std::vector<spline> splines;

	public:
        cubic_spline(const std::vector<c4::point<double>>& p) : splines(p.size()) {
			int n = (int)p.size();
			if( n < 3 )
				THROW_EXCEPTION("We need at least 3 points to fit cubic spline");

			for(int i : range(p)){
				splines[i].x = p[i].x;
				splines[i].a = p[i].y;
			}

            std::sort(splines.begin(), splines.end(), [](const spline& lhs, const spline& rhs) {return lhs.x < rhs.x; });

			splines[0].c = 0;

			std::vector<double> alpha(n - 1);
            std::vector<double> beta(n - 1);
			
			double A(0), B(0), C(0), F(0), h_i(0), h_i1(0), z(0);
			
			alpha[0] = 0;
			beta[0] = 0;

            for (int i : range(1, n - 1)) {
				h_i = splines[i].x - splines[i - 1].x, h_i1 = splines[i + 1].x - splines[i].x;
				A = h_i;
				C = 2. * (h_i + h_i1);
				B = h_i1;
				F = 6. * ((splines[i + 1].a - splines[i].a) / h_i1 - (splines[i].a - splines[i - 1].a) / h_i);
				z = (A * alpha[i - 1] + C);
				alpha[i] = -B / z;
				beta[i] = (F - A * beta[i - 1]) / z;
			}

			splines[n - 1].c = (F - A * beta[n - 2]) / (C + A * alpha[n - 2]);
            
            for (int i : range(1, n - 1).reverse()) {
                splines[i].c = alpha[i] * splines[i + 1].c + beta[i];
            }

            for (int i : range(1, n).reverse()) {
                double h_i = splines[i].x - splines[i - 1].x;
				splines[i].d = (splines[i].c - splines[i - 1].c) / h_i;
				splines[i].b = h_i * (2. * splines[i].c + splines[i - 1].c) / 6. + (splines[i].a - splines[i - 1].a) / h_i;
			}
		}

		double operator()(double x) const {
			const spline& s = select_spline(x);

			double dx = (x - s.x);
			double y = s.a + (s.b + (s.c / 2. + s.d * dx / 6.) * dx) * dx;

			return y;
		}

    private:
        const spline& select_spline(double x) const {
            if (x <= splines[0].x)
                return splines[0];

            if (x >= splines.back().x)
                return splines.back();

            int i = 0, j = int(splines.size()) - 1;
            while (i + 1 < j)
            {
                int k = i + (j - i) / 2;
                if (x <= splines[k].x)
                    j = k;
                else
                    i = k;
            }

            return splines[j];
        }
	};

    class lagrange {
        std::vector<double> c;
        std::vector<double> x;
        std::vector<double> ml;
        std::vector<double> mr;
    public:
        lagrange(const std::vector<c4::point<double> >& p) : c(p.size()), x(p.size()), ml(p.size()), mr(p.size()) {
            for (int i : c4::range(p)) {
                x[i] = p[i].x;
                double d = 1.;
                for (int j : c4::range(p)) {
                    if (i == j)
                        continue;
                    d *= p[i].x - p[j].x;
                }
                c[i] = p[i].y / d;
            }
        }

        double operator()(double t) const {
            std::vector<double> ml(x.size(), 1.);
            std::vector<double> mr(x.size(), 1.);

            for (int i : c4::range(1, ml.size()))
                ml[i] = ml[i - 1] * (t - x[i - 1]);
            
            for (int i = (int)x.size() - 2; i >= 0; i--)
                mr[i] = mr[i + 1] * (t - x[i + 1]);

            double r = 0;
            for (int i : c4::range(c)) {
                r += ml[i] * mr[i] * c[i];
            }

            return r;
        }
    };
};

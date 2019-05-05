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

#include <c4/simd.hpp>
#include <c4/math.hpp>
#include <c4/logger.hpp>
#include <c4/parallel.hpp>

#include <cmath>
#include <vector>
#include <fstream>
#include <iostream>
#include <numeric>
#include <functional>

void normalizeSimple(const std::vector<float>& x, const std::vector<float>& y, std::vector<std::pair<float, float>>& r) {
    c4::scoped_timer t("normalizeSimple");

    for (size_t i = 0; i < x.size(); i++) {
        float inorm = 1.f / sqrt(x[i] * x[i] + y[i] * y[i]);
        r[i].first = x[i] * inorm;
        r[i].second = y[i] * inorm;
    }
}

void normalizeParallel(const std::vector<float>& x, const std::vector<float>& y, std::vector<std::pair<float, float>>& r) {
    c4::scoped_timer t("normalizeParallel");

    c4::parallel_for(c4::range(x), [&](int i){
        float inorm = 1.f / sqrt(x[i] * x[i] + y[i] * y[i]);
        r[i].first = x[i] * inorm;
        r[i].second = y[i] * inorm;
    });
}

void normalizeSimd(const std::vector<float>& x, const std::vector<float>& y, std::vector<std::pair<float, float>>& r) {
    c4::scoped_timer t("normalizeSimd");

    size_t i = 0;

    for (; i + 4 < x.size(); i += 4) {
        c4::simd::float32x4 xi = c4::simd::load(&x[i]);
        c4::simd::float32x4 yi = c4::simd::load(&y[i]);

        c4::simd::float32x4 norm2 = xi * xi + yi * yi;

        c4::simd::float32x4 inorm = c4::simd::rsqrt(norm2);

        // multiply
        xi = xi * inorm;
        yi = yi * inorm;

        // store interleaved
        c4::simd::store_2_interleaved((float*)&r[i], { xi, yi });
    }

    for (; i < x.size(); i++) {
        float inorm = 1.f / sqrt(x[i] * x[i] + y[i] * y[i]);
        r[i].first = x[i] * inorm;
        r[i].second = y[i] * inorm;
    }
}

void normalizeParallelSimd(const std::vector<float>& x, const std::vector<float>& y, std::vector<std::pair<float, float>>& r) {
    c4::scoped_timer t("normalizeParallelSimd");

    c4::parallel_for(c4::range(x.size() / 4),[&](int i) {
        c4::simd::float32x4 xi = c4::simd::load(x.data() + 4 * i);
        c4::simd::float32x4 yi = c4::simd::load(y.data() + 4 * i);

        c4::simd::float32x4 norm2 = xi * xi + yi * yi;

        c4::simd::float32x4 inorm = c4::simd::rsqrt(norm2);

        // multiply
        xi = xi * inorm;
        yi = yi * inorm;

        // store interleaved
        c4::simd::store_2_interleaved((float*)(r.data() + 4 * i), { xi, yi });
    });

    for (int i : c4::range(x.size() / 4 * 4, x.size())) {
        float inorm = 1.f / sqrt(x[i] * x[i] + y[i] * y[i]);
        r[i].first = x[i] * inorm;
        r[i].second = y[i] * inorm;
    }
}

// vector normalization
int main(int argc, char* argv[]) {
    c4::parallel_invoke(
        [] {std::cout << c4::thread_pool::get_default_pool().get_thread_index(); },
        [] {std::cout << c4::thread_pool::get_default_pool().get_thread_index(); },
        [] {std::cout << c4::thread_pool::get_default_pool().get_thread_index(); });

    std::cout << std::endl;

    const int n = argc > 1 ? std::stoi(argv[1]) : 100000000;

    std::vector<float> x(n);
    std::vector<float> y(n);

    {
        c4::scoped_timer t("serial random fill");

        c4::fast_rand rnd(1);
        
        for (int i : c4::range(n))
            x[i] = float(rnd());
    }

    {
        c4::scoped_timer t("parallel random fill");

        // I know there's no need to pass it. Just wanted to play with enumerable_thread_specific
        c4::enumerable_thread_specific<c4::fast_rand> rnd_v(2);

        c4::parallel_for(c4::range(n), [&y, &rnd_v](c4::range r) {
            c4::fast_rand rnd = rnd_v.local();

            for(int i : r)
                y[i] = float(rnd());
        });
    }

    std::vector<std::pair<float, float> > res(n, std::pair<float, float>(-1.f, -1.f));

    const int random_check = rand() % n;

    normalizeSimple(x, y, res);
    std::cout << "(" << res[random_check].first << ", " << res[random_check].second << ")" << std::endl;
    std::fill(res.begin(), res.end(), std::pair<float, float>(-1.f, -1.f));
    
    normalizeSimd(x, y, res);
    std::cout << "(" << res[random_check].first << ", " << res[random_check].second << ")" << std::endl;
    std::fill(res.begin(), res.end(), std::pair<float, float>(-1.f, -1.f));

    normalizeParallel(x, y, res);
    std::cout << "(" << res[random_check].first << ", " << res[random_check].second << ")" << std::endl;
    std::fill(res.begin(), res.end(), std::pair<float, float>(-1.f, -1.f));

    normalizeParallelSimd(x, y, res);
    std::cout << "(" << res[random_check].first << ", " << res[random_check].second << ")" << std::endl;
    std::fill(res.begin(), res.end(), std::pair<float, float>(-1.f, -1.f));

    {
        c4::scoped_timer t("serial accumulate");
        double acc_serial = std::accumulate(x.begin(), x.end(), 0.);

        std::cout << "acc_serial = " << acc_serial << std::endl;
    }

    {
        c4::scoped_timer t("parallel accumulate");
        double acc_parallel = c4::parallel_reduce(x.begin(), x.end(), 0., std::plus<double>(), [](std::vector<float>::iterator first, std::vector<float>::iterator last) {
            return std::accumulate(first, last, 0.);
        });

        std::cout << "acc_parallel = " << acc_parallel << std::endl;
    }

    return 0;
}

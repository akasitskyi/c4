#include <c4/simd.hpp>
#include <c4/math.hpp>
#include <c4/range.hpp>

#include <cmath>
#include <vector>
#include <iostream>

std::vector<std::pair<float, float> > normalizeSimple(const std::vector<float>& x, const std::vector<float>& y) {
    std::vector<std::pair<float, float> > r(x.size());
    
    for (size_t i = 0; i < x.size(); i++) {
        float inorm = 1.f / sqrt(x[i] * x[i] + y[i] * y[i]);
        r[i].first = x[i] * inorm;
        r[i].second = y[i] * inorm;
    }

    return r;
}

std::vector<std::pair<float, float> > normalizeSimd(const std::vector<float>& x, const std::vector<float>& y) {
    std::vector<std::pair<float, float> > r(x.size());

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

    return r;
}

int main()
{
    // vector normalization
    constexpr int n = 10;
    std::vector<float> x(n);
    std::vector<float> y(n);
    
    for (int i : c4::range(n)) {
        x[i] = float(rand()) / RAND_MAX;
        y[i] = float(rand()) / RAND_MAX;
    }

    auto simple_res = normalizeSimple(x, y);
    auto simd_res = normalizeSimd(x, y);

    for (auto p : simple_res)
        std::cout << "(" << p.first << ", " << p.second << ")\t";
    
    std::cout << std::endl;

    for (auto p : simd_res)
        std::cout << "(" << p.first << ", " << p.second << ")\t";

    std::cout << std::endl;

    return 0;
}


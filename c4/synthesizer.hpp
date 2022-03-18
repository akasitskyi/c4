//MIT License
//
//Copyright(c) 2022 Alex Kasitskyi
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
#include "matrix.hpp"
#include "linear_algebra.hpp"

namespace c4 {

    class ADSR {
        int a;
        int d;
        float s;
        int r;
    public:
        ADSR(int a, int d, float s, int r) : a(a), d(d), s(s), r(r) {}

        void operator()(vector_ref<float>& v) {
            int sl = isize(v) - a - d - r;
            ASSERT_TRUE(sl >= 0);

            // Attack
            for (int i : range(a)) {
                v[i] *= i * 1. / a;
            }

            // Decay
            for (int i : range(d)) {
                v[a + i] *= (s * i + 1.f * (d - i)) / d;
            }

            //Sustain
            for (int i : range(sl)) {
                v[a + d + i] *= s;
            }

            // Release
            for (int i : range(r)) {
                v[a + d + sl + i] *= s * (r - i) / r;
            }
        }

        int adjustLength(int pressed) {
            return std::max(pressed, a + d) + r;
        }
    };

    class WaveGenerator {
        int rate;
    public:
        WaveGenerator(int rate) : rate(rate) {}

        std::vector<float> sine(float hz, int length) {
            std::vector<float> v(length);
            for (int i : range(v)) {
                float err = std::sin(5 * i * 2 * pi<float>() / rate) * 0.0001f;
                v[i] = std::sin(hz * (1.f + err) * i * 2 * pi<float>() / rate);
            }

            return v;
        }

        std::vector<float> saw(float hz, int length) {
            int period = rate / hz;

            std::vector<float> v(length);
            for (int i : range(v)) {
                v[i] = (i % period - period * 0.5f) / period;
            }

            return v;
        }

        std::vector<float> square(float hz, int length) {
            int period = rate / hz;

            std::vector<float> v(length);
            for (int i : range(v)) {
                v[i] = i % period < period / 2 ? -1.f : 1.f;
            }

            return v;
        }
    };

    class LowPassFilter {
        double a1 = 0;
        double a2 = 0;
        double b0 = 0;
        double b1 = 0;
        double b2 = 0;

        double px = 0;
        double ppx = 0;
        double py = 0;
        double ppy = 0;
    public:
        LowPassFilter(double hz, int sampleRate) {
            const double ita = 1.0 / std::tan(pi<double>() * hz / sampleRate);
            const double q = sqrt(2.0);
            b0 = 1.0 / (1.0 + q * ita + ita * ita);
            b1 = 2 * b0;
            b2 = b0;
            a1 = 2.0 * (ita * ita - 1.0) * b0;
            a2 = -(1.0 - q * ita + ita * ita) * b0;
        }

        float operator()(float x) {
            double y = b0 * x + b1 * px + b2 * ppx + a1 * py + a2 * ppy;
            ppx = px;
            px = x;
            ppy = py;
            py = y;
            return y;
        }
    };

static constexpr float as = 0.01f;
static constexpr float ds = 0.5f;
static constexpr float s = 0.5f;
static constexpr float rs = 0.1f;

    class Piano {
        int rate;
        ADSR adsr;
        WaveGenerator wg;
    public:
        Piano(int rate, int a, int d, float s, int r) : rate(rate), adsr(a, d, s, r), wg(rate) {}
        Piano(int rate) : Piano(rate, as* rate, ds* rate, s, rs* rate) {}

        std::vector<float> operator()(float hz, float pressedSec) {
            const int pressedSamples = pressedSec * rate;
            const int length = adsr.adjustLength(pressedSamples);

            std::vector<float> res = wg.saw(hz, length);

            float sw = 1;
            for (int m = 1; m < 10 && m * hz * 2 < rate; m++) {
                float w = 6.f / (std::pow(m, 1) + 5);
                res += wg.sine(m * hz, length) * w;
                sw += w;
            }

            res = res / sw;

            const int reflectDelay[] = { rate / 201, rate / 321, rate / 455 };
            const float reflectRate = 0.25f;

            for (auto rd : reflectDelay) {
                for (int i = 0; i + rd < isize(res); i++) {
                    res[i + rd] += res[i] * reflectRate;
                }
            }

            std::vector<LowPassFilter> lpfs;
            for (int k = 0; k < 4; k++) {
                lpfs.emplace_back(hz * 6, rate);
            }
            lpfs.emplace_back(4000, rate);
            lpfs.emplace_back(8000, rate);

            for (auto& x : res) {
                for (auto& f : lpfs) {
                    x = f(x);
                }
            }

            adsr(vector_ref(res));

            return res;
        }
    };
};

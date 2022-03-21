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
#include <deque>
#include <map>
#include <mutex>

#include "matrix.hpp"
#include "linear_algebra.hpp"

namespace c4 {

    class ADSR {
        int i = 0;
        bool released = false;
        int a;
        int d;
        float s;
        int r;
    public:
        ADSR(int a, int d, float s, int r) : a(a), d(d), s(s), r(r) {}

        float operator()(float x) {
            // Release
            if (released) {
                float ret = x * s * (r - i) / r;
                i++;
                return ret;
            }

            // Attack
            if (i < a) {
                float ret = x * i / a;
                i++;
                return ret;
            }
            
            // Decay
            if (i < a + d) {
                float ret = x * (s * i + 1.f * (d - i)) / d;
                i++;
                return ret;
            }

            //Sustain
            i++;
            return x * s;
        }

        int release() {
            i = 0;
            released = true;

            return r;
        }
    };

    class SineWaveGenerator {
        int rate;
        float hz;
        int i = 0;
    public:
        SineWaveGenerator(int rate, float hz) : rate(rate), hz(hz) {}

        float operator()() {
            const float err = std::sin(5 * i * 2 * pi<float>() / rate) * 0.0001f;
            const float r = std::sin(hz * (1.f + err) * i * 2 * pi<float>() / rate);
            i++;
            return r;
        }
    };

    class SawWaveGenerator {
        int rate;
        float hz;
        int i = 0;
    public:
        SawWaveGenerator(int rate, float hz) : rate(rate), hz(hz) {}

        float operator()() {
            int period = int(rate / hz);

            float r = (i % period - period * 0.5f) / period;
            i++;
            return r;
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
            return (float)y;
        }
    };

    //FIXME: !!!
    static constexpr float as = 0.01f;
    static constexpr float ds = 0.5f;
    static constexpr float s = 0.5f;
    static constexpr float rs = 0.1f;

    class PianoTone {
        int rate;
        float hz;
        SawWaveGenerator saw;
        std::vector<SineWaveGenerator> swgs;
        ADSR adsr;
        std::vector<LowPassFilter> lpfs;
    public:
        PianoTone(int rate, float hz, int a, int d, float s, int r) :
        rate(rate), hz(hz), saw(rate, hz), adsr(a, d, s, r) {
            for (int m = 1; m < 10 && m * hz * 2 < rate; m++) {
                swgs.emplace_back(rate, m * hz);
            }

            for (int k = 0; k < 4; k++) {
                lpfs.emplace_back(hz * 6, rate);
            }
            lpfs.emplace_back(4000, rate);
            lpfs.emplace_back(8000, rate);
        }
        
        PianoTone(int rate, float hz) : PianoTone(rate, hz, int(as* rate), int(ds* rate), s, int(rs* rate)) {
        }

        float operator()() {
            float res = saw();

            float sw = 1;
            for (int m : range(swgs)) {
                float w = 6.f / float(std::pow(m + 1, 1) + 5);
                res += swgs[m]() * w;
                sw += w;
            }

            res = res / sw;

            for (auto& f : lpfs) {
                res = f(res);
            }

            res = adsr(res);

            return res;
        }

        int release() {
            return adsr.release();
        }
    };

    class Piano {
        std::mutex mu;

        int sampleRate;
        std::map<int, PianoTone> playingTones;

        std::deque<float> add;
    public:
        Piano(int sampleRate) : sampleRate(sampleRate), add({ 0.f }) {}

        int press(int tone) {
            std::lock_guard<std::mutex> lock(mu);

            const float hz = (float)(27.5 * std::pow(2., (double)tone / 12.));

            playingTones.emplace(tone, PianoTone(sampleRate, hz));

            return 0;
        }

        int release(int tone) {
            std::lock_guard<std::mutex> lock(mu);

            // TODO: release should be added to add

            auto it = playingTones.find(tone);

            if (it == playingTones.end()) {
                return -1;
            }

            const int rem = it->second.release();

            if (rem > add.size()) {
                add.resize(rem);
            }
            for (int i : range(rem)) {
                add[i] += it->second();
            }

            playingTones.erase(tone);

            return 0;
        }

        float operator()() {
            float res = 0;
            for (auto& t : playingTones) {
                res += t.second();
            }

            res += add.front();
            add.pop_front();
            add.push_back(0.f);

            const int reflectDelay[] = { sampleRate / 201, sampleRate / 321, sampleRate / 455 };
            const float reflectRate = 0.25f;

            for (auto rd : reflectDelay) {
                if (rd >= add.size()) {
                    add.resize(rd + 1);
                }

                add[rd] += res * reflectRate;
            }

            return res;
        }
    };
};

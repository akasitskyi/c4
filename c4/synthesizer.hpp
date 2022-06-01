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
#include <algorithm>
#include <map>

#include "matrix.hpp"
#include "linear_algebra.hpp"
#include "ring_buffer.hpp"

namespace c4 {

    class LowPassFilter {
        float a1 = 0;
        float a2 = 0;
        float b0 = 0;
        //float b1 = 0;
        //float b2 = 0;

        float px = 0;
        float ppx = 0;
        float py = 0;
        float ppy = 0;
    public:
        LowPassFilter(double hz, int sampleRate) {
            const double ita = 1.0 / std::tan(pi<double>() * hz / sampleRate);
            const double q = sqrt(2.0);
            b0 = float(1.0 / (1.0 + q * ita + ita * ita));
            //b1 = float(2 * b0);
            //b2 = float(b0);
            a1 = float(2.0 * (ita * ita - 1.0) * b0);
            a2 = float(-(1.0 - q * ita + ita * ita) * b0);
        }

        float operator()(float x) {
            //            float y = b0 * x + b1 * px + b2 * ppx + a1 * py + a2 * ppy;
            float y = b0 * (x + px + px + ppx) + a1 * py + a2 * ppy;
            ppx = px;
            px = x;
            ppy = py;
            py = y;
            return y;
        }
    };

    class HighPassFilter {
        float a1 = 0;
        float a2 = 0;
        float b0 = 0;
        //float b1 = 0;
        //float b2 = 0;

        float px = 0;
        float ppx = 0;
        float py = 0;
        float ppy = 0;
    public:
        HighPassFilter(double hz, int sampleRate) {
            const double ita = 1.0 / std::tan(pi<double>() * hz / sampleRate);
            const double q = sqrt(2.0);
            b0 = float(1.0 / (1.0 + q * ita + ita * ita));
            a1 = float(2.0 * (ita * ita - 1.0) * b0);
            a2 = float(-(1.0 - q * ita + ita * ita) * b0);

            b0 = float(b0 * ita * ita);
            //b1 = -float(2 * b0);
            //b2 = float(b0);
        }

        float operator()(float x) {
            //            float y = b0 * x + b1 * px + b2 * ppx + a1 * py + a2 * ppy;
            float y = b0 * (x - px - px + ppx) + a1 * py + a2 * ppy;
            ppx = px;
            px = x;
            ppy = py;
            py = y;
            return y;
        }
    };

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

        bool done() {
            return released && i >= r;
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

            float r = ((i + period / 2) % period - period * 0.5f) / period;
            i++;
            return r;
        }
    };

    class GeneratedWaves {
        std::vector<std::vector<float>> notes;
    public:
        GeneratedWaves(int rate, double f0) : notes(88) {
            for (int i : range(notes)) {
                const float hz = float(f0 * std::pow(2, double(i) / 12.));

                const size_t period = lround((float)rate / hz);
                notes[i].resize(period);

                SawWaveGenerator saw(rate, hz);
                float sw = 1;
                std::vector<SineWaveGenerator> swgs;
                std::vector<float> w;
                for (int m = 1; m < 10 && m * hz * 2 < rate; m++) {
                    swgs.emplace_back(rate, m * hz);
                    w.push_back(6.f / float(std::pow(m, 1) + 5));
                    sw += w.back();;
                }

                for (auto& x : notes[i]) {
                    float res = saw();

                    for (int m : range(swgs)) {
                        res += swgs[m]() * w[m];
                    }

                    x = res / sw;
                }
            }
        }

        float get(int note, int i) {
            ASSERT_LESS(0, note);
            ASSERT_LESS(note, isize(notes));

            const auto& note0 = notes[note];

            return note0[i % isize(note0)];
        }
    };

    struct AdsrParams {
        float a = 0.01f;
        float d = 0.5f;
        float s = 0.5f;
        float r = 0.1f;

        static AdsrParams piano() {
            return { 0.01f, 0.5f, 0.5f, 0.1f };
        }
    };

    class PianoNote {
        std::shared_ptr<GeneratedWaves> waves;
        int rate;
        int note;
        ADSR adsr;
        LowPassFilter lpfs[4];
        int i = 0;
        int releaseTime = -1;
    public:
        PianoNote(std::shared_ptr<GeneratedWaves> waves, int rate, int note, float hz,
                                                    AdsrParams p = AdsrParams::piano()) :
        waves(waves), rate(rate), note(note),
            adsr(int(p.a* rate), int(p.d* rate), p.s, int(p.r* rate)),
            lpfs{ {std::min(hz * 6, rate * 0.4f), rate}, {std::min(hz * 6, rate * 0.4f), rate},
                  {std::min(hz * 6, rate * 0.4f), rate}, {std::min(hz * 6, rate * 0.4f), rate} }
        {}
        
        float operator()() {
        	if (i == releaseTime) {
        		release();
        	}

            float res = waves->get(note, i++);

            res = adsr(res);

            for (auto& f : lpfs) {
                res = f(res);
            }

            return res;
        }

        int release() {
            return adsr.release();
        }

        void setReleaseTime(float s) {
        	releaseTime = (int)lround(s * rate);
        }

        bool done() {
            return adsr.done();
        }
    };

    class ClickSoundGenerator {
        std::vector<float> data;
    public:
        ClickSoundGenerator(int rate, std::vector<float> f, std::vector<float> w) {
            ASSERT_EQUAL(isize(f), isize(w));

            const int a = int(rate * 0.002);
            const int d = int(rate * 0.01);
            const float s = 0.15f;
            const int st = int(rate * 0.05);
            const int r = int(rate * 0.05);

            data.resize(a + d + st + r);

            for (int k : range(f)) {
                SineWaveGenerator swg(rate, f[k]);

                for (int i : range(data)) {
                    data[i] +=  swg() * w[k];
                }
            }

            ADSR adsr(a, d, s, r);
            for (int i : range(data)) {
                data[i] = adsr(data[i]);
                if (i == a + d + st) {
                    adsr.release();
                }
            }
        }

        const auto& operator()() {
            return data;
        }
    };

    class Metronome {
        size_t i = 0;
        std::vector<float> loop;
    public:
        Metronome(int rate, float bpm, int K) {
            const int dt = int(rate * 60 / bpm);
            auto strong = ClickSoundGenerator(rate, { 1.5e3f, 3.91e3f },
                { 0.8f, 0.2f })();
            auto weak = ClickSoundGenerator(rate, { 1.21e3f, 3.03e3f },
                { 0.4f, 0.1f })();

            ASSERT_TRUE(weak.size() <= dt);
            ASSERT_TRUE(strong.size() <= dt);

            loop = strong;
            loop.resize(dt * K);

            for (int k = 1; k < K; k++) {
                std::copy(weak.begin(), weak.end(), loop.begin() + dt * k);
            }
        }

        float operator()() {
            return loop[i++ % loop.size()];
        }
    };

    class Piano {
        int sampleRate;
        // After the sampleRate!
        const int reflectDelay[5]{ sampleRate / 95, sampleRate / 123, sampleRate / 144, sampleRate / 166, sampleRate / 189 };
        const float reflectRate = 0.05f;

        float metronomeVolume = 1.f;
        double f0;
        std::shared_ptr<GeneratedWaves> waves;
        std::map<int, PianoNote> playingNotes;
        std::shared_ptr<Metronome> metronome;
        ring_buffer<float> add;
        std::vector<LowPassFilter> lpfs;
    public:
        Piano(int sampleRate) :
            sampleRate(sampleRate), f0(27.5),
            waves(new GeneratedWaves(sampleRate, f0)),
            add(*std::max_element(std::begin(reflectDelay), std::end(reflectDelay)) + 1)
        {
            lpfs.emplace_back(4000, sampleRate);
            lpfs.emplace_back(8000, sampleRate);
        }

        static float inline hz(int note) {
        	return (float)(27.5 * std::pow(2., (double)note / 12.));
        }

        int press(int note) {
            playingNotes.emplace(note, PianoNote(waves, sampleRate, note, hz(note)));

            return 0;
        }

		int playFor(int note, float duration) {
            AdsrParams p = AdsrParams::piano();
			PianoNote pn(waves, sampleRate, note, hz(note), p);
			pn.setReleaseTime(std::max(duration - p.r, 0.f));
			playingNotes.emplace(note, pn);

			return 0;
		}

        int release(int tone) {
            auto it = playingNotes.find(tone);

            if (it == playingNotes.end()) {
                return -1;
            }

            const int rem = it->second.release();

            return 0;
        }

        float operator()() {
            float res = 0;
            for (auto it = playingNotes.begin(); it != playingNotes.end();) {
                res += it->second();

                if (it->second.done()) {
                    it = playingNotes.erase(it);
                }
                else {
                    ++it;
                }
            }

            for (auto& lpf : lpfs) {
                res = lpf(res);
            }

            res += add.push(0.f);

            if (metronome) {
                res += metronomeVolume * (*metronome)();
            }

            for (auto rd : reflectDelay) {
                add[rd] += res * reflectRate;
            }

            return res;
        }

        void enableMetronome(float bpm, int K) {
            metronome.reset(new Metronome(sampleRate, bpm, K));
        }

        void setMetronomeVolume(float volume){
        	metronomeVolume = volume;
        }

        void disableMetronome() {
            metronome.reset();
        }

		bool metronomeActive() {
			return (bool)metronome;
		}

		void clearQueue() {
			playingNotes.clear();
			add.fill(0);
			disableMetronome();
		}
	};
};

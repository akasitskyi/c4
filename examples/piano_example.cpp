//MIT License
//
//Copyright(c) 2020 Alex Kasitskyi
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

#include <c4/wav.hpp>

#include <c4/synthesizer.hpp>

#include <cstdio>
#include <cstring>

using namespace std;
using namespace c4;

int main(int argc, char* argv[]) {
    try {
        const unsigned int channels = 1;
        const unsigned int sampleRate = 48000;
        const int startTone = 3 + 12;
        const int toneCount = 12 * 6 + 1;
        const float durationSec = 0.5f;
        const int toneDuration = int(durationSec * sampleRate);
        
        std::vector<float> data(toneCount * toneDuration);

        Piano piano(sampleRate);

        piano.setMetronomeVolume(0.5f);
        piano.enableMetronome(120, 4);

        {
            scoped_timer timer("Main loop");

            for (int t : range(toneCount / 2)) {
                piano.press(startTone + t);

                const int releaseTime = 2 * int(AdsrParams::piano().r * sampleRate);

                for (int i : range(toneDuration)) {
                    data[t * toneDuration + i] = piano();
                    if (i == toneDuration - releaseTime) {
                        piano.release(startTone + t);
                    }
                }
            }

            for (int t : range(toneCount / 2, toneCount)) {
                piano.playFor(startTone + t, durationSec - AdsrParams::piano().r);

                for (int i : range(toneDuration)) {
                    data[t * toneDuration + i] = piano();
                }
            }
        }

        std::vector<int16_t> result(data.size());

        c4::wav_f32_to_s16(result.data(), data.data(), result.size());

        c4::wav_data_format format;
        format.container = c4::wav_container_riff;
        format.format = c4::WAVE_FORMAT_PCM;
        format.channels = channels;
        format.sampleRate = sampleRate;
        format.bitsPerSample = 16;

        auto framesWritten = c4::wav_writer("piano.wav", format).wav_write_pcm_frames(result.size() / channels, result.data());
        ASSERT_EQUAL(framesWritten, result.size() / channels);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}

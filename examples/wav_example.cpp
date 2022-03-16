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
#include <c4/fft.hpp>

#include <c4/range.hpp>

#include <cstdio>
#include <cstring>

using namespace std;
using namespace c4;

int main(int argc, char* argv[]) {
    try {
        if (argc < 3) {
            throw std::logic_error("Usage: wav_example infile outfile [options]");
        }

        const std::string inputFile = argv[1];
        const std::string outputFile = argv[2];
        const float gain = argc < 4 ? 1.f : (float)atof(argv[3]);

        c4::scoped_timer t("wav");

        unsigned int channels;
        unsigned int sampleRate;
        uint64_t totalPCMFrameCount;
        std::vector<float> data;

        c4::wav_reader(inputFile).read_pcm_frames_f32(data, &channels, &sampleRate, &totalPCMFrameCount);
        if (data.empty()) {
            THROW_EXCEPTION("Can't read wav file: " + inputFile);
        }

        PRINT_DEBUG(sampleRate);

        ASSERT_EQUAL(channels, 1);

        std::vector<float> processed(data.size(), 0);

        const int frameSize = 16 * 1024;
        std::vector<float> buffer(frameSize);
        std::vector<std::complex<double>> F(frameSize);

        c4::STFT stft(frameSize);
        int phz = 0;

        for (int i = 0; i < data.size(); i += frameSize / 2) {
            std::fill(buffer.begin(), buffer.end(), 0);
            std::copy(data.begin() + i, data.begin() + std::min(i + frameSize, c4::isize(data)), buffer.begin());
            stft.fwd(c4::vector_ref(buffer), c4::vector_ref(F));

            // detect frequency
            std::vector<double> acc(frameSize / 2 + 1);
            for (int k = 2; k < isize(acc); k++) {
                acc[k] = std::abs(F[k]);

                // include harmonics?
                double hs = 0;
                int hn = 0;
                for (int j = k; j < isize(acc); j += k) {
                    hs += std::abs(F[j]);
                    hn++;
                }

                if (hn) {
                    acc[k] += hs / hn;
                }
            }

            int mx = max_element(acc.begin(), acc.end()) - acc.begin();

            int hz = (mx * sampleRate + frameSize / 2) / frameSize;
            if (hz != phz) {
                phz = hz;
                cout << hz << "Hz" << endl;
            }
        }

        std::vector<int16_t> result(totalPCMFrameCount * channels);

        c4::wav_f32_to_s16(result.data(), data.data(), result.size());

        c4::wav_data_format format;
        format.container = c4::wav_container_riff;
        format.format = c4::WAVE_FORMAT_PCM;
        format.channels = channels;
        format.sampleRate = sampleRate;
        format.bitsPerSample = 16;

        auto framesWritten = c4::wav_writer(outputFile, format).wav_write_pcm_frames(totalPCMFrameCount, result.data());
        ASSERT_EQUAL(framesWritten, totalPCMFrameCount);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}

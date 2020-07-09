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

#include <c4/range.hpp>

#include <cstdio>
#include <cstring>

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
        float* data = drwav_open_file_and_read_pcm_frames_f32(inputFile.c_str(), &channels, &sampleRate, &totalPCMFrameCount);
        if (data == NULL) {
            THROW_EXCEPTION("Can't read wav file: " + inputFile);
        }

        std::vector<int16_t> result(totalPCMFrameCount * 2);

        for (int i : c4::range(result.size())) {
            data[i] *= gain;
        }

        drwav_f32_to_s16(result.data(), data, totalPCMFrameCount * 2);

        drwav wav;
        drwav_data_format format;
        format.container = drwav_container_riff;
        format.format = DR_WAVE_FORMAT_PCM;
        format.channels = channels;
        format.sampleRate = sampleRate;
        format.bitsPerSample = 16;
        drwav_init_file_write(&wav, outputFile.c_str(), &format);

        auto framesWritten = drwav_write_pcm_frames(&wav, totalPCMFrameCount, result.data());
        ASSERT_EQUAL(framesWritten, totalPCMFrameCount);

        drwav_uninit(&wav);

        free(data);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}

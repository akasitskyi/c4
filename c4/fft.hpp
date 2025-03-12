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

#include <complex>

#include "matrix.hpp"

namespace c4 {
    class FFT {
        const int dim;
        std::vector<std::complex<double>> wt;

        template<typename InputT>
        void fft_internal(const InputT* in, std::complex<double>* out, int n, int inStep) {
            if (n == 1) {
                *out = *in;
                return;
            }
            if (n == 2) {
                out[0] = in[0] + in[inStep];
                out[1] = in[0] - in[inStep];
                return;
            }

            fft_internal(in, out, n / 2, inStep * 2);
            fft_internal(in + inStep, out + n / 2, n / 2, inStep * 2);

            for (int k = 0; k < n / 2; k++) {
                const auto w = wt[k * inStep];

                const auto y0k = out[k];
                const auto y1k = out[n / 2 + k];

                out[k] = y0k + w * y1k;
                out[n / 2 + k] = y0k - w * y1k;
            }
        }

    protected:
        FFT(const int dim, bool inv) : dim(dim), wt(dim) {
            ASSERT_TRUE((dim & (dim - 1)) == 0);

            std::complex<double> w = 1.;
            std::complex<double> w0 = std::exp((inv ? 2 : -2) * pi<double>() / dim * std::complex<double>(0, 1));

            for (auto& wi : wt) {
                wi = w;
                w = w * w0;
            }
        }
    
    public:
        FFT(int dim) : FFT(dim, false) {}

        template<typename InputT>
        void operator()(const vector_ref<InputT>& in, vector_ref<std::complex<double>> out) {
            ASSERT_EQUAL(in.size(), dim);
            ASSERT_EQUAL(out.size(), dim);

            fft_internal(in.data(), out.data(), in.size(), 1);
        }
    };
    
    struct IFFT : private FFT {
        std::vector<std::complex<double>> wt;

        IFFT(const int dim) : FFT(dim, true) {}

        void operator()(const vector_ref<std::complex<double>>& in, vector_ref<std::complex<double>> out) {
            FFT::operator()(in, out);

            for (int k : range(out.size())) {
                out[k] *= 1. / out.size();
            }
        }
    };

    class STFT {
        FFT fft;
        IFFT ifft;
        std::vector<double> w;
        std::vector<double> tmp;
    public:
        STFT(int dim) : fft(dim), ifft(dim), w(dim), tmp(dim) {
            for (int i : range(w)) {
                w[i] = std::sin((i + 0.5) * pi<double>() / dim);
            }
        }

        template<typename InputT>
        void fwd(const vector_ref<InputT>& in, vector_ref<std::complex<double>> out) {
            ASSERT_EQUAL(in.size(), w.size());
            for (int i : range(w)) {
                tmp[i] = in[i] * w[i];
            }

            fft(vector_ref(tmp), out);
        }
        
        void bwd(const vector_ref<std::complex<double>>& in, vector_ref<std::complex<double>> out) {
            ASSERT_EQUAL(out.size(), w.size());

            ifft(in, out);

            for (int i : range(w)) {
                out[i] *= w[i];
            }
        }
    };
}; // namespace c4

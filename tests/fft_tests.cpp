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

#include <iostream>

#include <c4/fft.hpp>
#include <c4/exception.hpp>
#include <c4/logger.hpp>

using namespace c4;
using namespace std;

class FftTest {
    const double EPS = 1.E-10;
    int dim;
    FFT fft;
    IFFT ifft;

    std::vector<double> a;
    std::vector<complex<double>> A;
    std::vector<complex<double>> iA;
public:
    FftTest(int dim) : dim(dim), fft(dim), ifft(dim), a(dim), A(dim), iA(dim) {}

    void testFftSingleSine(int period, int shift) {
        const double q = 2. * pi<double>() / period;
        for (int i : range(period)) {
            a[i] = std::sin((i + shift) * q);
        }

        for (int i : range(period, a.size())) {
            a[i] = a[i - period];
        }

        fft(vector_ref(a), vector_ref(A));

        for (int i : range(A.size() / 2)) {
            const double etalon = i == dim / period ? dim / 2 : 0.;
            if (!almost_equal(abs(A[i]), etalon, EPS * dim)) {
                THROW_EXCEPTION("abs(A[" + to_string(i) + "]) = " + to_string(A[i])
                    + ", while expected: " + to_string(etalon));
            }
        }

        ifft(vector_ref(A), vector_ref(iA));

        for (int i : range(a)) {
            if (!almost_equal(iA[i].real(), a[i], EPS) || !almost_equal(iA[i].imag(), 0., EPS)) {
                THROW_EXCEPTION("iA[" + to_string(i) + "] = " + to_string(iA[i])
                    + ", while expected: " + to_string(a[i]));
            }
        }
    }

    void testFftRandom(int seed) {
        fast_rand_float_normal rnd(seed);
        for (auto& x : a) {
            x = rnd();
        }

        {
            static time_printer tp("fft");
            scoped_timer st(tp);

            fft(vector_ref(a), vector_ref(A));
            ifft(vector_ref(A), vector_ref(iA));
        }

        for (int i : range(a)) {
            if (!almost_equal(iA[i].real(), a[i], EPS) || !almost_equal(iA[i].imag(), 0., EPS)) {
                THROW_EXCEPTION("iA[" + to_string(i) + "] = " + to_string(iA[i])
                    + ", while expected: " + to_string(a[i]));
            }
        }
    }
    
    void runTests() {
        for (int period = 2; period <= dim; period *= 2) {
            for (int shift = 0; shift < 3; shift++) {
                testFftSingleSine(period, shift);
            }
        }

        for (int seed = 0; seed < 42; seed++) {
            testFftRandom(seed);
        }
    }
 };


// ======================================================= MAIN =================================================================

int main() {
    try {
        for (int n = 2; n <= 128 * 1024; n *= 2) {
            FftTest fftTest(n);

            fftTest.runTests();
        }

        cout << "All tests passed OK" << endl;
    }
    catch (std::exception& e) {
        cout << e.what() << endl;
    }

    return 0;
}

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

#include <c4/lock_free_queue.hpp>
#include <c4/parallel.hpp>

using namespace std;
using namespace c4;

lock_free_queue<uint64_t, 1 << 20> q;

int main(int argc, char* argv[]) {
    try {
        const int readStep = 10;
        const int writeStep = 1000;
        const uint64_t writeIterations = 1000000000ll;
        const uint64_t readIterations = 2 * writeIterations;

        uint64_t eta = 0;

        auto Producer = [writeStep, writeIterations, &eta] {
            scoped_timer t("Producer");
            for (uint64_t i = 0; i < writeIterations; i++) {
                if (i % writeStep == 0) {
                    uint64_t x = i / writeStep;
                    q.push(x);
                    eta = eta * 13 + x;
                }
            }
        };

        uint64_t test = 0;

        auto Consumer = [readStep, readIterations, &test] {
            scoped_timer t("Consumer");
            for (uint64_t i = 0; i < readIterations; i++) {
                if (i % readStep == 0 && !q.empty()) {
                    test = test * 13 + q.pop_it();
                }
            }
        };

        scoped_timer t("total");
        parallel_invoke(Producer, Consumer);

        PRINT_DEBUG(eta);
        PRINT_DEBUG(test);

        if (eta != test) {
            THROW_EXCEPTION("ETA != TEST");
        }
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}

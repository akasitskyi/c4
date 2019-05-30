//MIT License
//
//Copyright(c) 2019 Alex Kasitskyi
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

#include <chrono>
#include <atomic>
#include <mutex>
#include <iostream>

namespace c4 {
    namespace detail {
        struct timer32 {
            const std::chrono::system_clock::time_point t0;
            timer32() : t0(std::chrono::system_clock::now()) {}

            uint32_t elapsed() const {
                return uint32_t(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - t0).count());
            }
        };
    };

    class progress_indicator {
        const detail::timer32 timer;
        std::atomic<uint32_t> last_ts;

        const uint32_t work_total;
        std::atomic<uint32_t> work_done;
        std::mutex mu;

    public:
        progress_indicator(uint32_t work_total) : last_ts(0), work_total(work_total), work_done(0){}

        void did_some(uint32_t amount) {
            work_done += amount;
            uint32_t t1 = timer.elapsed();

            uint32_t t0 = last_ts.exchange(t1);

            if (t1 > t0) {
                print();
            }
        }

        void print() {
            std::lock_guard<std::mutex> lock(mu);

            std::cout << work_done * 100 / work_total << "% done\r";
        }
    };

}; // namespace c4

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

#include <atomic>
#include <cstdint>

namespace c4 {
    // Lock-free queue for single producer -> single consumer.
    template <typename T, uint32_t capacity>
    class lock_free_queue {
        T buffer[capacity];
        std::atomic<uint32_t> wc = 0;
        std::atomic<uint32_t> rc = 0;

        static inline uint32_t mask(uint32_t i) {
            return i & (capacity - 1);
        }

    public:
        static_assert((capacity & (capacity-1)) == 0, "Capacity must be a power of 2");

        uint32_t size() const {
            return wc - rc;
        }

        bool empty() const {
            return rc == wc;
        }

        void push(const T& t) {
            buffer[mask(wc)] = t;
            ++wc;
        }

        T& front() {
            return buffer[mask(rc)];
        }

        void pop() {
            ++rc;
        }
    };
};

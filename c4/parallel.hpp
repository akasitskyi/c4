//MIT License
//
//Copyright(c) 2018 Alex Kasitskyi
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

#include <thread>
#include <mutex>
#include <future>

#include <queue>
#include <array>
#include <vector>
#include <functional>
#include <type_traits>

#include "range.hpp"

namespace c4 {
    class thread_pool {
        std::mutex mu;
        std::vector<std::thread> workers;
        std::queue<std::packaged_task<void()> > tasks;
        std::condition_variable condition;
        bool stop;

    public:
        thread_pool(unsigned int threads = 0) : stop(false) {
            if (threads == 0)
                threads = std::thread::hardware_concurrency();

            for (unsigned int i = 0; i < threads; i++) {
                workers.emplace_back([this] {
                    for (;;) {
                        std::packaged_task<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(this->mu);
                            condition.wait(lock, [this] { return stop || !tasks.empty(); });
                            if (stop && tasks.empty())
                                return;

                            task = std::move(tasks.front());
                            tasks.pop();
                        }

                        task();
                    }
                });
            }
        }

        template<class F>
        auto enqueue(F f) -> std::future<typename std::result_of<F()>::type> {
            using return_type = typename std::result_of<F()>::type;

            std::packaged_task<return_type()> task(f);
            std::future<return_type> res = task.get_future();

            std::lock_guard<std::mutex> lock(mu);

            tasks.emplace(std::move(task));
            condition.notify_one();

            return res;
        }

        void clear_queue() {
            std::lock_guard<std::mutex> lock(mu);
            while (!tasks.empty())
                tasks.pop();
        }

        ~thread_pool() {
            {
                std::unique_lock<std::mutex> lock(mu);
                stop = true;
            }

            condition.notify_all();

            for (std::thread& worker : workers)
                worker.join();
        }

        template<class iterator, class F>
        void parallel_for(iterator first, iterator last, F f) {
            //TODO: check inception, etc
            //std::thread::id caller_id = std::this_thread::get_id();

            int size = last - first;
            const int min_group_size = size / (int)workers.size();

            std::vector<int> groups(workers.size(), min_group_size);
            size -= (int)groups.size() * min_group_size;

            for (int k = 0; size > 0; k++) {
                groups[k]++;
                size--;
            }

            std::vector<std::future<void>> futures;

            for (int g : groups) {
                iterator group_first = first;
                iterator group_last = first + g;
                first = group_last;

                futures.emplace_back(enqueue([group_first, group_last, f] {
                    std::for_each(group_first, group_last, f);
                }));
            }

            for (auto& f : futures)
                f.wait();
        }

        template<class iterable, class F>
        void parallel_for(iterable c, F f) {
            parallel_for(c.begin(), c.end(), f);
        }
        
        template<class F0, class... F>
        void parallel_invoke(F0&& f0, F&&... f) {
            std::future<void> future = enqueue(f0);

            parallel_invoke(f...);

            future.wait();
        }

        static thread_pool& get_single() {
            static thread_pool tp;
            return tp;
        }

    private:
        
        void parallel_invoke() {}
    };

    template<class iterator, class F>
    void parallel_for(iterator first, iterator last, F f) {
        thread_pool::get_single().parallel_for(first, last, f);
    }

    template<class iterable, class F>
    void parallel_for(iterable c, F f) {
        thread_pool::get_single().parallel_for(c, f);
    }

    template<class... F>
    void parallel_invoke(F&&... f) {
        thread_pool::get_single().parallel_invoke(f...);
    }
};

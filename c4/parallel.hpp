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

#include <mutex>
#include <thread>
#include <future>
#include <cstdlib>

#include <queue>
#include <array>
#include <vector>
#include <algorithm>
#include <functional>
#include <type_traits>

#include "range.hpp"

namespace c4 {
    class thread_pool {
        std::mutex mu;
        std::vector<std::thread> workers;
        std::queue<std::function<void()> > tasks;
        std::condition_variable condition;
        bool stop;

    public:
        thread_pool(unsigned int threads = 0) : stop(false) {
            if (threads == 0){
                if (const char* c4_num_threads = std::getenv("C4_NUM_THREADS")) {
                    threads = std::strtol(c4_num_threads, nullptr, 10);
                }

                if (threads == 0) {
                    threads = std::thread::hardware_concurrency();
                }
            }

            for (unsigned int i = 0; i < threads; i++) {
                workers.emplace_back([this] {
                    for (;;) {
                        std::function<void()> task;
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

        int get_num_threads() const {
            return (int)workers.size();
        }

        int get_thread_index() const {
            std::thread::id caller_id = std::this_thread::get_id();

            for (int i : range(workers))
                if (workers[i].get_id() == caller_id)
                    return i;

            return -1;
        }

        template<class F>
        auto enqueue(F&& f) -> std::future<typename std::result_of<F()>::type> {
            using return_type = typename std::result_of<F()>::type;

            auto task = std::make_shared<std::packaged_task<return_type()>>((f));

            std::lock_guard<std::mutex> lock(mu);

            tasks.emplace([task]() { (*task)(); });
            condition.notify_one();

            return task->get_future();
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

    private:

        std::vector<size_t> init_groups(ptrdiff_t size, size_t grain_size) {
            assert(size >= 0);
            assert(grain_size > 0);

            std::vector<size_t> groups(size / grain_size, grain_size);
            size -= groups.size() * grain_size;

            for (int k = 0; size > 0; k++) {
                groups[k]++;
                size--;
            }

            return groups;
        }
    
    public:

        template<class iterator, class F>
        void parallel_for(iterator first, iterator last, size_t grain_size, F f) {
            std::vector<size_t> groups = init_groups(last - first, grain_size);
            std::vector<std::future<void>> futures;

            for (size_t g : groups) {
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

        template<class iterator, class T, class Reduction, class F>
        T parallel_reduce(iterator first, iterator last, size_t grain_size, T init, Reduction reduction, F f) {
            std::vector<size_t> groups = init_groups(last - first, grain_size);
            std::vector<std::future<T>> futures;

            for (size_t g : groups) {
                iterator group_first = first;
                iterator group_last = first + g;
                first = group_last;

                futures.emplace_back(enqueue([group_first, group_last, f] {
                    return f(group_first, group_last);
                }));
            }

            for (auto& f : futures)
                init = reduction(init, f.get());

            return init;
        }

        template<class iterator, class F>
        void parallel_for(iterator first, iterator last, F f) {
            assert(first <= last);
            const size_t grain_size = std::max<size_t>((last - first) / get_num_threads(), 1);
            parallel_for(first, last, grain_size, f);
        }

        template<class iterator, class T, class Reduction, class F>
        T parallel_reduce(iterator first, iterator last, T init, Reduction reduction, F f) {
            assert(first <= last);
            const size_t grain_size = std::max<size_t>((last - first) / get_num_threads(), 1);
            return parallel_reduce(first, last, grain_size, init, reduction, f);
        }
        
        template<class T, class Reduction, class F>
        T parallel_reduce(range r, int grain_size, T init, Reduction reduction, F f) {
            return parallel_reduce(r.begin(), r.end(), grain_size, init, reduction, [&](range::iterator first, range::iterator last) {
                return f(range(first, last));
            });
        }

        template<class iterable, class F>
        void parallel_for(iterable c, size_t grain_size, F f) {
            parallel_for(c.begin(), c.end(), grain_size, f);
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

    template<class T>
    class enumerable_thread_specific : public std::vector<T> {
        thread_pool& tp;
    public:
        enumerable_thread_specific(thread_pool& tp = thread_pool::get_single()) : std::vector<T>(tp.get_num_threads()), tp(tp) {}
        enumerable_thread_specific(const T& init, thread_pool& tp = thread_pool::get_single()) : std::vector<T>(tp.get_num_threads(), init), tp(tp) {}

        T& local() {
            return (*this)[tp.get_thread_index()];
        }
    };

    inline int get_num_threads() {
        return thread_pool::get_single().get_num_threads();
    }

    inline int get_thread_index() {
        return thread_pool::get_single().get_thread_index();
    }

    template<class iterator, class F>
    inline void parallel_for(iterator first, iterator last, size_t grain_size, F f) {
        thread_pool::get_single().parallel_for(first, last, grain_size, f);
    }

    template<class iterator, class F>
    inline void parallel_for(iterator first, iterator last, F f) {
        thread_pool::get_single().parallel_for(first, last, f);
    }

    template<class iterable, class F>
    inline void parallel_for(iterable c, size_t grain_size, F f) {
        thread_pool::get_single().parallel_for(c, grain_size, f);
    }

    template<class iterable, class F>
    inline void parallel_for(iterable c, F f) {
        thread_pool::get_single().parallel_for(c, f);
    }

    template<class iterator, class T, class Reduction, class F>
    inline T parallel_reduce(iterator first, iterator last, size_t grain_size, T init, Reduction reduction, F f) {
        return thread_pool::get_single().parallel_reduce(first, last, grain_size, init, reduction, f);
    }
    
    template<class T, class Reduction, class F>
    inline T parallel_reduce(range r, size_t grain_size, T init, Reduction reduction, F f) {
        return thread_pool::get_single().parallel_reduce(r, grain_size, init, reduction, f);
    }

    template<class iterator, class T, class Reduction, class F>
    inline T parallel_reduce(iterator first, iterator last, T init, Reduction reduction, F f) {
        return thread_pool::get_single().parallel_reduce(first, last, init, reduction, f);
    }

    template<class... F>
    inline void parallel_invoke(F&&... f) {
        thread_pool::get_single().parallel_invoke(f...);
    }
};

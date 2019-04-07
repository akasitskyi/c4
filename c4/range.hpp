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

#include <array>
#include <vector>
#include <string>
#include <cassert>
#include <cstddef>
#include <type_traits>

#include "math.hpp"

namespace c4 {

    template<class C>
    constexpr int isize(const C& c) {
        const auto N = c.size();
        assert(N <= (std::size_t)std::numeric_limits<int>::max());
        return (int)N;
    }

    template<class T, std::size_t N>
    constexpr int isize(const T(&c)[N]) {
        assert(N <= (std::size_t)std::numeric_limits<int>::max());
        return (int)N;
    }

    class range_reverse {
        friend class range;

        int begin_;
        int end_;

        range_reverse(int begin, int end) : begin_(begin), end_(end) {
            assert(begin >= end);
        }

    public:

        class iterator {
            int i;

        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = int;
            using difference_type = int;
            using pointer = int*;
            using reference = int&;

            iterator(int i) : i(i) {}

            iterator& operator++() {
                --i;
                return *this;
            }

            iterator operator++(int) {
                iterator r = *this;
                i--;
                return r;
            }

            int operator-(const iterator& other) const {
                return -(i - other.i);
            }

            iterator operator+(int n) const {
                iterator r = *this;
                r.i -= n;
                return r;
            }

            int operator*() {
                return i;
            }

            bool operator==(iterator other) const {
                return i == other.i;
            }

            bool operator!=(iterator other) const {
                return !(*this == other);
            }

            bool operator<(iterator other) const {
                return i > other.i;
            }

        };

        iterator begin() {
            return iterator(begin_);
        }

        iterator end() {
            return iterator(end_);
        }

        int size() const {
            return begin_ - end_;
        }
    };

    class range {
        int begin_;
        int end_;

    public:

        class iterator {
            int i;

        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = int;
            using difference_type = int;
            using pointer = int*;
            using reference = int&;

            iterator(int i) : i(i) {}

            iterator& operator++() {
                ++i;
                return *this;
            }

            iterator operator++(int) {
                iterator r = *this;
                i++;
                return r;
            }

            ptrdiff_t operator-(const iterator& other) const {
                return ptrdiff_t(i - other.i);
            }

            iterator operator+(ptrdiff_t n) const {
                assert(fits_within<int>(i + n));
                
                return iterator(int(i + n));
            }

            int operator*() {
                return i;
            }

            bool operator==(iterator other) const {
                return i == other.i;
            }

            bool operator!=(iterator other) const {
                return !(*this == other);
            }

            bool operator<(iterator other) const {
                return i < other.i;
            }

            bool operator>(iterator other) const {
                return i > other.i;
            }

            bool operator<=(iterator other) const {
                return i <= other.i;
            }

            bool operator>=(iterator other) const {
                return i >= other.i;
            }
        };

        range(iterator begin, iterator end) : begin_(*begin), end_(*end) {
            assert(begin_ <= end_);
        }

        template<class T1, class T2, class = typename std::enable_if<std::is_integral<T1>::value && std::is_integral<T2>::value>::type>
        range(T1 begin, T2 end) : begin_((int)begin), end_((int)end) {
            assert(fits_within<int>(begin) && fits_within<int>(end));
            assert((int)begin <= (int)end);
        }

        template<class T, class = typename std::enable_if<std::is_integral<T>::value>::type>
        range(T end) : begin_(0), end_((int)end) {
            assert(0 <= end);
            assert((uint64_t)end <= (uint64_t)std::numeric_limits<int>::max());
        }

        template<class T>
        range(const std::vector<T>& v) : begin_(0), end_((int)v.size()) {
            assert(fits_within<int>(v.size()));
        }

        range(const std::string& v) : begin_(0), end_((int)v.size()) {
            assert(fits_within<int>(v.size()));
        }

        template<class T, size_t n>
        range(const std::array<T, n>& v) : begin_(0), end_((int)n) {
            assert(fits_within<int>(n));
        }

        iterator begin() {
            return iterator(begin_);
        }

        iterator end() {
            return iterator(end_);
        }

        int size() const {
            return end_ - begin_;
        }

        range_reverse reverse() const {
            return { end_ - 1, begin_ - 1 };
        }
    };
};

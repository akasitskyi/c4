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
#include <type_traits>

namespace c4{
    template<int step>
	struct range_proxy {
		int begin_;
		int end_;
		
		class iterator {
			int i;
			
		public:
			iterator(int i) : i(i) {}
			
            iterator& operator++() {
                i += step;
                return *this;
            }

            iterator operator++(int) {
                iterator r = *this;
                i += step;
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
		};
		
        iterator begin() {
            return iterator(begin_);
        }

        iterator end() {
            return iterator(end_);
        }

        range_proxy<-step> reverse() const {
            return { end_ - step, begin_ - step };
        }
	};
	
	template<class T1, class T2>
	typename std::enable_if<std::is_integral<T1>::value && std::is_integral<T2>::value, range_proxy<1> >::type range(T1 begin, T2 end) {
        assert(std::numeric_limits<int>::min() <= begin && end <= std::numeric_limits<int>::max());
        assert((int)begin <= (int)end);
		
		return range_proxy<1>{(int)begin, (int)end};
	}
	
    template<class T>
    typename std::enable_if<std::is_integral<T>::value, range_proxy<1> >::type range(T end) {
        assert(0 <= end);
        assert(end <= std::numeric_limits<int>::max());

        return range_proxy<1>{ 0, (int)end };
    }

    template<class T>
	range_proxy<1> range(const std::vector<T>& v) {
		assert(v.size() <= (size_t)std::numeric_limits<int>::max());

		return range_proxy<1>{0, (int)v.size()};
	}

    template<class T, size_t n>
    range_proxy<1> range(const std::array<T, n>& v) {
        assert(n <= (size_t)std::numeric_limits<int>::max());

        return range_proxy<1>{ 0, (int)n };
    }
};

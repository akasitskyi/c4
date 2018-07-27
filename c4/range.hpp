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

#include <type_traits>
#include <vector>

namespace c4{
	struct range_proxy {
		int begin_;
		int end_;
		
		class iterator {
			int i;
			
		public:
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
	};
	
	template<class T>
	typename std::enable_if<std::is_integral<T>::value, range_proxy>::type range(T begin, T end) {
		assert(begin <= end);
		assert(std::numeric_limits<int>::min() <= begin && end <= std::numeric_limits<int>::max());
		
		return range_proxy{(int)begin, (int)end};
	}
	
    template<class T>
    typename std::enable_if<std::is_integral<T>::value, range_proxy>::type range(T end) {
        assert(0 <= end);
        assert(end <= std::numeric_limits<int>::max());

        return range_proxy{ 0, (int)end };
    }

    template<class T>
	range_proxy range(const std::vector<T>& v) {
		assert(v.size() <= (size_t)std::numeric_limits<int>::max());

		return range_proxy{0, (int)v.size()};
	}
};

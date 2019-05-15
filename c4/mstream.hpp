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

#include <istream>

namespace c4 {
    namespace detail {
        class imembuf : public std::streambuf {
        protected:
            imembuf(const char* ptr, size_t size) {
                char* p = const_cast<char*>(ptr);

                this->setg(p, p, p + size);
            }
        };

        class omembuf : public std::streambuf {
        protected:
            omembuf(char* p, size_t size) {
                this->setp(p, p, p + size);
            }
        };
    };

    class imstream : private detail::imembuf, public std::istream {
    public:
        imstream(const char* ptr, size_t size)
            : imembuf(ptr, size)
            , std::istream(static_cast<std::streambuf*>(this)) {
        }
    };

    class omstream : private detail::omembuf, public std::ostream {
    public:
        omstream(char* ptr, size_t size)
            : omembuf(ptr, size)
            , std::ostream(static_cast<std::streambuf*>(this)) {
        }
    };
};

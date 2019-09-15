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
#include <ostream>
#include <vector>

namespace c4 {
    namespace detail {
        class imembuf : public std::streambuf {
        protected:
            imembuf(const char* ptr, size_t size) {
                char* p = const_cast<char*>(ptr);

                setg(p, p, p + size);
            }
        };

        class omembuf : public std::streambuf {
        protected:
            omembuf(char* p, size_t size) {
                setp(p, p + size);
            }
        };

        class ovecbuf : public std::streambuf {
            static constexpr size_t min_size = 256;

            std::vector<char> v;

        protected:
            ovecbuf() : v(min_size) {
                setp(v.data(), v.data() + v.size());
            }

            virtual std::streambuf::int_type overflow(std::streambuf::int_type ch) {
                if (ch != traits_type::eof()) {
                    size_t size = v.size();

                    v.resize((size + min_size) * 3 / 2);

                    v[size++] = (char)ch;

                    setp(v.data() + size, v.data() + v.size());
                }

                return ch;
            }

            inline const std::vector<char>& get_vector() const {
                return v;
            }

            inline void swap_vector(std::vector<char>& t) {
                v.swap(t);
                setp(v.data(), v.data() + v.size());
            }
        };
    };

    class imstream : private detail::imembuf, public std::istream {
    public:
        imstream(const char* ptr, size_t size)
            : imembuf(ptr, size)
            , std::istream(static_cast<std::streambuf*>(this))
        {}
    };

    class omstream : private detail::omembuf, public std::ostream {
    public:
        omstream(char* ptr, size_t size)
            : omembuf(ptr, size)
            , std::ostream(static_cast<std::streambuf*>(this))
        {}
    };

    class ovstream : private detail::ovecbuf, public std::ostream {
    public:
        ovstream()
            : ovecbuf()
            , std::ostream(static_cast<std::streambuf*>(this))
        {}

        inline const std::vector<char>& get_vector() const{
            return detail::ovecbuf::get_vector();
        }

        inline void swap_vector(std::vector<char>& t) {
            return detail::ovecbuf::swap_vector(t);
        }
    };
};

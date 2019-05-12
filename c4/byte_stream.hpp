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

#include <string>
#include <cassert>
#include <fstream>
#include "exception.hpp"

namespace c4 {
    namespace detail {
        class MemoryByteInputStream {
            const uint8_t* ptr;
            const size_t size;

            size_t i;
        public:
            MemoryByteInputStream(const uint8_t* ptr, const size_t size) : ptr(ptr), size(size), i(0) {}

            inline uint8_t get_byte() {
                assert(i < size);
                return ptr[i++];
            }

            inline void skip(std::streamoff n) {
                assert(i + n <= size);
                i = size_t(i + n);
            }

            inline bool eof() const {
                return i >= size;
            }
        };

        class FileByteInputStream {
            std::ifstream fin;
        public:
            FileByteInputStream(const std::string& filename) : fin(filename, std::fstream::binary) {
                if (!fin.good())
                    THROW_EXCEPTION("Can't open file: " + filename);
            }

            inline uint8_t get_byte() {
                uint8_t r;
                fin.read((char*)&r, sizeof(r));
                assert(fin.good());
                return r;
            }

            inline void skip(std::streamoff n) {
                fin.seekg(fin.tellg() + n);
                assert(fin.good());
            }

            inline bool eof() const {
                return fin.eof();
            }
        };
    };

    template<class BaseByteInputStream>
    class byte_input_stream {
        BaseByteInputStream bbs;
    public:
        template<typename...Ts>
        explicit byte_input_stream(Ts... args) : bbs(args...) {}

        byte_input_stream(byte_input_stream<BaseByteInputStream>&) = delete;
        byte_input_stream<BaseByteInputStream>& operator=(byte_input_stream<BaseByteInputStream>&) = delete;

        inline uint8_t get8() {
            return bbs.get_byte();
        }

        inline uint16_t get16le() {
            return uint16_t(get8() | (get8() << 8));
        }

        inline uint16_t get16be() {
            return uint16_t((get8() << 8) | get8());
        }

        inline uint32_t get32le() {
            return uint32_t(get16le() | (get16le() << 16));
        }

        inline uint32_t get32be() {
            return uint32_t((get16be() << 16) | get16be());
        }

        inline void skip(std::streamoff n) {
            bbs.skip(n);
        }

        inline bool eof() const {
            return bbs.eof();
        }
    };

    typedef byte_input_stream<detail::MemoryByteInputStream> memory_byte_input_stream;
    typedef byte_input_stream<detail::FileByteInputStream> file_byte_input_stream;
};

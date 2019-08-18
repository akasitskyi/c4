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

#include <cstdio>
#include <fstream>
#include <string>

namespace c4 {
    class file_ptr {
        FILE* f;
    public:
        file_ptr(const char * filename, const char * mode) {
#ifdef _MSC_VER
            fopen_s(&f, filename, mode);
#else
            f = std::fopen(filename, mode);
#endif
        }

        operator FILE*() {
            return f;
        }

        int close() {
            if (f) {
                int ret = std::fclose(f);
                f = NULL;
                return ret;
            } else {
                return EOF;
            }
        }

        ~file_ptr() {
            if (f) {
                close();
            }
        }
    };

    std::ifstream::pos_type filesize(const std::string& filename) {
        std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
        return in.tellg();
    }
};

int fclose(c4::file_ptr& fp) = delete;

namespace std {
    int fclose(c4::file_ptr& fp) = delete;
};

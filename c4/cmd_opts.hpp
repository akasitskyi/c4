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

#include <vector>
#include <c4/range.hpp>
#include <c4/string.hpp>

namespace c4 {
    class cmd_opts {
        std::vector<std::string> args;

    public:

        cmd_opts(int argc, const char** argv) : args(argv, argv + argc) {}

        bool extract(const std::string& option) {
            auto it = std::find(args.begin(), args.end(), "--" + option);
            if (it != args.end()) {
                args.erase(it);
                return true;
            }

            return false;
        }

        template<typename T>
        T extract(const std::string& option, const T& default_value) {
            return extract_by_token("--" + option, default_value);
        }

        template<typename T>
        T extract(const char option, const T& default_value) {
            return extract_by_token(std::string("-") + c, default_value);
        }

        template<typename T>
        bool try_extract(const std::string& option, T& value) {
            return try_extract_by_token("--" + option, value);
        }

        template<typename T>
        bool try_extract(const char option, T& value) {
            return try_extract_by_token(std::string("-") + c, value);
        }

        std::vector<std::string> arguments_left() {
            return args;
        }

    private:
        template<typename T>
        T extract_by_token(const std::string& token, T r) {
            try_extract_by_token(r);

            return r;
        }

        template<typename T>
        bool try_extract_by_token(const std::string& token, T& r) {
            for (auto i : range(args.size() - 1).reverse()) {
                if (args[i] == token && args[i + 1][0] != '-') {
                    r = string_to<T>(args[i + 1]);
                    args.erase(args.begin() + i, args.begin() + i + 2);
                    return true;
                }
            }

            return false;
        }
    };
};

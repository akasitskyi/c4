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

#include <string>
#include <cctype>
#include <algorithm>

namespace c4 {
    inline bool ends_with(const std::string& s, const std::string& t) {
        return s.size() >= t.size() && s.substr(s.size() - t.size(), t.size()) == t;
    }

    inline std::string to_lower(const std::string& s) {
        std::string r;

        std::transform(s.begin(), s.end(), std::back_inserter(r), std::tolower);

        return r;
    }

    template<typename T>
    inline std::string to_string(T q, int w) {
        std::stringstream A;
        if (std::is_integral<T>())
            A << std::setfill('0') << std::setw(w) << q;
        else
            A << std::fixed << std::setprecision(w) << q;
        return A.str();
    }
}; // namespace c4

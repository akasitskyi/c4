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
#include <string>
#include <vector>

#include "range.hpp"
#include "logger.hpp"

namespace c4 {
    struct csv {
        std::vector<std::vector<std::string>> data;

        const char delimiter;
        const char line_separator;

        csv(const char delimiter = ',', const char line_separator = '\n') : delimiter(delimiter), line_separator(line_separator) {}

        void read(std::istream& in) {
            std::string s;
            for (int row = 1; std::getline(in, s); row++) {
                const auto quote_cnt = std::count(s.begin(), s.end(), '"');

                if (quote_cnt % 2) {
                    LOGE << "CSV: skipping row " << row << " '" << s << "'";
                    continue;
                }

                std::vector<char> quoted(s.size(), false);

                if (quote_cnt) {
                    bool quoted_mode = false;

                    for (int i = 0; i < isize(s); i++) {
                        if (s[i] == '"')
                            quoted_mode = !quoted_mode;

                        quoted[i] = quoted_mode;
                    }
                }

                std::vector<std::string> r;
                int prev = 0;
                for (int i : range(s)) {
                    if (!quoted[i] && s[i] == delimiter) {
                        r.push_back(s.substr(prev, i - prev));
                        prev = i + 1;
                    }
                }
                r.push_back(s.substr(prev, isize(s)));

                for (auto& t : r) {
                    if (t.size() >= 2 && t[0] == '"' && t[isize(t) - 1] == '"') {
                        t = t.substr(1, isize(t) - 2);
                    }
                }

                data.push_back(std::move(r));
            }
        }
    };
}; // namespace c4

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

#include <stdexcept>
#include <ostream>

#include "string.hpp"
#include "logger.hpp"

namespace c4 {

    class exception : public std::runtime_error {
    public:
        exception(std::string msg, std::string filename, int line) : runtime_error(msg + " at " + filename + ":" + std::to_string(line)) {}
    };

    struct ReturnCode {
        static constexpr int OK = 0;
        static constexpr int CANNOT_DECODE_INPUT = 1;

        static constexpr int UNKNOWN_ERROR = -1;
    };

    namespace detail {
        template<class F>
        int CallReturnInt(F f, std::true_type) {
            return static_cast<int>(f());
        }

        template<class F>
        int CallReturnInt(F f, std::false_type) {
            f();
            return ReturnCode::OK;
        }
    };

    template<class F>
    int safe_call(const std::string& file, int line, std::string& errorMessage, F f) throw() {
        try {
            return detail::CallReturnInt(f, std::is_convertible<decltype(f()), int>());
        }
        catch (std::exception &e) {
            LOGE << "Error: " << e.what() << " caught at " << __FILE__ << ":" << __LINE__;
            errorMessage = e.what();
            return ReturnCode::UNKNOWN_ERROR;
        }
        catch (...) {
            LOGE << "Unknown error caught at " << __FILE__ << ":" << __LINE__;
            errorMessage = "";
            return ReturnCode::UNKNOWN_ERROR;
        }
    }

    template<class Mutex, class F>
    int sync_safe_call(Mutex& mu, const std::string& file, int line, std::string& errorMessage, F f) throw() {
        std::lock_guard<Mutex> lock(mu);

        return safe_call(file, line, errorMessage, f);
    }

#define THROW_EXCEPTION(MSG) throw c4::exception(MSG, __FILE__, __LINE__)

#define ASSERT_TRUE(C) if( C ) {} else THROW_EXCEPTION("Runtime assertion failed: " #C)
#define ASSERT_EQUAL(A, B) { auto __a = (A); auto __b = (B); if( __a != __b ) THROW_EXCEPTION("Runtime assertion failed: " #A " == " #B ", " + c4::to_string(__a) + " != " + c4::to_string(__b)); }

};

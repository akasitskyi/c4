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
#include <vector>
#include <sstream>
#include <ostream>
#include <chrono>

#ifdef ANDROID
#include <android/log.h>
#else
#include <iostream>
#endif

namespace c4 {
    template<typename T1, typename T2>
    inline std::ostream& operator<<(std::ostream &out, const std::pair<T1, T2> &t) {
        out << "( " << t.first << " , " << t.second << " )";
        return out;
    }

    template<typename T>
    inline std::ostream& operator<<(std::ostream &out, const std::vector<T> &t) {
        out << "{ ";
        for(size_t i = 0; i < t.size(); i++) {
            if(i)
                out << ", ";
            out << t[i];
        }
        out << " }";
        return out;
    }

    enum LogLevel{
        LOG_ERROR,
        LOG_WARN,
        LOG_INFO,
        LOG_DEBUG,
        LOG_VERBOSE
    };

    class Logger {
    public:
        Logger(LogLevel level) : level(level) {}
        
        template<typename T>
        Logger& operator<<(const T& v) {
            ss << v;
            return *this;
        }

        ~Logger(){
            switch(level)
            {
#ifdef ANDROID
            case LOG_ERROR:
                __android_log_print(ANDROID_LOG_ERROR, "c4", "%s", ss.str().c_str());
                break;
            case LOG_WARN:
                __android_log_print(ANDROID_LOG_WARN, "c4", "%s", ss.str().c_str());
                break;
            case LOG_INFO:
                __android_log_print(ANDROID_LOG_INFO, "c4", "%s", ss.str().c_str());
                break;
#ifndef C4_DEBUG_OUTPUT_DISABLED
            case LOG_DEBUG:
                __android_log_print(ANDROID_LOG_DEBUG, "c4", "%s", ss.str().c_str());
                break;
            case LOG_VERBOSE:
                __android_log_print(ANDROID_LOG_VERBOSE, "c4", "%s", ss.str().c_str());
                break;
#endif
#else
            case LOG_ERROR :
                std::cerr << "E: " << (ss.str().c_str()) << std::endl;
                break;
            case LOG_WARN :
                std::cerr << "W: " << (ss.str().c_str()) << std::endl;
                break;
            case LOG_INFO :
                std::cout << "I: " << (ss.str().c_str()) << std::endl;
                break;
#ifndef C4_DEBUG_OUTPUT_DISABLED
            case LOG_DEBUG :
                std::cout << "D: " << (ss.str().c_str()) << std::endl;
                break;
            case LOG_VERBOSE :
                std::cout << "V: " << (ss.str().c_str()) << std::endl;
                break;
#endif
#endif
            }
        }
    private:
        std::stringstream ss;
        LogLevel level;
    };

#define LOGE c4::Logger(c4::LOG_ERROR)
#define LOGW c4::Logger(c4::LOG_WARN)
#define LOGI c4::Logger(c4::LOG_INFO)
#define LOGD c4::Logger(c4::LOG_DEBUG)
#define LOGV c4::Logger(c4::LOG_VERBOSE)

#define PRINT_DEBUG(X) LOGD << #X << " = " << (X) << " "

    class time_printer {
        friend class scoped_timer;
#ifndef C4_TIMER_DISABLED
            std::string name;
            c4::LogLevel logLevel;
            std::chrono::high_resolution_clock::duration t;
#endif
        public:
            time_printer(std::string name, c4::LogLevel logLevel = c4::LOG_VERBOSE)
#ifndef C4_TIMER_DISABLED
                : name(name), logLevel(logLevel), t(0)
#endif
            {}

#ifndef C4_TIMER_DISABLED
            ~time_printer() {
                c4::Logger(logLevel) << name << " time: " << std::chrono::duration<double>(t).count() << " seconds.";
            }
#endif
        };

    class scoped_timer {
#ifndef C4_TIMER_DISABLED
        time_printer* tp;
        std::string name;
        c4::LogLevel logLevel;
        std::chrono::high_resolution_clock::time_point t0;
#endif
    public:
        scoped_timer(std::string name, c4::LogLevel logLevel = c4::LOG_VERBOSE)
#ifndef C4_TIMER_DISABLED
            : tp(nullptr), name(name), logLevel(logLevel), t0(std::chrono::high_resolution_clock::now())
#endif
        {}

        scoped_timer(time_printer& tp)
#ifndef C4_TIMER_DISABLED
            : tp(&tp), t0(std::chrono::high_resolution_clock::now())
#endif
        {}

#ifndef C4_TIMER_DISABLED
        double elapsed() {
            return std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count();
        }

        ~scoped_timer() {
            if (tp) {
                tp->t += std::chrono::high_resolution_clock::now() - t0;
            }
            else {
                c4::Logger(logLevel) << name << " time: " << elapsed() << " seconds.";
            }
        }
#endif
    };

    class fps_counter {
        std::chrono::high_resolution_clock::time_point t0;
        int64_t n = -1;
    public:
        float fps() {
            auto now = std::chrono::high_resolution_clock::now();

            if (n == -1) {
                t0 = now;
                n = 0;
                return -1.f;
            }

            n++;

            float t = std::chrono::duration<float>(now - t0).count();

            if (t > 1.f && !(n & (n - 1))) {
                n /= 2;
                t0 += (now - t0) / 2;
                t /= 2;
            }

            return n / t;
        }
    };

}; // namespace c4

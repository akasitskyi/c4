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

#include <array>
#include <cstdint>
#include <cassert>
#include <functional>
#include <type_traits>

#ifdef __ARM_NEON__
#define USE_ARM_NEON
#else
#define USE_SSE
#endif

#if defined(__SSE4_1__) || defined(__AVX__)
#define USE_SSE4_1
#endif

#if defined(__SSE4_2__) || defined(__AVX__)
#define USE_SSE4_2
#endif

#ifdef USE_ARM_NEON
#include <arm_neon.h>
#endif

#ifdef USE_SSE
#include <xmmintrin.h>
#include <emmintrin.h>
#include <pmmintrin.h>
#include <tmmintrin.h>
#endif

#ifdef USE_SSE4_1
#include <smmintrin.h>
#endif

#ifdef USE_SSE4_2
#include <nmmintrin.h>
#endif

#include <stdexcept>
#include <string>

#define NOT_IMPLEMENTED throw std::logic_error("Not implemented at " __FILE__ ":" + std::to_string(__LINE__))

#define __C4_SIMD_WARNING_SLOW "The function may be slow due to the serial implementation"

#ifdef __GNUC__
    #if (__GNUC__ * 100 + __GNUC_MINOR__) <  405
        #define __C4_SIMD_SLOW__ __attribute__((deprecated))
    #else
        #define __C4_SIMD_SLOW__ __attribute__((deprecated(__C4_SIMD_WARNING_SLOW)))
    #endif
#else
    #if defined(_MSC_VER) || defined (__INTEL_COMPILER)  
        #define __C4_SIMD_SLOW__ __declspec(deprecated(__C4_SIMD_WARNING_SLOW))
    #else
        #define __C4_SIMD_SLOW__
    #endif
#endif

#ifdef USE_ARM_NEON
#define __C4_SIMD_SLOW_NEON__ __C4_SIMD_SLOW__
#define __C4_SIMD_SLOW_SSE__
#define __C4_SIMD_SLOW_SSE3__
#else
#define __C4_SIMD_SLOW_NEON__
#define __C4_SIMD_SLOW_SSE__ __C4_SIMD_SLOW__
#ifdef USE_SSE4_1
#define __C4_SIMD_SLOW_SSE3__
#else
#define __C4_SIMD_SLOW_SSE3__ __C4_SIMD_SLOW__
#endif
#endif

namespace c4 {
    namespace simd {
        struct int8x16 {
            typedef int8_t base_t;
            int8x16() = default;
#ifdef USE_ARM_NEON
            int8x16_t v;
            int8x16(int8_t x) : v(vdupq_n_s8(x)) {}
            int8x16(int8x16_t v) : v(v) {}
#else
            __m128i v;
            int8x16(int8_t x) : v(_mm_set1_epi8(x)) {}
            int8x16(__m128i v) : v(v) {}
#endif
        };

        struct uint8x16 {
            typedef uint8_t base_t;
            uint8x16() = default;
#ifdef USE_ARM_NEON
            uint8x16_t v;
            uint8x16(uint8_t x) : v(vdupq_n_u8(x)) {}
            uint8x16(uint8x16_t v) : v(v) {}
#else
            __m128i v;
            uint8x16(uint8_t x) : v(_mm_set1_epi8(x)) {}
            uint8x16(__m128i v) : v(v) {}
#endif
        };

        struct int16x8 {
            typedef int16_t base_t;
            int16x8() = default;
#if defined(USE_ARM_NEON)
            int16x8_t v;
            int16x8(int16_t x) : v(vdupq_n_s16(x)) {}
            int16x8(int16x8_t v) : v(v) {}
#else
            __m128i v;
            int16x8(uint16_t x) : v(_mm_set1_epi16(x)) {}
            int16x8(__m128i v) : v(v) {}
#endif
        };

        struct uint16x8 {
            typedef uint16_t base_t;
            uint16x8() = default;
#if defined(USE_ARM_NEON)
            uint16x8_t v;
            uint16x8(uint16_t x) : v(vdupq_n_u16(x)) {}
            uint16x8(uint16x8_t v) : v(v) {}
#else
            __m128i v;
            uint16x8(uint16_t x) : v(_mm_set1_epi16(x)) {}
            uint16x8(__m128i v) : v(v) {}
#endif
        };

        struct int32x4 {
            typedef int32_t base_t;
            int32x4() = default;
#ifdef USE_ARM_NEON
            int32x4_t v;
            int32x4(int32_t x) : v(vdupq_n_s32(x)) {}
            int32x4(int32x4_t v) : v(v) {}
#else
            __m128i v;
            int32x4(int32_t x) : v(_mm_set1_epi32(x)) {}
            int32x4(__m128i v) : v(v) {}
#endif
        };

        struct uint32x4 {
            typedef uint32_t base_t;
            uint32x4() = default;
#ifdef USE_ARM_NEON
            uint32x4_t v;
            uint32x4(uint32_t x) : v(vdupq_n_u32(x)) {}
            uint32x4(uint32x4_t v) : v(v) {}
#else
            __m128i v;
            uint32x4(uint32_t x) : v(_mm_set1_epi32(x)) {}
            uint32x4(__m128i v) : v(v) {}
#endif
        };

        struct float32x4 {
            typedef float base_t;
            float32x4() = default;
#ifdef USE_ARM_NEON
            float32x4_t v;
            float32x4(float x) : v(vdupq_n_f32(x)) {}
            float32x4(float32x4_t v) : v(v) {}
#else
            __m128 v;
            float32x4(float x) : v(_mm_set1_ps(x)) {}
            float32x4(__m128 v) : v(v) {}
#endif
        };

        namespace traits {
            template<class T>
            struct is_simd : std::false_type {};
            template<>
            struct is_simd<int8x16> : std::true_type {};
            template<>
            struct is_simd<uint8x16> : std::true_type {};
            template<>
            struct is_simd<int16x8> : std::true_type {};
            template<>
            struct is_simd<uint16x8> : std::true_type {};
            template<>
            struct is_simd<int32x4> : std::true_type {};
            template<>
            struct is_simd<uint32x4> : std::true_type {};
            template<>
            struct is_simd<float32x4> : std::true_type {};

            template<class T>
            struct is_integral : std::false_type {};
            template<>
            struct is_integral<int8x16> : std::true_type {};
            template<>
            struct is_integral<uint8x16> : std::true_type {};
            template<>
            struct is_integral<int16x8> : std::true_type {};
            template<>
            struct is_integral<uint16x8> : std::true_type {};
            template<>
            struct is_integral<int32x4> : std::true_type {};
            template<>
            struct is_integral<uint32x4> : std::true_type {};

            template<class T>
            struct is_signed : std::false_type {};
            template<>
            struct is_signed<int8x16> : std::true_type {};
            template<>
            struct is_signed<uint8x16> : std::false_type {};
            template<>
            struct is_signed<int16x8> : std::true_type {};
            template<>
            struct is_signed<uint16x8> : std::false_type {};
            template<>
            struct is_signed<int32x4> : std::true_type {};
            template<>
            struct is_signed<uint32x4> : std::false_type {};
            template<>
            struct is_signed<float32x4> : std::true_type {};

            template<class T>
            using is_unsigned = typename std::conditional<is_signed<T>::value, std::false_type, std::true_type>::type;

            template<class T, class = typename std::enable_if<is_simd<T>::value>::type>
            constexpr size_t length() {
                return sizeof(T) / sizeof(typename T::base_t);
            }
        };

        template<class T, int n, class = typename std::enable_if<traits::is_simd<T>::value>::type>
        struct tuple {
            T val[n];
        };

        typedef tuple<int8x16, 2> int8x16x2;
        typedef tuple<uint8x16, 2> uint8x16x2;
        typedef tuple<int16x8, 2> int16x8x2;
        typedef tuple<uint16x8, 2> uint16x8x2;
        typedef tuple<int32x4, 2> int32x4x2;
        typedef tuple<uint32x4, 2> uint32x4x2;
        typedef tuple<float32x4, 2> float32x4x2;

        typedef tuple<int8x16, 3> int8x16x3;
        typedef tuple<uint8x16, 3> uint8x16x3;
        typedef tuple<int16x8, 3> int16x8x3;
        typedef tuple<uint16x8, 3> uint16x8x3;
        typedef tuple<int32x4, 3> int32x4x3;
        typedef tuple<uint32x4, 3> uint32x4x3;
        typedef tuple<float32x4, 3> float32x4x3;

        typedef tuple<int8x16, 4> int8x16x4;
        typedef tuple<uint8x16, 4> uint8x16x4;
        typedef tuple<int16x8, 4> int16x8x4;
        typedef tuple<uint16x8, 4> uint16x8x4;
        typedef tuple<int32x4, 4> int32x4x4;
        typedef tuple<uint32x4, 4> uint32x4x4;
        typedef tuple<float32x4, 4> float32x4x4;

#ifdef USE_ARM_NEON
        int8x16x2 make_tuple(int8x16x2_t a) {
            return { a.val[0], a.val[1] };
        }

        uint8x16x2 make_tuple(uint8x16x2_t a) {
            return { a.val[0], a.val[1] };
        }
        
        int16x8x2 make_tuple(int16x8x2_t a) {
            return { a.val[0], a.val[1] };
        }

        uint16x8x2 make_tuple(uint16x8x2_t a) {
            return { a.val[0], a.val[1] };
        }

        int32x4x2 make_tuple(int32x4x2_t a) {
            return { a.val[0], a.val[1] };
        }

        uint32x4x2 make_tuple(uint32x4x2_t a) {
            return { a.val[0], a.val[1] };
        }

        float32x4x2 make_tuple(float32x4x2_t a) {
            return { a.val[0], a.val[1] };
        }

        int8x16x3 make_tuple(int8x16x3_t a) {
            return { a.val[0], a.val[1], a.val[2] };
        }

        uint8x16x3 make_tuple(uint8x16x3_t a) {
            return { a.val[0], a.val[1], a.val[2] };
        }

        int16x8x3 make_tuple(int16x8x3_t a) {
            return { a.val[0], a.val[1], a.val[2] };
        }

        uint16x8x3 make_tuple(uint16x8x3_t a) {
            return { a.val[0], a.val[1], a.val[2] };
        }

        int32x4x3 make_tuple(int32x4x3_t a) {
            return { a.val[0], a.val[1], a.val[2] };
        }

        uint32x4x3 make_tuple(uint32x4x3_t a) {
            return { a.val[0], a.val[1], a.val[2] };
        }

        float32x4x3 make_tuple(float32x4x3_t a) {
            return { a.val[0], a.val[1], a.val[2] };
        }

        int8x16x4 make_tuple(int8x16x4_t a) {
            return { a.val[0], a.val[1], a.val[2], a.val[3] };
        }

        uint8x16x4 make_tuple(uint8x16x4_t a) {
            return { a.val[0], a.val[1], a.val[2], a.val[3] };
        }

        int16x8x4 make_tuple(int16x8x4_t a) {
            return { a.val[0], a.val[1], a.val[2], a.val[3] };
        }

        uint16x8x4 make_tuple(uint16x8x4_t a) {
            return { a.val[0], a.val[1], a.val[2], a.val[3] };
        }

        int32x4x4 make_tuple(int32x4x4_t a) {
            return { a.val[0], a.val[1], a.val[2], a.val[3] };
        }

        uint32x4x4 make_tuple(uint32x4x4_t a) {
            return { a.val[0], a.val[1], a.val[2], a.val[3] };
        }

        float32x4x4 make_tuple(float32x4x4_t a) {
            return { a.val[0], a.val[1], a.val[2], a.val[3] };
        }

        int8x16x2_t make_neon(int8x16x2 a) {
            return { a.val[0].v, a.val[1].v };
        }

        uint8x16x2_t make_neon(uint8x16x2 a) {
            return { a.val[0].v, a.val[1].v };
        }

        int16x8x2_t make_neon(int16x8x2 a) {
            return { a.val[0].v, a.val[1].v };
        }

        uint16x8x2_t make_neon(uint16x8x2 a) {
            return { a.val[0].v, a.val[1].v };
        }

        int32x4x2_t make_neon(int32x4x2 a) {
            return { a.val[0].v, a.val[1].v };
        }

        uint32x4x2_t make_neon(uint32x4x2 a) {
            return { a.val[0].v, a.val[1].v };
        }

        float32x4x2_t make_neon(float32x4x2 a) {
            return { a.val[0].v, a.val[1].v };
        }

        int8x16x3_t make_neon(int8x16x3 a) {
            return { a.val[0].v, a.val[1].v, a.val[2].v };
        }

        uint8x16x3_t make_neon(uint8x16x3 a) {
            return { a.val[0].v, a.val[1].v, a.val[2].v };
        }

        int16x8x3_t make_neon(int16x8x3 a) {
            return { a.val[0].v, a.val[1].v, a.val[2].v };
        }

        uint16x8x3_t make_neon(uint16x8x3 a) {
            return { a.val[0].v, a.val[1].v, a.val[2].v };
        }

        int32x4x3_t make_neon(int32x4x3 a) {
            return { a.val[0].v, a.val[1].v, a.val[2].v };
        }

        uint32x4x3_t make_neon(uint32x4x3 a) {
            return { a.val[0].v, a.val[1].v, a.val[2].v };
        }

        float32x4x3_t make_neon(float32x4x3 a) {
            return { a.val[0].v, a.val[1].v, a.val[2].v };
        }

        int8x16x4_t make_neon(int8x16x4 a) {
            return { a.val[0].v, a.val[1].v, a.val[2].v, a.val[3].v };
        }

        uint8x16x4_t make_neon(uint8x16x4 a) {
            return { a.val[0].v, a.val[1].v, a.val[2].v, a.val[3].v };
        }

        int16x8x4_t make_neon(int16x8x4 a) {
            return { a.val[0].v, a.val[1].v, a.val[2].v, a.val[3].v };
        }

        uint16x8x4_t make_neon(uint16x8x4 a) {
            return { a.val[0].v, a.val[1].v, a.val[2].v, a.val[3].v };
        }

        int32x4x4_t make_neon(int32x4x4 a) {
            return { a.val[0].v, a.val[1].v, a.val[2].v, a.val[3].v };
        }

        uint32x4x4_t make_neon(uint32x4x4 a) {
            return { a.val[0].v, a.val[1].v, a.val[2].v, a.val[3].v };
        }

        float32x4x4_t make_neon(float32x4x4 a) {
            return { a.val[0].v, a.val[1].v, a.val[2].v, a.val[3].v };
        }
#endif

        template<class T, class = typename std::enable_if<traits::is_simd<T>::value>::type>
        struct half : public T {
            half(T t) : T(t) {}
        };

        // helpers

#ifdef USE_SSE
        inline __m128i _mm_separate_even_odd_8(__m128i x) {
            static const __m128i mask = _mm_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15);
            return _mm_shuffle_epi8(x, mask);
        }

        inline __m128i _mm_separate_even_odd_16(__m128i x) {
            static const __m128i mask = _mm_setr_epi8(0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15);
            return _mm_shuffle_epi8(x, mask);
        }

        inline __m128i _mm_separate_even_odd_32(__m128i x) {
            return _mm_shuffle_epi32(x, _MM_SHUFFLE(3, 1, 2, 0));
        }

        inline __m128i _mm_swap_lo_hi_64(__m128i x) {
            return _mm_shuffle_epi32(x, _MM_SHUFFLE(1, 0, 3, 2));
        }

        // Replace bit in x with bit in y when matching bit in mask is set
        inline __m128i _mm_blendv_si128(__m128i x, __m128i y, __m128i mask) {
            return _mm_or_si128(_mm_andnot_si128(mask, x), _mm_and_si128(mask, y));
        }

        inline __m128i _mm_set_0xff_si128() {
            __m128i r = _mm_setzero_si128();
            r = _mm_cmpeq_epi8(r, r);
            return r;
        }

        inline __m128i _mm_set_0x80_si128() {
            static const __m128i r = _mm_set1_epi8((int8_t)0x80);
            return r;
        }

        inline __m128i _mm_set_0x8000_si128() {
            return _mm_slli_epi16(_mm_set_0xff_si128(), 15);
        }
        inline __m128i _mm_set_0x80000000_si128() {
            return _mm_slli_epi32(_mm_set_0xff_si128(), 31);
        }

#ifndef USE_SSE4_1
        // Implementing SSE 4.1 instructions using SSSE3 and lower

        inline __m128i _mm_cvtepu8_epi16(__m128i a) {
            return _mm_unpacklo_epi8(a, _mm_setzero_si128());
        }

        inline __m128i _mm_cvtepu16_epi32(__m128i a) {
            return _mm_unpacklo_epi16(a, _mm_setzero_si128());
        }

        inline __m128i _mm_cvtepu32_epi64(__m128i a) {
            return _mm_unpacklo_epi32(a, _mm_setzero_si128());
        }

        inline __m128i _mm_cvtepi8_epi16(__m128i a) {
            __m128i sign = _mm_cmpgt_epi8(_mm_setzero_si128(), a);
            return _mm_unpacklo_epi8(a, sign);
        }

        inline __m128i _mm_cvtepi16_epi32(__m128i a) {
            __m128i sign = _mm_cmpgt_epi16(_mm_setzero_si128(), a);
            return _mm_unpacklo_epi16(a, sign);
        }

        inline __m128i _mm_cvtepi32_epi64(__m128i a) {
            __m128i sign = _mm_cmpgt_epi32(_mm_setzero_si128(), a);
            return _mm_unpacklo_epi32(a, sign);
        }

        inline __m128i _mm_max_epi8(__m128i a, __m128i b) {
            __m128i mask = _mm_cmpgt_epi8(b, a);
            return _mm_blendv_si128(a, b, mask);
        }

        inline __m128i _mm_max_epi32(__m128i a, __m128i b) {
            __m128i mask = _mm_cmpgt_epi32(b, a);
            return _mm_blendv_si128(a, b, mask);
        }

        inline __m128i _mm_max_epu16(__m128i a, __m128i b) {
            __m128i c = _mm_set_0x8000_si128();
            __m128i a_s = _mm_sub_epi16(a, c);
            __m128i b_s = _mm_sub_epi16(b, c);
            __m128i mn = _mm_max_epi16(a_s, b_s);
            return _mm_add_epi16(mn, c);
        }

        inline __m128i _mm_max_epu32(__m128i a, __m128i b) {
            __m128i c = _mm_set_0x80000000_si128();
            __m128i a_s = _mm_sub_epi32(a, c);
            __m128i b_s = _mm_sub_epi32(b, c);
            __m128i mask = _mm_cmpgt_epi32(b_s, a_s);

            return _mm_blendv_si128(a, b, mask);
        }

        inline __m128i _mm_min_epi8(__m128i a, __m128i b) {
            __m128i mask = _mm_cmpgt_epi8(a, b);
            return _mm_blendv_si128(a, b, mask);
        }

        inline __m128i _mm_min_epi32(__m128i a, __m128i b) {
            __m128i mask = _mm_cmpgt_epi32(a, b);
            return _mm_blendv_si128(a, b, mask);
        }

        inline __m128i _mm_min_epu16(__m128i a, __m128i b) {
            __m128i c = _mm_set_0x8000_si128();
            __m128i a_s = _mm_sub_epi16(a, c);
            __m128i b_s = _mm_sub_epi16(b, c);
            __m128i mn = _mm_min_epi16(a_s, b_s);
            return _mm_add_epi16(mn, c);
        }

        inline __m128i _mm_min_epu32(__m128i a, __m128i b) {
            __m128i c = _mm_set_0x80000000_si128();
            __m128i a_s = _mm_sub_epi32(a, c);
            __m128i b_s = _mm_sub_epi32(b, c);
            __m128i mask = _mm_cmpgt_epi32(a_s, b_s);

            return _mm_blendv_si128(a, b, mask);
        }

        inline __m128i _mm_packus_epi32(__m128i a, __m128i b) {
            __m128i zero = _mm_setzero_si128();
            a = _mm_separate_even_odd_16(a);
            b = _mm_separate_even_odd_16(b);
            __m128i hi = _mm_unpackhi_epi64(a, b);                  //hi part of result used for saturation
            __m128i lo = _mm_unpacklo_epi64(a, b);                  //hi part of result used for saturation
            __m128i negative_mask = _mm_cmpgt_epi16(zero, hi);      //if hi < 0 the result should be zero
            __m128i res = _mm_andnot_si128(negative_mask, lo);     //if mask is zero - do nothing, otherwise hi < 0 and the result is 0
            __m128i positive_mask = _mm_cmpgt_epi16(hi, zero);
            return _mm_or_si128(res, positive_mask);                //if hi > 0 we are out of 16bits need to saturaate to 0xffff
        }
            
#endif
#endif

        template<class dst_t, class src_t, class = typename std::enable_if<traits::is_integral<src_t>::value && traits::is_integral<dst_t>::value>::type>
        dst_t reinterpret(src_t a) {
            return dst_t{ (decltype(dst_t::v))a.v };
        }

        template<class dst_t, class src_t, int n, class = typename std::enable_if<traits::is_integral<src_t>::value && traits::is_integral<dst_t>::value>::type>
        tuple<dst_t, n> reinterpret(tuple<src_t, n> a) {
            return *reinterpret_cast<tuple<dst_t, n>*>(&a);
        }

        inline int8x16 reinterpret_signed(uint8x16 a) {
            return reinterpret<int8x16>(a);
        }

        inline int16x8 reinterpret_signed(uint16x8 a) {
            return reinterpret<int16x8>(a);
        }

        inline int32x4 reinterpret_signed(uint32x4 a) {
            return reinterpret<int32x4>(a);
        }

        inline uint8x16 reinterpret_unsigned(int8x16 a) {
            return reinterpret<uint8x16>(a);
        }

        inline uint16x8 reinterpret_unsigned(int16x8 a) {
            return reinterpret<uint16x8>(a);
        }

        inline uint32x4 reinterpret_unsigned(int32x4 a) {
            return reinterpret<uint32x4>(a);
        }

        template<class src_t, int n>
        auto reinterpret_signed(tuple<src_t, n> a) -> tuple<typename std::remove_reference<decltype(reinterpret_signed(src_t()))>::type, n> {
            tuple<typename std::remove_reference<decltype(reinterpret_signed(src_t()))>::type, n> r;
            for (int i = 0; i < n; i++)
                r.val[i] = reinterpret_signed(a.val[i]);
            return r;
        }

        template<class src_t, int n>
        auto reinterpret_unsigned(tuple<src_t, n> a) -> tuple<typename std::remove_reference<decltype(reinterpret_unsigned(src_t()))>::type, n> {
            tuple<typename std::remove_reference<decltype(reinterpret_unsigned(src_t()))>::type, n> r;
            for (int i = 0; i < n; i++)
                r.val[i] = reinterpret_unsigned(a.val[i]);
            return r;
        }

        inline int8x16 cmpgt(int8x16 a, int8x16 b) {
#ifdef USE_ARM_NEON
            return vcgtq_s8(a.v, b.v);
#else
            return _mm_cmpgt_epi8(a.v, b.v);
#endif
        }

        inline uint8x16 cmpgt(uint8x16 a, uint8x16 b) {
#ifdef USE_ARM_NEON
            return vcgtq_u8(a.v, b.v);
#else
            __m128i c0x80 = _mm_set_0x80_si128();
            __m128i as = _mm_sub_epi8(a.v, c0x80);
            __m128i bs = _mm_sub_epi8(b.v, c0x80);

            return _mm_cmpgt_epi8(as, bs);
#endif
        }

        inline int16x8 cmpgt(int16x8 a, int16x8 b) {
#ifdef USE_ARM_NEON
            return vcgtq_s16(a.v, b.v);
#else
            return _mm_cmpgt_epi16(a.v, b.v);
#endif
        }

        inline uint16x8 cmpgt(uint16x8 a, uint16x8 b) {
#ifdef USE_ARM_NEON
            return vcgtq_u16(a.v, b.v);
#else
            __m128i c0x8000 = _mm_set_0x8000_si128();
            __m128i as = _mm_sub_epi16(a.v, c0x8000);
            __m128i bs = _mm_sub_epi16(b.v, c0x8000);

            return _mm_cmpgt_epi16(as, bs);
#endif
        }

        inline int32x4 cmpgt(int32x4 a, int32x4 b) {
#ifdef USE_ARM_NEON
            return vcgtq_s32(a.v, b.v);
#else
            return _mm_cmpgt_epi32(a.v, b.v);
#endif
        }

        inline uint32x4 cmpgt(uint32x4 a, uint32x4 b) {
#ifdef USE_ARM_NEON
            return vcgtq_u32(a.v, b.v);
#else
            __m128i c0x80000000 = _mm_set_0x80000000_si128();
            __m128i as = _mm_sub_epi32(a.v, c0x80000000);
            __m128i bs = _mm_sub_epi32(b.v, c0x80000000);

            return _mm_cmpgt_epi32(as, bs);
#endif
        }

        // Minimum

        inline int8x16 min(int8x16 a, int8x16 b) {
#ifdef USE_ARM_NEON
            return vminq_s8(a.v, b.v);
#else
            return _mm_min_epi8(a.v, b.v);
#endif
        }

        inline uint8x16 min(uint8x16 a, uint8x16 b) {
#ifdef USE_ARM_NEON
            return vminq_u8(a.v, b.v);
#else
            return _mm_min_epu8(a.v, b.v);
#endif
        }

        inline int16x8 min(int16x8 a, int16x8 b) {
#ifdef USE_ARM_NEON
            return vminq_s16(a.v, b.v);
#else
            return _mm_min_epi16(a.v, b.v);
#endif
        }

        inline uint16x8 min(uint16x8 a, uint16x8 b) {
#ifdef USE_ARM_NEON
            return vminq_u16(a.v, b.v);
#else
            return _mm_min_epu16(a.v, b.v);
#endif
        }

        inline int32x4 min(int32x4 a, int32x4 b) {
#ifdef USE_ARM_NEON
            return vminq_s32(a.v, b.v);
#else
            return _mm_min_epi32(a.v, b.v);
#endif
        }

        inline uint32x4 min(uint32x4 a, uint32x4 b) {
#ifdef USE_ARM_NEON
            return vminq_u32(a.v, b.v);
#else
            return _mm_min_epu32(a.v, b.v);
#endif
        }

        inline float32x4 min(float32x4 a, float32x4 b) {
#ifdef USE_ARM_NEON
            return vminq_f32(a.v, b.v);
#else
            return _mm_min_ps(a.v, b.v);
#endif
        }

        // Maximum

        inline int8x16 max(int8x16 a, int8x16 b) {
#ifdef USE_ARM_NEON
            return vmaxq_s8(a.v, b.v);
#else
            return _mm_max_epi8(a.v, b.v);
#endif
        }

        inline uint8x16 max(uint8x16 a, uint8x16 b) {
#ifdef USE_ARM_NEON
            return vmaxq_u8(a.v, b.v);
#else
            return _mm_max_epu8(a.v, b.v);
#endif
        }

        inline int16x8 max(int16x8 a, int16x8 b) {
#ifdef USE_ARM_NEON
            return vmaxq_s16(a.v, b.v);
#else
            return _mm_max_epi16(a.v, b.v);
#endif
        }

        inline uint16x8 max(uint16x8 a, uint16x8 b) {
#ifdef USE_ARM_NEON
            return vmaxq_u16(a.v, b.v);
#else
            return _mm_max_epu16(a.v, b.v);
#endif
        }

        inline int32x4 max(int32x4 a, int32x4 b) {
#ifdef USE_ARM_NEON
            return vmaxq_s32(a.v, b.v);
#else
            return _mm_max_epi32(a.v, b.v);
#endif
        }

        inline uint32x4 max(uint32x4 a, uint32x4 b) {
#ifdef USE_ARM_NEON
            return vmaxq_u32(a.v, b.v);
#else
            return _mm_max_epu32(a.v, b.v);
#endif
        }

        inline float32x4 max(float32x4 a, float32x4 b) {
#ifdef USE_ARM_NEON
            return vmaxq_f32(a.v, b.v);
#else
            return _mm_max_ps(a.v, b.v);
#endif
        }


        // Interleave
        inline int8x16x2 interleave(int8x16x2 p) {
#ifdef USE_ARM_NEON
            return make_tuple(vzipq_s8(p.val[0].v, p.val[1].v));
#else
            return { _mm_unpacklo_epi8(p.val[0].v, p.val[1].v), _mm_unpackhi_epi8(p.val[0].v, p.val[1].v) };
#endif
        }

        inline uint8x16x2 interleave(uint8x16x2 p) {
            return reinterpret_unsigned(interleave(reinterpret_signed(p)));
        }

        inline int16x8x2 interleave(int16x8x2 p) {
#ifdef USE_ARM_NEON
            return make_tuple(vzipq_s16(p.val[0].v, p.val[1].v));
#else
            return { _mm_unpacklo_epi16(p.val[0].v, p.val[1].v), _mm_unpackhi_epi16(p.val[0].v, p.val[1].v) };
#endif
        }

        inline uint16x8x2 interleave(uint16x8x2 p) {
            return reinterpret_unsigned(interleave(reinterpret_signed(p)));
        }

        inline int32x4x2 interleave(int32x4x2 p) {
#ifdef USE_ARM_NEON
            return make_tuple(vzipq_s32(p.val[0].v, p.val[1].v));
#else
            return { _mm_unpacklo_epi32(p.val[0].v, p.val[1].v), _mm_unpackhi_epi32(p.val[0].v, p.val[1].v) };
#endif
        }

        inline uint32x4x2 interleave(uint32x4x2 p) {
            return reinterpret_unsigned(interleave(reinterpret_signed(p)));
        }

        inline float32x4x2 interleave(float32x4x2 p) {
#ifdef USE_ARM_NEON
            return make_tuple(vzipq_f32(p.val[0].v, p.val[1].v));
#else
            return { _mm_unpacklo_ps(p.val[0].v, p.val[1].v), _mm_unpackhi_ps(p.val[0].v, p.val[1].v) };
#endif
        }

        // Deinterleave
        inline int8x16x2 deinterleave(int8x16x2 p) {
#ifdef USE_ARM_NEON
            return make_tuple(vuzpq_s8(p.val[0].v, p.val[1].v));
#else
            __m128i x0 = p.val[0].v;                      // a0, b0, a1, b1, a2, b2, a3, b3, a4, b4, a5, b5, a6, b6, a7, b7
            __m128i x1 = p.val[1].v;                      // a8, b8, a9, b9,a10,b10,a11,b11,a12,b12,a13,b13,a14,b14,a15,b15
            __m128i y0, y1;

            x0 = _mm_separate_even_odd_8(x0);               // a0, a1, a2, a3, a4, a5, a6, a7, b0, b1, b2, b3, b4, b5, b6, b7
            x1 = _mm_separate_even_odd_8(x1);               // a8 ... a15, b8 ... b15

            y0 = _mm_unpacklo_epi64(x0, x1);            // a0, a1, a2, a3, a4, a5, a6, a7, a8, a9,a10,a11,a12,a13,a14,a15
            y1 = _mm_unpackhi_epi64(x0, x1);            // b0, b1, b2, b3, b4, b5, b6, b7, b8, b9,b10,b11,b12,b13,b14,b15

            return { y0, y1 };
#endif
        }

        inline uint8x16x2 deinterleave(uint8x16x2 p) {
            return reinterpret_unsigned(deinterleave(reinterpret_signed(p)));
        }

        inline int16x8x2 deinterleave(int16x8x2 p) {
#ifdef USE_ARM_NEON
            return make_tuple(vuzpq_s16(p.val[0].v, p.val[1].v));
#else
            __m128i x0 = p.val[0].v;                    // a0, b0, a1, b1, a2, b2, a3, b3
            __m128i x1 = p.val[1].v;                    // a4, b4, a5, b5, a6, b6, a7, b7
            __m128i y0, y1;

            x0 = _mm_separate_even_odd_16(x0);              // a0, a1, a2, a3, b0, b1, b2, b3
            x1 = _mm_separate_even_odd_16(x1);              // a4, a5, a6, a7, b4, b5, b6, b7

            y0 = _mm_unpacklo_epi64(x0, x1);            // a0, a1, a2, a3, a4, a5, a6, a7
            y1 = _mm_unpackhi_epi64(x0, x1);            // b0, b1, b2, b3, b4, b5, b6, b7

            return { y0, y1 };
#endif
        }

        inline uint16x8x2 deinterleave(uint16x8x2 p) {
            return reinterpret_unsigned(deinterleave(reinterpret_signed(p)));
        }

        inline int32x4x2 deinterleave(int32x4x2 p) {
#ifdef USE_ARM_NEON
            return make_tuple(vuzpq_s32(p.val[0].v, p.val[1].v));
#else
            __m128i x0 = p.val[0].v;          // a0, b0, a1, b1
            __m128i x1 = p.val[1].v;          // a2, b2, a3, b3
                                                               
            
            x0 = _mm_separate_even_odd_32(x0);    // a0, a1, b0, b1
            x1 = _mm_separate_even_odd_32(x1);    // a2, a3, b2, b3

            return { _mm_unpacklo_epi64(x0, x1), _mm_unpackhi_epi64(x0, x1) };
#endif
        }

        inline uint32x4x2 deinterleave(uint32x4x2 p) {
            return reinterpret_unsigned(deinterleave(reinterpret_signed(p)));
        }

        inline float32x4x2 deinterleave(float32x4x2 p) {
#ifdef USE_ARM_NEON
            return make_tuple(vuzpq_f32(p.val[0].v, p.val[1].v));
#else
            // a0, b0, a1, b1
            // a2, b2, a3, b3
            float32x4x2 ab;
            
            ab.val[0].v = _mm_shuffle_ps(p.val[0].v, p.val[1].v, _MM_SHUFFLE(2, 0, 2, 0));
            ab.val[1].v = _mm_shuffle_ps(p.val[0].v, p.val[1].v, _MM_SHUFFLE(3, 1, 3, 1));
            
            return ab;
#endif
        }

        // Long move, narrow
        inline int16x8x2 long_move(int8x16 a) {
#ifdef USE_ARM_NEON
            int8x8_t hi = vget_high_s8(a.v);
            int8x8_t lo = vget_low_s8(a.v);

            return int16x8x2{ vmovl_s8(hi), vmovl_s8(lo) };
#else
            int8x16 sign = cmpgt({ _mm_setzero_si128() }, a);
            return { _mm_unpacklo_epi8(a.v, sign.v), _mm_unpackhi_epi8(a.v, sign.v) };
#endif
        }

        inline int32x4x2 long_move(int16x8 a) {
#ifdef USE_ARM_NEON
            int16x4_t hi = vget_high_s16(a.v);
            int16x4_t lo = vget_low_s16(a.v);

            return { vmovl_s16(hi), vmovl_s16(lo) };
#else
            int16x8 sign = cmpgt({ _mm_setzero_si128() }, a);
            __m128i r0 = _mm_unpacklo_epi16(a.v, sign.v);
            __m128i r1 = _mm_unpackhi_epi16(a.v, sign.v);
            
            return { r0, r1 };
#endif
        }

        inline uint16x8x2 long_move(uint8x16 a) {
#ifdef USE_ARM_NEON
            uint8x8_t hi = vget_high_u8(a.v);
            uint8x8_t lo = vget_low_u8(a.v);

            return { vmovl_u8(hi), vmovl_u8(lo) };
#else
            return { _mm_unpacklo_epi8(a.v, _mm_setzero_si128()), _mm_unpackhi_epi8(a.v, _mm_setzero_si128()) };
#endif
        }

        inline uint32x4x2 long_move(uint16x8 a) {
#ifdef USE_ARM_NEON
            uint16x4_t hi = vget_high_u16(a.v);
            uint16x4_t lo = vget_low_u16(a.v);

            return { vmovl_u16(hi), vmovl_u16(lo) };
#else
            return { _mm_unpacklo_epi16(a.v, _mm_setzero_si128()), _mm_unpackhi_epi16(a.v, _mm_setzero_si128()) };
#endif
        }

        template<class src_t>
        auto long_move(tuple<src_t, 2> a) -> tuple<typename std::remove_reference<decltype(long_move(src_t()).val[0])>::type, 4> {
            auto r0 = long_move(a.val[0]);
            auto r1 = long_move(a.val[1]);
            return { r0.val[0], r0.val[1], r1.val[0], r1.val[1] };
        }

        inline int16x8 long_move(half<int8x16> a) {
#ifdef USE_ARM_NEON
            return vmovl_s8(vget_low_s8(a.v));
#else
            int8x16 sign = cmpgt(_mm_setzero_si128(), a);
            return _mm_unpacklo_epi8(a.v, sign.v);
#endif
        }

        inline int32x4 long_move(half<int16x8> a) {
#ifdef USE_ARM_NEON
            return vmovl_s16(vget_low_s16(a.v));
#else
            int16x8 sign = cmpgt(_mm_setzero_si128(), a);
            return _mm_unpacklo_epi16(a.v, sign.v);
#endif
        }

        inline uint16x8 long_move(half<uint8x16> a) {
#ifdef USE_ARM_NEON
            return vmovl_u8(vget_low_u8(a.v));
#else
            return _mm_unpacklo_epi8(a.v, _mm_setzero_si128());
#endif
        }

        inline uint32x4 long_move(half<uint16x8> a) {
#ifdef USE_ARM_NEON
            return vmovl_u16(vget_low_u16(a.v));
#else
            return _mm_unpacklo_epi16(a.v, _mm_setzero_si128());
#endif
        }

        // Narrow

        inline half<int8x16> narrow(int16x8 a) {
#ifdef USE_ARM_NEON
            int8x8_t t = vmovn_s16(a.v);
            return int8x16(vcombine_s8(t, t));
#else
            return int8x16(_mm_separate_even_odd_8(a.v));
#endif
        }

        inline half<uint8x16> narrow(uint16x8 a) {
#ifdef USE_ARM_NEON
            uint8x8_t t = vmovn_u16(a.v);
            return uint8x16(vcombine_u8(t, t));
#else
            return uint8x16(_mm_separate_even_odd_8(a.v));
#endif
        }

        inline half<int16x8> narrow(int32x4 a) {
#ifdef USE_ARM_NEON
            int16x4_t t = vmovn_s32(a.v);
            return int16x8(vcombine_s16(t, t));
#else
            return int16x8(_mm_separate_even_odd_16(a.v));
#endif
        }

        inline half<uint16x8> narrow(uint32x4 a) {
#ifdef USE_ARM_NEON
            uint16x4_t t = vmovn_u32(a.v);
            return uint16x8(vcombine_u16(t, t));
#else
            return uint16x8(_mm_separate_even_odd_16(a.v));
#endif
        }

        inline half<int8x16> narrow_saturate(int16x8 a) {
#ifdef USE_ARM_NEON
            int8x8_t t = vqmovn_s16(a.v);
            return int8x16(vcombine_s8(t, t));
#else
            return int8x16(_mm_packs_epi16(a.v, a.v));
#endif
        }

        inline half<uint8x16> narrow_unsigned_saturate(int16x8 a) {
#ifdef USE_ARM_NEON
            uint8x8_t t = vqmovun_s16(a.v);
            return uint8x16(vcombine_u8(t, t));
#else
            return uint8x16(_mm_packus_epi16(a.v, a.v));
#endif
        }

        inline half<int16x8> narrow_saturate(int32x4 a) {
#ifdef USE_ARM_NEON
            int16x4_t t = vqmovn_s32(a.v);
            return int16x8(vcombine_s16(t, t));
#else
            return int16x8(_mm_packs_epi32(a.v, a.v));
#endif
        }

        inline half<uint16x8> narrow_unsigned_saturate(int32x4 a) {
#ifdef USE_ARM_NEON
            uint16x4_t t = vqmovun_s32(a.v);
            return uint16x8(vcombine_u16(t, t));
#else
            return uint16x8(_mm_packus_epi32(a.v, a.v));
#endif
        }

        inline uint8x16 narrow(uint16x8x2 p) {
#ifdef USE_ARM_NEON
            uint8x8_t lo = vmovn_u16(p.val[0].v);
            uint8x8_t hi = vmovn_u16(p.val[1].v);

            return vcombine_u8(lo, hi);
#else
            alignas(16) const static int8_t index[]{ 0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15 };
            const static __m128i i = _mm_load_si128((const __m128i*)index);
            __m128i al = _mm_shuffle_epi8(p.val[0].v, i);
            __m128i bl = _mm_shuffle_epi8(p.val[1].v, i);

            return _mm_unpacklo_epi64(al, bl);
#endif
        }

        inline int8x16 narrow(int16x8x2 p) {
            return reinterpret_signed(narrow(reinterpret_unsigned(p)));
        }

        inline uint16x8 narrow(uint32x4x2 p) {
#ifdef USE_ARM_NEON
            uint16x4_t lo = vmovn_u32(p.val[0].v);
            uint16x4_t hi = vmovn_u32(p.val[1].v);

            return vcombine_u16(lo, hi);
#else
            __m128i al = _mm_separate_even_odd_16(p.val[0].v);
            __m128i bl = _mm_separate_even_odd_16(p.val[1].v);

            return _mm_unpacklo_epi64(al, bl);
#endif
        }

        inline int16x8 narrow(int32x4x2 p) {
            return reinterpret_signed(narrow(reinterpret_unsigned(p)));
        }

        template<class src_t>
        auto narrow(tuple<src_t, 4> v) -> tuple<typename std::remove_reference<decltype(narrow(tuple<src_t, 2>()))>::type, 2> {
            auto r0 = narrow(tuple<src_t, 2>{ v.val[0], v.val[1] });
            auto r1 = narrow(tuple<src_t, 2>{ v.val[2], v.val[3] });
            return { r0, r1 };
        }

        inline int8x16 narrow_saturate(int16x8x2 p) {
#ifdef USE_ARM_NEON
            int8x8_t lo = vqmovn_s16(p.val[0].v);
            int8x8_t hi = vqmovn_s16(p.val[1].v);

            return vcombine_s8(lo, hi);
#else
            return _mm_packs_epi16(p.val[0].v, p.val[1].v);
#endif
        }

        inline uint8x16 narrow_unsigned_saturate(int16x8x2 p) {
#ifdef USE_ARM_NEON
            uint8x8_t lo = vqmovun_s16(p.val[0].v);
            uint8x8_t hi = vqmovun_s16(p.val[1].v);

            return vcombine_u8(lo, hi);
#else
            return _mm_packus_epi16(p.val[0].v, p.val[1].v);
#endif
        }

        inline uint8x16 narrow_saturate(uint16x8x2 p) {
#ifdef USE_ARM_NEON
            uint8x8_t lo = vqmovn_u16(p.val[0].v);
            uint8x8_t hi = vqmovn_u16(p.val[1].v);

            return vcombine_u8(lo, hi);
#else
            return _mm_packs_epi16(p.val[0].v, p.val[1].v);
#endif
        }

        inline int16x8 narrow_saturate(int32x4x2 p) {
#ifdef USE_ARM_NEON
            int16x4_t lo = vqmovn_s32(p.val[0].v);
            int16x4_t hi = vqmovn_s32(p.val[1].v);

            return vcombine_s16(lo, hi);
#else
            return _mm_packs_epi32(p.val[0].v, p.val[1].v);
#endif
        }

        inline uint16x8 narrow_unsigned_saturate(int32x4x2 p) {
#ifdef USE_ARM_NEON
            uint16x4_t lo = vqmovun_s32(p.val[0].v);
            uint16x4_t hi = vqmovun_s32(p.val[1].v);

            return vcombine_u16(lo, hi);
#else
            return _mm_packus_epi32(p.val[0].v, p.val[1].v);
#endif
        }

        inline uint16x8 narrow_saturate(uint32x4x2 p) {
#ifdef USE_ARM_NEON
            uint16x4_t lo = vqmovn_u32(p.val[0].v);
            uint16x4_t hi = vqmovn_u32(p.val[1].v);

            return vcombine_u16(lo, hi);
#else
            return _mm_packs_epi16(p.val[0].v, p.val[1].v);
#endif
        }

        template<class src_t>
        auto narrow_saturate(tuple<src_t, 4> v) -> tuple<typename std::remove_reference<decltype(narrow_saturate(tuple<src_t, 2>()))>::type, 2> {
            auto r0 = narrow_saturate(tuple<src_t, 2>{ v.val[0], v.val[1] });
            auto r1 = narrow_saturate(tuple<src_t, 2>{ v.val[2], v.val[3] });
            return { r0, r1 };
        }

        template<class src_t>
        auto narrow_unsigned_saturate(tuple<src_t, 4> v) -> tuple<typename std::remove_reference<decltype(narrow_unsigned_saturate(tuple<src_t, 2>()))>::type, 2> {
            auto r0 = narrow_unsigned_saturate(tuple<src_t, 2>{ v.val[0], v.val[1] });
            auto r1 = narrow_unsigned_saturate(tuple<src_t, 2>{ v.val[2], v.val[3] });
            return { r0, r1 };
        }

        // Get low and get high
        template<class T, class = typename std::enable_if<traits::is_integral<T>::value>::type >
        half<T> get_low(T a) {
            return a;
        }

        template<class T, class = typename std::enable_if<traits::is_integral<T>::value>::type >
        half<T> get_high(T a) {
#ifdef USE_ARM_NEON
            uint8x16 t = reinterpret<uint8x16>(a);
            t.v = vcombine_u8(vget_high_u8(t.v), vget_low_u8(t.v));
            return reinterpret<T>(t);
#else
            return T(_mm_srli_si128(a.v, 8));
#endif
        }

        // Load

        inline int8x16 load(const int8_t* ptr) {
#ifdef USE_ARM_NEON
            return vld1q_s8(ptr);
#else
            return _mm_loadu_si128((__m128i *)ptr);
#endif
        }

        inline uint8x16 load(const uint8_t* ptr) {
#ifdef USE_ARM_NEON
            return vld1q_u8(ptr);
#else
            return { _mm_loadu_si128((__m128i *)ptr) };
#endif
        }

        inline int16x8 load(const int16_t* ptr) {
#ifdef USE_ARM_NEON
            return vld1q_s16(ptr);
#else
            return { _mm_loadu_si128((__m128i *)ptr) };
#endif
        }

        inline uint16x8 load(const uint16_t* ptr) {
#ifdef USE_ARM_NEON
            return vld1q_u16(ptr);
#else
            return { _mm_loadu_si128((__m128i *)ptr) };
#endif
        }

        inline int32x4 load(const int32_t* ptr) {
#ifdef USE_ARM_NEON
            return vld1q_s32(ptr);
#else
            return { _mm_loadu_si128((__m128i *)ptr) };
#endif
        }

        inline uint32x4 load(const uint32_t* ptr) {
#ifdef USE_ARM_NEON
            return vld1q_u32(ptr);
#else
            return { _mm_loadu_si128((__m128i *)ptr) };
#endif
        }

        inline float32x4 load(const float* ptr) {
#ifdef USE_ARM_NEON
            return vld1q_f32(ptr);
#else
            return { _mm_loadu_ps(ptr) };
#endif
        }

        inline half<int8x16> load_half(const int8_t* ptr) {
#ifdef USE_ARM_NEON
            int8x8_t t = vld1_s8(ptr);
            return int8x16(vcombine_s8(t, t));
#else
            return int8x16(_mm_loadl_epi64((__m128i *)ptr));
#endif
        }

        inline half<uint8x16> load_half(const uint8_t* ptr) {
#ifdef USE_ARM_NEON
            uint8x8_t t = vld1_u8(ptr);
            return uint8x16(vcombine_u8(t, t));
#else
            return uint8x16(_mm_loadl_epi64((__m128i *)ptr));
#endif
        }

        inline half<int16x8> load_half(const int16_t* ptr) {
#ifdef USE_ARM_NEON
            int16x4_t t = vld1_s16(ptr);
            return int16x8(vcombine_s16(t, t));
#else
            return int16x8(_mm_loadl_epi64((__m128i *)ptr));
#endif
        }

        inline half<uint16x8> load_half(const uint16_t* ptr) {
#ifdef USE_ARM_NEON
            uint16x4_t t = vld1_u16(ptr);
            return uint16x8(vcombine_u16(t, t));
#else
            return uint16x8(_mm_loadl_epi64((__m128i *)ptr));
#endif
        }

        inline half<int32x4> load_half(const int32_t* ptr) {
#ifdef USE_ARM_NEON
            int32x2_t t = vld1_s32(ptr);
            return int32x4(vcombine_s32(t, t));
#else
            return int32x4(_mm_loadl_epi64((__m128i *)ptr));
#endif
        }

        inline half<uint32x4> load_half(const uint32_t* ptr) {
#ifdef USE_ARM_NEON
            uint32x2_t t = vld1_u32(ptr);
            return uint32x4(vcombine_u32(t, t));
#else
            return uint32x4(_mm_loadl_epi64((__m128i *)ptr));
#endif
        }

        // Extend half to a full register
        // The high 64 bits are undefined
        template<class T>
        inline T extend(half<T> a) {
            return a.v;
        }

        // Combine two halfs
        template<class T>
        inline T combine(half<T> a, half<T> b) {
#ifdef USE_ARM_NEON
            int8x16 a8 = reinterpret<int8x16_t>(a);
            int8x16 b8 = reinterpret<int8x16_t>(a);
            int8x8_t a8h = vget_low_s8(a8);
            int8x8_t b8h = vget_low_s8(b8);
            return vcombine_s8(a8h, b8h);
#else
            __m128i a0 = _mm_srli_si128(_mm_slli_si128(a.v, 8), 8);
            __m128i b0 = _mm_srli_si128(b.v, 8);
            return _mm_or_si128(a0, b0);
#endif
        }

        // Load tuple of elements
        template<int n, typename T>
        inline auto load_tuple(const T* ptr) -> tuple<typename std::remove_reference<decltype(load(ptr))>::type, n> {
            tuple<typename std::remove_reference<decltype(load(ptr))>::type, n> r;

            for (int i = 0; i < n; i++)
                r.val[i] = load(ptr + i * 16 / sizeof(T));

            return r;
        }

        // Load two elements that are interleaved

        inline int8x16x2 load_2_interleaved(const int8_t* ptr) {
#ifdef USE_ARM_NEON
            return make_tuple(vld2q_s8(ptr));
#else
            return deinterleave({ load(ptr), load(ptr + 16) });
#endif
        }

        inline uint8x16x2 load_2_interleaved(const uint8_t* ptr) {
            return reinterpret_unsigned(load_2_interleaved((int8_t*)ptr));
        }

        inline int16x8x2 load_2_interleaved(const int16_t* ptr) {
#ifdef USE_ARM_NEON
            return make_tuple(vld2q_s16(ptr));
#else
            return deinterleave({ load(ptr), load(ptr + 8) });
#endif
        }

        inline uint16x8x2 load_2_interleaved(const uint16_t* ptr) {
            return reinterpret_unsigned(load_2_interleaved((int16_t*)ptr));
        }

        inline int32x4x2 load_2_interleaved(const int32_t* ptr) {
#ifdef USE_ARM_NEON
            return make_tuple(vld2q_s32(ptr));
#else
            return deinterleave({ load(ptr), load(ptr + 4) });
#endif
        }

        inline uint32x4x2 load_2_interleaved(const uint32_t* ptr) {
            return reinterpret_unsigned(load_2_interleaved((int32_t*)ptr));
        }

        inline float32x4x2 load_2_interleaved(const float* ptr) {
#ifdef USE_ARM_NEON
            return make_tuple(vld2q_f32(ptr));
#else
            return deinterleave({ load(ptr), load(ptr + 4) });
#endif
        }

        // read 16 bytes: a0, b0, ... a7, b7
        // return {A, B}
        inline uint16x8x2 load_2_interleaved_long(const uint8_t* ptr) {
#ifdef USE_ARM_NEON
            uint8x8x2_t u8 = vld2_u8(ptr);
            uint16x8x2 u16;
            u16.val[0] = vmovl_u8(u8.val[0]);
            u16.val[1] = vmovl_u8(u8.val[1]);

            return u16;
#else
            uint8x16 a = load(ptr);                   // a0, b0, a1, b1, a2, b2, a3, b3, a4, b4, a5, b5, a6, b6, a7, b7

            a.v = _mm_separate_even_odd_8(a.v);             // a0, a1, a2, a3, a4, a5, a6, a7, b0, b1, b2, b3, b4, b5, b6, b7

            return long_move(a);
#endif
        }

        // read 24 bytes: a0, b0, c0, ... a7, b7, c7
        // return {A, B, C}
        inline uint16x8x3 load_3_interleaved_long(const uint8_t* ptr) {
#ifdef USE_ARM_NEON
            uint8x8x3_t u8 = vld3_u8(ptr);
            uint16x8x3 u16;
            u16.val[0] = vmovl_u8(u8.val[0]);
            u16.val[1] = vmovl_u8(u8.val[1]);
            u16.val[2] = vmovl_u8(u8.val[2]);
            
            return u16;
#else
            __m128i v0 = _mm_loadu_si128((__m128i*)ptr);        // a0, b0, c0, a1, b1, c1, a2, b2, c2, a3, b3, c3, a4, b4, c4, a5
            __m128i v1 = _mm_loadl_epi64((__m128i*)(ptr + 16)); // b5, c5, a6, b6, c6, a7, b7, c7, ...

            static const __m128i mask_a0 = _mm_setr_epi8(0, -1, 3, -1, 6, -1, 9, -1, 12, -1, 15, -1, -1, -1, -1, -1);
            static const __m128i mask_a1 = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, -1, 5, -1);
            
            __m128i a0 = _mm_shuffle_epi8(v0, mask_a0);         // A0, A1, A2, A3, A4, A5,  0,  0
            __m128i a1 = _mm_shuffle_epi8(v1, mask_a1);         //  0,  0,  0,  0,  0,  0, A6, A7
            __m128i a = _mm_or_si128(a0, a1);                   // A0, A1, A2, A3, A4, A5, A6, A7

            static const __m128i mask_b0 = _mm_setr_epi8(1, -1, 4, -1, 7, -1, 10, -1, 13, -1, -1, -1, -1, -1, -1, -1);
            static const __m128i mask_b1 = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 3, -1, 6, -1);

            __m128i b0 = _mm_shuffle_epi8(v0, mask_b0);         // B0, B1, B2, B3, B4,  0,  0,  0
            __m128i b1 = _mm_shuffle_epi8(v1, mask_b1);         //  0,  0,  0,  0,  0, B5, B6, B7
            __m128i b = _mm_or_si128(b0, b1);                   // B0, B1, B2, B3, B4, B5, B6, B7

            static const __m128i mask_c0 = _mm_setr_epi8(2, -1, 5, -1, 6, -1, 11, -1, 14, -1, -1, -1, -1, -1, -1, -1);
            static const __m128i mask_c1 = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, -1, 4, -1, 7, -1);

            __m128i c0 = _mm_shuffle_epi8(v0, mask_c0);         // C0, C1, C2, C3, C4,  0,  0,  0
            __m128i c1 = _mm_shuffle_epi8(v1, mask_c1);         //  0,  0,  0,  0,  0, C5, C6, C7
            __m128i c = _mm_or_si128(c0, c1);                   // C0, C1, C2, C3, C4, C5, C6, C7

            return { a, b, c };
#endif
        }


        inline int8x16x4 load_4_interleaved(const int8_t* ptr) {
#ifdef USE_ARM_NEON
            return make_tuple(vld4q_s8(ptr));
#else
            __m128i x0, x1, x2, x3;
            __m128i y0, y1, y2, y3;

            x0 = _mm_loadu_si128((__m128i*)ptr + 0);    // a0, b0, c0, d0, a1, b1, c1, d1, a2, b2, c2, d2, a3, b3, c3, d3
            x1 = _mm_loadu_si128((__m128i*)ptr + 1);    // a4, b4, c4, d4, a5, b5, c5, d5, a6, b6, c6, d6, a7, b7, c7, d7
            x2 = _mm_loadu_si128((__m128i*)ptr + 2);    // a8, b8, c8, d8, a9, b9, c9, d9,a10,b10,c10,d10,a11,b11,c11,d11
            x3 = _mm_loadu_si128((__m128i*)ptr + 3);    //a12,b12,c12,d12,a13,b13,c13,d13,a14,b14,c14,d14,a15,b15,c15,d15

            y0 = _mm_unpacklo_epi8(x0, x1);             // a0, a4, b0, b4, c0, c4, d0, d4, a1, a5, b1, b5, c1, c5, d1, d5
            y1 = _mm_unpackhi_epi8(x0, x1);             // a2, a6, b2, b6, c2, c6, d2, d6, a3, a7, b3, b7, c3, c7, d3, d7
            y2 = _mm_unpacklo_epi8(x2, x3);             // a8,a12, b8,b12, c8,c12, d8,d12, a9,a13, b9,b13, c9,c13, d9,d13
            y3 = _mm_unpackhi_epi8(x2, x3);             //a10,a14,b10,b14,c10,c14,d10,d14,a11,a15,b11,b15,c11,c15,d11,d15

            x0 = _mm_unpacklo_epi8(y0, y1);             // a0, a2, a4, a6, b0, b2, b4, b6, c0, c2, c4, c6, d0, d2, d4, d6
            x1 = _mm_unpackhi_epi8(y0, y1);             // a1, a3, a5, a7, b1, b3, b5, b7, c1, c3, c5, c7, d1, c3, c5, d7
            x2 = _mm_unpacklo_epi8(y2, y3);             // a8,a10,a12,a14, b8,b10,b12,b14, c8,c10,c12,c14, d8,d10,d12,d14
            x3 = _mm_unpackhi_epi8(y2, y3);             // a9,a11,a13,a15, b9,b11,b13,b15, c9,c11,c13,c15, d9,d11,d13,d15

            y0 = _mm_unpacklo_epi8(x0, x1);             // a0, a1, a2, a3, a4, a5, a6, a7, b0, b1, b2, b3, b4, b5, b6, b7
            y1 = _mm_unpackhi_epi8(x0, x1);             // c0, c1, c2, c3, c4, c5, c6, c7, d0, d1, d2, d3, d4, d5, d6, d7
            y2 = _mm_unpacklo_epi8(x2, x3);             // a8, a9,a10,a11,a12,a13,a14,a15, b8, b9,b10,b11,b12,b13,b14,b15
            y3 = _mm_unpackhi_epi8(x2, x3);             // c8, c9,c10,c11,c12,c13,c14,c15, d8, d9,d10,d11,d12,d13,d14,d15

            x0 = _mm_unpacklo_epi64(y0, y2);            // a0, a1, a2, a3, a4, a5, a6, a7, a8, a9,a10,a11,a12,a13,a14,a15
            x1 = _mm_unpackhi_epi64(y0, y2);            // b0, b1, b2, b3, b4, b5, b6, b7, b8, b9,b10,b11,b12,b13,b14,b15
            x2 = _mm_unpacklo_epi64(y1, y3);            // c0, c1, c2, c3, c4, c5, c6, c7, c8, c9,c10,c11,c12,c13,c14,c15
            x3 = _mm_unpackhi_epi64(y1, y3);            // d0, d1, d2, d3, d4, d5, d6, d7, d8, d9,d10,d11,d12,d13,d14,d15

            return { x0, x1, x2, x3 };
#endif
        }

        inline uint8x16x4 load_4_interleaved(const uint8_t* ptr) {
            return reinterpret<uint8x16>(load_4_interleaved((int8_t*)ptr));
        }

        inline int16x8x4 load_4_interleaved(const int16_t* ptr) {
#ifdef USE_ARM_NEON
            return make_tuple(vld4q_s16(ptr));
#else
            __m128i x0, x1, x2, x3;
            __m128i y0, y1, y2, y3;

            x0 = _mm_loadu_si128((__m128i*)ptr + 0);    // a0, b0, c0, d0, a1, b1, c1, d1
            x1 = _mm_loadu_si128((__m128i*)ptr + 1);    // a2, b2, c2, d2, a3, b3, c3, d3
            x2 = _mm_loadu_si128((__m128i*)ptr + 2);    // a4, b4, c4, d4, a5, b5, c5, d5
            x3 = _mm_loadu_si128((__m128i*)ptr + 3);    // a6, b6, c6, d6, a7, b7, c7, d7

            y0 = _mm_unpacklo_epi16(x0, x1);            // a0, a2, b0, b2, c0, c2, d0, d2
            y1 = _mm_unpackhi_epi16(x0, x1);            // a1, a3, b1, b3, c1, c3, d1, d3
            y2 = _mm_unpacklo_epi16(x2, x3);            // a4, a6, b4, b6, c4, c6, d4, d6
            y3 = _mm_unpackhi_epi16(x2, x3);            // a5, a7, b5, b7, c5, c7, d5, d7

            x0 = _mm_unpacklo_epi16(y0, y1);            // a0, a1, a2, a3, b0, b1, b2, b3
            x1 = _mm_unpackhi_epi16(y0, y1);            // c0, c1, c2, c3, d0, d1, d2, d3
            x2 = _mm_unpacklo_epi16(y2, y3);            // a4, a5, a6, a7, b4, b5, b6, b7
            x3 = _mm_unpackhi_epi16(y2, y3);            // c4, c5, c6, c7, d4, d5, d6, d7

            y0 = _mm_unpacklo_epi64(x0, x2);            // a0, a1, a2, a3, a4, a5, a6, a7
            y1 = _mm_unpackhi_epi64(x0, x2);            // b0, b1, b2, b3, b4, b5, b6, b7
            y2 = _mm_unpacklo_epi64(x1, x3);            // c0, c1, c2, c3, c4, c5, c6, c7
            y3 = _mm_unpackhi_epi64(x3, x3);            // d0, d1, d2, d3, d4, d5, d6, d7

            return { y0, y1, y2, y3 };
#endif
        }

        inline uint16x8x4 load_4_interleaved(const uint16_t* ptr) {
            return reinterpret<uint16x8>(load_4_interleaved((int16_t*)ptr));
        }

        inline int32x4x4 load_4_interleaved(const int32_t* ptr) {
#ifdef USE_ARM_NEON
            return make_tuple(vld4q_s32(ptr));
#else
            __m128i x0, x1, x2, x3;
            __m128i y0, y1, y2, y3;

            x0 = _mm_loadu_si128((__m128i*)ptr + 0);    // a0, b0, c0, d0
            x1 = _mm_loadu_si128((__m128i*)ptr + 1);    // a1, b1, c1, d1
            x2 = _mm_loadu_si128((__m128i*)ptr + 2);    // a2, b2, c2, d2
            x3 = _mm_loadu_si128((__m128i*)ptr + 3);    // a3, b3, c3, d3

            y0 = _mm_unpacklo_epi32(x0, x1);            // a0, a1, b0, b1
            y1 = _mm_unpackhi_epi32(x0, x1);            // c0, c1, d0, d1
            y2 = _mm_unpacklo_epi32(x2, x3);            // a2, a3, b2, b3
            y3 = _mm_unpackhi_epi32(x2, x3);            // c2, c3, d2, d3

            x0 = _mm_unpacklo_epi64(y0, y2);            // a0, a1, a2, a3
            x1 = _mm_unpackhi_epi64(y0, y2);            // b0, b1, b2, b3
            x2 = _mm_unpacklo_epi64(y1, y3);            // c0, c1, c2, c3
            x3 = _mm_unpackhi_epi64(y1, y3);            // d0, d1, d2, d3

            return { x0, x1, x2, x3 };
#endif
        }

        inline uint32x4x4 load_4_interleaved(const uint32_t* ptr) {
            return reinterpret<uint32x4>(load_4_interleaved((int32_t*)ptr));
        }

        inline float32x4x4 load_4_interleaved(const float* ptr) {
#ifdef USE_ARM_NEON
            return make_tuple(vld4q_f32(ptr));
#else
            __m128 x0, x1, x2, x3;
            __m128 y0, y1, y2, y3;

            x0 = _mm_loadu_ps(ptr);
            x1 = _mm_loadu_ps(ptr + 4);
            x2 = _mm_loadu_ps(ptr + 8);
            x3 = _mm_loadu_ps(ptr + 12);

            y0 = _mm_unpacklo_ps(x0, x1);
            y1 = _mm_unpackhi_ps(x0, x1);
            y2 = _mm_unpacklo_ps(x2, x3);
            y3 = _mm_unpackhi_ps(x2, x3);

            x0 = _mm_movelh_ps(y0, y2);
            x1 = _mm_movehl_ps(y2, y0);
            x2 = _mm_movelh_ps(y1, y3);
            x3 = _mm_movehl_ps(y3, y1);

            return { x0, x1, x2, x3 };
#endif
        }

        // Long load
        inline int16x8 load_long(const int8_t* ptr) {
#ifdef USE_ARM_NEON
            return { vmovl_s8(vld1_s8(ptr)) };
#else
            __m128i a = _mm_loadl_epi64((__m128i*)ptr);
            __m128i sign = _mm_cmpgt_epi8(_mm_setzero_si128(), a);
            return { _mm_unpacklo_epi8(a, sign) };
#endif
        }

        inline uint16x8 load_long(const uint8_t* ptr) {
#ifdef USE_ARM_NEON
            return { vmovl_u8(vld1_u8(ptr)) };
#else
            __m128i a = _mm_loadl_epi64((__m128i*)ptr);
            __m128i r = _mm_unpacklo_epi8(a, _mm_setzero_si128());
            
            return r;
#endif
        }

        inline int32x4 load_long(const int16_t* ptr) {
#ifdef USE_ARM_NEON
            return { vmovl_s16(vld1_s16(ptr)) };
#else
            __m128i a = _mm_loadl_epi64((__m128i*)ptr);
            __m128i sign = _mm_cmpgt_epi16(_mm_setzero_si128(), a);
            return { _mm_unpacklo_epi16(a, sign) };
#endif
        }

        inline uint32x4 load_long(const uint16_t* ptr) {
#ifdef USE_ARM_NEON
            return { vmovl_u16(vld1_u16(ptr)) };
#else
            __m128i a = _mm_loadl_epi64((__m128i*)ptr);
            return { _mm_unpacklo_epi16(a, _mm_setzero_si128()) };
#endif
        }


        // Store

        inline void store(int8_t* ptr, int8x16 a) {
#ifdef USE_ARM_NEON
            vst1q_s8(ptr, a.v);
#else
            _mm_storeu_si128((__m128i *)ptr, a.v);
#endif
        }

        inline void store(uint8_t* ptr, uint8x16 a) {
            store((int8_t*)ptr, reinterpret_signed(a));
        }

        inline void store(int16_t* ptr, int16x8 a) {
#ifdef USE_ARM_NEON
            vst1q_s16(ptr, a.v);
#else
            _mm_storeu_si128((__m128i *)ptr, a.v);
#endif
        }

        inline void store(uint16_t* ptr, uint16x8 a) {
            store((int16_t*)ptr, reinterpret_signed(a));
        }

        inline void store(int32_t* ptr, int32x4 a) {
#ifdef USE_ARM_NEON
            vst1q_s32(ptr, a.v);
#else
            _mm_storeu_si128((__m128i *)ptr, a.v);
#endif
        }

        inline void store(uint32_t* ptr, uint32x4 a) {
            store((int32_t*)ptr, reinterpret_signed(a));
        }

        inline void store(float* ptr, float32x4 a) {
#ifdef USE_ARM_NEON
            vst1q_f32(ptr, a.v);
#else
            _mm_storeu_ps(ptr, a.v);
#endif
        }

        inline void store(int8_t* ptr, half<int8x16> a) {
#ifdef USE_ARM_NEON
            vst1_s8(ptr, vget_low_s8(a.v));
#else
            _mm_storel_epi64((__m128i *)ptr, a.v);
#endif
        }

        inline void store(uint8_t* ptr, half<uint8x16> a) {
#ifdef USE_ARM_NEON
            vst1_u8(ptr, vget_low_u8(a.v));
#else
            _mm_storel_epi64((__m128i *)ptr, a.v);
#endif
        }

        inline void store(int16_t* ptr, half<int16x8> a) {
#ifdef USE_ARM_NEON
            vst1_s16(ptr, vget_low_s16(a.v));
#else
            _mm_storel_epi64((__m128i *)ptr, a.v);
#endif
        }

        inline void store(uint16_t* ptr, half<uint16x8> a) {
#ifdef USE_ARM_NEON
            vst1_u16(ptr, vget_low_u16(a.v));
#else
            _mm_storel_epi64((__m128i *)ptr, a.v);
#endif
        }

        inline void store(int32_t* ptr, half<int32x4> a) {
#ifdef USE_ARM_NEON
            vst1_s32(ptr, vget_low_s32(a.v));
#else
            _mm_storel_epi64((__m128i *)ptr, a.v);
#endif
        }

        inline void store(uint32_t* ptr, half<uint32x4> a) {
#ifdef USE_ARM_NEON
            vst1_u32(ptr, vget_low_u32(a.v));
#else
            _mm_storel_epi64((__m128i *)ptr, a.v);
#endif
        }

        inline void store(float* ptr, half<float32x4> a) {
#ifdef USE_ARM_NEON
            vst1_f32(ptr, vget_low_f32(a.v));
#else
            _mm_storel_pi((__m64*)ptr, a.v);
#endif
        }


        inline void store_2_interleaved(int8_t* ptr, int8x16x2 a) {
#ifdef USE_ARM_NEON
            vst2q_s8(ptr, make_neon(a));
#else
            a = interleave(a);
            store(ptr, a.val[0]);
            store(ptr + 16, a.val[1]);
#endif
        }

        inline void store_2_interleaved(uint8_t* ptr, uint8x16x2 a) {
            store_2_interleaved((int8_t*)ptr, reinterpret_signed(a));
        }

        inline void store_2_interleaved(int16_t* ptr, int16x8x2 a) {
#ifdef USE_ARM_NEON
            vst2q_s16(ptr, make_neon(a));
#else
            a = interleave(a);
            store(ptr, a.val[0]);
            store(ptr + 8, a.val[1]);
#endif
        }

        inline void store_2_interleaved(uint16_t* ptr, uint16x8x2 a) {
            store_2_interleaved((int16_t*)ptr, reinterpret_signed(a));
        }

        inline void store_2_interleaved(int32_t* ptr, int32x4x2 a) {
#ifdef USE_ARM_NEON
            vst2q_s32(ptr, make_neon(a));
#else
            a = interleave(a);
            store(ptr, a.val[0]);
            store(ptr + 4, a.val[1]);
#endif
        }

        inline void store_2_interleaved(uint32_t* ptr, uint32x4x2 a) {
            store_2_interleaved((int32_t*)ptr, reinterpret_signed(a));
        }

        inline void store_2_interleaved(float* ptr, float32x4x2 a) {
#ifdef USE_ARM_NEON
            vst2q_f32(ptr, make_neon(a));
#else
            a = interleave(a);
            store(ptr, a.val[0]);
            store(ptr + 4, a.val[1]);
#endif
        }

        inline void store_2_interleaved_narrow_unsigned_saturate(uint8_t* ptr, int16x8x2 a) {
#ifdef USE_ARM_NEON
            uint8x8x2_t b;
            b.val[0] = vqmovun_s16(a.val[0].v);
            b.val[1] = vqmovun_s16(a.val[1].v);
            vst2_u8(ptr, b);
#else
            uint8x16 u8_0 = narrow_unsigned_saturate(a);
            uint8x16 u8_1 = _mm_swap_lo_hi_64(u8_0.v);

            uint8x16 r = _mm_unpacklo_epi8(u8_0.v, u8_1.v);

            store(ptr, r);
#endif
        }



        inline void store_4_interleaved(int8_t* ptr, int8x16x4 v) {
#ifdef USE_ARM_NEON
            vst4q_s8(ptr, make_neon(v));
#else
            int8x16x2 a = interleave({ v.val[0], v.val[2] });
            int8x16x2 b = interleave({ v.val[1], v.val[3] });

            int8x16x2 c = interleave({ a.val[0], b.val[0] });
            int8x16x2 d = interleave({ a.val[1], b.val[1] });

            store(ptr, c.val[0]);
            store(ptr + 16, c.val[1]);
            store(ptr + 32, d.val[0]);
            store(ptr + 48, d.val[1]);
#endif
        }

        inline void store_4_interleaved(uint8_t* ptr, uint8x16x4 a) {
            store_4_interleaved((int8_t*)ptr, reinterpret<int8x16>(a));
        }

        inline void store_4_interleaved(int16_t* ptr, int16x8x4 v) {
#ifdef USE_ARM_NEON
            vst4q_s16(ptr, make_neon(v));
#else
            int16x8x2 a = interleave({ v.val[0], v.val[2] });
            int16x8x2 b = interleave({ v.val[1], v.val[3] });

            int16x8x2 c = interleave({ a.val[0], b.val[0] });
            int16x8x2 d = interleave({ a.val[1], b.val[1] });

            store(ptr, c.val[0]);
            store(ptr + 8, c.val[1]);
            store(ptr + 16, d.val[0]);
            store(ptr + 24, d.val[1]);
#endif
        }

        inline void store_4_interleaved(uint16_t* ptr, uint16x8x4 a) {
            store_4_interleaved((int16_t*)ptr, reinterpret<int16x8>(a));
        }

        inline void store_4_interleaved(int32_t* ptr, int32x4x4 v) {
#ifdef USE_ARM_NEON
            vst4q_s32(ptr, make_neon(v));
#else
            int32x4x2 a = interleave({ v.val[0], v.val[2] });
            int32x4x2 b = interleave({ v.val[1], v.val[3] });

            int32x4x2 c = interleave({ a.val[0], b.val[0] });
            int32x4x2 d = interleave({ a.val[1], b.val[1] });

            store(ptr, c.val[0]);
            store(ptr + 4, c.val[1]);
            store(ptr + 8, d.val[0]);
            store(ptr + 12, d.val[1]);
#endif
        }

        inline void store_4_interleaved(uint32_t* ptr, uint32x4x4 a) {
            store_4_interleaved((int32_t*)ptr, reinterpret<int32x4>(a));
        }

        inline void store_4_interleaved(float* ptr, float32x4x4 v) {
#ifdef USE_ARM_NEON
            vst4q_f32(ptr, make_neon(v));
#else
            float32x4x2 a = interleave({ v.val[0], v.val[2] });
            float32x4x2 b = interleave({ v.val[1], v.val[3] });

            float32x4x2 c = interleave({ a.val[0], b.val[0] });
            float32x4x2 d = interleave({ a.val[1], b.val[1] });

            store(ptr, c.val[0]);
            store(ptr + 4, c.val[1]);
            store(ptr + 8, d.val[0]);
            store(ptr + 12, d.val[1]);
#endif
        }

        // Store narrow interleaved

#ifdef USE_SSE
        // helper
        inline void store_3div2_interleaved_8bit(__m128i* ptr, __m128i ab, __m128i c0) {
            static const __m128i blend_mask0 = _mm_setr_epi8(0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0);
            static const __m128i blend_mask1 = _mm_setr_epi8(0, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1);

            // ab: a0, a1, a2, a3, a4, a5, a6, a7, b0, b1, b2, b3, b4, b5, b6, b7
            // c0: c0, c1, c2, c3, c4, c5, c6, c7, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0

            // actually a merge of two masks: (-1 means we don't care)
            //  0,  8, -1,  1,  9, -1,  2, 10, -1,  3, 11, -1,  4, 12, -1, 5
            // -1, -1,  0, -1, -1,  1, -1, -1,  2, -1, -1,  3, -1, -1,  4, -1
            static const __m128i shuffle_mask0 = _mm_setr_epi8(0, 8, 0, 1, 9, 1, 2, 10, 2, 3, 11, 3, 4, 12, 4, 5);

            // actually a merge of two masks: (-1 means we don't care)
            // 13, -1,  6, 14, -1, 7 , 15, -1, -1, -1, -1, -1, -1, -1, -1, -1
            // -1,  5, -1, -1,  6, -1, -1,  7, -1, -1, -1, -1, -1, -1, -1, -1
            static const __m128i shuffle_mask1 = _mm_setr_epi8(13, 5, 6, 14, 6, 7, 15, 7, -1, -1, -1, -1, -1, -1, -1, -1);

            __m128i sh0 = _mm_shuffle_epi8(ab, shuffle_mask0);               // a0, b0,  *, a1, b1,  *, a2, b2,  *, a3, b3,  *, a4, b4,  *, a5
            __m128i sh1 = _mm_shuffle_epi8(c0, shuffle_mask0);               //  *,  *, c0,  *,  *, c1,  *,  *, c2,  *,  *, c3,  *,  *, c4,  *

            __m128i abc0 = _mm_blendv_si128(sh0, sh1, blend_mask0);            // a0, b0, c0, a1, b1, c1, a2, b2, c2, a3, b3, c3, a4, b4, c4, a5

            __m128i sh2 = _mm_shuffle_epi8(ab, shuffle_mask1);               // b5,  *, a6, b6,  *, a7, b7,  *, 0 ...
            __m128i sh3 = _mm_shuffle_epi8(c0, shuffle_mask1);               //  *, c5,  *,  *,  c6, *,  *, c7, 0 ...

            __m128i abc1 = _mm_blendv_si128(sh2, sh3, blend_mask1);            // b5, c5, a6, b6, c6, a7, b7, c7, 0 ...

            _mm_storeu_si128(ptr, abc0);
            _mm_storel_epi64(ptr + 1, abc1);
        }
#endif

        inline void store_3_interleaved_narrow_saturate(int8_t* ptr, int16x8x3 v) {
#ifdef USE_ARM_NEON
            int8x8x3_t a;
            a.val[0] = vqmovn_s16(v.val[0].v);
            a.val[1] = vqmovn_s16(v.val[1].v);
            a.val[2] = vqmovn_s16(v.val[2].v);
            vst3_s8(ptr, a);
#else
            __m128i ab = _mm_packs_epi16(v.val[0].v, v.val[1].v);            // a0, a1, a2, a3, a4, a5, a6, a7, b0, b1, b2, b3, b4, b5, b6, b7
            __m128i c0 = _mm_packs_epi16(v.val[2].v, _mm_setzero_si128());   // c0, c1, c2, c3, c4, c5, c6, c7, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0
            
            store_3div2_interleaved_8bit((__m128i*)ptr, ab, c0);
#endif
        }

        inline void store_3_interleaved_narrow_saturate(uint8_t* ptr, int16x8x3 v) {
#ifdef USE_ARM_NEON
            uint8x8x3_t a;
            a.val[0] = vqmovun_s16(v.val[0].v);
            a.val[1] = vqmovun_s16(v.val[1].v);
            a.val[2] = vqmovun_s16(v.val[2].v);
            vst3_u8(ptr, a);
#else
            __m128i ab = _mm_packus_epi16(v.val[0].v, v.val[1].v);            // a0, a1, a2, a3, a4, a5, a6, a7, b0, b1, b2, b3, b4, b5, b6, b7
            __m128i c0 = _mm_packus_epi16(v.val[2].v, _mm_setzero_si128());   // c0, c1, c2, c3, c4, c5, c6, c7, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0

            store_3div2_interleaved_8bit((__m128i*)ptr, ab, c0);
#endif
        }

        inline void store_3_interleaved_narrow_saturate(int16_t* ptr, int32x4x3 v) {
#ifdef USE_ARM_NEON
            int16x4x3_t a;
            a.val[0] = vqmovn_s32(v.val[0].v);
            a.val[1] = vqmovn_s32(v.val[1].v);
            a.val[2] = vqmovn_s32(v.val[2].v);
            vst3_s16(ptr, a);
#else
            NOT_IMPLEMENTED;
#endif
        }

        inline void store_3_interleaved_narrow_saturate(uint16_t* ptr, int32x4x3 v) {
#ifdef USE_ARM_NEON
            uint16x4x3_t a;
            a.val[0] = vqmovun_s32(v.val[0].v);
            a.val[1] = vqmovun_s32(v.val[1].v);
            a.val[2] = vqmovun_s32(v.val[2].v);
            vst3_u16(ptr, a);
#else
            NOT_IMPLEMENTED;
#endif
        }

        inline void store_4_interleaved_narrow_saturate(int8_t* ptr, int16x8x4 v) {
#ifdef USE_ARM_NEON
            int8x8x4_t a;
            a.val[0] = vqmovn_s16(v.val[0].v);
            a.val[1] = vqmovn_s16(v.val[1].v);
            a.val[2] = vqmovn_s16(v.val[2].v);
            a.val[3] = vqmovn_s16(v.val[3].v);
            vst4_s8(ptr, a);
#else
            int8x16x2 p;
            p.val[0] = _mm_packs_epi16(v.val[0].v, v.val[2].v);
            p.val[1] = _mm_packs_epi16(v.val[1].v, v.val[3].v);

            store_2_interleaved(ptr, p);
#endif
        }

        inline void store_4_interleaved_narrow_saturate(uint8_t* ptr, int16x8x4 v) {
#ifdef USE_ARM_NEON
            uint8x8x4_t a;
            a.val[0] = vqmovun_s16(v.val[0].v);
            a.val[1] = vqmovun_s16(v.val[1].v);
            a.val[2] = vqmovun_s16(v.val[2].v);
            a.val[3] = vqmovun_s16(v.val[3].v);
            vst4_u8(ptr, a);
#else
            uint8x16x2 p;
            p.val[0] = _mm_packus_epi16(v.val[0].v, v.val[2].v);
            p.val[1] = _mm_packus_epi16(v.val[1].v, v.val[3].v);

            store_2_interleaved(ptr, p);
#endif
        }

        inline void store_4_interleaved_narrow_saturate(int16_t* ptr, int32x4x4 v) {
#ifdef USE_ARM_NEON
            int16x4x4_t a;
            a.val[0] = vqmovn_s32(v.val[0].v);
            a.val[1] = vqmovn_s32(v.val[1].v);
            a.val[2] = vqmovn_s32(v.val[2].v);
            a.val[3] = vqmovn_s32(v.val[3].v);
            vst4_s16(ptr, a);
#else
            int16x8x2 p;
            p.val[0] = _mm_packs_epi32(v.val[0].v, v.val[2].v);
            p.val[1] = _mm_packs_epi32(v.val[1].v, v.val[3].v);

            store_2_interleaved(ptr, p);
#endif
        }

        inline void store_4_interleaved_narrow_saturate(uint16_t* ptr, int32x4x4 v) {
#ifdef USE_ARM_NEON
            uint16x4x4_t a;
            a.val[0] = vqmovun_s32(v.val[0].v);
            a.val[1] = vqmovun_s32(v.val[1].v);
            a.val[2] = vqmovun_s32(v.val[2].v);
            a.val[3] = vqmovun_s32(v.val[3].v);
            vst4_u16(ptr, a);
#else
            NOT_IMPLEMENTED;
#endif
        }

        // Helper function for cases where we don't have efficient simd implementation
        template<class T, class F>
        T serial(T a, T b, F f) {
            constexpr size_t n = traits::length<T>();

            alignas(16) typename T::base_t ta[n];
            alignas(16) typename T::base_t tb[n];

            store(ta, a);
            store(tb, b);

            for (size_t i = 0; i < n; i++)
                ta[i] = f(ta[i], tb[i]);

            return load(ta);
        }

        // Int <-> Float conversions

        inline float32x4 to_float(int32x4 a) {
#ifdef USE_ARM_NEON
            return vcvtq_f32_s32(a.v);
#else
            return  _mm_cvtepi32_ps(a.v);
#endif
        }

        inline tuple<float32x4, 2> to_float(tuple<int32x4, 2> a) {
            return { to_float(a.val[0]), to_float(a.val[1]) };
        }

        inline tuple<float32x4, 4> to_float(tuple<int32x4, 4> a) {
            return { to_float(a.val[0]), to_float(a.val[1]), to_float(a.val[2]), to_float(a.val[3]) };
        }

        inline int32x4 to_int(float32x4 a) {
#ifdef USE_ARM_NEON
            return vcvtq_s32_f32(a.v);
#else
            return _mm_cvttps_epi32(a.v);
#endif
        }

        inline tuple<int32x4, 2> to_int(tuple<float32x4, 2> a) {
            return { to_int(a.val[0]), to_int(a.val[1]) };
        }

        inline tuple<int32x4, 4> to_int(tuple<float32x4, 4> a) {
            return { to_int(a.val[0]), to_int(a.val[1]), to_int(a.val[2]), to_int(a.val[3]) };
        }


        // Addition

        inline int8x16 add(int8x16 a, int8x16 b) {
#ifdef USE_ARM_NEON
            return vaddq_s8(a.v, b.v);
#else
            return _mm_add_epi8(a.v, b.v);
#endif
        }

        inline uint8x16 add(uint8x16 a, uint8x16 b) {
#ifdef USE_ARM_NEON
            return vaddq_u8(a.v, b.v);
#else
            return _mm_add_epi8(a.v, b.v);
#endif
        }

        inline int16x8 add(int16x8 a, int16x8 b) {
#ifdef USE_ARM_NEON
            return vaddq_s16(a.v, b.v);
#else
            return _mm_add_epi16(a.v, b.v);
#endif
        }

        inline uint16x8 add(uint16x8 a, uint16x8 b) {
#ifdef USE_ARM_NEON
            return vaddq_u16(a.v, b.v);
#else
            return _mm_add_epi16(a.v, b.v);
#endif
        }

        inline int32x4 add(int32x4 a, int32x4 b) {
#ifdef USE_ARM_NEON
            return vaddq_s32(a.v, b.v);
#else
            return _mm_add_epi32(a.v, b.v);
#endif
        }

        inline uint32x4 add(uint32x4 a, uint32x4 b) {
#ifdef USE_ARM_NEON
            return vaddq_u32(a.v, b.v);
#else
            return _mm_add_epi32(a.v, b.v);
#endif
        }

        inline float32x4 add(float32x4 a, float32x4 b) {
#ifdef USE_ARM_NEON
            return vaddq_f32(a.v, b.v);
#else
            return _mm_add_ps(a.v, b.v);
#endif
        }

        // Long pairwise add

        inline int16x8 hadd(int8x16 a) {
#ifdef USE_ARM_NEON
            return vpaddlq_s8(a.v);
#else
            __m128i r0 = _mm_cvtepi8_epi16(a.v);
            __m128i r1 = _mm_cvtepi8_epi16(_mm_swap_lo_hi_64(a.v));

            return _mm_hadd_epi16(r0, r1);
#endif
        }

        inline uint16x8 hadd(uint8x16 a) {
#ifdef USE_ARM_NEON
            return vpaddlq_u8(a.v);
#else
            __m128i r0 = _mm_cvtepu8_epi16(a.v);
            __m128i r1 = _mm_cvtepu8_epi16(_mm_swap_lo_hi_64(a.v));

            return _mm_hadd_epi16(r0, r1);
#endif
        }

        inline int32x4 hadd(int16x8 a) {
#ifdef USE_ARM_NEON
            return vpaddlq_s16(a.v);
#else
            __m128i r0 = _mm_cvtepi16_epi32(a.v);
            __m128i r1 = _mm_cvtepi16_epi32(_mm_swap_lo_hi_64(a.v));

            return _mm_hadd_epi32(r0, r1);
#endif
        }

        inline uint32x4 hadd(uint16x8 a) {
#ifdef USE_ARM_NEON
            return vpaddlq_u16(a.v);
#else
            __m128i r0 = _mm_cvtepu16_epi32(a.v);
            __m128i r1 = _mm_cvtepu16_epi32(_mm_swap_lo_hi_64(a.v));

            return _mm_hadd_epi32(r0, r1);
#endif
        }


        // Subtraction

        inline int8x16 sub(int8x16 a, int8x16 b) {
#ifdef USE_ARM_NEON
            return vsubq_s8(a.v, b.v);
#else
            return _mm_sub_epi8(a.v, b.v);
#endif
        }

        inline uint8x16 sub(uint8x16 a, uint8x16 b) {
#ifdef USE_ARM_NEON
            return vsubq_u8(a.v, b.v);
#else
            return _mm_sub_epi8(a.v, b.v);
#endif
        }

        inline int16x8 sub(int16x8 a, int16x8 b) {
#ifdef USE_ARM_NEON
            return vsubq_s16(a.v, b.v);
#else
            return _mm_sub_epi16(a.v, b.v);
#endif
        }

        inline uint16x8 sub(uint16x8 a, uint16x8 b) {
#ifdef USE_ARM_NEON
            return vsubq_u16(a.v, b.v);
#else
            return _mm_sub_epi16(a.v, b.v);
#endif
        }

        inline int32x4 sub(int32x4 a, int32x4 b) {
#ifdef USE_ARM_NEON
            return vsubq_s32(a.v, b.v);
#else
            return _mm_sub_epi32(a.v, b.v);
#endif
        }

        inline uint32x4 sub(uint32x4 a, uint32x4 b) {
#ifdef USE_ARM_NEON
            return vsubq_u32(a.v, b.v);
#else
            return _mm_sub_epi32(a.v, b.v);
#endif
        }

        inline float32x4 sub(float32x4 a, float32x4 b) {
#ifdef USE_ARM_NEON
            return vsubq_f32(a.v, b.v);
#else
            return _mm_sub_ps(a.v, b.v);
#endif
        }

        inline uint8x16 sub_div2(uint8x16 a, uint8x16 b) {
#ifdef USE_ARM_NEON
            return vhsubq_u8(a.v, b.v);
#else
            return _mm_sub_epi8(a.v, _mm_avg_epu8(a.v, b.v));
#endif
        }

        inline uint16x8 sub_div2(uint16x8 a, uint16x8 b) {
#ifdef USE_ARM_NEON
            return vhsubq_u16(a.v, b.v);
#else
            return _mm_sub_epi16(a.v, _mm_avg_epu16(a.v, b.v));
#endif
        }

        inline uint32x4 sub_div2(uint32x4 a, uint32x4 b) {
#ifdef USE_ARM_NEON
            return vhsubq_u32(a.v, b.v);
#else
            __m128i a2 = _mm_srli_epi32(a.v, 1);
            __m128i b2 = _mm_srli_epi32(b.v, 1);
            __m128i r = _mm_sub_epi32(a2, b2);

            // the only situation r is incorrect, is if last bit of a is 0 and last bit of b is 1
            __m128i b_1 = _mm_andnot_si128(a.v, b.v);
            b_1 = _mm_srli_epi32(_mm_slli_epi32(b_1, 31), 31);

            return _mm_sub_epi32(r, b_1);
#endif
        }

        inline int8x16 sub_div2(int8x16 a, int8x16 b) {
#ifdef USE_ARM_NEON
            return vhsubq_s8(a.v, b.v);
#else
            const int8x16 c0x80 = _mm_set_0x80_si128(); // -128

            uint8x16 au = reinterpret_unsigned(sub(a, c0x80));
            uint8x16 bu = reinterpret_unsigned(sub(b, c0x80));
            
            return reinterpret_signed(sub_div2(au, bu));
#endif
        }

        inline int16x8 sub_div2(int16x8 a, int16x8 b) {
#ifdef USE_ARM_NEON
            return vhsubq_s16(a.v, b.v);
#else
            const int16x8 c0x8000 = _mm_set_0x8000_si128();

            uint16x8 au = reinterpret_unsigned(sub(a, c0x8000));
            uint16x8 bu = reinterpret_unsigned(sub(b, c0x8000));

            return reinterpret_signed(sub_div2(au, bu));
#endif
        }

        inline int32x4 sub_div2(int32x4 a, int32x4 b) {
#ifdef USE_ARM_NEON
            return vhsubq_s32(a.v, b.v);
#else
            __m128i a2 = _mm_srai_epi32(a.v, 1);
            __m128i b2 = _mm_srai_epi32(b.v, 1);
            __m128i r = _mm_sub_epi32(a2, b2);

            // the only situation r is incorrect, is if last bit of a is 0 and last bit of b is 1
            __m128i b_1 = _mm_andnot_si128(a.v, b.v);
            b_1 = _mm_srli_epi32(_mm_slli_epi32(b_1, 31), 31);

            return _mm_sub_epi32(r, b_1);
#endif
        }


        // Shifts

        // Shift left by a constant

        inline int8x16 shift_left(int8x16 a, int8_t n) {
#ifdef USE_ARM_NEON
            return vshlq_n_s8(a.v, n);
#else
            return _mm_and_si128(_mm_set1_epi8(0xff << n), _mm_slli_epi32(a.v, n));
#endif
        }
        
        inline uint8x16 shift_left(uint8x16 a, int8_t n) {
            return reinterpret_unsigned(shift_left(reinterpret_signed(a), n));
        }

        inline int16x8 shift_left(int16x8 a, int8_t n) {
#ifdef USE_ARM_NEON
            return vshlq_n_s16(a.v, n);
#else
            return _mm_slli_epi16(a.v, n);
#endif
        }

        inline uint16x8 shift_left(uint16x8 a, int8_t n) {
            return reinterpret_unsigned(shift_left(reinterpret_signed(a), n));
        }

        inline int32x4 shift_left(int32x4 a, int8_t n) {
#ifdef USE_ARM_NEON
            return vshlq_n_s32(a.v, n);
#else
            return _mm_slli_epi32(a.v, n);
#endif
        }

        inline uint32x4 shift_left(uint32x4 a, int8_t n) {
            return reinterpret_unsigned(shift_left(reinterpret_signed(a), n));
        }

        inline int8x16 shift_left_saturate(int8x16 a, int8_t n) {
#ifdef USE_ARM_NEON
            return vqshlq_n_s8(a.v, n);
#else
            __m128i a16_0 = _mm_cvtepi8_epi16(a.v);
            __m128i a16_1 = _mm_cvtepi8_epi16(_mm_swap_lo_hi_64(a.v));
            __m128i r16_0 = _mm_slli_epi16(a16_0, n);
            __m128i r16_1 = _mm_slli_epi16(a16_1, n);
            return _mm_packs_epi16(r16_0, r16_1);
#endif
        }

        inline uint8x16 shift_left_saturate(uint8x16 a, int8_t n) {
#ifdef USE_ARM_NEON
            return vqshlq_n_u8(a.v, n);
#else
            __m128i a16_0 = _mm_cvtepu8_epi16(a.v);
            __m128i a16_1 = _mm_cvtepu8_epi16(_mm_swap_lo_hi_64(a.v));
            __m128i r16_0 = _mm_slli_epi16(a16_0, n);
            __m128i r16_1 = _mm_slli_epi16(a16_1, n);
            return _mm_packus_epi16(r16_0, r16_1);
#endif
        }

        inline int16x8 shift_left_saturate(int16x8 a, int8_t n) {
#ifdef USE_ARM_NEON
            return vqshlq_n_s16(a.v, n);
#else
            __m128i a16_0 = _mm_cvtepi16_epi32(a.v);
            __m128i a16_1 = _mm_cvtepi16_epi32(_mm_swap_lo_hi_64(a.v));
            __m128i r16_0 = _mm_slli_epi32(a16_0, n);
            __m128i r16_1 = _mm_slli_epi32(a16_1, n);
            return _mm_packs_epi32(r16_0, r16_1);
#endif
        }

        inline uint16x8 shift_left_saturate(uint16x8 a, int8_t n) {
#ifdef USE_ARM_NEON
            return vqshlq_n_u16(a.v, n);
#else
            __m128i a16_0 = _mm_cvtepu16_epi32(a.v);
            __m128i a16_1 = _mm_cvtepu16_epi32(_mm_swap_lo_hi_64(a.v));
            __m128i r16_0 = _mm_slli_epi32(a16_0, n);
            __m128i r16_1 = _mm_slli_epi32(a16_1, n);
            return _mm_packus_epi32(r16_0, r16_1);
#endif
        }


        template<class T>
        inline T scalar_shift_left_signed(T a, T b) {
            return T(b >= 0 ? a << b : a >> -b);
        }

        template<class T>
        inline T scalar_shift_left_unsigned(T a, T b) {
            return T(a << b);
        }

        // Shifts by a packed variable
        __C4_SIMD_SLOW_SSE__ inline int8x16 shift_left(int8x16 a, int8x16 b) {
#ifdef USE_ARM_NEON
            return vshlq_s8(a.v, b.v);
#else
            // TODO: there should be a faster way
            return serial(a, b, scalar_shift_left_signed<int8_t>);
#endif
        }

        __C4_SIMD_SLOW_SSE__ inline uint8x16 shift_left(uint8x16 a, uint8x16 b) {
#ifdef USE_ARM_NEON
            return vshlq_u8(a.v, vreinterpretq_s8_u8(b.v));
#else
            return serial(a, b, scalar_shift_left_unsigned<uint8_t>);
#endif
        }

        __C4_SIMD_SLOW_SSE__ inline int16x8 shift_left(int16x8 a, int16x8 b) {
#ifdef USE_ARM_NEON
            return vshlq_s16(a.v, b.v);
#else
            return serial(a, b, scalar_shift_left_signed<int16_t>);
#endif
        }

        __C4_SIMD_SLOW_SSE__ inline uint16x8 shift_left(uint16x8 a, uint16x8 b) {
#ifdef USE_ARM_NEON
            return vshlq_u16(a.v, vreinterpretq_s16_u16(b.v));
#else
            return serial(a, b, scalar_shift_left_unsigned<uint16_t>);
#endif
        }

        __C4_SIMD_SLOW_SSE__ inline int32x4 shift_left(int32x4 a, int32x4 b) {
#ifdef USE_ARM_NEON
            return vshlq_s32(a.v, b.v);
#else
            return serial(a, b, scalar_shift_left_signed<int32_t>);
#endif
        }

        __C4_SIMD_SLOW_SSE__ inline uint32x4 shift_left(uint32x4 a, uint32x4 b) {
#ifdef USE_ARM_NEON
            return vshlq_u32(a.v, vreinterpretq_s32_u32(b.v));
#else
            return serial(a, b, scalar_shift_left_unsigned<uint32_t>);
#endif
        }

        // Shift right
        // Shifting in sign bits for signed types
        // Shifting in zero bits for unsigned types

        inline int8x16 shift_right(int8x16 a, int8_t n) {
#ifdef USE_ARM_NEON
            return vshrq_n_s8(a.v, n);
#else
            __m128i x = _mm_slli_epi16(_mm_srai_epi16(a.v, n + 8), 8);
            __m128i y = _mm_srli_epi16(_mm_srai_epi16(_mm_slli_epi16(a.v, 8), n), 8);

            return _mm_or_si128(x, y);
#endif
        }

        inline uint8x16 shift_right(uint8x16 a, int8_t n) {
#ifdef USE_ARM_NEON
            return vshrq_n_u8(a.v, n);
#else
            __m128i x = _mm_slli_epi16(_mm_srli_epi16(a.v, n + 8), 8);
            __m128i y = _mm_srli_epi16(_mm_slli_epi16(a.v, 8), n + 8);

            return _mm_or_si128(x, y);
#endif
        }

        inline int16x8 shift_right(int16x8 a, int8_t n) {
#ifdef USE_ARM_NEON
            return vshrq_n_s16(a.v, n);
#else
            return { _mm_srai_epi16(a.v, n) };
#endif
        }

        inline uint16x8 shift_right(uint16x8 a, int8_t n) {
#ifdef USE_ARM_NEON
            return vshrq_n_u16(a.v, n);
#else
            return _mm_srli_epi16(a.v, n);
#endif
        }

        inline int32x4 shift_right(int32x4 a, int8_t n) {
#ifdef USE_ARM_NEON
            return vshrq_n_s32(a.v, n);
#else
            return _mm_srai_epi32(a.v, n);
#endif
        }

        inline uint32x4 shift_right(uint32x4 a, int8_t n) {
#ifdef USE_ARM_NEON
            return vshrq_n_u32(a.v, n);
#else
            return _mm_srli_epi32(a.v, n);
#endif
        }

        // Logical operations

        template<class T, class = typename std::enable_if<traits::is_integral<T>::value>::type>
        inline T bitwise_and(T a, T b) {
            int8x16 a8 = reinterpret<int8x16>(a);
            int8x16 b8 = reinterpret<int8x16>(b);
            int8x16 r8;
#ifdef USE_ARM_NEON
            r8.v = vandq_s8(a8.v, b8.v);
#else
            r8.v = _mm_and_si128(a8.v, b8.v);
#endif
            return reinterpret<T>(r8);
        }

        template<class T, class = typename std::enable_if<traits::is_integral<T>::value>::type>
        inline T bitwise_or(T a, T b) {
            int8x16 a8 = reinterpret<int8x16>(a);
            int8x16 b8 = reinterpret<int8x16>(b);
            int8x16 r8;
#ifdef USE_ARM_NEON
            r8.v = vorrq_s8(a8.v, b8.v);
#else
            r8.v = _mm_or_si128(a8.v, b8.v);
#endif
            return reinterpret<T>(r8);
        }

        template<class T, class = typename std::enable_if<traits::is_integral<T>::value>::type>
        inline T bitwise_not(T a) {
            int8x16 a8 = reinterpret<int8x16>(a);
            int8x16 r8;
#ifdef USE_ARM_NEON
            r8.v = vmvnq_s8(a8.v);
#else
            __m128i c1 = _mm_cmpeq_epi8(a8.v, a8.v);
            return _mm_andnot_si128(a8.v, c1);
#endif
            return reinterpret<T>(r8);
        }

        // a & ~b
        template<class T, class = typename std::enable_if<traits::is_integral<T>::value>::type>
        inline T bitwise_and_not(T a, T b) {
            int8x16 a8 = reinterpret<int8x16>(a);
            int8x16 b8 = reinterpret<int8x16>(b);
            int8x16 r8;
#ifdef USE_ARM_NEON
            r8.v = vbicq_s8(a8.v, b8.v);
#else
            r8.v = _mm_andnot_si128(b8.v, a8.v);
#endif
            return reinterpret<T>(r8);
        }

        template<class T, class = typename std::enable_if<traits::is_integral<T>::value>::type>
        inline T bitwise_xor(T a, T b) {
            int8x16 a8 = reinterpret<int8x16>(a);
            int8x16 b8 = reinterpret<int8x16>(b);
            int8x16 r8;
#ifdef USE_ARM_NEON
            r8.v = veorq_s8(a8.v, b8.v);
#else
            r8.v = _mm_xor_si128(a8.v, b8.v);
#endif
            return reinterpret<T>(r8);
        }


        // Multiplication

        inline int16x8 mul_lo(int16x8 a, int16x8 b) {
#ifdef USE_ARM_NEON
            return vmulq_s16(a.v, b.v);
#else
            return _mm_mullo_epi16(a.v, b.v);
#endif
        }

        inline uint16x8 mul_lo(uint16x8 a, uint16x8 b) {
#ifdef USE_ARM_NEON
            return vmulq_u16(a.v, b.v);
#else
            return _mm_mullo_epi16(a.v, b.v);
#endif
        }

        inline int8x16 mul_lo(int8x16 a, int8x16 b) {
#ifdef USE_ARM_NEON
            return vmulq_s8(a.v, b.v);
#else
            int16x8x2 ap = long_move(a);
            int16x8x2 bp = long_move(b);

            ap.val[0] = mul_lo(ap.val[0], bp.val[0]);
            ap.val[1] = mul_lo(ap.val[1], bp.val[1]);

            return narrow(ap);
#endif
        }

        inline uint8x16 mul_lo(uint8x16 a, uint8x16 b) {
#ifdef USE_ARM_NEON
            return vmulq_u8(a.v, b.v);
#else
            uint16x8x2 ap = long_move(a);
            uint16x8x2 bp = long_move(b);

            ap.val[0] = mul_lo(ap.val[0], bp.val[0]);
            ap.val[1] = mul_lo(ap.val[1], bp.val[1]);

            return narrow(ap);
#endif
        }

        __C4_SIMD_SLOW_SSE3__ inline int32x4 mul_lo(int32x4 a, int32x4 b) {
#ifdef USE_ARM_NEON
            return vmulq_s32(a.v, b.v);
#else
#ifdef USE_SSE4_1
            return _mm_mullo_epi32(a.v, b.v);
#else
            return serial(a, b, std::multiplies<int32_t>());
#endif
#endif
        }

        __C4_SIMD_SLOW_SSE3__ inline uint32x4 mul_lo(uint32x4 a, uint32x4 b) {
#ifdef USE_ARM_NEON
            return vmulq_u32(a.v, b.v);
#else
#ifdef USE_SSE4_1
            return _mm_mullo_epi32(a.v, b.v);
#else
            return serial(a, b, std::multiplies<uint32_t>());
#endif
#endif
        }

        inline int16x8 mul_hi(int16x8 a, int16x8 b) {
#ifdef USE_ARM_NEON
            return vshrq_n_s16(vqdmulhq_s16(a.v, b.v), 1);
#else
            return _mm_mulhi_epi16(a.v, b.v);
#endif
        }

        __C4_SIMD_SLOW_SSE__ inline int32x4 mul_hi(int32x4 a, int32x4 b) {
#ifdef USE_ARM_NEON
            return vshrq_n_s32(vqdmulhq_s32(a.v, b.v), 1);
#else
            return serial(a, b, [](int32_t x, int32_t y) {return int32_t((int64_t(x) * int64_t(y)) >> 32); });
#endif
        }


        inline float32x4 mul(float32x4 a, float32x4 b) {
#ifdef USE_ARM_NEON
            return vmulq_f32(a.v, b.v);
#else
            return _mm_mul_ps(a.v, b.v);
#endif
        }

        // Assumes 32-bit integers in a and b fit into 16-bit signed
        // Same speed on NEON, but faster on SSE compared to 32-bit mul_lo
        inline int32x4 mul_16(int32x4 a, int32x4 b) {
#ifdef USE_ARM_NEON
            return vmulq_s32(a.v, b.v);
#else
            // make sure we have zero in high 16-bit
            __m128i x = _mm_srli_epi32(_mm_slli_epi32(a.v, 16), 16);
            __m128i y = _mm_srli_epi32(_mm_slli_epi32(b.v, 16), 16);

            return _mm_madd_epi16(x, y);
#endif
        }

        // Multiply accumulate: r = s + a * b
        inline int8x16 mul_acc(int8x16 s, int8x16 a, int8x16 b) {
#ifdef USE_ARM_NEON
            return vmlaq_s8(s.v, a.v, b.v);
#else
            return add(s, mul_lo(a, b));
#endif
        }

        inline uint8x16 mul_acc(uint8x16 s, uint8x16 a, uint8x16 b) {
#ifdef USE_ARM_NEON
            return vmlaq_u8(s.v, a.v, b.v);
#else
            return add(s, mul_lo(a, b));
#endif
        }

        inline int16x8 mul_acc(int16x8 s, int16x8 a, int16x8 b) {
#ifdef USE_ARM_NEON
            return vmlaq_s16(s.v, a.v, b.v);
#else
            return add(s, mul_lo(a, b));
#endif
        }

        inline uint16x8 mul_acc(uint16x8 s, uint16x8 a, uint16x8 b) {
#ifdef USE_ARM_NEON
            return vmlaq_u16(s.v, a.v, b.v);
#else
            return add(s, mul_lo(a, b));
#endif
        }

        __C4_SIMD_SLOW_SSE3__ inline int32x4 mul_acc(int32x4 s, int32x4 a, int32x4 b) {
#ifdef USE_ARM_NEON
            return vmlaq_s32(s.v, a.v, b.v);
#else
            return add(s, mul_lo(a, b));
#endif
        }

        // Assumes 32-bit integers in a and b fit into 16-bit signed
        // Same speed on NEON, but faster on SSE compared to 32-bit mul
        inline int32x4 mul_16_acc(int32x4 s, int32x4 a, int32x4 b) {
#ifdef USE_ARM_NEON
            return vmlaq_s32(s.v, a.v, b.v);
#else
            return add(s, mul_16(a, b));
#endif
        }
        
        __C4_SIMD_SLOW_SSE3__ inline uint32x4 mul_acc(uint32x4 s, uint32x4 a, uint32x4 b) {
#ifdef USE_ARM_NEON
            return vmlaq_u32(s.v, a.v, b.v);
#else
            return add(s, mul_lo(a, b));
#endif
        }

        inline float32x4 mul_acc(float32x4 s, float32x4 a, float32x4 b) {
#ifdef USE_ARM_NEON
            return vmlaq_f32(s.v, a.v, b.v);
#else
            return add(s, mul(a, b));
#endif
        }

        // Multiply subtract: r = s - a * b
        inline int8x16 mul_sub(int8x16 s, int8x16 a, int8x16 b) {
#ifdef USE_ARM_NEON
            return vmlsq_s8(s.v, a.v, b.v);
#else
            return sub(s, mul_lo(a, b));
#endif
        }

        inline uint8x16 mul_sub(uint8x16 s, uint8x16 a, uint8x16 b) {
#ifdef USE_ARM_NEON
            return vmlsq_u8(s.v, a.v, b.v);
#else
            return sub(s, mul_lo(a, b));
#endif
        }

        inline int16x8 mul_sub(int16x8 s, int16x8 a, int16x8 b) {
#ifdef USE_ARM_NEON
            return vmlsq_s16(s.v, a.v, b.v);
#else
            return sub(s, mul_lo(a, b));
#endif
        }

        inline uint16x8 mul_sub(uint16x8 s, uint16x8 a, uint16x8 b) {
#ifdef USE_ARM_NEON
            return vmlsq_u16(s.v, a.v, b.v);
#else
            return sub(s, mul_lo(a, b));
#endif
        }

        __C4_SIMD_SLOW_SSE3__ inline int32x4 mul_sub(int32x4 s, int32x4 a, int32x4 b) {
#ifdef USE_ARM_NEON
            return vmlsq_s32(s.v, a.v, b.v);
#else
            return sub(s, mul_lo(a, b));
#endif
        }

        // Assumes 32-bit integers in a and b fit into 16-bit signed
        // Same speed on NEON, but faster on SSE compared to 32-bit mul
        inline int32x4 mul_16_sub(int32x4 s, int32x4 a, int32x4 b) {
#ifdef USE_ARM_NEON
            return vmlaq_s32(s.v, a.v, b.v);
#else
            return sub(s, mul_16(a, b));
#endif
        }

        __C4_SIMD_SLOW_SSE3__ inline uint32x4 mul_sub(uint32x4 s, uint32x4 a, uint32x4 b) {
#ifdef USE_ARM_NEON
            return vmlaq_u32(s.v, a.v, b.v);
#else
            return sub(s, mul_lo(a, b));
#endif
        }

        inline float32x4 mul_sub(float32x4 s, float32x4 a, float32x4 b) {
#ifdef USE_ARM_NEON
            return vmlaq_f32(s.v, a.v, b.v);
#else
            return sub(s, mul(a, b));
#endif
        }

        // Average

        inline uint8x16 avg(uint8x16 a, uint8x16 b) {
#ifdef USE_ARM_NEON
            return vrhaddq_u8(a.v, b.v);
#else
            return _mm_avg_epu8(a.v, b.v);
#endif
        }

        inline uint16x8 avg(uint16x8 a, uint16x8 b) {
#ifdef USE_ARM_NEON
            return vrhaddq_u16(a.v, b.v);
#else
            return _mm_avg_epu16(a.v, b.v);
#endif
        }


        inline int8x16 avg(int8x16 a, int8x16 b) {
#ifdef USE_ARM_NEON
            return vrhaddq_s8(a.v, b.v);
#else
            const int8x16 c0x80 = _mm_set_0x80_si128();
            a = sub(a, c0x80);
            b = sub(b, c0x80);
            int8x16 sum = reinterpret_signed(avg(reinterpret_unsigned(a), reinterpret_unsigned(b)));
            return add(sum, c0x80);
#endif
        }

        inline int16x8 avg(int16x8 a, int16x8 b) {
#ifdef USE_ARM_NEON
            return vrhaddq_s16(a.v, b.v);
#else
            const int16x8 c0x8000 = _mm_set_0x8000_si128();
            a = sub(a, c0x8000);
            b = sub(b, c0x8000);
            int16x8 sum = reinterpret_signed(avg(reinterpret_unsigned(a), reinterpret_unsigned(b)));
            return add(sum, c0x8000);
#endif
        }

        inline int32x4 avg(int32x4 a, int32x4 b) {
#ifdef USE_ARM_NEON
            return vrhaddq_s32(a.v, b.v);
#else
            __m128i a2 = _mm_srai_epi32(a.v, 1);
            __m128i b2 = _mm_srai_epi32(b.v, 1);
            __m128i rounding = _mm_or_si128(a.v, b.v);
            rounding = _mm_slli_epi32(rounding, 31);
            rounding = _mm_srli_epi32(rounding, 31);
            __m128i sum = _mm_add_epi32(a2, b2);
            return _mm_add_epi32(sum, rounding);
#endif
        }

        inline uint32x4 avg(uint32x4 a, uint32x4 b) {
#ifdef USE_ARM_NEON
            return vrhaddq_u32(a.v, b.v);
#else
            uint32x4 a2 = shift_right(a, 1);
            uint32x4 b2 = shift_right(b, 1);
            uint32x4 rounding{ _mm_or_si128(a.v, b.v) };
            rounding = shift_left(rounding, 31);
            rounding = shift_right(rounding, 31);
            uint32x4 sum = add(a2, b2);
            return add(sum, rounding);
#endif
        }

        // Other
        inline float32x4 rsqrt(float32x4 a) {
#ifdef USE_ARM_NEON
            float32x4_t x = a.v;

            float32x4_t r = vrsqrteq_f32(x);
            float32x4_t m = vrsqrtsq_f32(x, vmulq_f32(r, r));
            r = vmulq_f32(m, r);

            return r;
#else
            return _mm_rsqrt_ps(a.v);
#endif
        }
        
        inline float32x4 reciprocal(float32x4 a) {
#ifdef USE_ARM_NEON
            float32x4_t x = a.v;
            float32x4_t r = vrecpeq_f32(x);

            r = vmulq_f32(vrecpsq_f32(x, r), r);

            return r;
#else
            return _mm_rcp_ps(a.v);
#endif
        }

        inline float32x4 sqrt(float32x4 a) {
            return mul(a, rsqrt(a));
        }

        inline float32x4 div(float32x4 a, float32x4 b) {
            return mul(a, reciprocal(b));
        }

        // Table look up

        static const uint8x16 c16(16);

        // r[i] = q[i] < 16 ? t[q[i]] : undefined
        inline uint8x16 look_up(uint8x16 t, uint8x16 q) {
#ifdef USE_ARM_NEON
            uint8x8x2_t vt{ vget_low_u8(t.v), vget_high_u8(t.v) };
            uint8x8x2_t vq{ vget_low_u8(q.v), vget_high_u8(q.v) };

            uint8x8_t low = vtbl2_u8(vt, vq.val[0]);
            uint8x8_t high = vtbl2_u8(vt, vq.val[1]);

            return vcombine_u8(low, high);
#else
            return _mm_shuffle_epi8(t.v, q.v);
#endif
        }

        // r[i] = q[i] < 16 ? t[q[i]] : r[i]
        inline uint8x16 look_up(uint8x16 r, uint8x16 t, uint8x16 q) {
#ifdef USE_ARM_NEON
            uint8x8x2_t vr{ vget_low_u8(r.v), vget_high_u8(r.v) };
            uint8x8x2_t vt{ vget_low_u8(t.v), vget_high_u8(t.v) };
            uint8x8x2_t vq{ vget_low_u8(q.v), vget_high_u8(q.v) };

            uint8x8_t low = vtbx2_u8(vr.val[0], vt, vq.val[0]);
            uint8x8_t high = vtbx2_u8(vr.val[1], vt, vq.val[1]);
            
            return vcombine_u8(low, high);
#else
            __m128i mask = _mm_andnot_si128(_mm_cmplt_epi8(q.v, _mm_setzero_si128()), _mm_cmplt_epi8(q.v, c16.v));

            __m128i sh0 = _mm_shuffle_epi8(t.v, q.v);

            return _mm_blendv_si128(r.v, sh0, mask);
#endif
        }

        // r[i] = q[i] < 16 * n ? t[q[i]] : undefined
        template<int n>
        inline uint8x16 look_up(const std::array<uint8x16, n>& t, uint8x16 q) {
            static_assert(1 <= n && n <= 16, "we can only address up to 256 elements");

            uint8x16 r = look_up(t[0], q);
            for (int i = 1; i < n; i++) {
                q = sub(q, c16);
                r = look_up(r, t[i], q);
            }
            
            return r;
        }

        // Count leading zeros
        inline uint8x16 clz(uint8x16 a) {
#ifdef USE_ARM_NEON
            return vclzq_u8(a.v);
#else
            static const __m128i clz4 = _mm_setr_epi8( 4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 );
            
            __m128i lo_clz = _mm_shuffle_epi8(clz4, a.v);
            a = shift_right(a, 4);
            __m128i hi_clz = _mm_shuffle_epi8(clz4, a.v);
            
            // we need to add lo_clz only if hi is zero
            __m128i mask = _mm_cmpeq_epi8(a.v, _mm_setzero_si128());
            lo_clz = _mm_and_si128(lo_clz, mask);
            
            return _mm_add_epi8(lo_clz, hi_clz);
#endif
        }

        inline int8x16 clz(int8x16 a) {
            return reinterpret_signed(clz(reinterpret_unsigned(a)));
        }

        // Operations on tuples
        template<class T, int n, class = typename std::enable_if<traits::is_simd<T>::value>::type>
        tuple<T, n> binary_operation(tuple<T, n> a, tuple<T, n> b, std::function<T(T, T)> f) {
            tuple<T, n> r;
            
            for (int i = 0; i < n; i++)
                r.val[i] = f(a.val[i], b.val[i]);
            
            return r;
        }

        template<class T, int n, class = typename std::enable_if<traits::is_simd<T>::value>::type>
        tuple<T, n> binary_operation(tuple<T, n> a, T b, std::function<T(T, T)> f) {
            tuple<T, n> r;

            for (int i = 0; i < n; i++)
                r.val[i] = f(a.val[i], b);

            return r;
        }

        template<class T, int n>
        tuple<T, n> add(tuple<T, n> a, tuple<T, n> b) {
            std::function<T(T, T)> f = static_cast<T(*)(T, T)>(add);
            return binary_operation(a, b, f);
        }

        template<class T, int n>
        tuple<T, n> add(tuple<T, n> a, T b) {
            std::function<T(T, T)> f = static_cast<T(*)(T, T)>(add);
            return binary_operation(a, b, f);
        }

        template<class T, int n>
        tuple<T, n> sub(tuple<T, n> a, tuple<T, n> b) {
            std::function<T(T, T)> f = static_cast<T(*)(T, T)>(sub);
            return binary_operation(a, b, f);
        }

        template<class T, int n>
        tuple<T, n> sub(tuple<T, n> a, T b) {
            std::function<T(T, T)> f = static_cast<T(*)(T, T)>(sub);
            return binary_operation(a, b, f);
        }

        template<class T, int n>
        tuple<T, n> mul(tuple<T, n> a, tuple<T, n> b) {
            std::function<T(T, T)> f = static_cast<T(*)(T, T)>(mul);
            return binary_operation(a, b, f);
        }

        template<class T, int n>
        tuple<T, n> mul(tuple<T, n> a, T b) {
            std::function<T(T, T)> f = static_cast<T(*)(T, T)>(mul);
            return binary_operation(a, b, f);
        }

        template<class T, int n>
        tuple<T, n> div(tuple<T, n> a, tuple<T, n> b) {
            std::function<T(T, T)> f = static_cast<T(*)(T, T)>(div);
            return binary_operation(a, b, f);
        }

        template<class T, int n>
        tuple<T, n> div(tuple<T, n> a, T b) {
            std::function<T(T, T)> f = static_cast<T(*)(T, T)>(div);
            return binary_operation(a, b, f);
        }


        template<class T, int n>
        tuple<T, n> min(tuple<T, n> a, tuple<T, n> b) {
            std::function<T(T, T)> f = static_cast<T(*)(T, T)>(min);
            return binary_operation(a, b, f);
        }

        template<class T, int n>
        tuple<T, n> min(tuple<T, n> a, T b) {
            std::function<T(T, T)> f = static_cast<T(*)(T, T)>(min);
            return binary_operation(a, b, f);
        }

        template<class T, int n>
        tuple<T, n> max(tuple<T, n> a, tuple<T, n> b) {
            std::function<T(T, T)> f = static_cast<T(*)(T, T)>(max);
            return binary_operation(a, b, f);
        }

        template<class T, int n>
        tuple<T, n> max(tuple<T, n> a, T b) {
            std::function<T(T, T)> f = static_cast<T(*)(T, T)>(max);
            return binary_operation(a, b, f);
        }


        // Basic operators
        template<class T, class = typename std::enable_if<traits::is_simd<T>::value, T>::type>
        T operator+(T a, T b) {
            return add(a, b);
        }

        template<class T, class = typename std::enable_if<traits::is_simd<T>::value, T>::type>
        T operator-(T a, T b) {
            return sub(a, b);
        }

        template<class T, class = typename std::enable_if<traits::is_simd<T>::value, T>::type>
        T operator*(T a, T b) {
            return mul(a, b);
        }
    }; // namespace simd
}; // namespace c4


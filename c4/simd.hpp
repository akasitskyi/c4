#pragma once

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

namespace c4 {
    namespace simd {

        struct int8x16_t {
            typedef int8_t base_t;
#ifdef USE_ARM_NEON
            int8x16_t v;
#else
            __m128i v;
            int8x16_t(__m128i v) : v(v) {}
#endif
        };

        struct uint8x16_t {
            typedef uint8_t base_t;

#ifdef USE_ARM_NEON
            uint8x16_t v;
#else
            __m128i v;
            uint8x16_t(__m128i v) : v(v) {}
#endif
        };

        struct int16x8_t {
            typedef int16_t base_t;

#if defined(USE_ARM_NEON)
            int16x8_t v;
#else
            __m128i v;
            int16x8_t(__m128i v) : v(v) {}
#endif
        };

        struct uint16x8_t {
            typedef uint16_t base_t;

#if defined(USE_ARM_NEON)
            uint16x8_t v;
#else
            __m128i v;
            uint16x8_t(__m128i v) : v(v) {}
#endif
        };

        struct int32x4_t {
            typedef int32_t base_t;
#ifdef USE_ARM_NEON
            int32x4_t v;
#else
            __m128i v;
            int32x4_t(__m128i v) : v(v) {}
#endif
        };

        struct uint32x4_t {
            typedef uint32_t base_t;
#ifdef USE_ARM_NEON
            uint32x4_t v;
#else
            __m128i v;
            uint32x4_t(__m128i v) : v(v) {}
#endif
        };

        struct int64x2_t {
            typedef int64_t base_t;
#ifdef USE_ARM_NEON
            int64x2_t v;
#else
            __m128i v;
            int64x2_t(__m128i v) : v(v) {}
#endif
        };

        struct uint64x2_t {
            typedef uint64_t base_t;
#ifdef USE_ARM_NEON
            uint64x2_t v;
#else
            __m128i v;
            uint64x2_t(__m128i v) : v(v) {}
#endif
        };

        struct float32x4_t {
            typedef float base_t;
#ifdef USE_ARM_NEON
            float32x4_t v;
#else
            __m128 v;
            float32x4_t(__m128 v) : v(v) {}
#endif
        };

        namespace traits {
            template<class T>
            struct is_simd : std::false_type {};
            template<>
            struct is_simd<int8x16_t> : std::true_type {};
            template<>
            struct is_simd<uint8x16_t> : std::true_type {};
            template<>
            struct is_simd<int16x8_t> : std::true_type {};
            template<>
            struct is_simd<uint16x8_t> : std::true_type {};
            template<>
            struct is_simd<int32x4_t> : std::true_type {};
            template<>
            struct is_simd<uint32x4_t> : std::true_type {};
            template<>
            struct is_simd<int64x2_t> : std::true_type {};
            template<>
            struct is_simd<uint64x2_t> : std::true_type {};
            template<>
            struct is_simd<float32x4_t> : std::true_type {};

            template<class T>
            struct is_integral : std::false_type {};
            template<>
            struct is_integral<int8x16_t> : std::true_type {};
            template<>
            struct is_integral<uint8x16_t> : std::true_type {};
            template<>
            struct is_integral<int16x8_t> : std::true_type {};
            template<>
            struct is_integral<uint16x8_t> : std::true_type {};
            template<>
            struct is_integral<int32x4_t> : std::true_type {};
            template<>
            struct is_integral<uint32x4_t> : std::true_type {};
            template<>
            struct is_integral<int64x2_t> : std::true_type {};
            template<>
            struct is_integral<uint64x2_t> : std::true_type {};

            template<class T>
            struct is_signed : std::false_type {};
            template<>
            struct is_signed<int8x16_t> : std::true_type {};
            template<>
            struct is_signed<uint8x16_t> : std::false_type {};
            template<>
            struct is_signed<int16x8_t> : std::true_type {};
            template<>
            struct is_signed<uint16x8_t> : std::false_type {};
            template<>
            struct is_signed<int32x4_t> : std::true_type {};
            template<>
            struct is_signed<uint32x4_t> : std::false_type {};
            template<>
            struct is_signed<int64x2_t> : std::true_type {};
            template<>
            struct is_signed<uint64x2_t> : std::false_type {};
            template<>
            struct is_signed<float32x4_t> : std::true_type {};

            template<class T, class = typename std::enable_if<is_simd<T>::value>::type>
            constexpr size_t length() {
                return sizeof(T) / sizeof(T::base_t);
            }
        };

        template<class T, class = typename std::enable_if<traits::is_simd<T>::value>::type>
        struct pair {
            T hi, lo;
        };

#ifdef USE_ARM_NEON
        // Load

        int8x16_t load(const int8_t* ptr) {
            return vld1q_s8(ptr);
        }

        uint8x16_t load(const uint8_t* ptr) {
            return vld1q_u8(ptr);
        }

        int16x8_t load(const int16_t* ptr) {
            return vld1q_s16(ptr);
        }

        uint16x8_t load(const uint16_t* ptr) {
            return vld1q_u16(ptr);
        }

        int32x4_t load(const int32_t* ptr) {
            return vld1q_s32(ptr);
        }

        uint32x4_t load(const uint32_t* ptr) {
            return vld1q_u32(ptr);
        }

        int64x2_t load(const int64_t* ptr) {
            return vld1q_s64(ptr);
        }

        uint64x2_t load(const uint64_t* ptr) {
            return vld1q_u64(ptr);
        }

        float32x4_t load(const float32_t* ptr) {
            return vld1q_f32(ptr);
        }

        // Store

        void store(int8_t* ptr, int8x16_t a) {
            vst1q_s8(ptr, a);
        }

        void store(uint8_t* ptr, uint8x16_t a) {
            vst1q_u8(ptr, a);
        }

        void store(int16_t* ptr, int16x8_t a) {
            vst1q_s16(ptr, a);
        }

        void store(uint16_t* ptr, uint16x8_t a) {
            vst1q_u16(ptr, a);
        }

        void store(int32_t* ptr, int32x4_t a) {
            vst1q_s32(ptr, a);
        }

        void store(uint32_t* ptr, uint32x4_t a) {
            vst1q_u32(ptr, a);
        }

        void store(int64_t* ptr, int64x2_t a) {
            vst1q_s64(ptr, a);
        }

        void store(uint64_t* ptr, uint64x2_t a) {
            vst1q_u64(ptr, a);
        }

        void store(float32_t* ptr, float32x4_t a) {
            vst1q_f32(ptr, a);
        }

        // Long move, narrow

        pair<int16x8_t> long_move(int8x16_t a) {
            int8x8_t hi = vget_high_s8(a.v);
            int8x8_t lo = vget_low_s8(a.v);

            return { vmovl_s8(hi), vmovl_s8(lo) };
        }

        pair<int32x4_t> long_move(int16x8_t a) {
            int16x4_t hi = vget_high_s16(a.v);
            int16x4_t lo = vget_low_s16(a.v);

            return { vmovl_s16(hi), vmovl_s16(lo) };
        }

        pair<int64x2_t> long_move(int32x4_t a) {
            int32x2_t hi = vget_high_s32(a.v);
            int32x2_t lo = vget_low_s32(a.v);

            return { vmovl_s32(hi), vmovl_s32(lo) };
        }

        pair<uint16x8_t> long_move(uint8x16_t a) {
            uint8x8_t hi = vget_high_u8(a.v);
            uint8x8_t lo = vget_low_u8(a.v);

            return { vmovl_u8(hi), vmovl_u8(lo) };
        }

        pair<uint32x4_t> long_move(uint16x8_t a) {
            uint16x4_t hi = vget_high_u16(a.v);
            uint16x4_t lo = vget_low_u16(a.v);

            return { vmovl_u16(hi), vmovl_u16(lo) };
        }

        pair<uint64x2_t> long_move(uint32x4_t a) {
            uint32x2_t hi = vget_high_u32(a.v);
            uint32x2_t lo = vget_low_u32(a.v);

            return { vmovl_u32(hi), vmovl_u32(lo) };
        }

        uint8x16_t narrow(pair<uint16x8_t> p) {
            uint16x4_t hi = vmovl_u8(p.hi);
            uint16x4_t lo = vmovl_u8(p.lo);

            return vcombine_u8(lo, hi);
        }

        int8x16_t narrow(pair<int16x8_t> p) {
            uint16x4_t hi = vmovl_u8(p.hi);
            uint16x4_t lo = vmovl_u8(p.lo);

            return vcombine_u8(lo, hi);
        }

        uint16x8_t narrow(pair<uint32x4_t> p) {
            uint16x4_t hi = vmovl_u8(p.hi);
            uint16x4_t lo = vmovl_u8(p.lo);

            return vcombine_u8(lo, hi);
        }

        int16x8_t narrow(pair<int32x4_t> p) {
            uint16x4_t hi = vmovl_u8(p.hi);
            uint16x4_t lo = vmovl_u8(p.lo);

            return vcombine_u8(lo, hi);
        }

        uint32x4_t narrow(pair<uint64x2_t> p) {
            uint16x4_t hi = vmovl_u8(p.hi);
            uint16x4_t lo = vmovl_u8(p.lo);

            return vcombine_u8(lo, hi);
        }

        int32x4_t narrow(pair<int64x2_t> p) {
            uint16x4_t hi = vmovl_u8(p.hi);
            uint16x4_t lo = vmovl_u8(p.lo);

            return vcombine_u8(lo, hi);
        }

        // Addition

        int8x16_t add(int8x16_t a, int8x16_t b) {
            return vaddq_s8(a.v, b.v);
        }

        uint8x16_t add(uint8x16_t a, uint8x16_t b) {
            return vaddq_u8(a.v, b.v);
        }

        int16x8_t add(int16x8_t a, int16x8_t b) {
            return vaddq_s16(a.v, b.v);
        }

        uint16x8_t add(uint16x8_t a, uint16x8_t b) {
            return vaddq_u16(a.v, b.v);
        }

        int32x4_t add(int32x4_t a, int32x4_t b) {
            return vaddq_s32(a.v, b.v);
        }

        uint32x4_t add(uint32x4_t a, uint32x4_t b) {
            return vaddq_u32(a.v, b.v);
        }

        int64x2_t add(int64x2_t a, int64x2_t b) {
            return vaddq_s64(a.v, b.v);
        }

        uint64x2_t add(uint64x2_t a, uint64x2_t b) {
            return vaddq_u64(a.v, b.v);
        }

        float32x4_t add(float32x4_t a, float32x4_t b) {
            return vaddq_f32(a.v, b.v);
        }

        // Subtraction

        int8x16_t sub(int8x16_t a, int8x16_t b) {
            return vsubq_s8(a.v, b.v);
        }

        uint8x16_t sub(uint8x16_t a, uint8x16_t b) {
            return vsubq_u8(a.v, b.v);
        }

        int16x8_t sub(int16x8_t a, int16x8_t b) {
            return vsubq_s16(a.v, b.v);
        }

        uint16x8_t sub(uint16x8_t a, uint16x8_t b) {
            return vsubq_u16(a.v, b.v);
        }

        int32x4_t sub(int32x4_t a, int32x4_t b) {
            return vsubq_s32(a.v, b.v);
        }

        uint32x4_t sub(uint32x4_t a, uint32x4_t b) {
            return vsubq_u32(a.v, b.v);
        }

        int64x2_t sub(int64x2_t a, int64x2_t b) {
            return vsubq_s64(a.v, b.v);
        }

        uint64x2_t sub(uint64x2_t a, uint64x2_t b) {
            return vsubq_u64(a.v, b.v);
        }

        float32x4_t sub(float32x4_t a, float32x4_t b) {
            return vsubq_f32(a.v, b.v);
        }

        // Multiplication
        int8x16_t mul(int8x16_t a, int8x16_t b) {
            return vmulq_s8(a.v, b.v);
        }

        uint8x16_t mul(uint8x16_t a, uint8x16_t b) {
            return vmulq_u8(a.v, b.v);
        }

        int16x8_t mul(int16x8_t a, int16x8_t b) {
            return vmulq_s16(a.v, b.v);
        }

        uint16x8_t mul(uint16x8_t a, uint16x8_t b) {
            return vmulq_u16(a.v, b.v);
        }

        int32x4_t mul(int32x4_t a, int32x4_t b) {
            return vmulq_s32(a.v, b.v);
        }

        uint32x4_t mul(uint32x4_t a, uint32x4_t b) {
            return vmulq_u32(a.v, b.v);
        }

        int64x2_t mul(int64x2_t a, int64x2_t b) {
            return vmulq_s64(a.v, b.v);
        }

        uint64x2_t mul(uint64x2_t a, uint64x2_t b) {
            return vmulq_u64(a.v, b.v);
        }

        float32x4_t mul(float32x4_t a, float32x4_t b) {
            return vmulq_f32(a.v, b.v);
        }


#else
        // Helpers    

        template<class dst_t, class src_t, class = typename std::enable_if<traits::is_integral<src_t>::value && traits::is_integral<dst_t>::value>::type>
        dst_t reinterpret(src_t a) {
            return dst_t{ a.v };
        }

        template<class dst_t, class src_t, class = typename std::enable_if<traits::is_integral<src_t>::value && traits::is_integral<dst_t>::value>::type>
        pair<dst_t> reinterpret(typename pair<src_t> a) {
            return pair<dst_t>{ a.hi.v, a.lo.v };
        }

        template<class T>
        T serial(T a, T b, std::function<typename T::base_t(typename T::base_t, typename T::base_t)> F) {
            constexpr size_t n = traits::length<T>();

            alignas(16) typename T::base_t ta[n];
            alignas(16) typename T::base_t tb[n];

            _mm_store_si128((__m128i*)ta, a.v);
            _mm_store_si128((__m128i*)tb, b.v);

            for (size_t i = 0; i < n; i++)
                ta[i] = F(ta[i], tb[i]);

            return _mm_load_si128((__m128i*)ta);;
        }

        int8x16_t cmpgt(int8x16_t a, int8x16_t b) {
            return _mm_cmpgt_epi8(a.v, b.v);
        }

        int16x8_t cmpgt(int16x8_t a, int16x8_t b) {
            return _mm_cmpgt_epi16(a.v, b.v);
        }

        int32x4_t cmpgt(int32x4_t a, int32x4_t b) {
            return _mm_cmpgt_epi32(a.v, b.v);
        }

        int64x2_t cmpgt(int64x2_t a, int64x2_t b) {
#ifdef USE_SSE4_1
            return _mm_cmpgt_epi64(a.v, b.v);
#else
            return serial(a, b, std::greater<int64_t>());
#endif
        }

        // Load

        int8x16_t load(const int8_t* ptr) {
            return _mm_loadu_si128((__m128i *)ptr);
        }

        uint8x16_t load(const uint8_t* ptr) {
            return _mm_loadu_si128((__m128i *)ptr);
        }

        int16x8_t load(const int16_t* ptr) {
            return _mm_loadu_si128((__m128i *)ptr);
        }

        uint16x8_t load(const uint16_t* ptr) {
            return _mm_loadu_si128((__m128i *)ptr);
        }

        int32x4_t load(const int32_t* ptr) {
            return _mm_loadu_si128((__m128i *)ptr);
        }

        uint32x4_t load(const uint32_t* ptr) {
            return _mm_loadu_si128((__m128i *)ptr);
        }

        int64x2_t load(const int64_t* ptr) {
            return _mm_loadu_si128((__m128i *)ptr);
        }

        uint64x2_t load(const uint64_t* ptr) {
            return _mm_loadu_si128((__m128i *)ptr);
        }

        float32x4_t load(const float* ptr) {
            return _mm_loadu_ps(ptr);
        }

        // Store

        void store(int8_t* ptr, int8x16_t a) {
            _mm_storeu_si128((__m128i *)ptr, a.v);
        }

        void store(uint8_t* ptr, uint8x16_t a) {
            _mm_storeu_si128((__m128i *)ptr, a.v);
        }

        void store(int16_t* ptr, int16x8_t a) {
            _mm_storeu_si128((__m128i *)ptr, a.v);
        }

        void store(uint16_t* ptr, uint16x8_t a) {
            _mm_storeu_si128((__m128i *)ptr, a.v);
        }

        void store(int32_t* ptr, int32x4_t a) {
            _mm_storeu_si128((__m128i *)ptr, a.v);
        }

        void store(uint32_t* ptr, uint32x4_t a) {
            _mm_storeu_si128((__m128i *)ptr, a.v);
        }

        void store(int64_t* ptr, int64x2_t a) {
            _mm_storeu_si128((__m128i *)ptr, a.v);
        }

        void store(uint64_t* ptr, uint64x2_t a) {
            _mm_storeu_si128((__m128i *)ptr, a.v);
        }

        void store(float* ptr, float32x4_t a) {
            _mm_storeu_ps(ptr, a.v);
        }

        // Long move, narrow

        pair<int16x8_t> long_move(int8x16_t a) {
            int8x16_t sign = cmpgt(_mm_setzero_si128(), a);
            return { _mm_unpackhi_epi8(a.v, sign.v), _mm_unpackhi_epi8(a.v, sign.v) };
        }

        pair<int32x4_t> long_move(int16x8_t a) {
            int16x8_t sign = cmpgt(_mm_setzero_si128(), a);
            return { _mm_unpackhi_epi16(a.v, sign.v), _mm_unpackhi_epi16(a.v, sign.v) };
        }

        pair<int64x2_t> long_move(int32x4_t a) {
            int32x4_t sign = cmpgt(_mm_setzero_si128(), a);
            return { _mm_unpackhi_epi32(a.v, sign.v), _mm_unpackhi_epi32(a.v, sign.v) };
        }

        pair<uint16x8_t> long_move(uint8x16_t a) {
            return { _mm_unpackhi_epi8(a.v, _mm_setzero_si128()), _mm_unpackhi_epi8(a.v, _mm_setzero_si128()) };
        }

        pair<uint32x4_t> long_move(uint16x8_t a) {
            return { _mm_unpackhi_epi16(a.v, _mm_setzero_si128()), _mm_unpackhi_epi16(a.v, _mm_setzero_si128()) };
        }

        pair<uint64x2_t> long_move(uint32x4_t a) {
            return { _mm_unpackhi_epi32(a.v, _mm_setzero_si128()), _mm_unpackhi_epi32(a.v, _mm_setzero_si128()) };
        }

        uint8x16_t narrow(pair<uint16x8_t> p) {
            alignas(16) const static int8_t index[]{ 0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15 };
            const static __m128i i = _mm_load_si128((const __m128i*)index);
            __m128i hi = _mm_shuffle_epi8(p.hi.v, i);
            __m128i lo = _mm_shuffle_epi8(p.lo.v, i);

            return _mm_unpacklo_epi64(hi, lo);
        }

        int8x16_t narrow(pair<int16x8_t> p) {
            return reinterpret<int8x16_t>(narrow(reinterpret<uint16x8_t>(p)));
        }

        uint16x8_t narrow(pair<uint32x4_t> p) {
            alignas(16) const static int8_t index[]{ 0,1, 4,5, 8,9, 12,13, 2,3, 6,7, 10,11, 14,15 };
            const static __m128i i = _mm_load_si128((const __m128i*)index);
            __m128i hi = _mm_shuffle_epi8(p.hi.v, i);
            __m128i lo = _mm_shuffle_epi8(p.lo.v, i);

            return _mm_unpacklo_epi64(hi, lo);
        }

        int16x8_t narrow(pair<int32x4_t> p) {
            return reinterpret<int16x8_t>(narrow(reinterpret<uint32x4_t>(p)));
        }

        uint32x4_t narrow(pair<uint64x2_t> p) {
            __m128i hi = _mm_shuffle_epi32(p.hi.v, _MM_SHUFFLE(3, 1, 2, 0));
            __m128i lo = _mm_shuffle_epi32(p.lo.v, _MM_SHUFFLE(3, 1, 2, 0));

            return _mm_unpacklo_epi64(hi, lo);
        }

        int32x4_t narrow(pair<int64x2_t> p) {
            return reinterpret<int32x4_t>(narrow(reinterpret<uint64x2_t>(p)));
        }

        // Addition

        int8x16_t add(int8x16_t a, int8x16_t b) {
            return _mm_add_epi8(a.v, b.v);
        }

        uint8x16_t add(uint8x16_t a, uint8x16_t b) {
            return _mm_add_epi8(a.v, b.v);
        }

        int16x8_t add(int16x8_t a, int16x8_t b) {
            return _mm_add_epi16(a.v, b.v);
        }

        uint16x8_t add(uint16x8_t a, uint16x8_t b) {
            return _mm_add_epi16(a.v, b.v);
        }

        int32x4_t add(int32x4_t a, int32x4_t b) {
            return _mm_add_epi32(a.v, b.v);
        }

        uint32x4_t add(uint32x4_t a, uint32x4_t b) {
            return _mm_add_epi32(a.v, b.v);
        }

        int64x2_t add(int64x2_t a, int64x2_t b) {
            return _mm_add_epi64(a.v, b.v);
        }

        uint64x2_t add(uint64x2_t a, uint64x2_t b) {
            return _mm_add_epi64(a.v, b.v);
        }

        float32x4_t add(float32x4_t a, float32x4_t b) {
            return _mm_add_ps(a.v, b.v);
        }

        // Subtraction

        int8x16_t sub(int8x16_t a, int8x16_t b) {
            return _mm_sub_epi8(a.v, b.v);
        }

        uint8x16_t sub(uint8x16_t a, uint8x16_t b) {
            return _mm_sub_epi8(a.v, b.v);
        }

        int16x8_t sub(int16x8_t a, int16x8_t b) {
            return _mm_sub_epi16(a.v, b.v);
        }

        uint16x8_t sub(uint16x8_t a, uint16x8_t b) {
            return _mm_sub_epi16(a.v, b.v);
        }

        int32x4_t sub(int32x4_t a, int32x4_t b) {
            return _mm_sub_epi32(a.v, b.v);
        }

        uint32x4_t sub(uint32x4_t a, uint32x4_t b) {
            return _mm_sub_epi32(a.v, b.v);
        }

        int64x2_t sub(int64x2_t a, int64x2_t b) {
            return _mm_sub_epi64(a.v, b.v);
        }

        uint64x2_t sub(uint64x2_t a, uint64x2_t b) {
            return _mm_sub_epi64(a.v, b.v);
        }

        float32x4_t sub(float32x4_t a, float32x4_t b) {
            return _mm_sub_ps(a.v, b.v);
        }

        int16x8_t mul(int16x8_t a, int16x8_t b) {
            return _mm_mullo_epi16(a.v, b.v);
        }

        uint16x8_t mul(uint16x8_t a, uint16x8_t b) {
            return _mm_mullo_epi16(a.v, b.v);
        }

        // Multiplication
        int8x16_t mul(int8x16_t a, int8x16_t b) {
            pair<int16x8_t> ap = long_move(a);
            pair<int16x8_t> bp = long_move(b);

            ap.hi = mul(ap.hi, bp.hi);
            ap.lo = mul(ap.lo, bp.lo);

            return narrow(ap);
        }

        uint8x16_t mul(uint8x16_t a, uint8x16_t b) {
            pair<uint16x8_t> ap = long_move(a);
            pair<uint16x8_t> bp = long_move(b);

            ap.hi = mul(ap.hi, bp.hi);
            ap.lo = mul(ap.lo, bp.lo);

            return narrow(ap);
        }

        int32x4_t mul(int32x4_t a, int32x4_t b) {
#ifdef USE_SSE4_1
            return _mm_mullo_epi32(a.v, b.v);
#else
            return serial(a, b, std::multiplies<int32_t>());
#endif
        }

        uint32x4_t mul(uint32x4_t a, uint32x4_t b) {
#ifdef USE_SSE4_1
            return _mm_mullo_epi32(a.v, b.v);
#else
            return serial(a, b, std::multiplies<uint32_t>());
#endif
        }

        int64x2_t mul(int64x2_t a, int64x2_t b) {
            return serial(a, b, std::multiplies<int64_t>());
        }

        uint64x2_t mul(uint64x2_t a, uint64x2_t b) {
            return serial(a, b, std::multiplies<uint64_t>());
        }

        float32x4_t mul(float32x4_t a, float32x4_t b) {
            return _mm_mul_ps(a.v, b.v);
        }

#endif
        // Basic operators
        template<class T, class = typename std::enable_if<is_simd<T>::value, T>::type>
        T operator+(T a, T b) {
            return add(a, b);
        }

        template<class T, class = typename std::enable_if<is_simd<T>::value, T>::type>
        T operator-(T a, T b) {
            return sub(a, b);
        }
    }; // namespace simd
}; // namespace c4

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

#include <c4/simd.hpp>
#include <c4/math.hpp>

#include <array>
#include <vector>
#include <string>
#include <cstring>
#include <numeric>
#include <algorithm>
#include <iostream>

#include <random>

using namespace std;
using namespace c4::simd;

// ===================================================== HELPERS ================================================================

template<class T>
T random() {
    static std::mt19937 mt;
    static std::uniform_int_distribution<long long> d;
    
    return (T)d(mt);
}

template<class T, int L, int R>
T random() {
    static std::mt19937 mt;
    static std::uniform_int_distribution<long long> d(L, R);

    return (T)d(mt);
}

template<>
float random<float>() {
    static std::mt19937 mt;
    static std::normal_distribution<float> d;

    return d(mt);
}

template<class T, int n>
array<T, n> random_array() {
    array<T, n> v;
    for (T& t : v)
        t = random<T>();
    return v;
}

template<class T, int n, int L, int R>
array<T, n> random_array() {
    array<T, n> v;
    for (T& t : v)
        t = random<T, L, R>();
    return v;
}

template<class T>
T all_ones() {
    T t;
    memset(&t, -1, sizeof(t));
    return t;
}

template<class T>
struct mask_t {
    typedef typename std::make_unsigned<T>::type type;
};

template<>
struct mask_t<float> {
    typedef uint32_t type;
};

template<class dst_t, class src_t>
dst_t saturate(src_t x) {
    x = max<src_t>(x, std::numeric_limits<dst_t>::min());
    x = min<src_t>(x, std::numeric_limits<dst_t>::max());
    return (dst_t)x;
}


class Exception : public std::runtime_error {
public:
    Exception(std::string msg, std::string filename, int line) : runtime_error(msg + " at " + filename + ":" + std::to_string(line)) {}
};

#define THROW_EXCEPTION(MSG) throw Exception(MSG, __FILE__, __LINE__)

#define ASSERT_TRUE(C) if( C ) {} else THROW_EXCEPTION("Runtime assertion failed: " #C)
#define ASSERT_EQUAL(A, B) if( (A) == (B) ) {} else THROW_EXCEPTION("Runtime assertion failed: " #A " == " #B ", " + to_string(A) + " != " + to_string(B))

// ====================================================== TESTS =================================================================

template<class T>
void test_greater() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto b = random_array<T, n>();
    auto r = random_array<typename mask_t<T>::type, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = c4::simd::greater(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], a[i] > b[i] ? all_ones<typename mask_t<T>::type>() : 0);
    }
}

void multitest_greater() {
    test_greater<int8_t>();
    test_greater<uint8_t>();
    test_greater<int16_t>();
    test_greater<uint16_t>();
    test_greater<int32_t>();
    test_greater<uint32_t>();
    test_greater<float>();
}

template<class T>
void test_less() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto b = random_array<T, n>();
    auto r = random_array<typename mask_t<T>::type, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = c4::simd::less(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], a[i] < b[i] ? all_ones<typename mask_t<T>::type>() : 0);
    }
}

void multitest_less() {
    test_less<int8_t>();
    test_less<uint8_t>();
    test_less<int16_t>();
    test_less<uint16_t>();
    test_less<int32_t>();
    test_less<uint32_t>();
    test_less<float>();
}

template<class T>
void test_greater_equal() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto b = random_array<T, n>();
    auto r = random_array<typename mask_t<T>::type, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = c4::simd::greater_equal(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], a[i] >= b[i] ? all_ones<typename mask_t<T>::type>() : 0);
    }
}

void multitest_greater_equal() {
    test_greater_equal<int8_t>();
    test_greater_equal<uint8_t>();
    test_greater_equal<int16_t>();
    test_greater_equal<uint16_t>();
    test_greater_equal<int32_t>();
    test_greater_equal<uint32_t>();
    test_greater_equal<float>();
}

template<class T>
void test_less_equal() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto b = random_array<T, n>();
    auto r = random_array<typename mask_t<T>::type, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = c4::simd::less_equal(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], a[i] <= b[i] ? all_ones<typename mask_t<T>::type>() : 0);
    }
}

void multitest_less_equal() {
    test_less_equal<int8_t>();
    test_less_equal<uint8_t>();
    test_less_equal<int16_t>();
    test_less_equal<uint16_t>();
    test_less_equal<int32_t>();
    test_less_equal<uint32_t>();
    test_less_equal<float>();
}

template<class T>
void test_equal() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto b = random_array<T, n>();
    auto r = random_array<typename mask_t<T>::type, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = c4::simd::equal(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], a[i] == b[i] ? all_ones<typename mask_t<T>::type>() : 0);
    }
}

void multitest_equal() {
    test_equal<int8_t>();
    test_equal<uint8_t>();
    test_equal<int16_t>();
    test_equal<uint16_t>();
    test_equal<int32_t>();
    test_equal<uint32_t>();
    test_equal<float>();
}


template<class T>
void test_min() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto b = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = min(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], min(a[i], b[i]));
    }
}

void multitest_min() {
    test_min<int8_t>();
    test_min<uint8_t>();
    test_min<int16_t>();
    test_min<uint16_t>();
    test_min<int32_t>();
    test_min<uint32_t>();
    test_min<float>();
}

template<class T>
void test_max() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto b = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = max(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], max(a[i], b[i]));
    }
}

void multitest_max() {
    test_max<int8_t>();
    test_max<uint8_t>();
    test_max<int16_t>();
    test_max<uint16_t>();
    test_max<int32_t>();
    test_max<uint32_t>();
    test_max<float>();
}

template<class T>
void test_interleave() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, 2 * n>();
    auto r = random_array<T, 2 * n>();

    auto va = load(a.data());
    auto vb = load(a.data() + n);
    c4::simd::tuple<decltype(va), 2> vavb{ va, vb };
    auto vr = interleave(vavb);

    store(r.data(), vr.val[0]);
    store(r.data() + n, vr.val[1]);

    for (int i = 0; i < 2 * n; i++) {
        ASSERT_EQUAL(r[i], i % 2 ? a[n + i / 2] : a[i/2]);
    }
}

void multitest_interleave() {
    test_interleave<int8_t>();
    test_interleave<uint8_t>();
    test_interleave<int16_t>();
    test_interleave<uint16_t>();
    test_interleave<int32_t>();
    test_interleave<uint32_t>();
    test_interleave<float>();
}

template<class T>
void test_deinterleave() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, 2 * n>();
    auto r = random_array<T, 2 * n>();

    auto va = load(a.data());
    auto vb = load(a.data() + n);
    c4::simd::tuple<decltype(va), 2> vavb{ va, vb };
    auto vr = deinterleave(vavb);

    store(r.data(), vr.val[0]);
    store(r.data() + n, vr.val[1]);

    for (int i = 0; i < 2 * n; i++) {
        ASSERT_EQUAL(a[i], i % 2 ? r[n + i / 2] : r[i / 2]);
    }
}

void multitest_deinterleave() {
    test_deinterleave<int8_t>();
    test_deinterleave<uint8_t>();
    test_deinterleave<int16_t>();
    test_deinterleave<uint16_t>();
    test_deinterleave<int32_t>();
    test_deinterleave<uint32_t>();
    test_deinterleave<float>();
}

template<class T>
void test_long_move() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();

    auto va = load(a.data());
    auto vr = long_move(va);
    auto r = random_array<typename std::remove_reference<decltype(vr.val[0])>::type::base_t, n>();

    store_tuple(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], a[i]);
    }
}

void multitest_long_move() {
    test_long_move<int8_t>();
    test_long_move<uint8_t>();
    test_long_move<int16_t>();
    test_long_move<uint16_t>();
}

template<class T>
void test_narrow() {
    constexpr int n = 32 / sizeof(T);
    auto a = random_array<T, n>();

    auto va = load_tuple<2>(a.data());
    auto vr = narrow(va);
    typedef typename std::remove_reference<decltype(vr)>::type::base_t dst_t;
    auto r = random_array<dst_t, n>();

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], (dst_t)a[i]);
    }
}

void multitest_narrow() {
    test_narrow<int16_t>();
    test_narrow<uint16_t>();
    test_narrow<int32_t>();
    test_narrow<uint32_t>();
}

template<class T>
void test_narrow_saturate() {
    constexpr int n = 32 / sizeof(T);
    auto a = random_array<T, n>();

    auto va = load_tuple<2>(a.data());
    auto vr = narrow_saturate(va);
    typedef typename std::remove_reference<decltype(vr)>::type::base_t dst_t;
    auto r = random_array<dst_t, n>();

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], saturate<dst_t>(a[i]));
    }
}

void multitest_narrow_saturate() {
    test_narrow_saturate<int16_t>();
    test_narrow_saturate<uint16_t>();
    test_narrow_saturate<int32_t>();
    test_narrow_saturate<uint32_t>();
}

template<class T>
void test_narrow_unsigned_saturate() {
    constexpr int n = 32 / sizeof(T);
    auto a = random_array<T, n>();

    auto va = load_tuple<2>(a.data());
    auto vr = narrow_unsigned_saturate(va);
    typedef typename std::remove_reference<decltype(vr)>::type::base_t dst_t;
    auto r = random_array<dst_t, n>();

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], saturate<dst_t>(a[i]));
    }
}

void multitest_narrow_unsigned_saturate() {
    test_narrow_unsigned_saturate<int16_t>();
    test_narrow_unsigned_saturate<int32_t>();
}

template<class T>
void test_get_low() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto r = random_array<T, n / 2>();

    auto va = load(a.data());
    auto vr = get_low(va);

    store(r.data(), vr);

    for (int i = 0; i < n / 2; i++) {
        ASSERT_EQUAL(r[i], a[i]);
    }
}

void multitest_get_low() {
    test_get_low<int8_t>();
    test_get_low<uint8_t>();
    test_get_low<int16_t>();
    test_get_low<uint16_t>();
    test_get_low<int32_t>();
    test_get_low<uint32_t>();
}

template<class T>
void test_get_high() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto r = random_array<T, n / 2>();

    auto va = load(a.data());
    auto vr = get_high(va);

    store(r.data(), vr);

    for (int i = 0; i < n / 2; i++) {
        ASSERT_EQUAL(r[i], a[n/2 + i]);
    }
}

void multitest_get_high() {
    test_get_high<int8_t>();
    test_get_high<uint8_t>();
    test_get_high<int16_t>();
    test_get_high<uint16_t>();
    test_get_high<int32_t>();
    test_get_high<uint32_t>();
}

template<class T>
void test_combine() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load_half(a.data());
    auto vb = load_half(a.data() + n / 2);

    auto vr = combine(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], a[i]);
    }
}

void multitest_combine() {
    test_combine<int8_t>();
    test_combine<uint8_t>();
    test_combine<int16_t>();
    test_combine<uint16_t>();
    test_combine<int32_t>();
    test_combine<uint32_t>();
}

template<class T>
void test_load_2_interleaved() {
    constexpr int n = 32 / sizeof(T);
    auto a = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load_2_interleaved(a.data());

    store_tuple(r.data(), va);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], a[2 * i % n + 2 * i / n]);
    }
}

void multitest_load_2_interleaved() {
    test_load_2_interleaved<int8_t>();
    test_load_2_interleaved<uint8_t>();
    test_load_2_interleaved<int16_t>();
    test_load_2_interleaved<uint16_t>();
    test_load_2_interleaved<int32_t>();
    test_load_2_interleaved<uint32_t>();
    test_load_2_interleaved<float>();
}

template<class T>
void test_load_3_interleaved() {
    constexpr int n = 48 / sizeof(T);
    auto a = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load_3_interleaved(a.data());

    store_tuple(r.data(), va);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], a[3 * i % n + 3 * i / n]);
    }
}

void multitest_load_3_interleaved() {
    test_load_3_interleaved<int8_t>();
    test_load_3_interleaved<uint8_t>();
    test_load_3_interleaved<int16_t>();
    test_load_3_interleaved<uint16_t>();
    test_load_3_interleaved<int32_t>();
    test_load_3_interleaved<uint32_t>();
    test_load_3_interleaved<float>();
}

template<class T>
void test_load_2_interleaved_long() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();

    auto va = load_2_interleaved_long(a.data());

    auto r = random_array<typename std::remove_reference<decltype(va.val[0])>::type::base_t, n>();

    store_tuple(r.data(), va);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], a[2 * i % n + 2 * i / n]);
    }
}

void multitest_load_2_interleaved_long() {
    test_load_2_interleaved_long<int8_t>();
    test_load_2_interleaved_long<uint8_t>();
    test_load_2_interleaved_long<int16_t>();
    test_load_2_interleaved_long<uint16_t>();
}

template<class T>
void test_load_3_interleaved_long() {
    constexpr int n = 24 / sizeof(T);
    auto a = random_array<T, n>();

    auto va = load_3_interleaved_long(a.data());

    auto r = random_array<typename std::remove_reference<decltype(va.val[0])>::type::base_t, n>();

    store_tuple(r.data(), va);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], a[3 * i % n + 3 * i / n]);
    }
}

void multitest_load_3_interleaved_long() {
    test_load_3_interleaved_long<int8_t>();
    test_load_3_interleaved_long<uint8_t>();
    test_load_3_interleaved_long<int16_t>();
    test_load_3_interleaved_long<uint16_t>();
}

template<class T>
void test_load_4_interleaved() {
    constexpr int n = 64 / sizeof(T);
    auto a = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load_4_interleaved(a.data());

    store_tuple(r.data(), va);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], a[4 * i % n + 4 * i / n]);
    }
}

void multitest_load_4_interleaved() {
    test_load_4_interleaved<int8_t>();
    test_load_4_interleaved<uint8_t>();
    test_load_4_interleaved<int16_t>();
    test_load_4_interleaved<uint16_t>();
    test_load_4_interleaved<int32_t>();
    test_load_4_interleaved<uint32_t>();
    test_load_4_interleaved<float>();
}

template<class T>
void test_load_4_interleaved_long() {
    constexpr int n = 32 / sizeof(T);
    auto a = random_array<T, n>();

    auto va = load_4_interleaved_long(a.data());

    auto r = random_array<typename std::remove_reference<decltype(va.val[0])>::type::base_t, n>();

    store_tuple(r.data(), va);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], a[4 * i % n + 4 * i / n]);
    }
}

void multitest_load_4_interleaved_long() {
    test_load_4_interleaved_long<int8_t>();
    test_load_4_interleaved_long<uint8_t>();
    test_load_4_interleaved_long<int16_t>();
    test_load_4_interleaved_long<uint16_t>();
}

template<class T>
void test_store_2_interleaved() {
    constexpr int n = 32 / sizeof(T);
    auto a = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load_tuple<2>(a.data());

    store_2_interleaved(r.data(), va);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(a[i], r[2 * i % n + 2 * i / n]);
    }
}

void multitest_store_2_interleaved() {
    test_store_2_interleaved<int8_t>();
    test_store_2_interleaved<uint8_t>();
    test_store_2_interleaved<int16_t>();
    test_store_2_interleaved<uint16_t>();
    test_store_2_interleaved<int32_t>();
    test_store_2_interleaved<uint32_t>();
    test_store_2_interleaved<float>();
}

template<class T>
void test_store_3_interleaved() {
    constexpr int n = 48 / sizeof(T);
    auto a = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load_tuple<3>(a.data());

    store_3_interleaved(r.data(), va);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(a[i], r[3 * i % n + 3 * i / n]);
    }
}

void multitest_store_3_interleaved() {
    test_store_3_interleaved<int8_t>();
    test_store_3_interleaved<uint8_t>();
    test_store_3_interleaved<int16_t>();
    test_store_3_interleaved<uint16_t>();
    test_store_3_interleaved<int32_t>();
    test_store_3_interleaved<uint32_t>();
    test_store_3_interleaved<float>();
}

template<class T>
void test_store_4_interleaved() {
    constexpr int n = 64 / sizeof(T);
    auto a = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load_tuple<4>(a.data());

    store_4_interleaved(r.data(), va);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(a[i], r[4 * i % n + 4 * i / n]);
    }
}

void multitest_store_4_interleaved() {
    test_store_4_interleaved<int8_t>();
    test_store_4_interleaved<uint8_t>();
    test_store_4_interleaved<int16_t>();
    test_store_4_interleaved<uint16_t>();
    test_store_4_interleaved<int32_t>();
    test_store_4_interleaved<uint32_t>();
    test_store_4_interleaved<float>();
}

template<class src_t, class dst_t>
void test_store_3_interleaved_narrow_saturate() {
    constexpr int n = 48 / sizeof(src_t);
    auto a = random_array<src_t, n>();
    auto r = random_array<dst_t, n>();

    auto va = load_tuple<3>(a.data());

    store_3_interleaved_narrow_saturate(r.data(), va);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(saturate<dst_t>(a[i]), r[3 * i % n + 3 * i / n]);
    }
}

void multitest_store_3_interleaved_narrow_saturate() {
    test_store_3_interleaved_narrow_saturate<int16_t, int8_t>();
    test_store_3_interleaved_narrow_saturate<int32_t, int16_t>();
}

template<class src_t, class dst_t>
void test_store_3_interleaved_narrow_unsigned_saturate() {
    constexpr int n = 48 / sizeof(src_t);
    auto a = random_array<src_t, n>();
    auto r = random_array<dst_t, n>();

    auto va = load_tuple<3>(a.data());

    store_3_interleaved_narrow_unsigned_saturate(r.data(), va);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(saturate<dst_t>(a[i]), r[3 * i % n + 3 * i / n]);
    }
}

void multitest_store_3_interleaved_narrow_unsigned_saturate() {
    test_store_3_interleaved_narrow_unsigned_saturate<int16_t, uint8_t>();
    test_store_3_interleaved_narrow_unsigned_saturate<int32_t, uint16_t>();
}

template<class src_t, class dst_t>
void test_store_4_interleaved_narrow_saturate() {
    constexpr int n = 64 / sizeof(src_t);
    auto a = random_array<src_t, n>();
    auto r = random_array<dst_t, n>();

    auto va = load_tuple<4>(a.data());

    store_4_interleaved_narrow_saturate(r.data(), va);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(saturate<dst_t>(a[i]), r[4 * i % n + 4 * i / n]);
    }
}

void multitest_store_4_interleaved_narrow_saturate() {
    test_store_4_interleaved_narrow_saturate<int16_t, int8_t>();
    test_store_4_interleaved_narrow_saturate<int16_t, uint8_t>();
    test_store_4_interleaved_narrow_saturate<int32_t, int16_t>();
    test_store_4_interleaved_narrow_saturate<int32_t, uint16_t>();
}


void test_to_float() {
    constexpr int n = 4;
    auto a = random_array<int32_t, n>();
    auto r = random_array<float, n>();

    auto va = load(a.data());
    auto vr = to_float(va);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], (float)a[i]);
    }
}

void test_to_int() {
    constexpr int n = 4;
    auto a = random_array<float, n>();
    auto r = random_array<int32_t, n>();

    auto va = load(a.data());
    auto vr = to_int(va);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], (int32_t)a[i]);
    }
}

template<class T>
void test_add() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto b = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = add(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], T(a[i] + b[i]));
    }
}

void multitest_add() {
    test_add<int8_t>();
    test_add<uint8_t>();
    test_add<int16_t>();
    test_add<uint16_t>();
    test_add<int32_t>();
    test_add<uint32_t>();
    test_add<float>();
}

template<class T>
void test_add_saturate() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto b = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = add_saturate(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        const T eta = c4::clamp<T>((int64_t)a[i] + b[i]);
        if (r[i] != eta) {
            cout << "oops" << endl;
        }
        ASSERT_EQUAL(r[i], eta);
    }
}

void multitest_add_saturate() {
    test_add_saturate<int8_t>();
    test_add_saturate<uint8_t>();
    test_add_saturate<int16_t>();
    test_add_saturate<uint16_t>();
    test_add_saturate<int32_t>();
    test_add_saturate<uint32_t>();
}

template<class T>
void test_hadd_long() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto va = load(a.data());
    auto vr = hadd_long(va);

    auto r = random_array<typename decltype(vr)::base_t, n / 2>();

    store(r.data(), vr);

    for (int i = 0; i < n / 2; i++) {
        ASSERT_EQUAL(r[i], a[2 * i] + a[2 * i + 1]);
    }
}

void multitest_hadd_long() {
    test_hadd_long<int8_t>();
    test_hadd_long<uint8_t>();
    test_hadd_long<int16_t>();
    test_hadd_long<uint16_t>();
}

template<class T>
void test_hadd() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, 2 * n>();

    auto va = load(a.data());
    auto vb = load(a.data() + n);

    auto vr = hadd(va, vb);

    auto r = random_array<typename decltype(vr)::base_t, n>();

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], T(a[2 * i] + a[2 * i + 1]));
    }
}

void multitest_hadd() {
    test_hadd<int16_t>();
    test_hadd<uint16_t>();
    test_hadd<int32_t>();
    test_hadd<uint32_t>();
    test_hadd<float>();
}

template<class T>
void test_sub() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto b = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = sub(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], T(a[i] - b[i]));
    }
}

void multitest_sub() {
    test_sub<int8_t>();
    test_sub<uint8_t>();
    test_sub<int16_t>();
    test_sub<uint16_t>();
    test_sub<int32_t>();
    test_sub<uint32_t>();
    test_sub<float>();
}

template<class T>
void test_sub_div2() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto b = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = sub_div2(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        int64_t t = (int64_t)a[i] - (int64_t)b[i];
        // arithmetic shift right
        t = t >= 0 ? t >> 1 : t >> 1 | (1ll << 63);
        T e = T(t);
        if (r[i] != e) {
            cout << "a[i] = " << a[i] << ", b[i] = " << b[i] << endl;
            cout << "r[i] = " << r[i] << ", a[i] - b[i] = " << e << endl;
        }
        ASSERT_EQUAL(r[i], e);
    }
}

void multitest_sub_div2() {
    test_sub_div2<int8_t>();
    test_sub_div2<uint8_t>();
    test_sub_div2<int16_t>();
    test_sub_div2<uint16_t>();
    test_sub_div2<int32_t>();
    test_sub_div2<uint32_t>();
}

template<class T>
void test_abs_diff() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto b = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = abs_diff(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], (T)std::abs((int64_t)a[i] - (int64_t)b[i]));
    }
}

template<>
void test_abs_diff<float>() {
    constexpr int n = 16 / sizeof(float);
    auto a = random_array<float, n>();
    auto b = random_array<float, n>();
    auto r = random_array<float, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = abs_diff(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], std::abs(a[i] - b[i]));
    }
}

void multitest_abs_diff() {
    test_abs_diff<int8_t>();
    test_abs_diff<uint8_t>();
    test_abs_diff<int16_t>();
    test_abs_diff<uint16_t>();
    test_abs_diff<int32_t>();
    test_abs_diff<uint32_t>();
    test_abs_diff<float>();
}

void test_sad() {
    constexpr int n = 16;
    auto a = random_array<uint8_t, n>();
    auto b = random_array<uint8_t, n>();
    auto r = random_array<uint32_t, 4>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = sad(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < 4; i++) {
        uint32_t e = 0;
        if (i % 2 == 0) {
            for (int j = 0; j < 8; j++)
                e += std::abs(a[i * 4 + j] - b[i * 4 + j]);
        }

        ASSERT_EQUAL(r[i], e);
    }
}

void test_sad_16() {
    constexpr int n = 16;
    auto a = random_array<uint8_t, n>();
    auto b = random_array<uint8_t, n>();
    auto r = random_array<uint16_t, 8>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = sad_16(va, vb);

    store(r.data(), vr);

    uint16_t e = 0;
    for (int i = 0; i < n; i++)
        e += (uint16_t)std::abs(a[i] - b[i]);

    uint16_t t = accumulate(begin(r), end(r), (uint16_t)0);
    
    ASSERT_EQUAL(t, e);
}

void test_sad_32() {
    constexpr int n = 16;
    auto a = random_array<uint8_t, n>();
    auto b = random_array<uint8_t, n>();
    auto r = random_array<uint32_t, 4>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = sad_32(va, vb);

    store(r.data(), vr);

    uint32_t e = 0;
    for (int i = 0; i < n; i++)
        e += (uint32_t)std::abs(a[i] - b[i]);

    uint32_t t = accumulate(begin(r), end(r), (uint32_t)0);

    ASSERT_EQUAL(t, e);
}

void test_sad_row() {
    constexpr int n = 16;
    auto a = random_array<uint8_t, n>();
    auto b = random_array<uint8_t, n>();
    auto r = random_array<uint32_t, 4>();

    auto vr = sad_row(a.data(), b.data(), n);

    store(r.data(), vr);

    const uint32_t s = r[0] + r[1] + r[2] + r[3];

    uint32_t e = 0;
    for (int i = 0; i < n; i++)
        e += std::abs(a[i] - b[i]);

    ASSERT_EQUAL(s, e);
}


template<class T>
void test_abs() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vr = abs(va);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], (T)std::abs(a[i]));
    }
}

void multitest_abs() {
    test_abs<int8_t>();
    test_abs<int16_t>();
    test_abs<int32_t>();
    test_abs<float>();
}

template<class T>
void test_abs_saturate() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vr = abs_saturate(va);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], saturate<T>(std::abs((int64_t)a[i])));
    }
}

void multitest_abs_saturate() {
    test_abs_saturate<int8_t>();
    test_abs_saturate<int16_t>();
    test_abs_saturate<int32_t>();
}

template<class T>
void test_neg() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vr = neg(va);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], (T)-a[i]);
    }
}

void multitest_neg() {
    test_neg<int8_t>();
    test_neg<int16_t>();
    test_neg<int32_t>();
    test_neg<float>();
}

template<class T>
void test_neg_saturate() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vr = neg_saturate(va);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], saturate<T>(-(int64_t)a[i]));
    }
}

void multitest_neg_saturate() {
    test_neg_saturate<int8_t>();
    test_neg_saturate<int16_t>();
    test_neg_saturate<int32_t>();
}


template<class T, int tb>
void test_shift_left() {
    constexpr int b = tb % (sizeof(T) * 8);
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vr = shift_left<b>(va);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], T((int64_t)a[i] << b));
    }
}

template<int b>
void multitest1_shift_left() {
    test_shift_left<int8_t, b>();
    test_shift_left<uint8_t, b>();
    test_shift_left<int16_t, b>();
    test_shift_left<uint16_t, b>();
    test_shift_left<int32_t, b>();
    test_shift_left<uint32_t, b>();

    multitest1_shift_left<b - 1>();
}

template<>
void multitest1_shift_left<0>() {
}

void multitest_shift_left() {
    multitest1_shift_left<31>();
}

template<class T>
void test_shift_left_v() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto b = random_array<T, n, 0, sizeof(T) * 8 - 1>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = shift_left(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], T(a[i] << b[i]));
    }
}

void multitest_shift_left_v() {
    test_shift_left_v<int8_t>();
    test_shift_left_v<uint8_t>();
    test_shift_left_v<int16_t>();
    test_shift_left_v<uint16_t>();
    test_shift_left_v<int32_t>();
    test_shift_left_v<uint32_t>();
}

template<class T, int tb>
void test_shift_left_saturate() {
    constexpr int b = tb % (sizeof(T) * 8);
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vr = shift_left_saturate<b>(va);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], saturate<T>((int64_t)a[i] << b));
    }
}

template<int b>
void multitest1_shift_left_saturate() {
    test_shift_left_saturate<int8_t, b>();
    test_shift_left_saturate<uint8_t, b>();
    test_shift_left_saturate<int16_t, b>();
    test_shift_left_saturate<uint16_t, b>();
    
    multitest1_shift_left_saturate<b - 1>();
}

template<>
void multitest1_shift_left_saturate<0>() {
}

void multitest_shift_left_saturate() {
    multitest1_shift_left_saturate<31>();
}

template<class T, int tb>
void test_shift_right() {
    constexpr int b = 1 + tb % (sizeof(T) * 8);
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vr = shift_right<b>(va);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], T((int64_t)a[i] >> b));
    }
}

template<int b>
void multitest1_shift_right() {
    test_shift_right<int8_t, b>();
    test_shift_right<uint8_t, b>();
    test_shift_right<int16_t, b>();
    test_shift_right<uint16_t, b>();
    test_shift_right<int32_t, b>();
    test_shift_right<uint32_t, b>();

    multitest1_shift_right<b - 1>();
}

template<>
void multitest1_shift_right<0>() {
}

void multitest_shift_right() {
    multitest1_shift_right<32>();
}

template<class T>
void test_bitwise_and() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto b = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = bitwise_and(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], T(a[i] & b[i]));
    }
}

void multitest_bitwise_and() {
    test_bitwise_and<int8_t>();
    test_bitwise_and<uint8_t>();
    test_bitwise_and<int16_t>();
    test_bitwise_and<uint16_t>();
    test_bitwise_and<int32_t>();
    test_bitwise_and<uint32_t>();
}

template<class T>
void test_bitwise_or() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto b = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = bitwise_or(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], T(a[i] | b[i]));
    }
}

void multitest_bitwise_or() {
    test_bitwise_or<int8_t>();
    test_bitwise_or<uint8_t>();
    test_bitwise_or<int16_t>();
    test_bitwise_or<uint16_t>();
    test_bitwise_or<int32_t>();
    test_bitwise_or<uint32_t>();
}

template<class T>
void test_bitwise_not() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vr = bitwise_not(va);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], T(~a[i]));
    }
}

void multitest_bitwise_not() {
    test_bitwise_not<int8_t>();
    test_bitwise_not<uint8_t>();
    test_bitwise_not<int16_t>();
    test_bitwise_not<uint16_t>();
    test_bitwise_not<int32_t>();
    test_bitwise_not<uint32_t>();
}

template<class T>
void test_bitwise_and_not() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto b = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = bitwise_and_not(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], T(a[i] & ~b[i]));
    }
}

void multitest_bitwise_and_not() {
    test_bitwise_and_not<int8_t>();
    test_bitwise_and_not<uint8_t>();
    test_bitwise_and_not<int16_t>();
    test_bitwise_and_not<uint16_t>();
    test_bitwise_and_not<int32_t>();
    test_bitwise_and_not<uint32_t>();
}

template<class T>
void test_bitwise_xor() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto b = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = bitwise_xor(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], T(a[i] ^ b[i]));
    }
}

void multitest_bitwise_xor() {
    test_bitwise_xor<int8_t>();
    test_bitwise_xor<uint8_t>();
    test_bitwise_xor<int16_t>();
    test_bitwise_xor<uint16_t>();
    test_bitwise_xor<int32_t>();
    test_bitwise_xor<uint32_t>();
}

template<class T>
void test_bitwise_or_not() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto b = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = bitwise_or_not(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], T(a[i] | ~b[i]));
    }
}

void multitest_bitwise_or_not() {
    test_bitwise_or_not<int8_t>();
    test_bitwise_or_not<uint8_t>();
    test_bitwise_or_not<int16_t>();
    test_bitwise_or_not<uint16_t>();
    test_bitwise_or_not<int32_t>();
    test_bitwise_or_not<uint32_t>();
}


template<class T>
void test_select() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<typename mask_t<T>::type, n>();

    for (auto& x : a)
        x = x % 2 ? all_ones<typename mask_t<T>::type>() : 0;

    auto b = random_array<T, n>();
    auto c = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vc = load(c.data());
    auto vr = select(va, vb, vc);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], a[i] ? b[i] : c[i]);
    }
}

void multitest_select() {
    test_select<int8_t>();
    test_select<uint8_t>();
    test_select<int16_t>();
    test_select<uint16_t>();
    test_select<int32_t>();
    test_select<uint32_t>();
    test_select<float>();
}

template<class T>
void test_mul_lo() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto b = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = mul_lo(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], T(a[i] * b[i]));
    }
}

void multitest_mul_lo() {
    test_mul_lo<int8_t>();
    test_mul_lo<uint8_t>();
    test_mul_lo<int16_t>();
    test_mul_lo<uint16_t>();
    test_mul_lo<int32_t>();
    test_mul_lo<uint32_t>();
}

template<class T>
void test_mul_hi() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto b = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = mul_hi(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], T(((int64_t)a[i] * (int64_t)b[i]) >> (sizeof(T) * 8)));
    }
}

void multitest_mul_hi() {
    test_mul_hi<int16_t>();
    test_mul_hi<uint16_t>();
}

void test_mul_hi_x2_round_saturate() {
    constexpr int n = 16 / sizeof(int16_t);
    auto a = random_array<int16_t, n>();
    auto b = random_array<int16_t, n>();
    auto r = random_array<int16_t, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = mul_hi_x2_round_saturate(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], int16_t((((a[i] * b[i]) >> 14) + 1) >> 1));
    }
}

void test_mul() {
    constexpr int n = 16 / sizeof(float);
    auto a = random_array<float, n>();
    auto b = random_array<float, n>();
    auto r = random_array<float, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = mul(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], a[i] * b[i]);
    }
}

template<class T>
void test_mul_acc() {
    constexpr int n = 16 / sizeof(T);
    auto s = random_array<T, n>();
    auto a = random_array<T, n>();
    auto b = random_array<T, n>();
    auto r = random_array<T, n>();

    auto vs = load(s.data());
    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = mul_acc(vs, va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], T(s[i] + a[i] * b[i]));
    }
}

void multitest_mul_acc() {
    test_mul_acc<int8_t>();
    test_mul_acc<uint8_t>();
    test_mul_acc<int16_t>();
    test_mul_acc<uint16_t>();
    test_mul_acc<int32_t>();
    test_mul_acc<uint32_t>();
    test_mul_acc<float>();
}

template<class T>
void test_mul_sub() {
    constexpr int n = 16 / sizeof(T);
    auto s = random_array<T, n>();
    auto a = random_array<T, n>();
    auto b = random_array<T, n>();
    auto r = random_array<T, n>();

    auto vs = load(s.data());
    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = mul_sub(vs, va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], T(s[i] - a[i] * b[i]));
    }
}

void multitest_mul_sub() {
    test_mul_sub<int8_t>();
    test_mul_sub<uint8_t>();
    test_mul_sub<int16_t>();
    test_mul_sub<uint16_t>();
    test_mul_sub<int32_t>();
    test_mul_sub<uint32_t>();
    test_mul_sub<float>();
}

template<class T>
void test_avg() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto b = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = avg(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        int64_t s = (int64_t)a[i] + (int64_t)b[i];
        T e = T((s + 1) >> 1);
        ASSERT_EQUAL(r[i], e);
    }
}

void multitest_avg() {
    test_avg<int8_t>();
    test_avg<uint8_t>();
    test_avg<int16_t>();
    test_avg<uint16_t>();
    test_avg<int32_t>();
    test_avg<uint32_t>();
}

void test_rsqrt() {
    constexpr int n = 16 / sizeof(float);
    auto a = random_array<float, n>();
    auto r = random_array<float, n>();

    // sqrt is undefined for negative numbers
    for (float& f : a)
        f = abs(f);

    auto va = load(a.data());
    auto vr = rsqrt(va);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        float e = 1.f / sqrt(a[i]);
        float abs_err = abs(r[i] - e);
        float err = abs(e) > 0.f ? abs_err / abs(e) : abs_err;
        ASSERT_TRUE( err < 0.00037f);
    }
}

void test_reciprocal() {
    constexpr int n = 16 / sizeof(float);
    auto a = random_array<float, n>();
    auto r = random_array<float, n>();

    auto va = load(a.data());
    auto vr = reciprocal(va);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        float e = 1.f / a[i];
        float abs_err = abs(r[i] - e);
        float err = abs(e) > 0.f ? abs_err / abs(e) : abs_err;
        ASSERT_TRUE(err < 0.00037f);
    }
}

void test_sqrt() {
    constexpr int n = 16 / sizeof(float);
    auto a = random_array<float, n>();
    auto r = random_array<float, n>();

    // sqrt is undefined for negative numbers
    for (float& f : a)
        f = abs(f);

    auto va = load(a.data());
    auto vr = sqrt(va);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        float e = sqrt(a[i]);
        float abs_err = abs(r[i] - e);
        float err = abs(e) > 0.f ? abs_err / abs(e) : abs_err;
        ASSERT_TRUE(err < 0.00037f);
    }
}

void test_div() {
    constexpr int n = 16 / sizeof(float);
    auto a = random_array<float, n>();
    auto b = random_array<float, n>();
    auto r = random_array<float, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = div(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        float e = a[i] / b[i];
        float abs_err = abs(r[i] - e);
        float err = abs(e) > 0.f ? abs_err / abs(e) : abs_err;
        ASSERT_TRUE(err < 0.00037f);
    }
}

void test_look_up() {
    auto a = random_array<uint8_t, 256>();
    auto b = random_array<uint8_t, 16>();
    auto r = random_array<uint8_t, 16>();

    std::array<uint8x16, 16> va;
    for (size_t i = 0; i < va.size(); i++)
        va[i] = load(a.data() + i * 16);

    auto vb = load(b.data());

    auto vr = look_up(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < 16; i++) {
        ASSERT_EQUAL(r[i], a[b[i]]);
    }
}

template<class T>
void test_clz () {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vr = clz(va);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], c4::clz(a[i]));
    }
}

void multitest_clz() {
    test_clz<int8_t>();
    test_clz<uint8_t>();
    test_clz<int16_t>();
    test_clz<uint16_t>();
    test_clz<int32_t>();
    test_clz<uint32_t>();
}



// ======================================================= MAIN =================================================================

int main()
{
    try {
        const int n_steps = 1000;

        for (int k = 0; k < n_steps; k++) {
            multitest_greater();
            multitest_less();
            multitest_greater_equal();
            multitest_less_equal();
            multitest_equal();
            multitest_min();
            multitest_max();
            multitest_interleave();
            multitest_deinterleave();
            multitest_long_move();
            multitest_narrow();
            multitest_narrow_saturate();
            multitest_narrow_unsigned_saturate();
            multitest_get_low();
            multitest_get_high();
            multitest_combine();
            multitest_load_2_interleaved();
            multitest_load_3_interleaved();
            multitest_load_2_interleaved_long();
            multitest_load_3_interleaved_long();
            multitest_load_4_interleaved();
            multitest_load_4_interleaved_long();
            multitest_store_2_interleaved();
            multitest_store_3_interleaved();
            multitest_store_4_interleaved();
            multitest_store_3_interleaved_narrow_saturate();
            multitest_store_3_interleaved_narrow_unsigned_saturate();
            multitest_store_4_interleaved_narrow_saturate();
            test_to_float();
            test_to_int();
            multitest_add();
            multitest_add_saturate();
            multitest_hadd_long();
            multitest_hadd();
            multitest_sub();
            multitest_sub_div2();
            multitest_abs_diff();
            test_sad();
            test_sad_16();
            test_sad_32();
            test_sad_row();
            multitest_abs();
            multitest_abs_saturate();
            multitest_neg();
            multitest_neg_saturate();
            multitest_shift_left();
            multitest_shift_left_v();
            multitest_shift_left_saturate();
            multitest_shift_right();
            multitest_bitwise_and();
            multitest_bitwise_or();
            multitest_bitwise_not();
            multitest_bitwise_and_not();
            multitest_bitwise_xor();
            multitest_bitwise_or_not();
            multitest_select();
            multitest_mul_lo();
            multitest_mul_hi();
            test_mul_hi_x2_round_saturate();
            test_mul();
            multitest_mul_acc();
            multitest_mul_sub();
            multitest_avg();
            test_rsqrt();
            test_reciprocal();
            test_sqrt();
            test_div();
            test_look_up();
            multitest_clz();
        }

        cout << "All tests passed OK" << endl;
    }
    catch (std::exception& e) {
        cout << e.what() << endl;
    }

    return 0;
}


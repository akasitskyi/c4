#include <c4/simd.hpp>

#include <array>
#include <vector>
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

template<class T>
T all_ones() {
    T t;
    memset(&t, -1, sizeof(t));
    return t;
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
void test_cmpgt() {
    constexpr int n = 16 / sizeof(T);
    auto a = random_array<T, n>();
    auto b = random_array<T, n>();
    auto r = random_array<T, n>();

    auto va = load(a.data());
    auto vb = load(b.data());
    auto vr = cmpgt(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], a[i] > b[i] ? all_ones<T>() : 0);
    }
}

void multitest_cmpgt() {
    test_cmpgt<int8_t>();
    test_cmpgt<uint8_t>();
    test_cmpgt<int16_t>();
    test_cmpgt<uint16_t>();
    test_cmpgt<int32_t>();
    test_cmpgt<uint32_t>();
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
    test_mul_hi<int32_t>();
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

void test_mul_16() {
    constexpr int n = 16 / sizeof(int32_t);
    auto a = random_array<int16_t, n>();
    auto b = random_array<int16_t, n>();
    auto r = random_array<int32_t, n>();

    auto va = load_long(a.data());
    auto vb = load_long(b.data());
    auto vr = mul_16(va, vb);

    store(r.data(), vr);

    for (int i = 0; i < n; i++) {
        ASSERT_EQUAL(r[i], (int32_t)a[i] * (int32_t)b[i]);
    }
}


// ======================================================= MAIN =================================================================

int main()
{
    try {
        const int n_steps = 1000;

        for (int k = 0; k < n_steps; k++) {
            multitest_cmpgt();
            multitest_min();
            multitest_max();
            multitest_interleave();
            multitest_deinterleave();
            // TODO: long_move
            // TODO: narrow
            // TODO: narrow_saturate
            // TODO: narrow_unsigned_saturate
            // TODO: get_low
            // TODO: get_high
            // TODO: extend
            // TODO: combine
            // TODO: load_2_interleaved
            // TODO: load_2_interleaved_long
            // TODO: load_3_interleaved_long
            // TODO: load_4_interleaved
            // TODO: load_long
            // TODO: store_2_interleaved
            // TODO: store_4_interleaved
            // TODO: store_3_interleaved_narrow_saturate
            // TODO: store_4_interleaved_narrow_saturate
            test_to_float();
            test_to_int();
            multitest_add();
            // TODO: hadd
            multitest_sub();
            multitest_sub_div2();
            // TODO: shift_left
            // TODO: shift_left_saturate
            // TODO: shift_right
            multitest_bitwise_and();
            multitest_bitwise_or();
            multitest_bitwise_not();
            multitest_bitwise_and_not();
            multitest_bitwise_xor();
            multitest_mul_lo();
            multitest_mul_hi();
            test_mul();
            test_mul_16();
            // TODO: mul_acc
            // TODO: mul_sub
            // TODO: mul_16_acc
            // TODO: avg
            // TODO: rsqrt
            // TODO: reciprocal
            // TODO: sqrt
            // TODO: div
            // TODO: look_up
            // TODO: clz
        }
    }
    catch (std::exception& e) {
        cout << e.what() << endl;
    }

    return 0;
}


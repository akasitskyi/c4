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

#include <vector>
#include <cassert>

#include "range.hpp"
#include "geometry.hpp"

namespace c4 {
    template<class T>
    class matrix_ref;

    template<class T>
    class vector_ref {
        friend class matrix_ref<T>;
    private:
        vector_ref(const vector_ref& o) : size_(o.size_), ptr_(o.ptr_) {}
        vector_ref& operator=(const vector_ref&) = delete;

        bool is_inside(int x) const {
            return 0 <= x && x < size_;
        }

    protected:
        int size_;
        T* ptr_;

        vector_ref() : size_(0), ptr_(nullptr) {}

    public:
        vector_ref(int length, T* ptr) : size_(length), ptr_(ptr) {}
        vector_ref(std::vector<T>& v) : size_((int)v.size()), ptr_(v.data()) {
            assert(v.size() == (size_t)size_);
        }

        int size() const {
            return size_;
        }

        T* data() {
            return ptr_;
        }

        const T* data() const {
            return ptr_;
        }

        T* begin() {
            return ptr_;
        }

        const T* begin() const {
            return ptr_;
        }

        T* end() {
            return ptr_ + size_;
        }

        const T* end() const {
            return ptr_ + size_;
        }

        T& operator[](int i) {
            assert(is_inside(i));
            return ptr_[i];
        }

        const T& operator[](int i) const {
            assert(is_inside(i));
            return ptr_[i];
        }

        operator T*() {
            return ptr_;
        }

        operator const T*() const {
            return ptr_;
        }

        template<class U>
        explicit operator U*() {
            return reinterpret_cast<U*>(ptr_);
        }

        template<class U>
        explicit operator const U*() const {
            return reinterpret_cast<const U*>(ptr_);
        }

        static const vector_ref<T> create_const(int size, const T* ptr) {
            return vector_ref<T>(size, const_cast<T*>(ptr));
        }
    };

    struct matrix_dimensions {
        int height;
        int width;

        int area() const {
            return height * width;
        }
    };

    inline bool operator==(const matrix_dimensions& a, const matrix_dimensions& b) {
        return a.height == b.height && a.width == b.width;
    }

    template<class T>
    class matrix_ref {
    private:
        matrix_ref(const matrix_ref& o) : height_(o.height_), width_(o.width_), stride_(o.stride_), ptr_(o.ptr_) {}
        matrix_ref& operator=(const matrix_ref&) = delete;

    protected:
        int height_;
        int width_;
        int stride_;
        T* ptr_;

        matrix_ref() : height_(0), width_(0), stride_(0), ptr_(nullptr) {}

    public:
        matrix_ref(int height, int width, int stride, T* ptr) : height_(height), width_(width), stride_(stride), ptr_(ptr) {}
        matrix_ref(matrix_dimensions dims, int stride, T* ptr) : height_(dims.height), width_(dims.width), stride_(stride), ptr_(ptr) {}

        class iterator {
            friend class matrix_ref<T>;

            matrix_ref<T>& m;
            int i;
            int j;

            iterator(matrix_ref<T>& m, int i, int j) : m(m), i(i), j(j) {}

        public:
            iterator& operator++() {
                if (++j == m.width_) {
                    ++i;
                    j = 0;
                }
                return *this;
            }

            iterator operator++(int) {
                iterator r = *this;
                return ++r;
            }

            T& operator*() {
                return m[i][j];
            }

            bool operator==(iterator other) const {
                return i == other.i && j == other.j;
            }
            
            bool operator!=(iterator other) const {
                return !(*this == other);
            }
        };

        iterator begin() {
            return iterator{ *this, 0, 0 };
        }

        iterator end() {
            return iterator{ *this, height_, 0 };
        }

        class const_iterator {
            friend class matrix_ref<T>;

            const matrix_ref<T>& m;
            int i;
            int j;

            const_iterator(const matrix_ref<T>& m, int i, int j) : m(m), i(i), j(j) {}

        public:
            const_iterator & operator++() {
                if (++i == m.width_) {
                    ++j;
                    i = 0;
                }
                return *this;
            }

            const_iterator operator++(int) {
                iterator r = *this;
                return ++r;
            }

            const T& operator*() {
                return m[i][j];
            }

            bool operator==(const_iterator other) const {
                return i == other.i && j == other.j;
            }

            bool operator!=(const_iterator other) const {
                return !(*this == other);
            }
        };

        const_iterator begin() const {
            return const_iterator{ *this, 0, 0 };
        }

        const_iterator end() const {
            return const_iterator(*this, height_, width_);
        }

        matrix_dimensions dimensions() const {
            return { height_, width_ };
        }

        rectangle<int> rect() const {
            return rectangle<int>(0, 0, width_, height_);
        }

        int height() const {
            return height_;
        }

        int width() const {
            return width_;
        }

        int stride() const {
            return stride_;
        }

        int stride_bytes() const {
            return stride_ * sizeof(T);
        }

        bool is_inside(int y, int x) const {
            return 0 <= y && y < height_ && 0 <= x && x < width_;
        }

        bool is_inside(const rectangle<int>& r) const {
            return 0 <= r.y && r.y + r.h <= height_ && 0 <= r.x && r.x + r.w <= width_;
        }

        T* data() {
            return ptr_;
        }

        const T* data() const {
            return ptr_;
        }

        vector_ref<T> operator[](int i) {
            assert(0 <= i && i < height_);
            return vector_ref<T>(width_, ptr_ + i * stride_);
        }

        const vector_ref<T> operator[](int i) const {
            assert(0 <= i && i < height_);
            return vector_ref<T>::create_const(width_, ptr_ + i * stride_);
        }

        const T& clamp_get(int i, int j) const {
            i = std::max(i, 0);
            i = std::min(i, height_ - 1);

            j = std::max(j, 0);
            j = std::min(j, width_ - 1);

            return operator[](i)[j];
        }

        const T& clamp_get(const point<float>& p) const {
            return clamp_get(int(p.y + 0.5f), int(p.x + 0.5f));
        }

        matrix_ref<T> submatrix(const rectangle<int>& r) {
            assert(is_inside(r));
            return matrix_ref<T>(r.h, r.w, stride_, ptr_ + r.y * stride_ + r.x);
        }

        const matrix_ref<T> submatrix(const rectangle<int>& r) const {
            assert(is_inside(r));
            return matrix_ref<T>(r.h, r.w, stride_, ptr_ + r.y * stride_ + r.x);
        }

        matrix_ref<T> submatrix(int i, int j, int h, int w) {
            return submatrix(rectangle<int>(j, i, w, h));
        }

        const matrix_ref<T> submatrix(int i, int j, int h, int w) const {
            return submatrix(rectangle<int>(j, i, w, h));
        }

        static const matrix_ref<T> create_const(int height, int width, int stride, const T* ptr) {
            return matrix_ref<T>(height, width, stride, const_cast<T*>(ptr));
        }
    };

    namespace detail {
        template<class T>
        class matrix_buffer {
        protected:
            std::vector<T> v;
            matrix_buffer() {}
            matrix_buffer(size_t size) : v(size) {}
            matrix_buffer(size_t size, const T& init) : v(size, init) {}
        };
    };

    template<class T>
    class matrix : private detail::matrix_buffer<T>, public matrix_ref<T> {
    public:
        matrix(int height, int width, int stride) : detail::matrix_buffer<T>(height * stride), matrix_ref<T>(height, width, stride, detail::matrix_buffer<T>::v.data()) {}
        matrix(int height, int width, int stride, const T& init) : detail::matrix_buffer<T>(height * stride, init), matrix_ref<T>(height, width, stride, detail::matrix_buffer<T>::v.data()) {}
        matrix(int height, int width) : matrix(height, width, width) {}
        matrix(int height, int width, const T& init) : matrix(height, width, width, init) {}
        matrix(matrix_dimensions dims, int stride) : matrix(dims.height, dims.width, stride) {}
        matrix(matrix_dimensions dims) : matrix(dims.height, dims.width) {}
        matrix() : matrix(0, 0, 0) {}
        
        matrix& operator=(const matrix& b) {
            resize(b.dimensions());
            std::copy(b.v.begin(), b.v.end(), this->v.begin());

            return *this;
        }

        matrix(const matrix& b) {
            resize(b.dimensions());
            std::copy(b.v.begin(), b.v.end(), this->v.begin());
        }

        matrix& operator=(const matrix_ref<T>& b) {
            resize(b.dimensions());

            for (int i = 0; i < b.height(); i++)
                std::copy(b[i].begin(), b[i].end(), (*this)[i].begin());

            return *this;
        }

        matrix(const matrix_ref<T>& b) {
            resize(b.dimensions());

            for(int i = 0; i < b.height(); i++)
                std::copy(b[i].data(), b[i].data() + b.width(), (*this)[i].data());
        }

        void resize(int height, int width, int stride){
            this->height_ = height;
            this->width_ = width;
            this->stride_ = stride;
            this->v.resize(height * stride);
            this->ptr_ = this->v.data();
        }

        void resize(int height, int width){
            resize(height, width, width);
        }

        void shrink_to_fit(){
            this->v.shrink_to_fit();
        }

        void clear_and_shrink() {
            resize(0, 0);
            this->v.shrink_to_fit();
        }

        void resize(matrix_dimensions dims){
            resize(dims.height, dims.width);
        }

        template <typename Archive>
        void save(Archive& archive) const {
            archive(this->height_, this->width_, this->stride_);
            archive(this->v);
        }

        template <typename Archive>
        void load(Archive& archive) {
            int height, width, stride;
            archive(height, width, stride);
            resize(height, width, stride);
            archive(this->v);
        }
    };

    template<class T1, class T2>
    inline matrix_ref<T1>& operator+=(matrix_ref<T1>& a, const matrix_ref<T2>& b) {
        assert(a.height() == b.height() && a.width() == b.width());

        for (int i = 0; i < a.height(); i++) {
            for (int j = 0; j < a.width(); j++) {
                a[i][j] += b[i][j];
            }
        }

        return a;
    }

    template<class T1, class T2>
    inline matrix<decltype(T1() + T2())> operator+(const matrix_ref<T1>& a, const matrix_ref<T2>& b) {
        assert(a.dimensions() == b.dimensions());

        matrix<decltype(T1() + T2())> r(a.dimensions());

        for (int i = 0; i < a.height(); i++) {
            for (int j = 0; j < a.width(); j++) {
                r[i][j] = a[i][j] + b[i][j];
            }
        }

        return r;
    }

    template<class T1, class T2>
    inline matrix_ref<T1>& operator-=(matrix<T1>& a, const matrix<T2>& b) {
        assert(a.dimensions() == b.dimensions());

        for (int i = 0; i < a.height(); i++) {
            for (int j = 0; j < a.width(); j++) {
                a[i][j] -= b[i][j];
            }
        }

        return a;
    }

    template<class T1, class T2>
    inline matrix<decltype(T1() - T2())> operator-(const matrix_ref<T1>& a, const matrix_ref<T2>& b) {
        assert(a.dimensions() == b.dimensions());

        matrix<decltype(T1() - T2())> r(a.dimensions());

        for (int i = 0; i < a.height(); i++) {
            for (int j = 0; j < a.width(); j++) {
                r[i][j] = a[i][j] - b[i][j];
            }
        }

        return r;
    }

    template<class T1, class T2>
    inline matrix<decltype(T1() * T2())> entrywise_mul(const matrix_ref<T1>& a, const matrix_ref<T2>& b) {
        assert(a.dimensions() == b.dimensions());

        matrix<decltype(T1() * T2())> r(a.dimensions());

        for (int i = 0; i < a.height(); i++) {
            for (int j = 0; j < a.width(); j++) {
                r[i][j] = a[i][j] * b[i][j];
            }
        }

        return r;
    }

    template<class T1, class T2>
    inline matrix<decltype(T1() / T2())> entrywise_divl(const matrix_ref<T1>& a, const matrix_ref<T2>& b) {
        assert(a.dimensions() == b.dimensions());

        matrix<decltype(T1() / T2())> r(a.dimensions());

        for (int i = 0; i < a.height(); i++) {
            for (int j = 0; j < a.width(); j++) {
                r[i][j] = a[i][j] / b[i][j];
            }
        }

        return r;
    }


    template<class T1, class T2, class T3>
    inline c4::matrix<decltype(T1() * T2() + T3())> entrywise_madd(const c4::matrix_ref<T1>& img, T2 alpha, T3 beta) {
        c4::matrix<decltype(T1() * T2() + T3())> res(img.height(), img.width());

        for (int i = 0; i < img.height(); i++) {
            for (int j = 0; j < img.width(); j++) {
                res[i][j] = img[i][j] * alpha + beta;
            }
        }

        return res;
    }

    template<typename T>
    inline void rotate90cw(matrix<T>& mat) {
        matrix<T> src = mat;

        mat.resize(src.width(), src.height());

        for (int i = 0; i < mat.height(); i++) {
            for (int j = 0; j < mat.width(); j++) {
                mat[i][j] = src[src.height() - j - 1][i];
            }
        }
    }

    template<typename T>
    inline void rotate180(matrix<T>& mat) {
        for (int i = 0; i < mat.height() / 2; i++) {
            for (int j = 0; j < mat.width(); j++) {
                std::swap(mat[i][j], mat[mat.height() - i - 1][mat.width() - j - 1]);
            }
        }

        if (mat.height() % 2) {
            for (int j = 0; j < mat.width() / 2; j++) {
                std::swap(mat[mat.height() / 2][j], mat[mat.height() / 2][mat.width() - j - 1]);
            }
        }
    }

    template<typename T>
    inline void rotate270cw(matrix<T>& mat) {
        matrix<T> src = mat;

        mat.resize(src.width(), src.height());

        for (int i = 0; i < mat.height(); i++) {
            for (int j = 0; j < mat.width(); j++) {
                mat[i][j] = src[j][src.width() - i - 1];
            }
        }
    }

    template<typename T>
    inline void flip_vertical(matrix_ref<T>& mat) {
        for (int i = 0; i < mat.height() / 2; i++) {
            for (int j = 0; j < mat.width(); j++) {
                std::swap(mat[i][j], mat[mat.height() - i - 1][j]);
            }
        }
    }

    template<class T1, class T2, class F>
    inline void transform(const vector_ref<T1>& src, vector_ref<T2>& dst, F f) {
        assert(src.size() == dst.size());

        for (int i : range(src.size()))
            dst[i] = f(src[i]);
    }

    template<class T1, class T2, class F>
    inline void transform(const vector_ref<T1>& src, vector_ref<T2>&& dst, F f) {
        transform(src, dst, f);
    }

    template<class T1, class T2, class F>
    inline void transform(const matrix_ref<T1>& src, matrix_ref<T2>& dst, F f) {
        assert(src.height() == dst.height());

        for (int i : range(src.height()))
            transform(src[i], dst[i], f);
    }

    template<class T1, class T2, class F>
    inline void transform(const matrix_ref<T1>& src, matrix_ref<T2>&& dst, F f) {
        transform(src, dst, f);
    }

    template<class T, class F>
    inline auto transform(const matrix_ref<T>& src, F f) -> matrix<typename std::result_of<F(T)>::type> {
        matrix<typename std::result_of<F(T)>::type> dst(src.dimensions());

        transform(src, dst, f);

        return dst;
    }

    template<class T, class F>
    inline void transform_inplace(matrix_ref<T>& src, F f) {
        transform(src, src, f);
    }

    template<class T1, class T2, class T3, class F>
    inline void transform(const vector_ref<T1>& src_a, const vector_ref<T2>& src_b, vector_ref<T2>& dst, F f) {
        assert(src_a.size() == src_b.size() && src_a.size() == dst.size());

        for (int i : range(dst.size()))
            dst[i] = f(src_a[i], src_b[i]);
    }

    template<class T1, class T2, class T3, class F>
    inline void transform(const matrix_ref<T1>& src_a, const matrix_ref<T1>& src_b, matrix_ref<T2>& dst, F f) {
        assert(src_a.dimensions() == src_b.dimensions() && src_a.dimensions() == dst.dimensions());

        for (int i : range(dst.height()))
            transform(src_a[i], src_b[i], dst[i], f);
    }

    template<class T1, class T2, class F>
    inline auto transform(const matrix_ref<T1>& src_a, const matrix_ref<T2>& src_b, F f) -> matrix<typename std::result_of<F(T1(), T2())>::type> {
        assert(src_a.dimensions() == src_b.dimensions());

        matrix<typename std::result_of<F(T1(), T2())>::type> dst(src_a.dimensions());

        transform(src_a, src_b, dst, f);

        return dst;
    }
};

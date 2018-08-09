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

#include "math.hpp"
#include "simd.hpp"
#include "range.hpp"
#include "pixel.hpp"
#include "matrix.hpp"

namespace c4 {

    void bilateral_filter(c4::matrix_ref<c4::pixel<uint8_t>>& dst, float sd, float sr, c4::rgb_weights rgbWeights) {
        const int wr = int(rgbWeights.wR() * 255);
        const int wg = int(rgbWeights.wG() * 255);
        const int wb = int(rgbWeights.wB() * 255);

        using namespace c4::simd;

        const uint16x8 vwr(wr);
        const uint16x8 vwg(wg);
        const uint16x8 vwb(wb);

        const int r = int(std::sqrt(3.f) * sd);
        const int colorThreshold = int(sr * std::sqrt(6.f) * 255);

        const uint16x8 v_color_threshold(colorThreshold);


        c4::matrix<uint16_t> src_r(dst.height(), dst.width() + 2 * r);
        c4::matrix<uint16_t> src_g(dst.height(), dst.width() + 2 * r);
        c4::matrix<uint16_t> src_b(dst.height(), dst.width() + 2 * r);

        for (int i : c4::range(dst.height())) {
            // repack
            int j = 0;
            for (; j + 8 < dst.width(); j += 8) {
                const uint16x8x3 p = load_3_interleaved_long((uint8_t*)&dst[i][j]);

                store(&src_r[i][r + j], p.val[0]);
                store(&src_g[i][r + j], p.val[1]);
                store(&src_b[i][r + j], p.val[2]);
            }

            for (; j < dst.width(); j++) {
                src_r[i][r + j] = dst[i][j].r;
                src_g[i][r + j] = dst[i][j].g;
                src_b[i][r + j] = dst[i][j].b;
            }

            // clamp to edges
            std::fill(src_r[i].begin(), src_r[i].begin() + r, dst[i][0].r);
            std::fill(src_g[i].begin(), src_g[i].begin() + r, dst[i][0].g);
            std::fill(src_b[i].begin(), src_b[i].begin() + r, dst[i][0].b);

            std::fill(src_r[i].end() - r, src_r[i].end(), dst[i][dst.width() - 1].r);
            std::fill(src_g[i].end() - r, src_g[i].end(), dst[i][dst.width() - 1].g);
            std::fill(src_b[i].end() - r, src_b[i].end(), dst[i][dst.width() - 1].b);
        }

        for(int i : c4::range(dst.height())) {
            const uint16_t* psrc0_r = src_r[i].data();
            const uint16_t* psrc0_g = src_g[i].data();
            const uint16_t* psrc0_b = src_b[i].data();

            const int i0 = std::max<int>(i - r, 0);
            const int i1 = std::min<int>(i + r + 1, src_r.height());

            int j = 0;
            for (; j + 8 < dst.width(); j += 8) {
                const int j0 = r + j - r;
                const int j1 = r + j + r + 1;

                uint32x4x2 nom_r{ uint32x4{ 0 }, uint32x4{ 0 } };
                uint32x4x2 nom_g{ uint32x4{ 0 }, uint32x4{ 0 } };
                uint32x4x2 nom_b{ uint32x4{ 0 }, uint32x4{ 0 } };

                uint32x4x2 denom{ uint32x4{ 0 }, uint32x4{ 0 } };

                const uint16x8 p0_r = load(psrc0_r + j + r);
                const uint16x8 p0_g = load(psrc0_g + j + r);
                const uint16x8 p0_b = load(psrc0_b + j + r);

                for (int ii : c4::range(i0, i1)) {
                    const uint16_t* psrc1_r = src_r[ii].data();
                    const uint16_t* psrc1_g = src_g[ii].data();
                    const uint16_t* psrc1_b = src_b[ii].data();

                    for (int jj : c4::range(j0, j1)) {
                        const uint16x8 p_r = load(psrc1_r + jj);
                        const uint16x8 p_g = load(psrc1_g + jj);
                        const uint16x8 p_b = load(psrc1_b + jj);

                        uint16x8 abs_diff_r = abs_diff(p_r, p0_r);
                        uint16x8 abs_diff_g = abs_diff(p_g, p0_g);
                        uint16x8 abs_diff_b = abs_diff(p_b, p0_b);

                        uint16x8 d = mul_lo(abs_diff_r, vwr);
                        d = mul_acc(d, abs_diff_g, vwg);
                        d = mul_acc(d, abs_diff_b, vwb);

                        uint16x8 w = sub_saturate(v_color_threshold, d);

                        uint32x4x2 rw = mul_long(p_r, w);
                        uint32x4x2 gw = mul_long(p_g, w);
                        uint32x4x2 bw = mul_long(p_b, w);

                        nom_r = add(nom_r, rw);
                        nom_g = add(nom_g, gw);
                        nom_b = add(nom_b, bw);

                        denom = add(denom, long_move(w));
                    }
                }

                float32x4x2 inv_denom = reciprocal(to_float(reinterpret_signed(denom)));

                float32x4x2 r = mul(to_float(reinterpret_signed(nom_r)), inv_denom);
                float32x4x2 g = mul(to_float(reinterpret_signed(nom_g)), inv_denom);
                float32x4x2 b = mul(to_float(reinterpret_signed(nom_b)), inv_denom);

                int16x8x3 dp;
                dp.val[0] = narrow(round_to_int(r));
                dp.val[1] = narrow(round_to_int(g));
                dp.val[2] = narrow(round_to_int(b));

                store_3_interleaved_narrow_unsigned_saturate((uint8_t*)(dst[i].data() + j), dp);
            }

            for (; j < dst.width(); j++) {
                int j0 = r + j - r;
                int j1 = r + j + r + 1;

                c4::pixel<int> nom(0);
                int denom = 0;

                const int p0_r = src_r[i][j + r];
                const int p0_g = src_g[i][j + r];
                const int p0_b = src_b[i][j + r];

                for(int ii : c4::range(i0, i1)) {
                    for(int jj : c4::range(j0, j1)) {
                        const int p_r = src_r[ii][jj];
                        const int p_g = src_g[ii][jj];
                        const int p_b = src_b[ii][jj];

                        int d = std::abs(p_r - p0_r) * wr + std::abs(p_g - p0_g) * wg + std::abs(p_b - p0_b) * wb;
                        int w = colorThreshold - d;
                        w &= ~(w >> 31);// w = max(w, 0)

                        nom.r += p_r * w;
                        nom.g += p_g * w;
                        nom.b += p_b * w;
                        denom += w;
                    }
                }

                int hd = denom / 2;

                dst[i][j].r = (nom.r + hd) / denom;
                dst[i][j].g = (nom.g + hd) / denom;
                dst[i][j].b = (nom.b + hd) / denom;
            }
        }
    }

};

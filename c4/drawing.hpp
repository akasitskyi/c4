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

#include "matrix.hpp"
#include "range.hpp"
#include "pixel.hpp"
#include "geometry.hpp"

namespace c4 {
    template<class pixel_t, class coord_t>
    inline void draw_line(matrix_ref<pixel_t>& img, coord_t x0, coord_t y0, coord_t x1, coord_t y1, pixel_t color, int thickness = 1){
        int T = (int)std::max(abs(x0 - x1), abs(y0 - y1));

        for(int t : range(T)){
            int x = int(x0 + t * (x1 - x0) / T);
            int y = int(y0 + t * (y1 - y0) / T);
            
            for (int dx : range(-thickness / 2, thickness - thickness / 2)) {
                for (int dy : range(-thickness / 2, thickness - thickness / 2)) {
                    if (img.is_inside(y + dy, x + dx)) {
                        img[y + dy][x + dx] = color;
                    }
                }
            }
        }
    }

    template<class pixel_t>
    inline void draw_line(matrix_ref<pixel_t>& img, c4::point<double> p0, c4::point<double> p1, pixel_t color, int thickness = 1){
        draw_line(img, p0.x, p0.y, p1.x, p1.y, color, thickness);
    }

    template<class pixel_t>
    inline void draw_arc(matrix_ref<pixel_t>& img, c4::point<double> center, float R, float alpha0, float alpha1, pixel_t color, int thickness = 1){
        int T = int(2 * R);

        for(int t : range(T)){
            float alpha = alpha0 + t * (alpha1 - alpha0) / T;
            int x = int(center.x + R * cos(alpha));
            int y = int(center.y + R * sin(alpha));
            
            for(int dx : range(-thickness / 2, thickness - thickness / 2))
                for(int dy : range(-thickness / 2, thickness - thickness / 2))
                    img[y + dy][x + dx] = color;
        }
    }

    template<class pixel_t>
    inline void draw_arc(matrix_ref<pixel_t>& img, c4::point<double> center, c4::point<double> p0, c4::point<double> p1, c4::pixel<uint8_t> color, int thickness = 1){
        double R = (dist(center, p0) + dist(center, p1)) / 2;
        double alpha0 = (p0 - center).polar_angle();
        double alpha1 = (p1 - center).polar_angle();

        if( alpha1 < alpha0 )
            alpha1 += 2 * std::numbers::pi_v<double>;

        if( alpha1 < alpha0 || alpha1 - alpha0 > 2 * std::numbers::pi_v<double> )
            THROW_EXCEPTION("Something's wrong with arc drawing");

        draw_arc(img, center, (float)R, (float)alpha0, (float)alpha1, color, thickness);
    }

    template<class pixel_t>
    inline void draw_rect(matrix_ref<pixel_t>& img, rectangle<int> r, pixel_t color, int thickness = 1){
        int y0 = std::max(r.y, thickness / 2);
        int x0 = std::max(r.x, thickness / 2);

        int y1 = std::min(r.y + r.h - 1, img.height() - (thickness - thickness / 2));
        int x1 = std::min(r.x + r.w - 1, img.width() - (thickness - thickness / 2));
        
        for(int d : range(-thickness / 2, thickness - thickness / 2)) {
            for(int y : range(y0, y1 + 1)) {
                img[y][x0 + d] = color;
                img[y][x1 + d] = color;
            }
            for(int x : range(x0, x1 + 1)) {
                img[y0 + d][x] = color;
                img[y1 + d][x] = color;
            }
        }
    }

    template<class pixel_t>
    inline void draw_point(matrix_ref<pixel_t>& img, int y0, int x0, pixel_t color, int thickness = 1){
        for (int d : c4::range(-thickness / 2, thickness - thickness / 2)) {
            if( img.is_inside(y0, x0 + d) )
                img[y0][x0 + d] = color;
            if( img.is_inside(y0 + d, x0) )
                img[y0 + d][x0] = color;
        }
    }

    static const int DRAW_CHAR_DIM = 8;

    template<class pixel_t>
    inline void draw_char(matrix_ref<pixel_t>& img, int x0, int y0, char c, pixel_t fg_color, pixel_t bg_color, int scale = 1){
        static const int offset = 32;
        static const int cnt = 128 - offset;

        static const uint64_t chars[cnt]{
            0ULL, 1736164147711186944ULL, 7378697189679169536ULL, 7853932798879362048ULL, 9150980724243758080ULL, 14325418493450044928ULL,
            9006285520967204352ULL, 1736164044630392832ULL, 2034554115190430720ULL, 4052127010610755584ULL, 1790983498433441792ULL, 6782331001509888ULL,
            812675072ULL, 543279808512ULL, 3158016ULL, 436317242639040512ULL, 9150970118627326976ULL, 1763291299001040384ULL, 18230016032871742976ULL,
            18230015479995366400ULL, 14323354222882326016ULL, 18374335726989802496ULL, 18374335730211027968ULL, 18374130152658830848ULL,
            18374341768935046656ULL, 18374342324059569152ULL, 6781788124348416ULL, 6781788126459904ULL, 436317242641022464ULL, 35604385538932224ULL,
            13898231836372303360ULL, 18230015205003769856ULL, 9150970152886270976ULL, 9150970290422334976ULL, 18230227139204939264ULL, 9150963426967617024ULL,
            18374342086762626048ULL, 18374335721520430592ULL, 18374335464828878848ULL, 9150963555917299200ULL, 14323354222882375168ULL, 9114749187613687296ULL,
            434041040265969152ULL, 14323354222848820736ULL, 13889313184914800128ULL, 17221438243326051840ULL, 17795647195817369088ULL, 9150970049907850752ULL,
            18374342087702003712ULL, 9150970050008514048ULL, 18374342087668581888ULL, 9150963690135027200ULL, 9114749187606976512ULL, 14323354221942865408ULL,
            14323354256671305728ULL, 14327875482744188416ULL, 14323354220734760448ULL, 14323415482135149568ULL, 18374130662601850368ULL,
            4340397124404132864ULL, 13898231836356511232ULL, 4340357386762730496ULL, 1168821468277112832ULL, 65024ULL, 3474553500795404288ULL,
            139393143064064ULL, 277931608178176ULL, 139366332694016ULL, 280129691909120ULL, 280104827682304ULL, 280103824703488ULL, 139366836403712ULL,
            218557044409856ULL, 138641948573184ULL, 6622953242112ULL, 218557044278784ULL, 211934100127232ULL, 262778293503488ULL, 271402587840000ULL,
            139392203554304ULL, 280129695563776ULL, 280129692302848ULL, 280129695434240ULL, 139367360691712ULL, 138641948547072ULL, 218557040754176ULL,
            218557142593536ULL, 218626032330240ULL, 218557035890176ULL, 218555820211200ULL, 279303816281600ULL, 4340397330562563072ULL, 1736164148113840128ULL,
            4340357360992926720ULL, 106595149938688ULL, 0ULL };

        if( c < offset || c - offset >= cnt)
            THROW_EXCEPTION(std::string("Char not defined: ") + c);

        ASSERT_TRUE(scale >= 1);

        const int scaledDim = DRAW_CHAR_DIM * scale;

        if (y0 < 0 || y0 + scaledDim > img.height() || x0 < 0 || x0 + scaledDim > img.width())
            return;

        const auto mask = chars[c - offset];

        for (int i : range(scaledDim)) {
            for (int j : range(scaledDim)) {
                img[y0 + i][x0 + j] = ((mask >> (63 - (i / scale * DRAW_CHAR_DIM + j / scale))) & 1) ? fg_color : bg_color;
            }
        }
    }

    template<class pixel_t>
    inline void draw_string(matrix_ref<pixel_t>& img, int x0, int y0, const std::string& s, pixel_t fg_color, pixel_t bg_color, int scale = 1){
        const int scaledDim = DRAW_CHAR_DIM * scale;

        for(int k : range(s))
            draw_char(img, x0 + k * scaledDim, y0, s[k], fg_color, bg_color, scale);
    }
};

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

namespace c4 {
    template<class pixel_t, class coord_t>
    void draw_line(matrix_ref<pixel_t>& img, coord_t x0, coord_t y0, coord_t x1, coord_t y1, pixel_t color, int thickness = 1){
        int T = (int)max(abs(x0 - x1), abs(y0 - y1));

        for(int t : range(T)){
            int x = int(x0 + t * (x1 - x0) / T);
            int y = int(y0 + t * (y1 - y0) / T);
            
            for(int dx : range(-thickness / 2, thickness - thickness / 2))
                for(int dy : range(-thickness / 2, thickness - thickness / 2))
                    img[y + dy][x + dx] = color;
        }
    }

    template<class pixel_t>
    void draw_line(matrix_ref<pixel_t>& img, c4::point<double> p0, c4::point<double> p1, pixel_t color, int thickness = 1){
        draw_line(img, p0.x, p0.y, p1.x, p1.y, color, thickness);
    }

    template<class pixel_t>
    void draw_arc(matrix_ref<pixel_t>& img, c4::point<double> center, float R, float alpha0, float alpha1, pixel_t color, int thickness = 1){
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
    void draw_arc(matrix_ref<pixel_t>& img, c4::point<double> center, c4::point<double> p0, c4::point<double> p1, c4::pixel<uint8_t> color, int thickness = 1){
        double R = (dist(center, p0) + dist(center, p1)) / 2;
        double alpha0 = (p0 - center).polar_angle();
        double alpha1 = (p1 - center).polar_angle();

        if( alpha1 < alpha0 )
            alpha1 += 2 * c4::pi<double>();

        if( alpha1 < alpha0 || alpha1 - alpha0 > 2 * c4::pi<double>() )
            THROW_EXCEPTION("Something's wrong with arc drawing");

        draw_arc(img, center, (float)R, (float)alpha0, (float)alpha1, color, thickness);
    }

    template<class pixel_t>
    void draw_rect(matrix_ref<pixel_t>& img, int x0, int y0, int width, int height, pixel_t color, int thickness = 1){
        y0 = max(y0, thickness / 2);
        x0 = max(x0, thickness / 2);

        int y1 = min(y0 + height - 1, img.height() - (thickness - thickness / 2));
        int x1 = min(x0 + width - 1, img.width() - (thickness - thickness / 2));
        
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
    void draw_point(matrix_ref<pixel_t>& img, int y0, int x0, pixel_t color, int thickness = 1){
        FOR(d, -thickness / 2, thickness - thickness / 2){
            if( img.is_inside(y0, x0 + d) )
                img[y0][x0 + d] = color;
            if( img.is_inside(y0 + d, x0) )
                img[y0 + d][x0] = color;
        }
    }

    template<class pixel_t>
    void draw_digit(matrix_ref<pixel_t>& img, int x0, int y0, int d, pixel_t fg_color, pixel_t bg_color){
        static const string digits[10] = {
            "......####"
            "..###..###"
            ".####..###"
            ".####..###"
            ".####..###"
            ".####..###"
            ".####..###"
            ".####..###"
            "..###..###"
            "......####",

            "....######"
            "###..#####"
            "###..#####"
            "###..#####"
            "###..#####"
            "###..#####"
            "###.##.###"
            "###.##.###"
            "###..#.###"
            ".......###",

            "#......###"
            "######..##"
            "######..##"
            "######..##"
            "##......##"
            "#......###"
            "#.########"
            "#.########"
            "#.########"
            "#......###",

            "#.......##"
            "######..##"
            "######..##"
            "######..##"
            "###.....##"
            "###.....##"
            "######..##"
            "######..##"
            "######..##"
            "#......###",

            "##.#######"
            "##.###.###"
            "##.###.###"
            "##.###.###"
            "##.###.###"
            "##......##"
            "##......##"
            "######.###"
            "######.###"
            "######.###",

            "###.....##"
            "###..#####"
            "###..#####"
            "###..#####"
            "###.....##"
            "###......#"
            "#######..#"
            "#######..#"
            "##..###..#"
            "##......##",

            "##..######"
            "##..######"
            "##..######"
            "##..######"
            "##..######"
            "##......##"
            "##.......#"
            "##..###..#"
            "##..###..#"
            "##.......#",

            "###......#"
            "###.####.#"
            "########.#"
            "########.#"
            "#######..#"
            "######..##"
            "#####..###"
            "#####..###"
            "#####..###"
            "#####..###",

            "####....##"
            "####..#.##"
            "####..#.##"
            "####..#.##"
            "####.....#"
            "###......."
            "###.####.."
            "###.####.."
            "###..###.."
            "###......#",

            "###......."
            "###..###.."
            "###..####."
            "###......."
            "####......"
            "#########."
            "#########."
            "#########."
            "########.."
            "########.."
        };

        if( d < 0 || d > 9 )
            THROW_EXCEPTION("Not a digit");

        if (y0 < 0 || y0 + 10 > image.height() || x0 < 0 || x0 + 10 > image.width())
            return;

        for(int i : range(10))
            for(int j : range(10))
                if( digits[d][i * 10 + j] == '.' )
                    image[y0 + i][x0 + j] = fg_color;
                else
                    image[y0 + i][x0 + j] = bg_color;
    }

    template<class pixel_t>
    void draw_number(matrix_ref<pixel_t>& img, int x0, int y0, int d, pixel_t fg_color, pixel_t bg_color){
        std::string s = std::to_string(d);

        for(int k : range(s))
            draw_digit(image, x0 + k * 10, y0, int(s[k] - '0'), fg_color, bg_color);
    }
};

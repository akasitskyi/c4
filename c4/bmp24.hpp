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
#include <cstdint>
#include <ostream>
#include <istream>
#include <fstream>

#include "matrix.hpp"
#include "pixel.hpp"

namespace c4 {
    namespace detail {
        inline void write_binary(std::ostream& out, uint8_t t) {
            out.write((const char*)&t, sizeof(t));
        }

        template<class T>
        inline void write_binary(std::ostream& out, T t) {
            uint8_t v[sizeof(T)];
            for (int i = 0; i < sizeof(T); i++) {
                v[i] = uint8_t(t & 0xff);
                t >>= 8;
            }
            out.write((const char*)v, sizeof(T));
        }

        inline int bmp_stride(int width) {
            return (width * 3 + 3) & ~3;
        }

        inline void write_bmp_header(std::ostream& out, int width, int height) {
            // file header
            write_binary(out, uint8_t('B'));
            write_binary(out, uint8_t('M'));
            write_binary(out, uint32_t(14 + 40 + bmp_stride(width) * height));
            write_binary(out, uint16_t(0));
            write_binary(out, uint16_t(0));
            write_binary(out, uint32_t(14 + 40));

            // bitmap header
            write_binary(out, uint32_t(40));
            write_binary(out, uint32_t(width));
            write_binary(out, uint32_t(height));
            write_binary(out, uint16_t(1));
            write_binary(out, uint16_t(24));
            write_binary(out, uint32_t(0));
            write_binary(out, uint32_t(0));
            write_binary(out, uint32_t(0));
            write_binary(out, uint32_t(0));
            write_binary(out, uint32_t(0));
            write_binary(out, uint32_t(0));
        }

        inline uint8_t get8(std::istream& in) {
            uint8_t r;
            in.read((char*)&r, sizeof(r));
            return r;
        }

        inline void skip(std::istream& in, int n) {
            in.seekg(in.tellg() + std::streamoff(n));
        }

        inline int get16le(std::istream& in) {
            return get8(in) | (get8(in) << 8);
        }

        inline uint32_t get32le(std::istream& in) {
            return get16le(in) | (get16le(in) << 16);
        }
    }

    struct bmp_header {
        uint32_t img_width;
        uint32_t img_height;
        int bpp, offset, hsz;
    };

    inline bmp_header read_bmp_header(std::istream& in) {
        using namespace c4::detail;

        bmp_header info;

        if (get8(in) != 'B' || get8(in) != 'M')
            throw std::logic_error("Not a BMP");

        get32le(in); // discard filesize
        get16le(in); // discard reserved
        get16le(in); // discard reserved
        info.offset = get32le(in);
        info.hsz = get32le(in);

        if (info.hsz != 12 && info.hsz != 40 && info.hsz != 56 && info.hsz != 108 && info.hsz != 124)
            throw std::logic_error("Corrupted BMP");

        if (info.hsz == 12) {
            info.img_width = get16le(in);
            info.img_height = get16le(in);
        }
        else {
            info.img_width = get32le(in);
            info.img_height = get32le(in);
        }

        if (get16le(in) != 1)
            throw std::logic_error("Corrupted BMP");

        info.bpp = get16le(in);
        if (info.hsz != 12) {
            int compress = get32le(in);
            if (compress == 1 || compress == 2)
                throw std::logic_error("BMP type not supported: RLE");

            get32le(in); // discard sizeof
            get32le(in); // discard hres
            get32le(in); // discard vres
            get32le(in); // discard colorsused
            get32le(in); // discard max important
            if (info.hsz == 40 || info.hsz == 56) {
                if (info.hsz == 56) {
                    get32le(in);
                    get32le(in);
                    get32le(in);
                    get32le(in);
                }
                if (info.bpp == 16 || info.bpp == 32) {
                    if (compress == 0) {
                        // do nothing
                    }
                    else if (compress == 3) {
                        // skip rgb masks
                        get32le(in);
                        get32le(in);
                        get32le(in);
                    }
                    else
                        throw std::logic_error("Corrupted BMP");
                }
            }
            else {
                int i;
                if (info.hsz != 108 && info.hsz != 124)
                    throw std::logic_error("Corrupted BMP");
                // skip rgb masks
                get32le(in);
                get32le(in);
                get32le(in);
                get32le(in);
                get32le(in); // discard color space
                for (i = 0; i < 12; ++i)
                    get32le(in); // discard color space parameters
                if (info.hsz == 124) {
                    get32le(in); // discard rendering intent
                    get32le(in); // discard offset of profile data
                    get32le(in); // discard size of profile data
                    get32le(in); // discard reserved
                }
            }
        }
        return info;
    }


    inline void read_bmp24(std::istream& in, matrix<pixel<uint8_t>>& out) {
        using namespace c4::detail;

        bmp_header info = read_bmp_header(in);

        bool flip_vertically = ((int)info.img_height) > 0;
        info.img_height = abs((int)info.img_height);

        if (info.bpp != 24)
            throw std::logic_error("BMP type not supported: bpp = " + std::to_string(info.bpp));

        out.resize(info.img_height, info.img_width);

        skip(in, info.offset - 14 - info.hsz);

        int pad = (-3 * info.img_width) & 3;

        for (int j = 0; j < (int)info.img_height; ++j) {
            for (int i = 0; i < (int)info.img_width; ++i) {
                out[j][i].b = get8(in);
                out[j][i].g = get8(in);
                out[j][i].r = get8(in);
            }
            skip(in, pad);
        }

        if (flip_vertically) {
            flip_vertical(out);
        }
    }

    inline void read_bmp24(const std::string& filepath, matrix<pixel<uint8_t>>& out) {
        std::ifstream fin(filepath, std::ifstream::binary);
        read_bmp24(fin, out);
    }

    inline void write_bmp24(std::ostream& out, const matrix_ref<pixel<uint8_t>>& img) {
        using namespace c4::detail;

        write_bmp_header(out, img.width(), img.height());

        std::vector<char> row(bmp_stride(img.width()));

        for (int j = img.height() - 1; j >= 0; j--) {
            const pixel<uint8_t>* src_row = img[j].data();

            // rgb -> bgr
            for (int i = 0; i < img.width(); i++) {
                row[3 * i + 0] = src_row[i].b;
                row[3 * i + 1] = src_row[i].g;
                row[3 * i + 2] = src_row[i].r;
            }

            out.write(row.data(), row.size());
        }
    }

    inline void write_bmp24(std::ostream& out, const matrix_ref<uint8_t>& img) {
        using namespace c4::detail;

        write_bmp_header(out, img.width(), img.height());

        std::vector<char> row(bmp_stride(img.width()));

        for (int j = img.height() - 1; j >= 0; j--) {
            const uint8_t* src_row = img[j].data();

            // rgb -> bgr
            for (int i = 0; i < img.width(); i++) {
                row[3 * i + 0] = src_row[i];
                row[3 * i + 1] = src_row[i];
                row[3 * i + 2] = src_row[i];
            }

            out.write(row.data(), row.size());
        }
    }

    template<class Image>
    inline void write_bmp24(const std::string& filepath, const Image& img) {
        std::ofstream fout(filepath, std::ofstream::binary);
        write_bmp24(fout, img);
    }
};

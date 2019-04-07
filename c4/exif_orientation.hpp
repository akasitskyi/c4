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

// https://www.media.mit.edu/pia/Research/deepview/exif.html

#pragma once

#include <cstring>

namespace c4 {
    namespace detail {
        inline uint16_t get_uint16(const unsigned char * buf, bool intel) {
            return intel ?
                (uint16_t(buf[1]) << 8) | buf[0]
                :
                (uint16_t(buf[0]) << 8) | buf[1];
        }

        inline uint16_t get_uint32(const unsigned char * buf, bool intel) {
            return intel ?
                (uint32_t(buf[3]) << 24) | (uint32_t(buf[2]) << 16) | (uint32_t(buf[1]) << 8) | buf[0]
                :
                (uint32_t(buf[0]) << 24) | (uint32_t(buf[1]) << 16) | (uint32_t(buf[2]) << 8) | buf[3];
        }
    };

    enum class ExifOrienation {
        UNSPECIFIED = 0,
        UPPER_LEFT = 1,
        LOWER_RIGHT = 3,
        UPPER_RIGHT = 6,
        LOWER_LEFT = 8,
        UNDEFINED = 9,
        PARSE_ERROR = -1
    };

    inline ExifOrienation read_exif_orientation(const uint8_t *buf, uint32_t len) {
        // JPEG Exif sanity check
        if (!buf || len < 4 || buf[0] != 0xFF || buf[1] != 0xD8 || buf[2] != 0xFF || buf[3] != 0xE1)
            return ExifOrienation::PARSE_ERROR;

        uint32_t offs = 4;

        if (offs + 4 > len)
            return ExifOrienation::PARSE_ERROR;

        const uint16_t section_length = detail::get_uint16(buf + offs, false);
        if (offs + section_length > len || section_length < 16)
            return ExifOrienation::PARSE_ERROR;
        offs += 2;

        // EXIF header
        if (offs + 6 > len || std::memcmp(buf + offs, "Exif\0\0", 6))
            return ExifOrienation::PARSE_ERROR;
        offs += 6;

        if (offs + 8 > len)
            return ExifOrienation::PARSE_ERROR;

        bool intel = true;

        if (std::memcmp(buf + offs, "II", 2) == 0)
            intel = true;
        else if (std::memcmp(buf + offs, "MM", 2) == 0)
            intel = false;
        else
            return ExifOrienation::PARSE_ERROR;
        offs += 2;

        if (0x002a != detail::get_uint16(buf + offs, intel))
            return ExifOrienation::PARSE_ERROR;
        offs += 2;

        uint32_t first_ifd_offset = detail::get_uint32(buf + offs, intel);
        offs += first_ifd_offset - 4;
        if (offs >= len)
            return ExifOrienation::PARSE_ERROR;

        // IFD0 (main image)
        if (offs + 2 > len)
            return ExifOrienation::PARSE_ERROR;
        int num_entries = detail::get_uint16(buf + offs, intel);
        offs += 2;

        if (offs + 4 + 12 * num_entries > len)
            return ExifOrienation::PARSE_ERROR;

        while (num_entries-- > 0) {
            const auto tag = detail::get_uint16(buf + offs, intel);
            const auto format = detail::get_uint16(buf + offs + 2, intel);
            const auto length = detail::get_uint32(buf + offs + 4, intel);

            if (tag == 0x112 && format == 3 && length == 1) {
                const uint16_t orientation = detail::get_uint16(buf + offs + 8, intel);
                return ExifOrienation(orientation);
            }

            offs += 12;
        }

        return ExifOrienation::UNSPECIFIED;
    }
};

//MIT License
//
//Copyright(c) 2019 Alex Kasitskyi
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

#include "image.hpp"
#include "bmp24.hpp"
#include "string.hpp"

namespace c4 {
	class image_dumper {
        std::string suffix;
		bool enabled;
		int cnt;

        image_dumper() : suffix(""), enabled(false), cnt(0) {}
        image_dumper(const image_dumper&) = delete;

	public:
		
		static image_dumper& getInstance(){
			static image_dumper imageDumper;
			return imageDumper;
		}

		static void init(std::string suffix, bool enabled){
			getInstance().suffix = suffix;
			getInstance().enabled = enabled;
			getInstance().cnt = 0;
		}

        std::string nextCnt(){
			return c4::to_string(cnt++, 2);
		}

		void dump(const c4::rgb_image& image, std::string title){
            c4::write_bmp24(nextCnt() + "-" + title + "-" + suffix + ".bmp", image);
		}

		template<typename PixelT>
		void dump(const c4::matrix_ref<PixelT>& image, std::string title){
            dump(c4::transform(image, [](const PixelT& p) {return c4::pixel<uint8_t>(c4::clamp<uint8_t>(p)); }), title);
		}

		bool isEnabled(){
			return enabled;
		}

        std::string getSuffix(){
			return suffix;
		}
	};

    template<class Image>
    void dump_image(const Image& image, const std::string& title) {
        if (image_dumper::getInstance().isEnabled()) {
            image_dumper::getInstance().dump(image, title);
        }
    }

    template<class Image>
    void force_dump_image(const Image& image, const std::string& title) {
        image_dumper::getInstance().dump(image, title);
    }
};

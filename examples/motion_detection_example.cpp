//MIT License
//
//Copyright(c) 2025 Alex Kasitskyi
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

#include <memory>

#include <c4/jpeg.hpp>
#include <c4/drawing.hpp>
#include <c4/string.hpp>
#include <c4/image_dumper.hpp>
#include <c4/motion_detection.hpp>

int main(int argc, char* argv[]) {
    try{
		c4::Logger::setLogLevel(c4::LOG_VERBOSE);
        c4::scoped_timer timer("main");

        c4::image_dumper::getInstance().init("", true);

		c4::matrix<uint8_t> prev, cur;
        c4::read_jpeg("img0.jpg", prev);
        c4::read_jpeg("img1.jpg", cur);
        //c4::read_jpeg("imgs/230.jpg", prev);
        //c4::read_jpeg("imgs/231.jpg", cur);

		//prev = prev.submatrix(192, 64, 192, 192);
		//cur = cur.submatrix(192, 64, 192, 192);

        c4::MotionDetector md;
		c4::MotionDetector::Params params;
		params.blockSize = 32;
		params.maxShift = 16;

		const int downscale = 2;

		std::vector<c4::rectangle<int>> ignore{{400, 900, 1120, 180}};

		c4::MotionDetector::Motion motion;
		{
			c4::matrix<uint8_t> prevDown;
			c4::matrix<uint8_t> curDown;

			c4::downscale_bilinear_nx(prev, prevDown, downscale);
			c4::downscale_bilinear_nx(cur, curDown, downscale);

			std::vector<c4::rectangle<int>> ignoreDown;
			for (const auto& r : ignore) {
				ignoreDown.emplace_back(r.x / downscale, r.y / downscale, r.w / downscale, r.h / downscale);
			}
			motion = md.detect(prevDown, curDown, params, ignoreDown);
			motion.shift = motion.shift * downscale;
		}

		c4::matrix<uint8_t> cura(cur.dimensions());
		motion.apply(cur, cura);

		if (c4::image_dumper::getInstance().isEnabled()) {
			for (auto& r : ignore) {
				c4::draw_rect(cur, r, uint8_t(255), 1);
			}

			c4::point<double> mid(cur.width() / 2, cur.height() / 2);
			c4::dump_image(prev, "prev");
			c4::draw_line(cur, mid, mid + motion.shift, uint8_t(255), 1);
			c4::draw_point(cur, mid + motion.shift, uint8_t(255), 5);
			c4::draw_string(cur, 20, 20, "shift: " + c4::to_string(motion.shift.x, 2) + ", " + c4::to_string(motion.shift.y, 2)
				+ ", scale: " + c4::to_string(motion.scale, 4)
				+ ", alpha: " + c4::to_string(motion.alpha, 4), uint8_t(255), uint8_t(0), 2);
			c4::dump_image(cur, "cur");

			c4::dump_image(cura, "cura");
		}
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}

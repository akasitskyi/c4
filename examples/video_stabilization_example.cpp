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
#include <c4/serialize.hpp>
#include <c4/image_dumper.hpp>
#include <c4/video_stabilization.hpp>

int main(int argc, char* argv[]) {
    try{
		c4::Logger::setLogLevel(c4::LOG_DEBUG);
        c4::scoped_timer timer("main");

        c4::image_dumper::getInstance().init("", true);

		const std::string in_filemask = "imgs/%03d.jpg";
		const std::string out_filemask = "out/%03d.jpg";

		c4::VideoStabilization::Params params;
		const std::vector<c4::rectangle<int>> ignore;//{{400, 900, 1120, 180}};
		const int downscale = 2;
		std::vector<c4::rectangle<int>> ignoreDown;
		for (const auto& r : ignore) {
			ignoreDown.emplace_back(r.x / downscale, r.y / downscale, r.w / downscale, r.h / downscale);
		}
		params.x_smooth = 25;
		params.y_smooth = 25;
		c4::VideoStabilization vs(params);
		c4::matrix<uint8_t> image;

		char buf[1024];

		for (int fc = 1; ; fc++) {
			try {
				std::sprintf(buf, in_filemask.c_str(), fc);
				c4::read_jpeg(buf, image);
			} catch (const std::exception& e) {
				LOGD_EXCEPTION(e);
				break;
			}

			c4::VideoStabilization::FramePtr frame = std::make_shared<c4::VideoStabilization::Frame>();

			c4::downscale_bilinear_nx(image, *frame, downscale);

			c4::MotionDetector::Motion motion = vs.process(frame, ignoreDown);
			motion.shift = motion.shift * downscale;

			c4::matrix<uint8_t> stabilized(image.dimensions());
			
			motion.apply(image, stabilized);
				
			c4::draw_string(stabilized, 20, 15, "frame " + c4::to_string(fc, 3), uint8_t(255), uint8_t(0), 2);

			c4::draw_string(stabilized, 20, 45, "shift: " + c4::to_string(motion.shift.x, 2) + ", " + c4::to_string(motion.shift.y, 2)
				+ ", scale: " + c4::to_string(motion.scale, 4)
				+ ", alpha: " + c4::to_string(motion.alpha, 4), uint8_t(255), uint8_t(0), 2);

			std::sprintf(buf, out_filemask.c_str(), fc);
			c4::write_jpeg(buf, stabilized);
		}
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}

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

#include <c4/jpeg.hpp>
#include <c4/drawing.hpp>
#include <c4/string.hpp>
#include <c4/serialize.hpp>
#include <c4/image_dumper.hpp>
#include <c4/motion_detection.hpp>

int main(int argc, char* argv[]) {
	std::cout << argv[0] << std::endl;
    try{
        c4::scoped_timer timer("main time");

        c4::image_dumper::getInstance().init("", true);

		std::vector<c4::matrix<uint8_t>> images(2);
        for (int i : c4::range(images)) {
            c4::read_jpeg("img" + c4::to_string(i) + ".jpg", images[i]);
        }
		//std::vector<c4::matrix<uint8_t>> images(750);
  //      for (int i : c4::range(images)) {
  //          c4::read_jpeg("imgs/" + c4::to_string(i+1, 3) + ".jpg", images[i]);
  //      }

        c4::MotionDetector md;

		std::vector<c4::rectangle<int>> ignore{{400, 900, 1120, 180}};

        for (int i : c4::range(ssize(images) - 1)) {
			auto motion = md.detect(images[i], images[i + 1], ignore);
			c4::point<double> mid(images[i].width() / 2, images[i].height() / 2);

			for (auto& r : ignore) {
				c4::draw_rect(images[i+1], r, uint8_t(255), 1);
			}

			c4::draw_line(images[i+1], mid, mid + motion.shift * 10, uint8_t(255), 2);
			c4::draw_point(images[i + 1], mid + motion.shift * 10, uint8_t(255), 6);
			c4::draw_string(images[i + 1], 10, 10, "shift: " + c4::to_string(motion.shift.x, 2) + ", " + c4::to_string(motion.shift.y, 2)
                + ", scale: " + c4::to_string(motion.scale, 4)
                + ", alpha: " + c4::to_string(motion.alpha, 4), uint8_t(255), uint8_t(0), 2);
			c4::dump_image(images[i+1], "cur");
			c4::dump_image(images[i], "prev");

			c4::matrix<uint8_t> cura(images[i].height(), images[i].width());
			for (int y : c4::range(cura.height())) {
				for (int x : c4::range(cura.width())) {
					c4::point<double> p(x, y);
					cura[y][x] = images[i+1].clamp_get(motion.apply(p));
				}
			}

			c4::dump_image(cura, "cura");
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}

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

        c4::MotionDetector md;

        for (int i : c4::range(ssize(images) - 1)) {
			auto motion = md.detect(images[i], images[i + 1]);
			c4::point<double> mid(images[i].width() / 2, images[i].height() / 2);

			c4::draw_line(images[i+1], mid, mid + motion.shift, uint8_t(255), 2);
			c4::draw_string(images[i + 1], 10, 10, "shift: " + c4::to_string(motion.shift.x) + ", " + c4::to_string(motion.shift.y)
                + ", scale: " + c4::to_string(motion.scale, 2)
                + ", alpha: " + c4::to_string(motion.alpha, 2), uint8_t(255), uint8_t(0), 2);
			c4::dump_image(images[i+1], "out-" + c4::to_string(i) + ".jpg");
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}

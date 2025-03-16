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

class JpegFrameReader : public c4::VideoStabilization::FrameReader {
	const std::string filemask;
	int fc;
public:
	JpegFrameReader(std::string filemask, int fc_start = 0) : filemask(filemask), fc(fc_start) {
	}

	virtual bool read(c4::matrix<uint8_t>& frame) {
		char buf[1024];
		std::sprintf(buf, filemask.c_str(), fc++);
        try {
			c4::read_jpeg(buf, frame);
		} catch (const std::exception& e) {
			LOGD_EXCEPTION(e);
			return false;
		}
		return true;
	}
};

class JpegFrameWriter : public c4::VideoStabilization::FrameWriter {
	const std::string filemask;
	int fc;
public:
	JpegFrameWriter(std::string filemask, int fc_start = 0) : filemask(filemask), fc(fc_start) {
	}
	
	virtual void write(const c4::matrix<uint8_t>& frame) {
		char buf[1024];
		std::sprintf(buf, filemask.c_str(), fc++);
		c4::write_jpeg(buf, frame);
	}
};

int main(int argc, char* argv[]) {
	std::cout << argv[0] << std::endl;
    try{
        c4::scoped_timer timer("main");

        c4::image_dumper::getInstance().init("", true);

		std::shared_ptr<c4::VideoStabilization::FrameReader> reader = 
			//std::make_shared<JpegFrameReader>("img%d.jpg", 0);
			std::make_shared<JpegFrameReader>("imgs/%03d.jpg", 1);
		std::shared_ptr<c4::VideoStabilization::FrameWriter> writer = std::make_shared<JpegFrameWriter>("out/%03d.jpg", 1);

		c4::VideoStabilization vs(reader, writer);

		vs.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}

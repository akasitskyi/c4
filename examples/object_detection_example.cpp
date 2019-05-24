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

#include <c4/jpeg.hpp>

#include <fstream>

#include <map>
#include <unordered_map>

#include <c4/math.hpp>
#include <c4/mstream.hpp>
#include <c4/logger.hpp>
#include <c4/json.hpp>
#include <c4/geometry.hpp>
#include <c4/scaling.hpp>
#include <c4/drawing.hpp>
#include <c4/object_detection.hpp>


#include <c4/image.hpp>
#include <c4/color_plane.hpp>
#include <c4/bmp24.hpp>
#include <c4/serialize.hpp>
#include <c4/lbp.hpp>
#include <c4/dataset.hpp>
#include <c4/math.hpp>

int main(int argc, char* argv[]) {
    try{
        std::ifstream fin("matrix_regression.dat", std::ifstream::binary);
        c4::serialize::input_archive in(fin);

        __c4::matrix_regression<> mr;
        in(mr);
        
        __c4::window_detector<c4::LBP, 256> wd(mr, c4::LBP());

        __c4::scaling_detector<c4::LBP, 256> sd(wd, 0.95f);

        c4::dataset test_set(wd.dimensions());
        test_set.load_dlib("labels_ibug_300W_test.json");

        const double test_mse = c4::mean_squared_error(mr.predict(test_set.rx), test_set.y);
        LOGD << "test_mse: " << test_mse;


        c4::matrix<uint8_t> img, scaled;
        c4::read_jpeg(argv[1], img);

        c4::downscale_nx(img, scaled, 8);

        const auto dets = sd.detect(scaled, 0.97);

        for (const auto& d : dets) {
            c4::draw_rect(scaled, d.rect.x, d.rect.y, d.rect.w, d.rect.h, uint8_t(255), 1);
        }

        c4::write_bmp24("fd.bmp", scaled);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}

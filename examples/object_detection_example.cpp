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
#include <c4/drawing.hpp>
#include <c4/serialize.hpp>
#include <c4/image_dumper.hpp>
#include <c4/meta_data_set.hpp>
#include <c4/classification_metrics.hpp>
#include <c4/dataset.hpp>
#include <c4/simple_cv.hpp>


int main(int argc, char* argv[]) {
    try{
        c4::meta_data_set test_meta;
        test_meta.load_vggface2("C:/vggface2/test/", "C:/github/vggface2_meta/bb_landmark/loose_landmark_test.csv", 1.5, 16);

        const double iou_threshold = 0.5;

        PRINT_DEBUG(test_meta.data.size());

        std::vector<c4::image_file_metadata> detections_c4(test_meta.data.size());

        c4::image_dumper::getInstance().init("", true);
        {
            c4::scaling_detector<c4::lbpx<3>, 256> sd;
            c4::load("face_detector_24_lbpx3.dat", sd);

            c4::progress_indicator progress("detection", (uint32_t)test_meta.data.size());

            c4::scoped_timer timer("c4 detect time");

            c4::parallel_for(c4::range(test_meta.data), [&](int k) {
                const auto& t = test_meta.data[k];

                c4::matrix<uint8_t> img;

                c4::read_jpeg(t.filepath, img);

                c4::image_file_metadata& ifm = detections_c4[k];
                ifm.filepath = t.filepath;

                std::vector<c4::detection> candidates;
                const auto dets = sd.detect(img, candidates);

                for (const auto& d : candidates) {
                    const auto irect = c4::rectangle<int>(d.rect);
                    c4::draw_rect(img, irect, uint8_t(128), 1);
                }

                for (const auto& d : dets) {
                    const auto irect = c4::rectangle<int>(d.rect);
                    ifm.objects.push_back({ irect,{} });
                    c4::draw_rect(img, irect, uint8_t(255), 1);
                    c4::draw_string(img, irect.x, irect.y, c4::to_string(d.conf, 2), uint8_t(255), uint8_t(0));
                }

                for (const auto& g : t.objects) {
                    c4::draw_rect(img, g.rect, uint8_t(0), 1);
                }

                std::string fn = t.filepath;

                std::replace(fn.begin(), fn.end(), ':', '_');
                std::replace(fn.begin(), fn.end(), '/', '_');
                std::replace(fn.begin(), fn.end(), '\\', '_');

                const auto eval = c4::evaluate_object_detection(t.objects, ifm.objects, iou_threshold);

                if (eval.fp > 0) {
                    c4::dump_image(img, fn);
                }

                progress.did_some(1);
            });

            auto res = c4::evaluate_object_detection(test_meta.data, detections_c4, iou_threshold);

            PRINT_DEBUG(res.recall());
            PRINT_DEBUG(res.precission());
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}

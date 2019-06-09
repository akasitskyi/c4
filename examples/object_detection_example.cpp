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

#include <c4/lbp.hpp>
#include <c4/jpeg.hpp>
#include <c4/drawing.hpp>
#include <c4/serialize.hpp>
#include <c4/image_dumper.hpp>
#include <c4/meta_data_set.hpp>
#include <c4/classification_metrics.hpp>
#include <c4/object_detection.hpp>
#include <c4/dataset.hpp>

#include <dlib/image_processing/frontal_face_detector.h>


int main(int argc, char* argv[]) {
    try{
        c4::meta_data_set test_meta;
        test_meta.load_vggface2("C:/vggface2/test/", "C:/vggface2/test/loose_landmark_test.csv", 1.5, 32);

        PRINT_DEBUG(test_meta.data.size());

        {
            c4::image_dumper::getInstance().init("c4", false);

            const auto sd = c4::load_scaling_detector("matrix_regression_28_1.5_1000it_neg10_step3_div8_nns.dat");

            if(0){
                const int sample_size = 28;
                const float neg_to_pos_ratio = 10.f;
                const c4::matrix_dimensions sample_dims{ sample_size, sample_size };
                c4::dataset<c4::LBP> test_set(sample_dims);
                test_set.load(test_meta, 0, neg_to_pos_ratio, neg_to_pos_ratio * 1.2f);
                std::cout << "test size: " << test_set.y.size() << std::endl;

                const double test_mse = c4::mean_squared_error(sd.wd.mr.predict(test_set.rx), test_set.y);
                PRINT_DEBUG(test_mse);
            }

            std::vector<c4::image_file_metadata> detections(test_meta.data.size());

            c4::progress_indicator progress((uint32_t)test_meta.data.size());

            c4::scoped_timer timer("c4 detect time");

            c4::parallel_for(c4::range(test_meta.data), [&](int k) {
                const auto& t = test_meta.data[k];

                c4::matrix<uint8_t> img;

                c4::read_jpeg(t.filepath, img);

                c4::image_file_metadata& ifm = detections[k];
                ifm.filepath = t.filepath;

                const auto dets = sd.detect(img, 0.65);

                for (const auto& d : dets) {
                    const auto irect = c4::rectangle<int>(d.rect);
                    ifm.objects.push_back({ irect,{} });
                    c4::draw_rect(img, irect, uint8_t(255), 1);
                }

                for (const auto& g : t.objects) {
                    c4::draw_rect(img, g.rect, uint8_t(0), 1);
                }

                c4::dump_image(img, "fd");

                progress.did_some(1);
            });

            auto res = c4::evaluate_object_detection(test_meta.data, detections, 0.7);

            PRINT_DEBUG(res.recall());
            PRINT_DEBUG(res.precission());
        }

        if(0){
            c4::image_dumper::getInstance().init("dlib", false);

            c4::enumerable_thread_specific<dlib::frontal_face_detector> dlib_fd_ts(dlib::get_frontal_face_detector());

            std::vector<c4::image_file_metadata> detections(test_meta.data.size());

            c4::progress_indicator progress((uint32_t)test_meta.data.size());

            c4::scoped_timer timer("dlib detect time");

            c4::parallel_for(c4::range(test_meta.data), [&](int k) {
                const auto& t = test_meta.data[k];
                auto& dlib_fd = dlib_fd_ts.local();

                c4::matrix<uint8_t> img;

                c4::read_jpeg(t.filepath, img);

                dlib::array2d<unsigned char> dimg(img.height(), img.width());
                for (int i : c4::range(img.height())) {
                    std::copy(img[i].begin(), img[i].end(), &dimg[i][0]);
                }

                std::vector<dlib::rectangle> dets = dlib_fd(dimg);

                c4::image_file_metadata& ifm = detections[k];
                ifm.filepath = t.filepath;

                for (const auto& d : dets) {
                    const auto irect = c4::rectangle<int>(d.left(), d.top(), d.width(), d.height());
                    ifm.objects.push_back({ irect,{} });
                    c4::draw_rect(img, irect, uint8_t(255), 1);
                }

                for (const auto& g : t.objects) {
                    c4::draw_rect(img, g.rect, uint8_t(0), 1);
                }

                c4::dump_image(img, "fd");

                progress.did_some(1);
            });

            auto res = c4::evaluate_object_detection(test_meta.data, detections, 0.5);

            PRINT_DEBUG(res.recall());
            PRINT_DEBUG(res.precission());
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}

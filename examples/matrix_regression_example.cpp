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
#include <c4/string.hpp>
#include <c4/matrix_regression.hpp>
#include <c4/classification_metrics.hpp>
#include <c4/object_detection.hpp>

#include <c4/image.hpp>
#include <c4/color_plane.hpp>
#include <c4/bmp24.hpp>
#include <c4/serialize.hpp>
#include <c4/dataset.hpp>
#include <c4/cmd_opts.hpp>

#include <c4/simple_cv.hpp>

int main(int argc, char* argv[]) {
    try{
        c4::cmd_opts opts;
        auto sample_size = opts.add_required<int>("sample_size");
        auto face_scale = opts.add_required<float>("face_scale");
        auto max_shift = opts.add_required<int>("max_shift");
        auto train_load_step = opts.add_required<int>("train_load_step");
        auto iterations = opts.add_required<int>("iterations");
        auto base = opts.add_optional<std::string>("base", "-");
        auto neg_to_pos_ratio = opts.add_optional<float>("neg_to_pos_ratio", 1.f);

        opts.parse(argc, argv);

        constexpr int N = 1;

        PRINT_DEBUG(N);
        PRINT_DEBUG(sample_size);
        PRINT_DEBUG(face_scale);
        PRINT_DEBUG(max_shift);
        PRINT_DEBUG(train_load_step);
        PRINT_DEBUG(iterations);
        PRINT_DEBUG(base);
        PRINT_DEBUG(neg_to_pos_ratio);

        c4::meta_data_set train_meta;
        train_meta.load_vggface2("C:/vggface2/train/", "C:/github/vggface2_meta/bb_landmark/loose_landmark_train.csv", face_scale, train_load_step);

        const c4::matrix_dimensions sample_dims{ sample_size, sample_size };

        typedef c4::lbpx<N> lbp;
        c4::dataset<lbp> train_set(sample_dims);
        c4::dataset<lbp> test_set(sample_dims);

        train_set.load(train_meta, max_shift, neg_to_pos_ratio, neg_to_pos_ratio * 1.2f);
        std::cout << "pos ratio: " << std::accumulate(train_set.y.begin(), train_set.y.end(), 0.) / train_set.y.size() << std::endl;
        std::cout << "train size: " << train_set.y.size() << std::endl;

        c4::meta_data_set test_meta;
        test_meta.load_vggface2("C:/vggface2/test/", "C:/github/vggface2_meta/bb_landmark/loose_landmark_test.csv", face_scale, 16);

        test_set.load(test_meta, 0, neg_to_pos_ratio, neg_to_pos_ratio * 1.2f);
        std::cout << "test size: " << test_set.y.size() << std::endl;

        c4::byte_matrix_regression mr;

        if (std::string(base) != "-") {
            c4::load(base, mr);
        }

        mr.train(train_set.rx, train_set.y, test_set.rx, test_set.y, iterations);

        const c4::window_detector<lbp, 256> wd(mr, 0.41f);
        const c4::scaling_detector<lbp, 256> sd(wd, 1.f, 0.9f);

        c4::save(sd, "face_detector_" + c4::to_string(sample_size) + "_lbpx" + std::to_string(N) + ".dat");

        c4::image_dumper::getInstance().init("", false);

        std::vector<c4::image_file_metadata> detections(test_meta.data.size());

        c4::progress_indicator progress((uint32_t)test_meta.data.size());

        c4::parallel_for (c4::range(test_meta.data), [&](int i){
            const auto& t = test_meta.data[i];

            c4::matrix<uint8_t> img;

            c4::read_jpeg(t.filepath, img);

            c4::image_file_metadata& ifm = detections[i];
            ifm.filepath = t.filepath;

            const auto dets = sd.detect(img);

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

        auto res = c4::evaluate_object_detection(test_meta.data, detections, 0.5);

        PRINT_DEBUG(res.recall());
        PRINT_DEBUG(res.precission());
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}

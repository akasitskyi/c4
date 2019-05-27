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

#include <c4/image.hpp>
#include <c4/color_plane.hpp>
#include <c4/bmp24.hpp>
#include <c4/serialize.hpp>
#include <c4/dataset.hpp>
#include <c4/cmd_opts.hpp>

template<typename T>
void merge_vectors(std::vector<std::vector<T>>& src, std::vector<T>& dst) {
    size_t capacity = std::accumulate(src.begin(), src.end(), dst.size(), [](size_t a, const std::vector<T>& v) { return a + v.size(); });
    dst.reserve(capacity);

    for (auto& t : src) {
        dst.insert(dst.end(), std::make_move_iterator(t.begin()), std::make_move_iterator(t.end()));
        t.clear();
        t.shrink_to_fit();
    }
}


int main(int argc, char* argv[]) {
    try{
        // step 100, 40, k2, sym, rect 3.0:
        // train size: 939874
        // test size: 211606
        // it500  - 0.0175486

        // step 100, 42, k1, sym, rect 3.0:
        // train size: 341386
        // test size: 211606
        // it500  - 0.0181418

        // step 100, 42, k2, sym, rect 3.0:
        // train size: 939874
        // test size: 211606
        // it500  - 0.0175391
        // it1000 - 0.0172517

        // step 20, 42, k2, sym, rect 3.0:
        // train size: 4687386
        // test size: 211606
        // it500  - 0.0168998
        // it1000 - 0.0160926

        // step 10, 42, k1, sym, rect 3.0:
        // train size: 3399126
        // test size: 211606
        // it100  - 0.0233916
        // it200  - 0.019282
        // it300  - 0.0178331
        // it400  - 0.0171015
        // it500  - 0.016661
        // it1000 - 0.0158567

        // step 10, 42, k2, sym, rect 3.0:
        // train size: 9361442
        // test size: 211606
        // it100  - 0.023423
        // it200  - 0.0193024
        // it300  - 0.0178743
        // it400  - 0.0171582
        // it500  - 0.0167218
        // it1000 - 0.0158606

        // step 5, 42, k1, sym, rect 3.0:
        // train size: 6805994
        // test size: 211606
        // it100  - 0.0232507
        // it200  - 0.0191452
        // it300  - 0.0176935
        // it400  - 0.0169541
        // it500  - 0.0165028
        // it1000 - 0.0156271

        // step 3, 42, k1, sym, rect 3.0:
        // train size: 11361096
        // test size: 211606
        // it100  - 0.0233746
        // it200  - 0.0192269
        // it300  - 0.0177527
        // it400  - 0.0169967
        // it500  - 0.0165323
        // it1000 - 0.015609

        //*step 1, 42, k0, sym, rect 3.0:
        // train size: 3837654
        // test size: 211606
        // it100  - 0.0229642
        // it200  - 0.0189504
        // it300  - 0.0175291
        // it400  - 0.0168034
        // it500  - 0.0163626
        // it1000 - 0.0155334

        // step 100, 42, k3, sym, rect 3.0:
        // train size: 1821108
        // test size: 211606
        // it500  - 0.0176195

        // step 100, 44, k2, sym, rect 3.0:
        // train size: 939874
        // test size: 211606
        // it500  - 0.0176728

        c4::cmd_opts opts;
        auto sample_size = opts.add_required<int>("sample_size");
        auto face_scale = opts.add_required<float>("face_scale");
        auto max_shift = opts.add_required<int>("max_shift");
        auto train_load_step = opts.add_required<int>("train_load_step");
        auto iterations = opts.add_required<int>("iterations");

        opts.parse(argc, argv);

        PRINT_DEBUG(sample_size);
        PRINT_DEBUG(face_scale);
        PRINT_DEBUG(max_shift);
        PRINT_DEBUG(train_load_step);

        const c4::matrix_dimensions sample_dims{ sample_size, sample_size };
        const c4::matrix_dimensions negative_sample_dims{ 64, 64 };
        c4::dataset train_set(sample_dims, negative_sample_dims);
        //train_set.load_vggface2("C:/vggface2/train/", "C:/vggface2/train/loose_bb_train.csv", max_shift, train_load_step);
        train_set.load_vggface2_landmark("C:/vggface2/train/", "C:/vggface2/train/loose_landmark_train.csv", face_scale, 3.0, max_shift, train_load_step);

        std::cout << "train size: " << train_set.y.size() << std::endl;
        std::cout << "positive ratio: " << std::accumulate(train_set.y.begin(), train_set.y.end(), 0.f) / train_set.y.size() << std::endl;

        c4::dataset test_set(sample_dims, negative_sample_dims);
        //test_set.load_vggface2("C:/vggface2/test/", "C:/vggface2/test/loose_bb_test.csv");
        test_set.load_vggface2_landmark("C:/vggface2/test/", "C:/vggface2/test/loose_landmark_test.csv", face_scale, 3.0, 0, 1);

        std::cout << "test size: " << test_set.y.size() << std::endl;
        std::cout << "positive ratio: " << std::accumulate(test_set.y.begin(), test_set.y.end(), 0.f) / test_set.y.size() << std::endl;

        c4::matrix_regression<> mr;

        mr.train(train_set.rx, train_set.y, test_set.rx, test_set.y, iterations, true);

        {
            std::ofstream fout("matrix_regression.dat", std::ofstream::binary);

            c4::serialize::output_archive out(fout);

            out(mr);
        }
        {
            std::ifstream fin("matrix_regression.dat", std::ifstream::binary);
            c4::serialize::input_archive in(fin);

            c4::matrix_regression<> mr1;
            in(mr1);

            const double train_mse = c4::mean_squared_error(mr1.predict(train_set.rx), train_set.y);
            const double test_mse = c4::mean_squared_error(mr1.predict(test_set.rx), test_set.y);

            LOGD << "RELOAD: train_mse: " << train_mse << ", test_mse: " << test_mse;
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}

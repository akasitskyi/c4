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
        // step 20, it100, k2, sym:
        // 64 - 0.0367619

        // step 20, it100, k3, sym:
        // 64 - 0.0308738

        // step 20, it100, k4, sym:
        // 64 - 0.0278687

        // step 20, it100, k5, sym:
        // 64 - 0.0263316

        // step 20, it100, k6, sym:
        // 64 - 0.0253624

        // step 20, it100, k7, sym:
        // 64 - 0.0247578

        // step 20, it100, k8, sym:
        // 64 - 0.0243022

        // step 20, it100, k9, sym:
        // 64 - 0.0239536

        // step 20, it100, k10, sym:
        // 64 - 0.0236716

        // step 20, it100, k12, sym:
        // 64 - 0.0234398


        // step 20, it200, k2, sym:
        // 64 - 0.0368679

        // step 20, it200, k10, sym:
        // 64 - 0.0219404


        // step 20, it500, k10, sym:
        // 64 - 0.020858

        // step 20, it500, k12, sym:
        // 64 - 0.0205338


        // step 10, it500, k6, sym:
        // 64 - 0.0194836

        // step 10, it500, k10, sym:
        // 64 - 0.0180317

        // step 10, it500, k12, sym:
        // 64 - 0.0180055


        // step  3, it500, k10, sym:
        // 64 - 0.0160341

        // step  3, it500, k8, sym:
        // 52 - 0.0160341

        // step  3, it500, k6, sym:
        // 40 - 0.0152601

        // step  3, it500, k5, sym:
        // 32 - 0.0180542

        // step  3, it500, k6, sym:
        // 32 - 0.0172336


        // step  1, it500, k6, sym:
        // 32 - 0.0159794

        // step  1, it500, k7, sym:
        // 32 - 0.015796

        // step  1, it500, k8, sym:
        // 32 - 0.0157435

        // step  1, it500, k8, sym:
        // 40 - 0.0143788

        // step  1, it500, k9, sym:
        // 40 - 0.0143286

        // step  1, it500, k10, sym:
        // 40 - 0.0143431

        // step  1, it500, k10, sym:
        // 48 - 0.0150839


        // step  1, it1000, k9, sym:
        // 40 - 0.0139379

        c4::cmd_opts opts;
        auto sample_size = opts.add_required<int>("sample_size");
        auto max_shift = opts.add_required<int>("max_shift");
        auto train_load_step = opts.add_required<int>("train_load_step");
        auto iterations = opts.add_required<int>("iterations");

        opts.parse(argc, argv);

        const c4::matrix_dimensions sample_dims{ sample_size, sample_size };
        c4::dataset train_set(sample_dims);
        train_set.load_vggface2("C:/vggface2/train/", "C:/vggface2/train/loose_bb_train.csv", max_shift, train_load_step);

        std::cout << "train size: " << train_set.y.size() << std::endl;
        std::cout << "positive ratio: " << std::accumulate(train_set.y.begin(), train_set.y.end(), 0.f) / train_set.y.size() << std::endl;

        c4::dataset test_set(sample_dims);
        test_set.load_vggface2("C:/vggface2/test/", "C:/vggface2/test/loose_bb_test.csv");

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

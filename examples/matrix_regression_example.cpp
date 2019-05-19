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
#include <c4/lbp.hpp>

template<class TForm>
void generate_samples(const c4::matrix_dimensions& sample_size, const c4::matrix<uint8_t>& img, const std::vector<c4::rectangle<int>>& rects, std::vector<c4::matrix<uint8_t>>& train_x, std::vector<float>& train_y, int k, TForm tform) {
    c4::matrix<uint8_t> sample(sample_size);

    int positive = 0;

    for (const auto& r : rects) {
        for (int dx = -k; dx <= k; dx++) {
            for (int dy = -k; dy <= k; dy++) {
                c4::rectangle<int> r1(r.x + dx, r.y + dy, r.w, r.h);

                const auto cropped_rect = r1.intersect(img.rect());
                c4::scale_bilinear(img.submatrix(cropped_rect), sample);
                train_x.push_back(tform(sample));
                train_y.push_back(1.f);

                positive++;
            }
        }
    }

    if (img.height() <= sample.height() || img.width() <= sample.width())
        return;

    c4::fast_rand rnd;
    int negatives = positive * 1;
    while (negatives--) {
        c4::rectangle<int> r;
        r.y = rnd() % (img.height() - sample.height());
        r.x = rnd() % (img.width() - sample.width());
        r.h = sample_size.height;
        r.w = sample_size.width;

        double iou_max = 0.;
        for (const auto& t : rects) {
            iou_max = std::max(iou_max, c4::intersection_over_union(r, t));
        }
        if (iou_max < 0.1) {
            train_x.push_back(tform(img.submatrix(r.y, r.x, r.h, r.w)));
            train_y.push_back(0.f);
        }
    }
}

struct dataset {
    std::vector<c4::matrix<uint8_t>> x;
    std::vector<float> y;
    const c4::matrix_dimensions sample_size;

    dataset(const c4::matrix_dimensions& sample_size) : sample_size(sample_size) {}

    void load(const std::string& labels_filepath, const int k, const int sample) {
        c4::fast_rand rnd;

        c4::json data_json;

        std::string root = "C:/portraits-data/ibug_300W_large_face_landmark_dataset/";
        std::ifstream train_data_fin(root + labels_filepath);

        train_data_fin >> data_json;

        for (const auto& image_info : data_json["images"]) {
            std::string filename = image_info["filename"];

            // Right now we only have jpeg input...
            if (!c4::ends_with(c4::to_lower(filename), ".jpg") || rnd() % sample)
                continue;

            c4::matrix<uint8_t> img;
            c4::read_jpeg(root + filename, img);

            const auto& boxes = image_info["boxes"];

            std::vector<c4::rectangle<int>> rects;
            for (const auto& box : boxes) {
                c4::rectangle<int> r(box["left"], box["top"], box["width"], box["height"]);
                rects.push_back(r);
            }

            generate_samples(sample_size, img, rects, x, y, k, c4::LBP());
        }
    }
};

int main(int argc, char* argv[]) {
    try{
        // FULL TEST:
        // step 20, k2:
        // 64 - 0.0396186

        // step 20, k2, sym:
        // 64 - 0.0313635

        // step 20, k3:
        // 64 - 0.0308812

        // step 20, k3, sym:
        // 64 - 0.0261583

        // step 20, k4:
        // 64 - 0.0263632

        // step 20, k4, sym:
        // 64 - 0.0237412

        const c4::matrix_dimensions sample_size{ 64, 64 };
        dataset train_set(sample_size);
        train_set.load("labels_ibug_300W_train.json", 4, 20);

        std::cout << "train size: " << train_set.y.size() << std::endl;
        std::cout << "positive ratio: " << std::accumulate(train_set.y.begin(), train_set.y.end(), 0.f) / train_set.y.size() << std::endl;

        dataset test_set(sample_size);
        test_set.load("labels_ibug_300W_test.json", 0, 1);

        std::cout << "test size: " << test_set.y.size() << std::endl;
        std::cout << "positive ratio: " << std::accumulate(test_set.y.begin(), test_set.y.end(), 0.f) / test_set.y.size() << std::endl;

        __c4::matrix_regression<> mr;

        mr.train(train_set.x, train_set.y, test_set.x, test_set.y, true);

        {
            std::ofstream fout("matrix_regression.dat", std::ofstream::binary);

            c4::serialize::output_archive out(fout);

            out(mr);
        }
        {
            std::ifstream fin("matrix_regression.dat", std::ifstream::binary);
            c4::serialize::input_archive in(fin);

            __c4::matrix_regression<> mr1;
            in(mr1);

            const double train_mse = c4::mean_squared_error(mr1.predict(train_set.x), train_set.y);
            const double test_mse = c4::mean_squared_error(mr1.predict(test_set.x), test_set.y);

            LOGD << "RELOAD: train_mse: " << train_mse << ", test_mse: " << test_mse;
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}

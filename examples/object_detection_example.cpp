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

#include <c4/math.hpp>
#include <c4/logger.hpp>
#include <c4/json.hpp>
#include <c4/matrix_regression.hpp>


#include <c4/image.hpp>
#include <c4/color_plane.hpp>
#include <c4/bmp24.hpp>


c4::matrix<uint8_t> generate_matrix(float mean, float sd) {
    c4::fast_rand_float_normal rnd;

    c4::matrix<uint8_t> r(2, 2);

    for (auto& v : r)
        for (auto& x : v)
            x = c4::clamp<uint8_t>(mean + rnd() * sd);

    return r;
}

int main(int argc, char* argv[]) {
    try{
        int width, height, n;
        unsigned char *data = stbi_load("test.jpg", &width, &height, &n, 0);

        c4::rgb_image img(height, width);

        c4::rgb_to_img(data, width, height, width * n, c4::RgbByteOrder::RGB, img);
        stbi_image_free(data);

        c4::write_bmp24("test.bmp", img);

        return 0;

        std::vector<c4::matrix<uint8_t>> train_x;
        std::vector<float> train_y;

        c4::json train_data;
        std::ifstream train_data_fin("C:/portraits-data/ibug_300W_large_face_landmark_dataset/labels_ibug_300W_train.json");

        train_data_fin >> train_data;

        for (const auto& image_info : train_data["images"]) {

        }

        const float sd = 1000.f;

        c4::fast_rand rnd;

        for (int i : c4::range(1000)) {
            const float y = float(rnd() % 2);
            const float mean = 100.f + 100.f * y;

            train_x.push_back(generate_matrix(mean, sd));
            train_y.push_back(y);
        }

        std::cout << "positive ratio: " << std::accumulate(train_y.begin(), train_y.end(), 0.f) / train_y.size() << std::endl;

        __c4::matrix_regression<> mr;

        mr.train(train_x, train_y);

        std::cout << mr.predict(generate_matrix(100.f, sd)) << std::endl;
        std::cout << mr.predict(generate_matrix(150.f, sd)) << std::endl;
        std::cout << mr.predict(generate_matrix(200.f, sd)) << std::endl;

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}

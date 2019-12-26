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

#include <c4/logger.hpp>
#include <c4/json.hpp>
#include <c4/jpeg.hpp>
#include <c4/drawing.hpp>
#include <c4/meta_data_set.hpp>

#include <c4/shape_predictor_trainer.hpp>

#include <random>

int main(int argc, const char** argv) {
    try {
		if(argc != 3)
			THROW_EXCEPTION("train-shape-predictor params: <train.json> <test.json>");

        c4::meta_data_set faces_train, faces_test;

        faces_train.load_dlib("C:/portraits-data/ibug_300W_large_face_landmark_dataset/", argv[1], 1.5, 1, true);
        faces_train.add_noise_to_rects(0.08f);
        faces_test.load_dlib("C:/portraits-data/ibug_300W_large_face_landmark_dataset/", argv[2], 1.5, 1, true);
        faces_test.add_noise_to_rects(0.08f);

        std::vector<c4::matrix<uint8_t>> images_train(faces_train.data.size());
        c4::parallel_for(c4::range(images_train), [&](int i){
            c4::read_jpeg(faces_train.data[i].filepath, images_train[i]);
        });

        std::vector<c4::matrix<uint8_t>> images_test(faces_test.data.size());
        c4::parallel_for(c4::range(images_test), [&](int i) {
            c4::read_jpeg(faces_test.data[i].filepath, images_test[i]);
        });

        PRINT_DEBUG(c4::isize(faces_train.data));
        PRINT_DEBUG(c4::isize(faces_test.data));

		c4::shape_predictor_trainer trainer;

        trainer.train(images_train, faces_train.data, images_test, faces_test.data);
	} catch (std::exception &e) {
            LOGE << "Error: " << e.what() << " caught at " << __FILE__ << ":" << __LINE__;
    }

	return 0;
}

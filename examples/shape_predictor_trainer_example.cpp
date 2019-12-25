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

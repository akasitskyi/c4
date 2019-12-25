#pragma once

#include <numeric>

#include "shape_predictor.hpp"

#include "range.hpp"
#include "parallel.hpp"
#include "exception.hpp"
#include "progress_indicator.hpp"

namespace c4 {
	double test_shape_predictor(const shape_predictor& sp, const std::vector<c4::matrix<uint8_t>>& images, const std::vector<c4::image_file_metadata>& objects) {
        int sn = 0;
        for (const auto& vo : objects)
            sn += c4::isize(vo.objects) * sp.landmarks_count();

        double sum = c4::parallel_reduce(c4::range(objects), 1, 0., std::plus<double>(), [&](c4::range r) {
            double sum = 0;
            
            for (int i : r) {
                for(const auto& o : objects[i].objects) {
                    c4::object_on_image det = sp(images[i], o.rect);

                    auto d = o.landmarks - det.landmarks;

                    sum += sqrt(c4::dot_product(d, d)) / (o.rect.w + o.rect.h);
                }
            }
            
            return sum;
        });

		return sum / sn;
	}

	struct shape_predictor_trainer {
		c4::fast_rand rnd_uint;
        c4::fast_rand_float_uniform rnd_float;
        c4::fast_rand_float_normal rnd_normal;
	
        int tree_depth = 4;
        int num_trees = 2000;
        float nu = 0.2f;
        int oversampling_amount = 10;
        float lambda0 = 0.2f;
        int num_test_splits = 64;
        int dump_step = 100;
        
        shape_predictor_trainer() {}

        std::string make_filename(int trees) const {
			return "sp_" + std::to_string(oversampling_amount) + "os_" + std::to_string(num_test_splits) + "ts_" + std::to_string(int(nu * 100 + 0.5)) + "nu_" + std::to_string(trees) + "t.dat.ulz";
		}

        struct sample_range {
            c4::range r;
            std::vector<c4::point<float>> sum;
        };

        void train(const std::vector<c4::matrix<uint8_t>>& images, const std::vector<c4::image_file_metadata>& objects, const std::vector<c4::matrix<uint8_t>>& testImages, const std::vector<c4::image_file_metadata>& testObjects) {
            c4::scoped_timer timer("ShapePredictorTrainer::train");

            using namespace impl;
			ASSERT_EQUAL(images.size(), objects.size());

            const int num_parts = c4::isize(objects[0].objects[0].landmarks);

            for(const auto& v : objects)
                for(const auto& o : v.objects)
					ASSERT_EQUAL(c4::isize(o.landmarks), num_parts);

            std::vector<training_sample> samples;
            const std::vector<c4::point<float>> initial_shape = populate_training_sample_shapes(images, objects, samples);

            c4::progress_indicator pbar(num_trees);

			std::cout << "Fitting trees..." << "samples.size() = " << samples.size() << std::endl;

            std::vector<impl::regression_tree> forests;

            std::vector<std::string> stats;

            auto loss_str = [&](shape_predictor& sp) {
                double scoreTrain = test_shape_predictor(sp, images, objects);
                double scoreTest = test_shape_predictor(sp, testImages, testObjects);

                return std::to_string(c4::isize(forests)) + "\t train = " + std::to_string(scoreTrain) + ", test = " + std::to_string(scoreTest);
            };

            std::cout << loss_str(shape_predictor(initial_shape, forests)) << std::endl;

            float lambda = lambda0;
            while(c4::isize(forests) < num_trees) {
                impl::regression_tree tree;

                const int num_split_nodes = (1 << tree_depth) - 1;
                const int num_leafs = 1 << tree_depth;
                std::vector<sample_range> ranges_and_sums(num_split_nodes + num_leafs, { c4::range(samples), std::vector<c4::point<float>>(initial_shape.size()) });

                for (int i : c4::range(samples))
                    ranges_and_sums[0].sum += samples[i].target_shape - samples[i].current_shape;

                for (int i : c4::range(tree_depth)) {
                    const int begin = (1 << i) - 1;
                    const int end = left_child(begin);

                    const impl::split_feature split = generate_split(initial_shape, samples, begin, end, images, ranges_and_sums, lambda);
                    tree.splits.push_back(split);

                    for (int j : c4::range(begin, end)) {
                        const auto& rj = ranges_and_sums[j].r;
                        int mid = int(std::partition(samples.begin() + rj.begin_, samples.begin() + rj.end_, [&](const training_sample& s) {
                            return split.evaluate(images[s.image_idx], s.current_shape, s.tform_to_img);
                        }) - samples.begin());

                        ranges_and_sums[left_child(j)].r.begin_ = rj.begin_;
                        ranges_and_sums[left_child(j)].r.end_ = mid;
                        ranges_and_sums[right_child(j)].r.begin_ = mid;
                        ranges_and_sums[right_child(j)].r.end_ = rj.end_;
                    }
                }

                tree.leaf_values.resize(num_leafs, regression_tree::leaf_value_t(samples[0].target_shape.size()));

                for (int i : c4::range(tree.leaf_values)) {
                    const auto& rns = ranges_and_sums[num_split_nodes + i];
                    if (rns.r.size() == 0)
                        continue;

                    const auto v = rns.sum * (nu / rns.r.size());
                    std::copy(v.begin(), v.end(), tree.leaf_values[i].begin());

                    for (int j : rns.r) {
                        samples[j].current_shape += v;
                    }
                }

                forests.push_back(tree);
                lambda *= 0.999f;
			
                pbar.did_some(1);

				if( c4::isize(forests) % dump_step == 0 ){
                    std::string filename = make_filename(c4::isize(forests));
					c4::save_compressed(shape_predictor(initial_shape, forests), filename);

                    shape_predictor sp;
                    c4::load_compressed(filename, sp);
					
                    std::string s = loss_str(sp);
                    std::cout << s << std::endl;
                    stats.push_back(s);
				}
			}
        }

    private:

        struct training_sample {
            int image_idx;
			
			c4::affine_trasform<float> tform_to_img;

            std::vector<c4::point<float>> target_shape;

            std::vector<c4::point<float>> current_shape;

            void swap(training_sample& item){
                std::swap(image_idx, item.image_idx);
                std::swap(tform_to_img, item.tform_to_img);
				target_shape.swap(item.target_shape);
                current_shape.swap(item.current_shape);
            }
        };

        impl::split_feature randomly_generate_split_feature(const std::vector<c4::point<float>>& initial_shape, const float lambda) {
            const int parts = c4::isize(initial_shape);

            impl::split_feature feat;

            do {
                feat.l1 = rnd_uint() % parts;
                feat.l2 = rnd_uint() % parts;
            } while (feat.l1 >= feat.l2);

            feat.alpha_num = rnd_uint() % (impl::split_feature::ALPHA_DENOM + 3) - 1;

            feat.d1 = rnd_normal() * lambda / sqrt(2.f);
            feat.d2 = rnd_normal() * lambda / sqrt(2.f);

            float norm = (initial_shape[feat.l1] - initial_shape[feat.l2]).length();
            ASSERT_TRUE(norm > 0);

            feat.d1 /= norm;
            feat.d2 /= norm;

            return feat;
        }

        impl::split_feature generate_split(const std::vector<c4::point<float>>& initial_shape, const std::vector<training_sample>& samples, int begin, int end, const std::vector<c4::matrix<uint8_t>>& images, std::vector<sample_range>& ranges_and_sums, const float lambda) {
            // generate a bunch of random splits and test them and return the best one.

            // sample the random features we test in this function
            std::vector<impl::split_feature> feats;
            feats.reserve(num_test_splits);
            for (int i : c4::range(num_test_splits))
                feats.push_back(randomly_generate_split_feature(initial_shape, lambda));

            using namespace impl;

            const shape_t zero_shape(samples[0].current_shape.size());

            std::vector<std::vector<shape_t>> left_sums(num_test_splits, std::vector<shape_t>(ranges_and_sums.size(), zero_shape));

            std::vector<double> scores(num_test_splits);

            // now compute the sums of vectors that go left for each feature
            c4::parallel_for(c4::range(num_test_splits), 1, [&](int i) {
                auto& feat = feats[i];

                std::vector<shape_t> lhs(ranges_and_sums.size(), zero_shape);
                std::vector<shape_t> rhs(ranges_and_sums.size());
                std::vector<int> lci(ranges_and_sums.size(), 0);

                std::vector<std::tuple<float, int, int>> f(samples.size());

                for (int k : c4::range(begin, end)) {
                    for (int j : ranges_and_sums[k].r) {
                        std::get<0>(f[j]) = feat.f(images[samples[j].image_idx], samples[j].current_shape, samples[j].tform_to_img);
                        std::get<1>(f[j]) = k;
                        std::get<2>(f[j]) = j;
                    }
                }

                std::sort(f.begin(), f.end(), [](std::tuple<float, int, int> a, std::tuple<float, int, int> b) {return std::get<0>(a) > std::get<0>(b); });

                for (int l : c4::range(c4::isize(f) - 1)) {
                    const int k = std::get<1>(f[l]);
                    const int j = std::get<2>(f[l]);

                    lhs[k] += samples[j].target_shape;
                    lhs[k] -= samples[j].current_shape;
                    lci[k]++;

                    if (std::get<0>(f[l]) == std::get<0>(f[l + 1]))
                        continue;

                    double score = 0.0;

                    for (int k : c4::range(begin, end)) {
                        const int rci = ranges_and_sums[k].r.size() - lci[k];

                        if (lci[k]) {
                            score += c4::dot_product(lhs[k], lhs[k]) / lci[k];
                        }

                        if (rci) {
                            rhs[k] = ranges_and_sums[k].sum;
                            rhs[k] -= lhs[k];
                            score += c4::dot_product(rhs[k], rhs[k]) / rci;
                        }
                    }

                    if (score > scores[i]) {
                        scores[i] = score;
                        feat.k = std::get<0>(f[l]);
                        left_sums[i] = lhs;
                    }
                }
            });

            const int best_feat = int(std::max_element(scores.begin(), scores.end()) - scores.begin());

            for (int k : c4::range(begin, end)) {
                ranges_and_sums[impl::left_child(k)].sum = left_sums[best_feat][k];
                ranges_and_sums[impl::right_child(k)].sum = ranges_and_sums[k].sum - left_sums[best_feat][k];
            }

            return feats[best_feat];
        }

        std::vector<c4::point<float>> populate_training_sample_shapes(const std::vector<c4::matrix<uint8_t>>& images, const std::vector<c4::image_file_metadata>& objects, std::vector<training_sample>& samples) {
            samples.clear();
            std::vector<c4::point<float>> mean_shape(objects[0].objects[0].landmarks.size());
            int count = 0;

            for(int i : c4::range(objects)){
                for(auto& o : objects[i].objects){
                    training_sample sample;
                    sample.image_idx = i;
                    sample.tform_to_img = impl::unnormalizing_tform(o.rect);

                    std::transform(o.landmarks.begin(), o.landmarks.end(), std::back_inserter(sample.target_shape), sample.tform_to_img.inverse());

                    std::fill_n(std::back_inserter(samples), oversampling_amount, sample);

                    mean_shape += sample.target_shape;
                    ++count;
                }
            }

            mean_shape = mean_shape * (1.f / count);

            for(int i : c4::range(samples)){
                if (i % oversampling_amount == 0){
                    samples[i].current_shape = mean_shape;
                }else{
                    const int rand_idx1 = rnd_uint()%samples.size();
                    const int rand_idx2 = rnd_uint()%samples.size();
                    const float alpha = rnd_float();
                    samples[i].current_shape = alpha*samples[rand_idx1].target_shape + (1-alpha)*samples[rand_idx2].target_shape;
                }
            }

            return mean_shape;
        }
    };
};// namespace zeroalex

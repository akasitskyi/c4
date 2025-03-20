//MIT License
//
//Copyright(c) 2025 Alex Kasitskyi
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

#pragma once

#include <memory>
#include <deque>

#include "motion_detection.hpp"


namespace c4 {
	class VideoStabilization {
	public:
		class FrameReader {
		public:
			virtual bool read(matrix<uint8_t>& frame) = 0;
		};

		class FrameWriter {
		public:
			virtual void write(const matrix<uint8_t>& frame) = 0;
		};

		VideoStabilization(std::shared_ptr<FrameReader> reader, std::shared_ptr<FrameWriter> writer)
			: reader(reader), writer(writer) {
		}
		
		void run(const MotionDetector::Params &params, const std::vector<rectangle<int>> ignore = {}) {
			c4::scoped_timer timer("VideoStabilization::run()");

			MotionDetector md;

			const int q_length = 25;

			std::deque<matrix<uint8_t>> frame_q;
			std::deque<MotionDetector::Motion> motion_q;

			MotionDetector::Motion accMotion;

			matrix<uint8_t> frame;
			if (!reader->read(frame)) {
				THROW_EXCEPTION("Failed to read first frame");
			}
			frame_q.push_back(frame);
			motion_q.push_back(accMotion);

			matrix_dimensions frame_dims = frame.dimensions();

			int counter = 0;

			bool done = false;
			do {
				if(!done){
					done = !reader->read(frame);
				}
				
				if(!done){
					frame_q.push_back(frame);

					matrix<uint8_t> &prev = frame_q[frame_q.size() - 2];
					matrix<uint8_t> &cur = frame_q[frame_q.size() - 1];

					motion_q.push_back(md.detect(prev, cur, params, ignore));
				}

				if (frame_q.size() == q_length || done) {
					const matrix<uint8_t>& frame = frame_q.front();

					const MotionDetector::Motion avgMotion = average(motion_q);
					MotionDetector::Motion curMotion = motion_q.front();

					curMotion.shift -= avgMotion.shift;
					curMotion.scale /= avgMotion.scale;
					curMotion.alpha -= avgMotion.alpha;

					accMotion = accMotion.combine(curMotion);
					
					matrix<uint8_t> stabilized(frame.height(), frame.width());
					
					for (int i : range(frame.height())) {
						for (int j : range(frame.width())) {
							point<double> p(j, i);
							point<double> p1 = accMotion.apply(frame, p);
							stabilized[i][j] = frame.get_interpolate(p1);
						}
					}
					
					c4::draw_string(stabilized, 20, 15, "frame " + c4::to_string(++counter, 3), uint8_t(255), uint8_t(0), 2);

					c4::draw_string(stabilized, 20, 45, "shift: " + c4::to_string(accMotion.shift.x, 2) + ", " + c4::to_string(accMotion.shift.y, 2)
						+ ", scale: " + c4::to_string(accMotion.scale, 4)
						+ ", alpha: " + c4::to_string(accMotion.alpha, 4), uint8_t(255), uint8_t(0), 2);

					writer->write(stabilized);
					frame_q.pop_front();
					motion_q.pop_front();
				}
			} while (!frame_q.empty());
		}

	private:
		std::shared_ptr<FrameReader> reader;
		std::shared_ptr<FrameWriter> writer;

		static MotionDetector::Motion average(const std::deque<MotionDetector::Motion>& motions) {
			MotionDetector::Motion sum;
			double lscale = 0;
			for (const auto& m : motions) {
				sum.shift = sum.shift + m.shift;
				lscale += std::log2(m.scale);
				sum.alpha += m.alpha;
			}
			sum.shift = sum.shift * (1. / motions.size());
			sum.scale = std::exp2(lscale / motions.size());
			sum.alpha /= motions.size();
			return sum;
		}
	};
};

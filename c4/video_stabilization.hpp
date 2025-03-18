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

#include "motion_detection.hpp"
#include <deque>


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
		
		void run() {
			c4::scoped_timer timer("VideoStabilization::run()");

			MotionDetector md;
			MotionDetector::Params params;

			const int q_length = 2;

			do {
				matrix<uint8_t> frame;
				bool done = !reader->read(frame);

				if(!done){
					q.push_back(frame);
				}

				if (q.size() > 1) {
					matrix<uint8_t> &prev = q[0];
					matrix<uint8_t> &cur = q[1];

					auto motion = md.detect(prev, cur, params);
					c4::point<double> mid(frame.width() / 2, frame.height() / 2);

					matrix<uint8_t> stabilized(frame.height(), frame.width());
					
					for (int i : range(stabilized.height())) {
						for (int j : range(stabilized.width())) {
							point<double> p(j, i);
							point<double> p1 = motion.apply(cur, p);
							stabilized[i][j] = cur.get_interpolate(p1);
						}
					}

					cur = stabilized;
				}
				if (q.size() == q_length || done) {
					writer->write(q.front());
					q.pop_front();
				}
			} while (!q.empty());
		}

	private:
		std::shared_ptr<FrameReader> reader;
		std::shared_ptr<FrameWriter> writer;

		std::deque<matrix<uint8_t>> q;
	};
};

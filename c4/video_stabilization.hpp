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
#include "parallel.hpp"

namespace c4 {
	class VideoStabilization {
	public:
		typedef matrix<uint8_t> Frame;
		typedef std::shared_ptr<Frame> FramePtr;

		struct Params : public MotionDetector::Params {
			int x_smooth = 50;
			int y_smooth = 50;
			int scale_smooth = 50;
			int alpha_smooth = 50;
		};

		VideoStabilization(const Params &params, const std::vector<rectangle<int>> ignore = {})
			: params(params), ignore(ignore) {
		}

		MotionDetector::Motion process(FramePtr frame) {
			c4::scoped_timer timer("VideoStabilization::process()");

			const int q_length = std::max(std::max(params.x_smooth, params.y_smooth), std::max(params.alpha_smooth, params.scale_smooth));

			if (prev) {
				MotionDetector::Motion curMotion = MotionDetector::detect(*prev, *frame, params, ignore);
				
				motion_q.push_back(curMotion);
				if (motion_q.size() > q_length) {
					motion_q.pop_front();
				}

				const MotionDetector::Motion avgMotion = average();

				curMotion.shift -= avgMotion.shift;
				curMotion.scale /= avgMotion.scale;
				curMotion.alpha -= avgMotion.alpha;

				accMotion = accMotion.combine(curMotion);
			}

			prev = frame;

			return accMotion;
		}

	private:
		Params params;
		std::vector<rectangle<int>> ignore;
		
		std::deque<MotionDetector::Motion> motion_q;
		FramePtr prev = nullptr;
		MotionDetector::Motion accMotion;

		MotionDetector::Motion average() const {
			MotionDetector::Motion sum;
			if (motion_q.empty()) {
				return sum;
			}
			double lscale = 0;
			for (int i = 0; i < isize(motion_q); i++) {
				const auto& m = motion_q[isize(motion_q) - i - 1];

				if (i < params.x_smooth)
					sum.shift.x += m.shift.x;
				if (i < params.y_smooth)
					sum.shift.y += m.shift.y;
				if (i < params.scale_smooth)
					lscale += std::log2(m.scale);
				if (i < params.alpha_smooth)
					sum.alpha += m.alpha;
			}

			sum.shift.x /= std::min(isize(motion_q), params.x_smooth);
			sum.shift.y /= std::min(isize(motion_q), params.y_smooth);
			sum.scale = std::exp2(lscale / std::min(isize(motion_q), params.scale_smooth));
			sum.alpha /= std::min(isize(motion_q), params.alpha_smooth);

			return sum;
		}
	};
};

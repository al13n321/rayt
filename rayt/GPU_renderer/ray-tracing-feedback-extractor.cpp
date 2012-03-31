#include "ray-tracing-feedback-extractor.h"
#include "debug.h"
#include "profiler.h"
using namespace std;
using namespace boost;

namespace rayt {

	const int NO_HIT = (1 << 30) - 1 + (1 << 30);

	RayTracingFeedbackExtractor::RayTracingFeedbackExtractor(int frame_width, int frame_height, shared_ptr<CLContext> context) {
		frame_width_ = frame_width;
		frame_height_ = frame_height;
		count_ = frame_width * frame_height;
		context_ = context;
		
		fault_blocks_.reserve(count_);
		hit_blocks_.reserve(count_);
		sorted_.Resize(count_);
		
		expected_faults_ = 1 << 16;
		hits_to_extract_ = 4000;

		sort_.reset(new GPUSort(count_, context));
		scan_.reset(new GPUScan(count_, context));

		index_buffer_.reset(new CLBuffer(0, count_ * sizeof(cl_int), context));
		unique_buffer_.reset(new CLBuffer(0, count_ * sizeof(cl_int), context));

		node_to_block_kernel_  .reset(new CLKernel("GPU_renderer/kernels", "feedback_compression.cl", "", "NodeToBlockKernel"   , context));
		difference_flag_kernel_.reset(new CLKernel("GPU_renderer/kernels", "feedback_compression.cl", "", "DifferenceFlagKernel", context));
		compress_kernel_       .reset(new CLKernel("GPU_renderer/kernels", "feedback_compression.cl", "", "CompressKernel"      , context));
	}

	RayTracingFeedbackExtractor::~RayTracingFeedbackExtractor() {}
        
	void RayTracingFeedbackExtractor::Process(CLBuffer &faults_and_hits, int nodes_in_block) {
		GetUnique(faults_and_hits, nodes_in_block);
		ProcessUnique();
		AdjustExpectedFaults();
	}

	void RayTracingFeedbackExtractor::GetUnique(CLBuffer &faults_and_hits, int nodes_in_block) {
		CLEvent ev;

		PROFILE_TIMER_START("GPU");

		node_to_block_kernel_->SetIntArg(0, count_);
		node_to_block_kernel_->SetIntArg(1, nodes_in_block);
		node_to_block_kernel_->SetBufferArg(2, &faults_and_hits);
		node_to_block_kernel_->Run1D(count_, NULL, &ev);

		ev.WaitFor();

		PROFILE_TIMER_START("GPU sort");

		sort_->Sort(faults_and_hits);

		PROFILE_TIMER_STOP();

		difference_flag_kernel_->SetIntArg(0, count_);
		difference_flag_kernel_->SetBufferArg(1, &faults_and_hits);
		difference_flag_kernel_->SetBufferArg(2, index_buffer_.get());
		difference_flag_kernel_->Run1D(count_, NULL, &ev);

		ev.WaitFor();

		PROFILE_TIMER_START("GPU scan");

		scan_->Scan(*index_buffer_);

		PROFILE_TIMER_STOP();

		PROFILE_TIMER_STOP(); // GPU

		CLEventList wait;

		PROFILE_TIMER_START("GPU/BUS compr/size");

		index_buffer_->Read((count_ - 1) * sizeof(cl_int), sizeof(cl_int), &unique_count_, NULL, &ev);

		wait.Add(ev);

		compress_kernel_->SetIntArg(0, count_);
		compress_kernel_->SetBufferArg(1, index_buffer_.get());
		compress_kernel_->SetBufferArg(2, &faults_and_hits);
		compress_kernel_->SetBufferArg(3, unique_buffer_.get());
		compress_kernel_->Run1D(count_, NULL, &ev);

		wait.Add(ev);

		wait.WaitFor();

		PROFILE_TIMER_STOP();

		++unique_count_;
	}

	void RayTracingFeedbackExtractor::ProcessUnique() {
		CLEvent ev;

		fault_blocks_.clear();
		hit_blocks_.clear();

		int stride = expected_faults_;
		int size = 0;
		while (size < unique_count_ && (size == 0 || sorted_[size - 1] < 0)) { // read while not all faults are read
			stride = min(stride, unique_count_ - size);
		
			PROFILE_TIMER_START("BUS");
			PROFILE_TIMER_START("BUS read faults");

			unique_buffer_->Read(size * sizeof(cl_int), stride * sizeof(cl_int), sorted_.begin() + size, NULL, &ev);

			ev.WaitFor();

			PROFILE_TIMER_STOP();
			PROFILE_TIMER_STOP();

			for (int i = size; i < size + stride; ++i) {
				assert(sorted_[i] != 0);
				if (sorted_[i] < 0) {
					fault_blocks_.push_back(-sorted_[i] - 1);
				} else {
					if (sorted_[i] != NO_HIT)
						hit_blocks_.push_back(sorted_[i] - 1);
				}
			}

			size += stride;
			stride *= 2;
		}

		// extract a random range of hits
		if (size < unique_count_) {
			int hits_length = unique_count_ - size; // don't count hits that are already loaded
			int hit_seg_start = rand() % hits_length;
			int hit_seg_length = min(hits_to_extract_, hits_length);

			// break cyclic range into one or two ordinary ranges
			vector<pair<int, int> > segs;
			if (hit_seg_start + hit_seg_length <= hits_length) {
				segs.push_back(make_pair(hit_seg_start, hit_seg_length));
			} else {
				segs.push_back(make_pair(0, hit_seg_start + hit_seg_length - hits_length));
				segs.push_back(make_pair(hit_seg_start, hits_length - hit_seg_start));
			}

			for (int si = 0; si < (int)segs.size(); ++si) {

				PROFILE_TIMER_START("BUS");
				PROFILE_TIMER_START("BUS read hits");

				unique_buffer_->Read((size + segs[si].first) * sizeof(cl_int), segs[si].second * sizeof(cl_int), sorted_.begin(), NULL, &ev);

				ev.WaitFor();

				PROFILE_TIMER_STOP();
				PROFILE_TIMER_STOP();

				for (int i = 0; i < segs[si].second; ++i) {
					assert(sorted_[i] > 0);
					if (sorted_[i] != NO_HIT)
						hit_blocks_.push_back(sorted_[i] - 1);
				}
			}
		}
	}

	void RayTracingFeedbackExtractor::AdjustExpectedFaults() {
		if (expected_faults_ > 1 && (int)fault_blocks_.size() < expected_faults_)
			expected_faults_ /= 2;
		else if ((int)fault_blocks_.size() > expected_faults_)
			expected_faults_ *= 2;
	}

	int RayTracingFeedbackExtractor::hits_to_extract() const {
		return hits_to_extract_;
	}

	void RayTracingFeedbackExtractor::set_hits_to_extract(int value) {
		hits_to_extract_ = value;
	}

	const std::vector<int>& RayTracingFeedbackExtractor::fault_blocks() const {
		return fault_blocks_;
	}

	const std::vector<int>& RayTracingFeedbackExtractor::hit_blocks() const {
		return hit_blocks_;
	}

}

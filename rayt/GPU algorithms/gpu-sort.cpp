#include "gpu-sort.h"
using namespace std;

namespace rayt {

	GPUSort::GPUSort(int length, boost::shared_ptr<CLContext> context) {
		length_ = length;
		context_ = context;
		temp_buffer_.reset(new CLBuffer(0, length_ * sizeof(cl_int), context));
		local_sort_kernel_.reset(new CLKernel("GPU algorithms/kernels/merge_sort.cl", "", "LocalMergeSortKernel", context));
		merge_sort_pass_kernel_.reset(new CLKernel("GPU algorithms/kernels/merge_sort.cl", "", "GlobalMergeSortPassKernel", context));
	}

	GPUSort::~GPUSort() {}
        
	void GPUSort::Sort(CLBuffer &data) {
		CLEventList wait;
		CLEvent ev;

		local_sort_kernel_->SetIntArg(0, length_);
		local_sort_kernel_->SetBufferArg(1, &data);
		local_sort_kernel_->SetLocalBufferArg(2, local_sort_kernel_->work_group_size() * sizeof(cl_int));
		local_sort_kernel_->Run1D(length_, NULL, &ev);

		CLBuffer *in = &data;
		CLBuffer *out = temp_buffer_.get();

		for (int len = local_sort_kernel_->work_group_size(); len < length_; len *= 2) {
			wait.Clear();
			wait.Add(ev);

			merge_sort_pass_kernel_->SetIntArg(0, length_);
			merge_sort_pass_kernel_->SetIntArg(1, len);
			merge_sort_pass_kernel_->SetBufferArg(2, in);
			merge_sort_pass_kernel_->SetBufferArg(3, out);
			merge_sort_pass_kernel_->Run1D(length_, &wait, &ev);

			swap(in, out);
		}

		if (in != &data) {
			wait.Clear();
			wait.Add(ev);

			data.CopyFrom(*in, &wait, &ev);
		}

		ev.WaitFor();
	}

}

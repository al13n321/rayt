#include "gpu-scan.h"
using namespace std;

namespace rayt {

	GPUScan::GPUScan(int length, boost::shared_ptr<CLContext> context) {
		length_ = length;
		context_ = context;
		temp_buffer_.reset(new CLBuffer(0, length_ * sizeof(cl_int), context));
		scan_pass_kernel_.reset(new CLKernel("GPU algorithms/kernels/scan.cl", "", "ScanPassKernel", context));
	}

	GPUScan::~GPUScan() {}
        
	void GPUScan::Scan(CLBuffer &data) {
		CLEventList wait;
		CLEvent ev;

		CLBuffer *in = &data;
		CLBuffer *out = temp_buffer_.get();

		for (int len = 1; len < length_; len *= 2) {
			scan_pass_kernel_->SetIntArg(0, length_);
			scan_pass_kernel_->SetIntArg(1, len);
			scan_pass_kernel_->SetBufferArg(2, in);
			scan_pass_kernel_->SetBufferArg(3, out);
			scan_pass_kernel_->Run1D(length_, &wait, &ev);

			wait.Clear();
			wait.Add(ev);

			swap(in, out);
		}

		if (in != &data) {
			data.CopyFrom(*in, &wait, &ev);

			wait.Clear();
			wait.Add(ev);
		}

		wait.WaitFor();
	}

}

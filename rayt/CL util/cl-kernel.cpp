#include "cl-kernel.h"
#include "string-util.h"
#include "kernel-preprocessor.h"
using namespace std;
using namespace boost;

namespace rayt {

    CLKernel::CLKernel(std::string file_name, std::string params, std::string kernel_name, shared_ptr<CLContext> context) {
        assert(context);
        
        context_ = context;
        
		string text;
		
		if (!ReadFile(file_name, text))
			crash("failed to read " + file_name);
        
        int err;
        
        const char *text_c = text.c_str();
        program_ = clCreateProgramWithSource(context->context(), 1, &text_c, NULL, &err);
        if (!program_ || err != CL_SUCCESS)
            crash("failed to create program");
        
        cl_device_id device_id = context_->device_id();
        int build_err = clBuildProgram(program_, 1, &device_id, params.c_str(), NULL, NULL);

		size_t len;
        static char buffer[65536];
        
        err = clGetProgramBuildInfo(program_, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer) - 1, buffer, &len);
        if (err != CL_SUCCESS)
            crash("failed to get program build log");
        buffer[len] = 0;
        cout << buffer << endl;

        if (build_err != CL_SUCCESS)
            crash("failed to build program");
        
        kernel_ = clCreateKernel(program_, kernel_name.c_str(), &err);
        if (!kernel_ || err != CL_SUCCESS)
            crash("failed to create compute kernel");
        
        size_t wgsize;
        err = clGetKernelWorkGroupInfo(kernel_, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &wgsize, NULL);
        if (err != CL_SUCCESS)
            crash("failed to retrieve kernel work group info");
        
        work_group_size_ = static_cast<int>(wgsize);
    }
    
    CLKernel::~CLKernel() {
        context_->WaitForAll();
        clReleaseKernel(kernel_);
        clReleaseProgram(program_);
    }
    
    void CLKernel::SetArg(int index, int size, void *data) {
        assert(index >= 0);
        assert(size > 0);
        assert(data);
        int err = clSetKernelArg(kernel_, index, size, data);
        if (err != CL_SUCCESS)
            crash("failed to set kernel arg");
    }
    
    void CLKernel::SetIntArg(int index, int val) {
        SetArg(index, sizeof(cl_int), &val);
    }
    
    void CLKernel::SetFloatArg(int index, float val) {
        SetArg(index, sizeof(cl_float), &val);
    }
    
    void CLKernel::SetBufferArg(int index, const CLBuffer *buf) {
        cl_mem mem = buf->buffer();
        SetArg(index, sizeof(cl_mem), &mem);
    }
    
    void CLKernel::SetFloat16Arg(int index, fmat4 mat) {
        SetArg(index, sizeof(cl_float16), mat.m);
    }
    
    void CLKernel::Run2D(int size0, int size1, bool blocking) {
        int s0 = 1;
        while (s0 < size0 && s0 * s0 < work_group_size_) // something nearly square
            s0 *= 2;
        s0 = min(s0, size0);
        int s1 = min(work_group_size_ / s0, size1);
        
        size_t local[2] = { s0, s1 };
        size_t global[2] = { (size0 + s0 - 1) / s0 * s0, (size1 + s1 - 1) / s1 * s1 };
        
        int err = clEnqueueNDRangeKernel(context_->queue(), kernel_, 2, NULL, global, local, 0, NULL, NULL);
        if (err != CL_SUCCESS)
            crash("failed to enqueue kernel");
        
        if (blocking)
            context_->WaitForAll();
    }
    
}

#pragma once

#include "cl-context.h"
#include "cl-buffer.h"
#include "mat.h"

namespace rayt {

    class CLKernel {
    public:
        CLKernel(std::string filename, std::string params, std::string kernel_name, boost::shared_ptr<CLContext> context);
        ~CLKernel();
        
        void SetArg(int index, int size, void *data);
        void SetIntArg(int index, int val);
        void SetFloatArg(int index, float val);
        void SetBufferArg(int index, const CLBuffer *buf); // please don't modify (in kernel) contents of const CLBuffer
        void SetFloat16Arg(int index, fmat4 mat);
        
        void Run2D(int size0, int size1, bool blocking);
    private:
        boost::shared_ptr<CLContext> context_;
        cl_program program_;
        cl_kernel kernel_;
        int work_group_size_;
        
        DISALLOW_COPY_AND_ASSIGN(CLKernel);
    };
    
}

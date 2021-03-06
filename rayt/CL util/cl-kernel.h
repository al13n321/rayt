#pragma once

#include "cl-context.h"
#include "cl-buffer.h"
#include "mat.h"

namespace rayt {

    class CLKernel {
    public:
		CLKernel(std::string file_name, std::string params, std::string kernel_name, boost::shared_ptr<CLContext> context);

		// file_name relative to include_dir
		CLKernel(std::string include_dir, std::string file_name, std::string params, std::string kernel_name, boost::shared_ptr<CLContext> context);

        ~CLKernel();
        
		int work_group_size() const;

        void SetArg(int index, int size, void *data);
		void SetLocalBufferArg(int index, int size); // equivalent to SetArg(index, size, NULL)
        void SetIntArg(int index, int val);
        void SetFloat4Arg(int index, fvec3 xyz, float w);
        void SetFloatArg(int index, float val);
        void SetBufferArg(int index, const CLBuffer *buf); // please don't modify (in kernel) contents of const CLBuffer
        void SetFloat16Arg(int index, fmat4 mat);
        
		void Run1D(int size, const CLEventList *wait_list, CLEvent *out_event);
        void Run2D(int size0, int size1, const CLEventList *wait_list, CLEvent *out_event);
    private:
        boost::shared_ptr<CLContext> context_;
        cl_program program_;
        cl_kernel kernel_;
        int work_group_size_;
        
		void Init(std::string text, std::string params, std::string kernel_name, boost::shared_ptr<CLContext> context);

        DISALLOW_COPY_AND_ASSIGN(CLKernel);
    };
    
}

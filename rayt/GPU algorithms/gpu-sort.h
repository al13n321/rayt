#pragma once

#include "cl-buffer.h"
#include "cl-kernel.h"

namespace rayt {

	// sorts signed int's
    class GPUSort {
    public:
        GPUSort(int length, boost::shared_ptr<CLContext> context);
        ~GPUSort();
        
        void Sort(CLBuffer &data);
    private:
		boost::scoped_ptr<CLBuffer> temp_buffer_;
		boost::scoped_ptr<CLKernel> local_sort_kernel_;
		boost::scoped_ptr<CLKernel> merge_sort_pass_kernel_;
        int length_;
		boost::shared_ptr<CLContext> context_;
		
		DISALLOW_COPY_AND_ASSIGN(GPUSort);
    };
    
}

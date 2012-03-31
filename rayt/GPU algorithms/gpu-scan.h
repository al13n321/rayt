#pragma once

#include "cl-buffer.h"
#include "cl-kernel.h"

namespace rayt {

	// calculates inclusive prefix sums of signed int's
    class GPUScan {
    public:
        GPUScan(int length, boost::shared_ptr<CLContext> context);
        ~GPUScan();
        
        void Scan(CLBuffer &data);
    private:
		boost::scoped_ptr<CLBuffer> temp_buffer_;
		boost::scoped_ptr<CLKernel> scan_pass_kernel_;
        int length_;
		boost::shared_ptr<CLContext> context_;
		
		DISALLOW_COPY_AND_ASSIGN(GPUScan);
    };
    
}

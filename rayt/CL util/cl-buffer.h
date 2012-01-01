#pragma once

#include <boost/shared_ptr.hpp>
#include "cl-context.h"

namespace rayt {

    class CLBuffer {
    public:
        CLBuffer(cl_mem_flags flags, int size, boost::shared_ptr<CLContext> context);
        ~CLBuffer();
        
        cl_mem buffer(); // can be null
        
        void Write(int start, int length, const void *data);
    private:
        int size_;
        boost::shared_ptr<CLContext> context_;
        cl_mem buffer_;
    };
    
}

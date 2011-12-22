#pragma once

#include "common.h"

namespace rayt {

    class CLBuffer {
    public:
        CLBuffer(cl_context context, cl_mem_flags flags, int size);
        ~CLBuffer();
        
        cl_mem buffer(); // can be null
    private:
        cl_mem buffer_;
    };
    
}
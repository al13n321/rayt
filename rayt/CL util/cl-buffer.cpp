//
//  CLBuffer.cpp
//  rayt
//
//  Created by Michael Kolupaev on 12/23/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#include "cl-buffer.h"

namespace rayt {

    CLBuffer::CLBuffer(cl_context context, cl_mem_flags flags, int size) {
        buffer_ = clCreateBuffer(context, flags, size, NULL, NULL);
    }
    
    CLBuffer::~CLBuffer() {
        clReleaseMemObject(buffer_);
    }
    
    cl_mem CLBuffer::buffer() {
        return buffer_;
    }
    
}

//
//  CLBuffer.cpp
//  rayt
//
//  Created by Michael Kolupaev on 12/23/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#include "cl-buffer.h"
#include <cassert>

namespace rayt {

    CLBuffer::CLBuffer(cl_mem_flags flags, int size, boost::shared_ptr<CLContext> context) {
        assert(size > 0);
        assert(context);
        size_ = size;
        context_ = context;
        buffer_ = clCreateBuffer(context->context(), flags, size, NULL, NULL);
        assert(buffer_);
#ifdef DEBUG_CLBUFFER
		debug_buffer_ = new char[size];
#endif
    }
    
    CLBuffer::~CLBuffer() {
        clReleaseMemObject(buffer_);
#ifdef DEBUG_CLBUFFER
		delete[] debug_buffer_;
#endif
    }
    
    cl_mem CLBuffer::buffer() const {
        return buffer_;
    }
    
    int CLBuffer::size() const {
        return size_;
    }
    
    void CLBuffer::Write(int start, int length, const void *data, bool blocking) {
        assert(start >= 0 && start < size_);
        assert(length > 0 && start + length <= size_);
        assert(data);
		int ret = clEnqueueWriteBuffer(context_->queue(), buffer_, blocking, start, length, data, 0, NULL, NULL);
		assert(ret == CL_SUCCESS);

#ifdef DEBUG_CLBUFFER
		memcpy(debug_buffer_ + start, data, length);
#endif
    }
    
    void CLBuffer::Read(int start, int length, void *data, bool blocking) const {
        assert(start >= 0);
        assert(length > 0);
        assert(data);
		clFinish(context_->queue()); // to finish all writes; TODO: use events
        int ret = clEnqueueReadBuffer(context_->queue(), buffer_, blocking, start, length, data, 0, NULL, NULL);
		assert(ret == CL_SUCCESS);

#ifdef DEBUG_CLBUFFER
		clFinish(context_->queue());
		assert(!memcmp(debug_buffer_ + start, data, length));
#endif
    }
    
}

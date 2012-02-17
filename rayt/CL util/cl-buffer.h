#pragma once

#include <boost/shared_ptr.hpp>
#include "cl-context.h"

namespace rayt {

#define DEBUG_CLBUFFER // keep a copy of buffer data in memory and check all GPU reads against it

    class CLBuffer {
    public:
        CLBuffer(cl_mem_flags flags, int size, boost::shared_ptr<CLContext> context);
        ~CLBuffer();
        
        // NULL if failed to create buffer
        // please don't modify contents of buffer in const CLBuffer
        cl_mem buffer() const;
        int size() const;
        
        void Write(int start, int length, const void *data, bool blocking);
        void Read(int start, int length, void *data, bool blocking) const;
    private:
        int size_;
        boost::shared_ptr<CLContext> context_;
        cl_mem buffer_;

#ifdef DEBUG_CLBUFFER
		char *debug_buffer_;
#endif
    };
    
}

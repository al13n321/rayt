#pragma once

#include <boost/shared_ptr.hpp>
#include "cl-context.h"
#include "cl-event.h"

namespace rayt {

    class CLBuffer {
    public:
        CLBuffer(cl_mem_flags flags, int size, boost::shared_ptr<CLContext> context);
        ~CLBuffer();
        
        // NULL if failed to create buffer
        // please don't modify contents of buffer in const CLBuffer
        cl_mem buffer() const;
        int size() const;
        
		void CopyFrom(CLBuffer &buf, int src_start, int dest_start, int length, const CLEventList *wait_list, CLEvent *out_event);
		void CopyFrom(CLBuffer &buf, const CLEventList *wait_list, CLEvent *out_event);

		void Write(int start, int length, const void *data, bool blocking, const CLEventList *wait_list, CLEvent *out_event);
		
		void Read(int start, int length, void *data, const CLEventList *wait_list, CLEvent *out_event);

		// intended for debug purposes, so it is blocking and does clFinish before reading, expect poor performance
        void Read(int start, int length, void *data) const;

    private:
        int size_;
        boost::shared_ptr<CLContext> context_;
        cl_mem buffer_;

		DISALLOW_COPY_AND_ASSIGN(CLBuffer);
    };
    
}

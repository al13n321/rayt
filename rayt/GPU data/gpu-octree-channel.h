#pragma once

#include "common.h"
#include "cl-buffer.h"

namespace rayt {
    
    // TODO: image channels
    class GPUOctreeChannel {
    public:
        GPUOctreeChannel(cl_context context, int node_size, int max_nodes_count);
        
        // in bytes
        int node_size() const;
        
        // CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_WRITE_ONLY
        CLBuffer* cl_buffer();
    private:
        int node_size_;
        CLBuffer cl_buffer_;
    };
    
}
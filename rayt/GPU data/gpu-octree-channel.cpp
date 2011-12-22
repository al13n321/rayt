#include "gpu-octree-channel.h"

namespace rayt {
    
    GPUOctreeChannel::GPUOctreeChannel(cl_context context, int node_size, int max_nodes_count) : node_size_(node_size), cl_buffer_(context, CL_MEM_READ_ONLY, node_size * max_nodes_count) {}
    
    int GPUOctreeChannel::node_size() const {
        return node_size_;
    }
    
    CLBuffer* GPUOctreeChannel::cl_buffer() {
        return &cl_buffer_;
    }
    
}

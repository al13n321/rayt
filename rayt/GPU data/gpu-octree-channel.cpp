#include "gpu-octree-channel.h"
#include <cassert>
using namespace boost;

namespace rayt {
    
    GPUOctreeChannel::GPUOctreeChannel(int bytes_in_node, int max_nodes_count, std::string name, shared_ptr<CLContext> context) {
        assert(bytes_in_node > 0);
        assert(max_nodes_count > 0);
        bytes_in_node_ = bytes_in_node;
        name_ = name;
        context_ = context;
        cl_buffer_.reset(new CLBuffer(CL_MEM_READ_ONLY, max_nodes_count * bytes_in_node, context));
    }
    
    const std::string& GPUOctreeChannel::name() const {
        return name_;
    }
    
    int GPUOctreeChannel::bytes_in_node() const {
        return bytes_in_node_;
    }
    
    CLBuffer* GPUOctreeChannel::cl_buffer() {
        return cl_buffer_.get();
    }
    
}

#pragma once

#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include "cl-buffer.h"
#include "cl-context.h"

namespace rayt {
    
    // TODO: image channels
    class GPUOctreeChannel {
    public:
        GPUOctreeChannel(int bytes_in_node, int max_nodes_count, std::string name, boost::shared_ptr<CLContext> context);
        
        const std::string& name() const;
        
        // in bytes
        int bytes_in_node() const;
        
        // CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_WRITE_ONLY
        // please don't modify its contents from outside
        CLBuffer* cl_buffer();
    private:
        boost::shared_ptr<CLContext> context_;
        std::string name_;
        int bytes_in_node_;
        boost::scoped_ptr<CLBuffer> cl_buffer_;
    };
    
}

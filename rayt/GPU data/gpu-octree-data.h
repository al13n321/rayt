#pragma once

#include "common.h"
#include <vector>
#include "gpu-octree-channel.h"

namespace rayt {
    
    class GPUOctreeData {
    public:
        GPUOctreeData(int nodes_in_block, int max_blocks_count);
        
        GPUOctreeChannel *nodes_link_channel();
        const std::vector<GPUOctreeChannel>& channels(); // includes nodes_link_channel at index 0
        GPUOctreeChannel *AddChannel(int node_size);
        
        int bytes_in_block() const;
        
        // data must be bytes_in_block() bytes in length
        // data is a concatenation of data's for each channel in order: nodes_link_channel, added channels in order of addition
        // data for each channel is a concatenation of data's for each node of the block
        void UploadBlock(int index, const void *data);
    private:
        int nodes_in_block_;
        int max_nodes_count_;
        std::vector<GPUOctreeChannel> channels_;
    };
    
}

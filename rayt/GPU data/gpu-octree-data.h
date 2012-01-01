#pragma once

#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include "cl-context.h"
#include "gpu-octree-channel.h"

namespace rayt {
    
    // Contains a node link buffer and octree channels. Knows about blocking. Allows uploading raw bytes. Doesn't know about link consistency, blocks allocation, LRU.
    
    class GPUOctreeData {
    public:
        GPUOctreeData(int nodes_in_block, int max_blocks_count, boost::shared_ptr<CLContext> context);
        
        GPUOctreeChannel* AddChannel(int bytes_in_node, std::string name);
        GPUOctreeChannel* ChannelByName(std::string name);
        GPUOctreeChannel* ChannelByIndex(int index);
        
        int bytes_in_block() const;
        
        // data must be bytes_in_block() bytes in length
        // data is a concatenation of data's for each added channel in order of addition
        // data for each channel is a concatenation of data's for each node of the block
        void UploadBlock(int index, const void *data);
        
        // data must be bytes_count in length
        void UploadBlockPart(int channel_index, int block_index, int first_byte, int bytes_count, const void *data);
    private:
        boost::shared_ptr<CLContext> context_;
        int nodes_in_block_;
        int max_blocks_count_;
        std::vector<boost::shared_ptr<GPUOctreeChannel> > channels_;
        std::map<std::string, boost::shared_ptr<GPUOctreeChannel> > channel_by_name_;
    };
    
}

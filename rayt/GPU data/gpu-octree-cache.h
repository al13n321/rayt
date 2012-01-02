#pragma once

#include "gpu-octree-data.h"
#include "stored-octree-block.h"

namespace rayt {

    class GPUOctreeCache {
    public:
        GPUOctreeCache(int nodes_in_block, int max_blocks_count, boost::shared_ptr<CLContext> context);
        ~GPUOctreeCache();
        
        GPUOctreeChannel* AddChannel(int bytes_in_node, std::string name);
        GPUOctreeChannel* ChannelByName(std::string name);
        GPUOctreeChannel* ChannelByIndex(int index);
        
        int bytes_in_block() const;
        
        bool BlockUsed(int block_index_in_cache);
        int FaultBlockIndexByCachePointer(int node_index_in_cache);
        bool UploadBlock(const StoredOctreeBlock &block);
    private:
        boost::shared_ptr<CLContext> context_;
        
        DISALLOW_COPY_AND_ASSIGN(GPUOctreeCache);
    };
    
}

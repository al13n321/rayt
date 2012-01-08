#pragma once

#include <list>
#include "gpu-octree-data.h"
#include "stored-octree-block.h"

namespace rayt {

    // manages LRU cache, decides which block to unload, bot doesn't decide, which to load; manages links consistency: converts node links channel from storage format to GPU format (see docs)
    class GPUOctreeCache {
    public:
        GPUOctreeCache(int nodes_in_block, int max_blocks_count, const StoredOctreeChannelSet &channels, boost::shared_ptr<CLContext> context);
        ~GPUOctreeCache();
        
        const GPUOctreeData* data() const;
        const bool full() const;
        
        bool BlockInCache(int block_index) const;
        
        // returns -1 if block is not in cache
        int BlockIndexInCache(int block_index) const;
        
        void MarkBlockAsUsed(int block_index_in_cache);
        
        // modifies block contents!
        // if blocking is false, block should be valid until next clFinish or similar call
        // parent must be loaded
        void UploadBlock(StoredOctreeBlock &block, bool blocking);
    private:
        struct CachedBlockRoot {
            ushort pointer_index_in_parent;
            uchar parent_children_mask;
        };
        
        struct CachedBlockInfo {
            int block_index; // index in tree
            int parent_block_index; // index in cache
            int roots_count;
            CachedBlockRoot roots[8];
            std::list<int>::iterator lru_queue_iterator;
        };
        
        int nodes_in_block_;
        int max_blocks_count_;
        boost::shared_ptr<CLContext> context_;
        boost::scoped_ptr<GPUOctreeData> data_;
        std::map<int, int> block_to_cache_index_;
        std::vector<CachedBlockInfo> cache_contents_;
        std::list<int> lru_queue_; // least recently used is first, most recently used is last; any node is always closer to beginning than its parent
        int first_free_index_;
        
        int UnloadLRUBlock(bool blocking); // returns freed index in cache
        
        DISALLOW_COPY_AND_ASSIGN(GPUOctreeCache);
    };
    
}

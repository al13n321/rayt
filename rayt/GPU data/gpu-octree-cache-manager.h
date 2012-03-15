#pragma once

#include "stored-octree-loader.h"
#include "gpu-octree-data.h"
#include "gpu-octree-cache.h"

namespace rayt {

    // manages loading data to GPU octree cache; prevents infinite loading-unloading of data (when working set is larger than cache)
	// TODO: add methods to get recommendations about
	// a) finishing the session (because the cache is not big enough to hold everything requested)
	// b) decreasing detail level (for the same reason)
    class GPUOctreeCacheManager {
    public:
        GPUOctreeCacheManager(int max_blocks_count, boost::shared_ptr<StoredOctreeLoader> loader, boost::shared_ptr<CLContext> context);
        ~GPUOctreeCacheManager();
        
        const GPUOctreeData* data() const;
        int root_node_index() const;
        
        // session - a set of transactions; no block can be loaded more than once during a session; if some block was loaded during this session, then was thrown away from cache, then was requested again, this last request will be ignored; typically session is a frame
        void StartRequestSession();
        
        // transaction - a set of requests; the processing of the requests is guaranteed to be finished when transaction ends (but may be finished earlier); typically transaction is an iteration of feedback-driven loading
        void StartRequestTransaction();
        
		void MarkBlockAsUsed(int block_index_in_cache);
        void RequestBlock(int block_index);
        
        void EndRequestTransaction();
        
        void EndRequestSession();
    private:
        boost::shared_ptr<CLContext> context_;
        int max_blocks_count_;
        int root_node_index_;
        boost::shared_ptr<StoredOctreeLoader> loader_;
        boost::scoped_ptr<GPUOctreeCache> cache_;
        std::vector<boost::shared_ptr<StoredOctreeBlock> > uploaded_blocks_buffer_; // holds blocks of current transaction; never popped to prevent unnecessary allocations, instead its current virtual size is kept in uploaded_blocks_buffer_index_
        int uploaded_blocks_buffer_index_;

		std::set<int> blocks_loaded_in_session_;
        
        StoredOctreeBlock* PushBlock();
        void PopAllBlocks();
        
        void InitialFillCache();
        
        DISALLOW_COPY_AND_ASSIGN(GPUOctreeCacheManager);
    };
    
}

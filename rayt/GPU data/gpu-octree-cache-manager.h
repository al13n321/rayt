#pragma once

#include <set>
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
        
		// fills cache with nodes closest to the root
        void InitialFillCache();
        
        // session - a set of transactions; typically session is a frame
        void StartRequestSession();
        
        // transaction - a set of requests; the processing of the requests is guaranteed to be finished when transaction ends (but may be finished earlier); typically transaction is an iteration of feedback-driven loading
        void StartRequestTransaction();
        
		void MarkParentAsUsed(int block_index_in_cache); // needed for duplicate nodes
		void MarkBlockAsUsed(int block_index_in_cache);
        void RequestBlock(int block_index);

		// if true, all the blocks in cache were loaded or marked as used during current transaction; this usually means that it is useless to request anything else during this transaction
		bool TransactionFilledCache() const;

		// if true, all the blocks in cache were loaded or marked as used during current session; this usually means that cache size is insufficient to render the frame in one transaction; the frame may still be rendered in several transaction, but with current implementation the feedback-driven loading may end up in an endless loop of loading and unloading the same blocks
		bool SessionFilledCache() const;
        
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
		std::vector<int> transaction_blocks_;

		bool session_in_progress_;
		bool transaction_in_progress_;

		boost::shared_ptr<GPUOctreeCache::LRUMarker> session_lru_marker_;
		boost::shared_ptr<GPUOctreeCache::LRUMarker> transaction_lru_marker_;
        
		// storage for blocks while they are uploaded asynchronously to GPU
		// returned pointer valid until next PushBlock or PopAllBlocks;
		// can call context_->WaitForAll() and pop all blocks
        StoredOctreeBlock* PushBlock();

        void PopAllBlocks();
        
        DISALLOW_COPY_AND_ASSIGN(GPUOctreeCacheManager);
    };
    
}

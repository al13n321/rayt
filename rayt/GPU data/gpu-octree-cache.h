#pragma once

#include <list>
#include "gpu-octree-data.h"
#include "stored-octree-block.h"

namespace rayt {

    // manages LRU cache, decides which block to unload, but doesn't decide, which to load; manages links consistency: converts node links channel from storage format to GPU format (see docs)
    class GPUOctreeCache {
	private:
		struct LRUQueueEntry;
    public:
		// marks some position in LRU queue
		// becomes invalid after deallocation of GPUOctreeCache (obviously)
		class LRUMarker {
		public:
			~LRUMarker();
			
			// true if all blocks in the cache were uploaded or marked as used after the marker was inserted
			bool is_hit() const;
		private:
			bool is_hit_; // if true, lru_queue_iterator_ is invalid
			std::list<LRUQueueEntry> *lru_queue_; // points to GPUOctreeCache::lru_queue_
			std::list<LRUQueueEntry>::iterator lru_queue_iterator_;

			LRUMarker();

			friend class GPUOctreeCache;

			DISALLOW_COPY_AND_ASSIGN(LRUMarker);
		};

        GPUOctreeCache(int nodes_in_block, int max_blocks_count, const StoredOctreeChannelSet &channels, boost::shared_ptr<CLContext> context);
        ~GPUOctreeCache();
        
        const GPUOctreeData* data() const;
        const bool full() const;
        const int loaded_blocks_count() const;
        const int max_blocks_count() const;
        
        bool IsBlockInCache(int block_index) const;
		bool IsParentInCache(int block_index) const;
        
        // returns -1 if block is not in cache
        int BlockIndexInCache(int block_index) const;
        
        void MarkBlockAsUsed(int block_index_in_cache);
		void MarkParentAsUsed(int block_index);

		boost::shared_ptr<LRUMarker> InsertLRUMarker();
        
        // modifies block contents (even with blocking)!
        // if blocking is false, block should be valid until next clFinish or similar call;
        // parent block must be in cache
        void UploadBlock(StoredOctreeBlock &block, bool blocking);
    private:
        struct CachedBlockRoot {
            ushort pointer_index_in_parent;
            uchar parent_children_mask;
			uint loaded_pointer_in_parent;   // storage for non-blocking write to CLBuffer
			uint unloaded_pointer_in_parent; // storage for non-blocking write to CLBuffer
			uint far_pointer_value;
        };

        struct CachedBlockInfo {
            int block_index; // index in tree
            int parent_block_index; // index in cache
            int roots_count;
            CachedBlockRoot roots[8];
            std::list<LRUQueueEntry>::iterator lru_queue_iterator;
			std::vector<int> child_blocks;

			CLEventList far_pointer_write_events;
			std::vector<CLEventList> block_write_events; // one list for each channel
        };

		struct LRUQueueEntry {
			enum {
				kLRUQueueBlock,
				kLRUQueueMarker,
			} type;
			union {
				int block_index_in_cache; // for kLRUQueueBlock
				LRUMarker *marker;        // for kLRUQueueMarker
			} data;

			LRUQueueEntry(int block_index) {
				type = kLRUQueueBlock;
				data.block_index_in_cache = block_index;
			}

			LRUQueueEntry(LRUMarker *marker) {
				type = kLRUQueueMarker;
				data.marker = marker;
			}
		};
        
        int nodes_in_block_;
        int max_blocks_count_;
        boost::shared_ptr<CLContext> context_;
        boost::scoped_ptr<GPUOctreeData> data_;
        std::map<int, int> block_to_cache_index_;
		std::map<int, int> block_to_parent_cache_index_;
        std::vector<CachedBlockInfo> cache_contents_;
        std::list<LRUQueueEntry> lru_queue_; // least recently used is first, most recently used is last; any node is always closer to beginning than its parent
        int first_free_index_;
        
        int UnloadLRUBlock(bool blocking); // returns freed index in cache
		void DequeueMarkers();
        
        DISALLOW_COPY_AND_ASSIGN(GPUOctreeCache);
    };
    
}

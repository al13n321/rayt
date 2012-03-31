#include "gpu-octree-cache.h"
#include "octree-constants.h"
#include "binary-util.h"
#include "profiler.h"
#include <list>
using namespace std;
using namespace boost;

namespace rayt {

    GPUOctreeCache::GPUOctreeCache(int nodes_in_block, int max_blocks_count, const StoredOctreeChannelSet &channels, boost::shared_ptr<CLContext> context) {
        assert(context);
        assert(nodes_in_block > 0);
        assert(max_blocks_count > 0);
        assert(channels[0].name == kNodeLinkChannelName);
        
        context_ = context;
        max_blocks_count_ = max_blocks_count;
        nodes_in_block_ = nodes_in_block;
		data_.reset(new GPUOctreeData(nodes_in_block, max_blocks_count, max_blocks_count * 8, channels, context));
        first_free_index_ = 0;
        cache_contents_.resize(max_blocks_count);
    }
    
    GPUOctreeCache::~GPUOctreeCache() {}
    
    const GPUOctreeData* GPUOctreeCache::data() const {
        return data_.get();
    }
    
    const bool GPUOctreeCache::full() const {
        return first_free_index_ == max_blocks_count_;
    }
    
    const int GPUOctreeCache::loaded_blocks_count() const {
        return first_free_index_;
    }
    
    const int GPUOctreeCache::max_blocks_count() const {
        return max_blocks_count_;
    }
    
    bool GPUOctreeCache::IsBlockInCache(int block_index) const {
        return block_to_cache_index_.count(block_index) > 0;
    }
    
    bool GPUOctreeCache::IsParentInCache(int block_index) const {
        return block_to_parent_cache_index_.count(block_index) > 0;
    }
    
    int GPUOctreeCache::BlockIndexInCache(int block_index) const {
        if (block_to_cache_index_.count(block_index))
            return ((map<int, int>&)block_to_cache_index_)[block_index]; // a workaround for missing const operator[] in some map implementations
        else
            return -1;
    }
    
    void GPUOctreeCache::MarkBlockAsUsed(int index) {
        // touch me and all my ancestors; it's important to do it from bottom to top to avoid unloading non-leaves
        for (int idx = index; idx != kRootBlockParent; idx = cache_contents_[idx].parent_block_index) {
            lru_queue_.erase(cache_contents_[idx].lru_queue_iterator);
            cache_contents_[idx].lru_queue_iterator = lru_queue_.insert(lru_queue_.end(), LRUQueueEntry(idx));
        }

		DequeueMarkers();
    }

	void GPUOctreeCache::MarkParentAsUsed(int index) {
		assert(block_to_parent_cache_index_.count(index));
		MarkBlockAsUsed(block_to_parent_cache_index_[index]);
	}

	/*

	Some documentation on updating parent pointers when loading and unloading a block.

	When loading/unloading a block, we need to update up to 8 pointers in parent block.
	These updates are better be non-blocking. So, we need to make sure these updates are not reordered incorrectly.
	Also, we need 4 bytes of storage for each updated pointer, and this storage must be valid until the write is complete (asynchronously).

	Here is what happens when we load and unload a block, and then load and unload another block in the same cache slot.


	action    parent1.block_write_events[0]    parent2.block_write_events[0]    child.roots[i].loaded_pointer_in_parent    child.roots[i].unloaded_pointer_in_parent

	load 1     in wait list, out event                   ---                           stores pointer being written                            ---
	unload 1   blocking wait, out event                  ---                                      ---                                 stores pointer being written
	load 2               ---                      in wait list, out event              stores pointer being written                            ---
	unload 2             ---                      blocking wait, out event                        ---                                 stores pointer being written


	Blocking waits are ok because between loading and unloading there is typically a clFinish, so there's actually no delay.
	Blocking wait between unload and load is avoided.

	*/
    
	void GPUOctreeCache::UploadBlock(StoredOctreeBlock &block, bool blocking) {
        assert(!block_to_cache_index_.count(block.header.block_index));
        
        int index;
		if (first_free_index_ == max_blocks_count_) {
			if (block.header.parent_block_index != kRootBlockParent) {
				assert(block_to_parent_cache_index_.count(block.header.block_index));
				MarkParentAsUsed(block.header.block_index); // prevent unloading parent
			}
            index = UnloadLRUBlock(blocking);
		} else {
            index = first_free_index_++;
		}

        CachedBlockInfo &info = cache_contents_[index];

		PROFILE_TIMER_START("CPU process links");

		// make pointers absolute and enumerate child blocks
        char *data = reinterpret_cast<char*>(block.data.data());
        for (int n = 0; n < nodes_in_block_; ++n) {
            uint link = BinaryUtil::ReadUint(data);
            uchar children_mask = link & 255;
            if (children_mask) {
                link >>= 8;
                bool is_fault = !!(link & 1);
                link >>= 1;
                if (!is_fault) {
                    link = (link + (1 << 21) - n) << 1; // convert index from block-relative to this-node-relative and add far pointer bit
                    link = (link << 9) + (0 << 8) + children_mask;
                    BinaryUtil::WriteUint(link, data);
				} else {
					if (!block_to_parent_cache_index_.count(link)) {
						block_to_parent_cache_index_[link] = index;
						info.child_blocks.push_back(link);
					}
				}
            }
            data += kNodeLinkSize;
        }

		PROFILE_TIMER_STOP();
        
        // upload block
		if (info.block_write_events.size() != data_->channels_count())
			info.block_write_events.resize(data_->channels_count());
		vector<CLEvent> evs;
		data_->UploadBlock(index, block.data.data(), blocking, &info.block_write_events, &evs);

		PROFILE_VALUE_ADD("uploaded blocks", 1);

		for (int i = 0; i < (int)evs.size(); ++i) {
			info.block_write_events[i].Clear();
			info.block_write_events[i].Add(evs[i]);

			PROFILE_CL_EVENT(evs[i], "BUS");
			PROFILE_CL_EVENT(evs[i], "BUS upload block");
		}
        
        // if I'm not root
        if (block.header.parent_block_index != kRootBlockParent) {
            assert(block_to_parent_cache_index_.count(block.header.block_index));
            int parent_index = block_to_parent_cache_index_[block.header.block_index];
			CachedBlockInfo &parent_info = cache_contents_[parent_index];
			CLEventList new_far_ptr_events;
			CLEventList new_block_events;
			CLEvent temp_event;
            
            info.parent_block_index = parent_index;
            info.roots_count = block.header.roots_count;
            
            // update pointers in parent block and fill parents in cache structure
            // TODO: upload consecutive updated pointers with one call
            for (uint i = 0; i < block.header.roots_count; ++i) {
                StoredOctreeBlockRoot r = block.header.roots[i];
				CachedBlockRoot &ir = info.roots[i];
                
                ir.pointer_index_in_parent = r.parent_pointer_index;
                ir.parent_children_mask = r.parent_pointer_children_mask;

				uint absolute_ptr = index * nodes_in_block_ + r.pointed_child_index;
				int diff = absolute_ptr - (parent_index * nodes_in_block_ + r.parent_pointer_index);
				uint ptr;
				if (abs(diff) >= (1 << 21)) { // need far pointer
					uint far_ptr_index = index * 8 + i;

					ptr = (far_ptr_index << 1) + 1;

					// upload far pointer
					void *far_ptr_data = &ir.far_pointer_value;
					BinaryUtil::WriteUint(absolute_ptr, far_ptr_data);
					data_->UploadFarPointer(far_ptr_index, far_ptr_data, false, &parent_info.far_pointer_write_events, &temp_event);
					new_far_ptr_events.Add(temp_event);

					PROFILE_CL_EVENT(temp_event, "BUS");
					PROFILE_CL_EVENT(temp_event, "BUS upload far ptr");
					PROFILE_VALUE_ADD("updated far pointers", 1);
				} else {
					ptr = ((1 << 21) + diff) << 1;
				}
				uint new_value = (ptr << 9) | r.parent_pointer_children_mask;
				int offset = r.parent_pointer_index * kNodeLinkSize;
				
				void *new_value_data = &ir.loaded_pointer_in_parent; // this pointer should be valid until next clFinish if blocking is false
				BinaryUtil::WriteUint(new_value, new_value_data);
                
				data_->UploadBlockPart(0, parent_index, offset, kNodeLinkSize, new_value_data, false, &parent_info.block_write_events[0], &temp_event);
				new_block_events.Add(temp_event);
				
				PROFILE_CL_EVENT(temp_event, "BUS");
				PROFILE_CL_EVENT(temp_event, "BUS upload ptr");
				PROFILE_VALUE_ADD("updated pointers", 1);
            }

			if (new_far_ptr_events.size())
				parent_info.far_pointer_write_events = new_far_ptr_events;
			parent_info.block_write_events[0] = new_block_events; // only node links channel is involved
        } else {
            info.parent_block_index = kRootBlockParent;
        }
        
        // add me to structures
        block_to_cache_index_[block.header.block_index] = index;
        info.block_index = block.header.block_index;
        info.lru_queue_iterator = lru_queue_.insert(lru_queue_.begin(), LRUQueueEntry(index));
        MarkBlockAsUsed(index);
    }

	void GPUOctreeCache::DequeueMarkers() {
		while (!lru_queue_.empty() && lru_queue_.begin()->type == LRUQueueEntry::kLRUQueueMarker) {
			lru_queue_.begin()->data.marker->is_hit_ = true;
			lru_queue_.erase(lru_queue_.begin());
		}
	}
    
    int GPUOctreeCache::UnloadLRUBlock(bool blocking) {
		assert(!lru_queue_.empty() && lru_queue_.front().type == LRUQueueEntry::kLRUQueueBlock);
		int index = lru_queue_.begin()->data.block_index_in_cache;
        
        CachedBlockInfo &info = cache_contents_[index];
		int parent_index = info.parent_block_index;
        assert(parent_index != kRootBlockParent); // shouldn't remove root from cache
		CachedBlockInfo &parent_info = cache_contents_[parent_index];
        
		CLEventList new_block_events;
		CLEvent temp_event;

		parent_info.block_write_events[0].WaitFor();

        // update pointers in parent
        // TODO: upload consecutive updated pointers with one call
        for (int i = 0; i < info.roots_count; ++i) {
            uint new_value = (info.block_index << 9) + (1 << 8) + info.roots[i].parent_children_mask;
			void *new_value_data = &info.roots[i].unloaded_pointer_in_parent; // this pointer should be valid until next clFinish if blocking is false
			BinaryUtil::WriteUint(new_value, new_value_data);
            data_->UploadBlockPart(0, parent_index, info.roots[i].pointer_index_in_parent * kNodeLinkSize, kNodeLinkSize, new_value_data, blocking, &parent_info.block_write_events[0], &temp_event);
			new_block_events.Add(temp_event);
			PROFILE_CL_EVENT(temp_event, "BUS");
			PROFILE_CL_EVENT(temp_event, "BUS unload pointer");
			PROFILE_VALUE_ADD("updated pointers", 1);
        }

		parent_info.block_write_events[0] = new_block_events;

		for (int i = 0; i < (int)info.child_blocks.size(); ++i) {
			block_to_parent_cache_index_.erase(info.child_blocks[i]);
		}

        info.child_blocks.clear();

		// remove me from structures
        lru_queue_.erase(lru_queue_.begin());
        block_to_cache_index_.erase(info.block_index);
        
		DequeueMarkers();

        return index;
    }

	shared_ptr<GPUOctreeCache::LRUMarker> GPUOctreeCache::InsertLRUMarker() {
		shared_ptr<LRUMarker> m(new LRUMarker());
		m->lru_queue_ = &lru_queue_;
		m->lru_queue_iterator_ = lru_queue_.insert(lru_queue_.end(), LRUQueueEntry(m.get()));
		return m;
	}

	GPUOctreeCache::LRUMarker::~LRUMarker() {
		if (!is_hit_)
			lru_queue_->erase(lru_queue_iterator_);
	}
			
	bool GPUOctreeCache::LRUMarker::is_hit() const {
		return is_hit_;
	}

	GPUOctreeCache::LRUMarker::LRUMarker() {
		is_hit_ = false;
	}

}

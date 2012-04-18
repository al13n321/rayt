#include "gpu-octree-cache-manager.h"
#include <queue>
#include "octree-constants.h"
#include "binary-util.h"
#include "profiler.h"
using namespace std;
using namespace boost;

namespace rayt {
    
	const int kMinUploadedBufferBytes = 10 * 1000 * 1000;

	GPUOctreeCacheManager::GPUOctreeCacheManager(int max_blocks_count, boost::shared_ptr<StoredOctreeLoader> loader, boost::shared_ptr<CLContext> context) {
        assert(max_blocks_count > 0);
        assert(loader);
        assert(context);
        
        context_ = context;
        max_blocks_count_ = max_blocks_count;
        loader_ = loader;
        cache_.reset(new GPUOctreeCache(loader->header().nodes_in_block, max_blocks_count, loader->header().channels, context));
        
        uploaded_blocks_buffer_index_ = 0;

		int bytes_in_block = loader->BytesInBlockContent();
		int uploaded_buffer_size = kMinUploadedBufferBytes / bytes_in_block + 1;

		uploaded_blocks_buffer_.resize(uploaded_buffer_size);
		for (int i = 0; i < uploaded_buffer_size; ++i)
			uploaded_blocks_buffer_[i].reset(new StoredOctreeBlock());

		session_in_progress_ = transaction_in_progress_ = false;
    }
    
    GPUOctreeCacheManager::~GPUOctreeCacheManager() {}
    
    const GPUOctreeData* GPUOctreeCacheManager::data() const {
        return cache_->data();
    }
    
    int GPUOctreeCacheManager::root_node_index() const {
        return root_node_index_;
    }
    
    std::vector<boost::shared_ptr<StoredOctreeBlock> > uploaded_blocks_buffer_; // holds blocks of current transaction; never popped to prevent unnecessary allocations, instead its current virtual size is kept in uploaded_blocks_buffer_index_
    int uploaded_blocks_buffer_index_;
    
    StoredOctreeBlock* GPUOctreeCacheManager::PushBlock() {
		if (uploaded_blocks_buffer_index_ == uploaded_blocks_buffer_.size()) {
			context_->WaitForAll();
			uploaded_blocks_buffer_index_ = 0;
		}
        return uploaded_blocks_buffer_[uploaded_blocks_buffer_index_++].get();
    }
    
    void GPUOctreeCacheManager::PopAllBlocks() {
        uploaded_blocks_buffer_index_ = 0;
    }
    
    void GPUOctreeCacheManager::InitialFillCache() {
        // use BFS to load highest nodes until cache is full
        int nodes_in_block = loader_->header().nodes_in_block;
        queue<int> load_queue;
        load_queue.push(loader_->header().root_block_index);
        while (!cache_->full() && !load_queue.empty()) {
            int index = load_queue.front();
            load_queue.pop();
            if (cache_->IsBlockInCache(index))
                continue;
            
            StoredOctreeBlock *block = PushBlock();
            
			if (cache_->loaded_blocks_count() % 100 == 0)
                cout << cache_->loaded_blocks_count() << " / " << cache_->max_blocks_count() << " blocks loaded" << endl;
            
			if (!loader_->LoadBlock(index, block))
				crash("failed to load block");
			
			bool its_root = false;
			int root_index_in_block;
			if (block->header.parent_block_index == kRootBlockParent) {
				its_root = true;
				root_index_in_block = block->header.roots[0].pointed_child_index;
			}
            
			const char *data = reinterpret_cast<const char*>(block->data.data());
            for (int i = 0; i < nodes_in_block; ++i) {
                // non-existing nodes are handled correctly because they are filled with zeros (and have zero children mask)
                uint link = BinaryUtil::ReadUint(data);
                uchar children_mask;
				bool fault;
				bool duplicate;
				uint ptr;
				UnpackStoredNodeLink(link, children_mask, fault, duplicate, ptr);
                if (children_mask && fault) {
                    load_queue.push(ptr);
                }
                data += kNodeLinkSize;
            }

            cache_->UploadBlock(*block, false);

			if (its_root)
				root_node_index_ = cache_->BlockIndexInCache(index) * nodes_in_block + root_index_in_block; // we can't take this data from block header because UploadBlock might have spoiled it
        }
		context_->WaitForAll();
		PopAllBlocks();
    }

	void GPUOctreeCacheManager::StartRequestSession() {
		assert(!session_in_progress_);

		session_in_progress_ = true;

		session_lru_marker_ = cache_->InsertLRUMarker();
	}
        
	void GPUOctreeCacheManager::StartRequestTransaction() {
		assert(!transaction_in_progress_ && session_in_progress_);

		transaction_in_progress_ = true;

		transaction_lru_marker_ = cache_->InsertLRUMarker();
	}
    
	void GPUOctreeCacheManager::MarkBlockAsUsed(int block_index_in_cache) {
		assert(transaction_in_progress_);

		cache_->MarkBlockAsUsed(block_index_in_cache);
	}
    
	void GPUOctreeCacheManager::MarkParentAsUsed(int block_index_in_cache) {
		assert(transaction_in_progress_);

		cache_->MarkParentAsUsed(block_index_in_cache, false);
	}

	void GPUOctreeCacheManager::RequestBlock(int index) {
		assert(transaction_in_progress_);

		if (cache_->IsBlockInCache(index))
            return;

		// defer actual loading until transaction end to mark all parents as used before uploading anything
		transaction_blocks_.push_back(index);

		cache_->MarkParentAsUsed(index, true);
	}
    
	void GPUOctreeCacheManager::EndRequestTransaction() {
		assert(transaction_in_progress_);

		for (int i = 0; i < (int)transaction_blocks_.size(); ++i) {
			if (TransactionFilledCache())
				break;

			int index = transaction_blocks_[i];

			StoredOctreeBlock *block = PushBlock();

			PROFILE_TIMER_START("HDD");
			PROFILE_TIMER_START("HDD? load block");

			if (!loader_->LoadBlock(index, block))
				crash("failed to load block");

			PROFILE_TIMER_STOP();
			PROFILE_TIMER_STOP();

			bool its_root = false;
			int root_index_in_block;
			if (block->header.parent_block_index == kRootBlockParent) {
				its_root = true;
				root_index_in_block = block->header.roots[0].pointed_child_index;
			}
	        
			cache_->UploadBlock(*block, false);

			if (its_root)
				root_node_index_ = cache_->BlockIndexInCache(index) * loader_->header().nodes_in_block + root_index_in_block; // we can't take this data from block header because UploadBlock might have spoiled it
		}

		transaction_blocks_.clear();

		context_->WaitForAll();

		PROFILE_CL_EVENTS_FINISH();

		transaction_in_progress_ = false;

		transaction_lru_marker_.reset();

		PopAllBlocks();
	}
    
	void GPUOctreeCacheManager::EndRequestSession() {
		assert(session_in_progress_ && !transaction_in_progress_);

		session_in_progress_ = false;

		session_lru_marker_.reset();
	}

	bool GPUOctreeCacheManager::TransactionFilledCache() const {
		assert(transaction_in_progress_);
		return transaction_lru_marker_->is_hit();
	}

	bool GPUOctreeCacheManager::SessionFilledCache() const {
		assert(session_in_progress_);
		return session_lru_marker_->is_hit();
	}
    
}

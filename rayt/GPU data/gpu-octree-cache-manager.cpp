#include "gpu-octree-cache-manager.h"
#include <queue>
#include "octree-constants.h"
#include "binary-util.h"
using namespace std;
using namespace boost;

namespace rayt {
    
    GPUOctreeCacheManager::GPUOctreeCacheManager(int max_blocks_count, boost::shared_ptr<StoredOctreeLoader> loader, boost::shared_ptr<CLContext> context) {
        assert(max_blocks_count > 0);
        assert(loader);
        assert(context);
        
        context_ = context;
        max_blocks_count_ = max_blocks_count;
        loader_ = loader;
        cache_.reset(new GPUOctreeCache(loader->header().nodes_in_block, max_blocks_count, loader->header().channels, context));
        
        uploaded_blocks_buffer_index_ = 0;
        
        InitialFillCache();
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
        if (uploaded_blocks_buffer_index_ == uploaded_blocks_buffer_.size())
            uploaded_blocks_buffer_.push_back(shared_ptr<StoredOctreeBlock>(new StoredOctreeBlock()));
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
            StoredOctreeBlock *block = PushBlock();
            int index = load_queue.front();
            load_queue.pop();
            if (cache_->IsBlockInCache(index))
                continue;
            
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
                int children_mask = link & 255;
                link >>= 8;
                if (children_mask && (link & 1)) { // far pointer
                    link >>= 1;
                    load_queue.push(link);
                }
                data += kNodeLinkSize;
            }

            cache_->UploadBlock(*block, false); // TODO: fix GPUOctreeCache and make it non-blocking

			if (its_root)
				root_node_index_ = cache_->BlockIndexInCache(index) * nodes_in_block + root_index_in_block; // we can't take this data from block header because UploadBlock might have spoiled it
        }
        context_->WaitForAll();
        PopAllBlocks();
    }

	void StartRequestSession();
        
        // transaction - a set of requests; the processing of the requests is guaranteed to be finished when transaction ends (but may be finished earlier); typically transaction is an iteration of feedback-driven loading
        void StartRequestTransaction();
        
		void MarkBlockAsUsed(int block_index_in_cache);
        void RequestBlock(int block_index);
        
        void EndRequestTransaction();
        
        void EndRequestSession();
    
}

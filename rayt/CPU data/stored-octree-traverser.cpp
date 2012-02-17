#include "stored-octree-traverser.h"
#include "binary-util.h"
#include "octree-constants.h"
using namespace std;
using namespace boost;

namespace rayt {

	StoredOctreeTraverser::Node::~Node() {}
	
	StoredOctreeTraverser::Node::Node(StoredOctreeTraverser *traverser, boost::shared_ptr<StoredOctreeBlock> block, int index_in_block) {
		traverser_ = traverser;
		block_ = block;
		index_in_block_ = index_in_block;
	}

	StoredOctreeTraverser* StoredOctreeTraverser::Node::traverser() {
		return traverser_;
	}

	const void* StoredOctreeTraverser::Node::DataForChannel(const string &name) {
		const StoredOctreeChannelSet &chset = traverser_->loader()->header().channels;
		int off = chset.OffsetToChannel(name);
		if (off == -1)
			return NULL;
		int nodes_in_block = traverser_->loader()->header().nodes_in_block;
		const char *p = reinterpret_cast<const char*>(block_->data.data());
		return p + off * nodes_in_block + chset[name].bytes_in_node * index_in_block_;
	}

	vector<shared_ptr<StoredOctreeTraverser::Node> > StoredOctreeTraverser::Node::Children() {
		const void *link_data = DataForChannel(kNodeLinkChannelName);
		uint link = BinaryUtil::ReadUint(link_data);
		uint children_mask = link & ((1 << 8) - 1);
		bool isfar = !!(link & (1 << 8));
		link >>= 9;

		boost::shared_ptr<StoredOctreeBlock> nblock;
		int nindex;
		
		if (isfar) {
			nblock.reset(new StoredOctreeBlock());
			nindex = -1;
		
			if (!traverser_->loader()->LoadBlock(link, nblock.get()))
				crash("failed to load block");

			for (uint r = 0; r < nblock->header.roots_count; ++r) {
				StoredOctreeBlockRoot &root = nblock->header.roots[r];
				if (root.parent_pointer_index == index_in_block_) {
					assert(nindex == -1);
					nindex = root.pointed_child_index;
				}
			}
			assert(nindex != -1);
		} else {
			nblock = block_;
			nindex = link;
		}

		vector<shared_ptr<Node> > res(8);

		for (int i = 0; i < 8; ++i) {
			if (children_mask & (1 << i)) {
				res[i].reset(new Node(traverser_, nblock, nindex));
				++nindex;
			}
		}

		return res;
	}

	StoredOctreeTraverser::StoredOctreeTraverser(shared_ptr<StoredOctreeLoader> loader) {
		loader_ = loader;
	}

	StoredOctreeTraverser::~StoredOctreeTraverser() {}
        
	StoredOctreeLoader* StoredOctreeTraverser::loader() {
		return loader_.get();
	}

	shared_ptr<StoredOctreeTraverser::Node> StoredOctreeTraverser::Root() {
		shared_ptr<StoredOctreeBlock> block(new StoredOctreeBlock());
		if(!loader_->LoadBlock(loader_->header().root_block_index, block.get()))
			crash("failed to load block");
		return shared_ptr<Node>(new Node(this, block, block->header.roots[0].pointed_child_index));
	}

}

#include "stored-octree-traverser.h"
#include "binary-util.h"
#include "octree-constants.h"
using namespace std;
using namespace boost;

namespace rayt {

	StoredOctreeTraverser::Node::~Node() {}

	const void* StoredOctreeTraverser::Node::DataForChannel(const std::string &name) {
		char *d = reinterpret_cast<char*>(data_.data());
		return d + channels_->OffsetToChannel(name) - kNodeLinkSize;
	}

	vector<StoredOctreeTraverser::Node*> StoredOctreeTraverser::Node::Children() {
		return children_;
	}
		
	StoredOctreeTraverser::Node::Node(StoredOctreeBlock &block, const StoredOctreeChannelSet *channels, int nodes_in_block, int node_index) {
		channels_ = channels;
		data_.Resize(channels->SumBytesInNode() - kNodeLinkSize);
		char *in_data = reinterpret_cast<char*>(block.data.data());
		char *out_data = reinterpret_cast<char*>(data_.data());
		int offset = 0;
		for (int c = 1; c < channels->size(); ++c) {
			int s = (*channels)[c].bytes_in_node;
			memcpy(out_data + offset, in_data + (offset + kNodeLinkSize) * nodes_in_block + s * node_index, s);
			offset += s;
		}
	}

	StoredOctreeTraverser::StoredOctreeTraverser(boost::shared_ptr<StoredOctreeLoader> loader) {
		loader_ = loader;
		root_ = NULL;

		for (int i = 0; i < static_cast<int>(loader->header().blocks_count); ++i) {
			LoadBlockIfNotLoaded(i);
		}

		assert(root_);
		CheckTree(root_);
	}
    
	StoredOctreeTraverser::~StoredOctreeTraverser() {}
        
	StoredOctreeTraverser::Node* StoredOctreeTraverser::Root() {
		return root_;
	}

	void StoredOctreeTraverser::LoadBlockIfNotLoaded(int block_index) {
		if (loaded_blocks_.count(block_index))
			return;

		StoredOctreeBlock block;
		if (!loader_->LoadBlock(block_index, &block))
			crash("failed to load block");

		int parent_block = block.header.parent_block_index;

		if (parent_block == kRootBlockParent) {
			root_ = TraverseBlock(block, block.header.roots[0].pointed_child_index);
		} else {
			LoadBlockIfNotLoaded(parent_block);

			for (int ri = 0; ri < static_cast<int>(block.header.roots_count); ++ri) {
				StoredOctreeBlockRoot &r = block.header.roots[ri];
				int parent_node = r.parent_pointer_index;
				Node *n = nodes_[make_pair(parent_block, parent_node)].get();
				assert(n);
				assert(n->initial_node_link_ == r.parent_pointer_value);
				n->children_ = vector<Node*>(8, NULL);
				int children_mask = UnpackStoredChildrenMask(r.parent_pointer_value);
				int p = 0;
				for (int i = 0; i < 8; ++i) {
					if (children_mask & (1 << i)) {
						n->children_[i] = TraverseBlock(block, r.pointed_child_index + p);
						++p;
					}
				}
			}
		}

		loaded_blocks_.insert(block_index);
	}

	StoredOctreeTraverser::Node* StoredOctreeTraverser::TraverseBlock(StoredOctreeBlock &block, int node_index) {
		assert(!nodes_.count(make_pair(block.header.block_index, node_index)));
		Node *n = new Node(block, &loader_->header().channels, loader_->header().nodes_in_block, node_index);
		nodes_[make_pair(block.header.block_index, node_index)] = shared_ptr<Node>(n);
		char *data = reinterpret_cast<char*>(block.data.data());
		uint node_link = BinaryUtil::ReadUint(data + node_index * kNodeLinkSize);
		n->initial_node_link_ = node_link;
		uchar children_mask;
		bool fault;
		bool duplicate;
		uint ptr;
		UnpackStoredNodeLink(node_link, children_mask, fault, duplicate, ptr);
		if (!children_mask) {
			n->children_.resize(8, NULL);
		} else if (!fault) {
			n->children_.resize(8, NULL);
			int p = 0;
			for (int i = 0; i < 8; ++i) {
				if (children_mask & (1 << i)) {
					n->children_[i] = TraverseBlock(block, ptr + p);
					++p;
				}
			}
		}
		return n;
	}

	void StoredOctreeTraverser::CheckTree(Node *n) {
		if (n->children_.empty())
			crash("unexpected fault after loading all blocks");
		for (int i = 0; i < 8; ++i)
			if (n->children_[i])
				CheckTree(n->children_[i]);
	}

}

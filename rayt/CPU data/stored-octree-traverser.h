#pragma once

#include <set>
#include "stored-octree-loader.h"

namespace rayt {

	class StoredOctreeTraverser {
	public:
		class Node {
		public:
			~Node();

			// returns NULL if there's no such channel
			const void* DataForChannel(const std::string &name);

			// size 8, can contain NULLs
			std::vector<Node*> Children();
		private:
			friend class StoredOctreeTraverser;

			std::vector<Node*> children_;
			Buffer data_;
			const StoredOctreeChannelSet *channels_;
			uint initial_node_link_; // only for check

			// doesn't initialize children
			Node(StoredOctreeBlock &block, const StoredOctreeChannelSet *channels, int nodes_in_block, int node_index);

			DISALLOW_COPY_AND_ASSIGN(Node);
		};

		StoredOctreeTraverser(boost::shared_ptr<StoredOctreeLoader> loader);
        ~StoredOctreeTraverser();
        
		Node* Root();
    private:
		boost::shared_ptr<StoredOctreeLoader> loader_;
		std::map<std::pair<int, int>, boost::shared_ptr<Node> > nodes_;
		std::set<int> loaded_blocks_;
		Node *root_;

		void LoadBlockIfNotLoaded(int block_index);
		Node* TraverseBlock(StoredOctreeBlock &block, int node_index);
		void CheckTree(Node *n);

        DISALLOW_COPY_AND_ASSIGN(StoredOctreeTraverser);
    };

}

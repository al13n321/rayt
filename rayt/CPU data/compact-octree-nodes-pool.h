#pragma once

#include "compact-octree-structures.h"
#include "array.h"

namespace rayt {

// just allocates and deallocates nodes, doesn't care for their contents
// always has root node, other nodes are allocated and deallocated 8 at once; so total number of nodes is always 8n+1
class CompactOctreeNodesPool {
public:
	CompactOctreeNodesPool();
	~CompactOctreeNodesPool();

	CompactOctreeNode* root_node();

	CompactOctreeNode* AllocateEightNodes();
	CompactOctreeNode* AllocateEightEmptyNodes();
	CompactOctreeLeaf* AllocateLeaf();

	void FreeEightNodes(CompactOctreeNode *begin);
	void FreeLeaf(CompactOctreeLeaf *leaf);
private:
	CompactOctreeNode root_;

	DISALLOW_COPY_AND_ASSIGN(CompactOctreeNodesPool);
};

}

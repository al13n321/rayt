#include "compact-octree-nodes-pool.h"
#include <memory.h>

namespace rayt {

CompactOctreeNodesPool::CompactOctreeNodesPool() {
	root_.type = kCompactOctreeEmptyLeaf;
}
CompactOctreeNodesPool::~CompactOctreeNodesPool() {}

CompactOctreeNode* CompactOctreeNodesPool::root_node() {
	return &root_;
}

CompactOctreeNode* CompactOctreeNodesPool::AllocateEightNodes() {
	return new CompactOctreeNode[8];
}

CompactOctreeNode* CompactOctreeNodesPool::AllocateEightEmptyNodes() {
	CompactOctreeNode *res = AllocateEightNodes();
	memset(res, 0, sizeof(*res) * 8);
	return res;
}

CompactOctreeLeaf* CompactOctreeNodesPool::AllocateLeaf() {
	return new CompactOctreeLeaf();
}

static void FreeEightNodes(CompactOctreeNode *begin) {
	delete[] begin;
}

static void FreeLeaf(CompactOctreeLeaf *leaf) {
	delete leaf;
}

}

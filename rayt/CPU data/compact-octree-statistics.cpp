#include "compact-octree-statistics.h"
#include <iostream>
using namespace std;

namespace rayt {

static void CalculateRecursive(CompactOctreeNode *node, CompactOctreeStatistics *statistics) {
	switch(node->type) {
		case kCompactOctreeEmptyLeaf:
			++statistics->empty_leaves_count;
			break;
		case kCompactOctreeFilledLeaf:
			++statistics->filled_leaves_count;
			break;
		case kCompactOctreeInternalNode:
			++statistics->internal_nodes_count;
			break;
	}

	if (node->type == kCompactOctreeInternalNode)
		for (int i = 0; i < 8; ++i)
			CalculateRecursive(node->first_child_or_leaf.first_child + i, statistics);
}

CompactOctreeStatistics CalculateCompactOctreeStatistics(CompactOctreeNode *root) {
	CompactOctreeStatistics res;
	res.empty_leaves_count = res.filled_leaves_count = res.internal_nodes_count = 0;
	CalculateRecursive(root, &res);
	return res;
}
    
void WriteCompactOctreeStatistics(CompactOctreeNode *root) {
    CompactOctreeStatistics s = CalculateCompactOctreeStatistics(root);
    cout << s.internal_nodes_count << " internal nodes." << endl;
    cout << s.filled_leaves_count << " filled leaves." << endl;
    cout << s.empty_leaves_count << " empty leaves." << endl;
    cout << endl;
}

}

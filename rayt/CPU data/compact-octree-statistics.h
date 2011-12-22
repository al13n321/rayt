#pragma once

#include "compact-octree-nodes-pool.h"

namespace rayt {

struct CompactOctreeStatistics {
	int internal_nodes_count;
	int filled_leaves_count;
	int empty_leaves_count;
};

CompactOctreeStatistics CalculateCompactOctreeStatistics(CompactOctreeNode *root);

}

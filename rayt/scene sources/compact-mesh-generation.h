#pragma once

#include "compact-octree-nodes-pool.h"

namespace rayt {

void GenerateSphere(CompactOctreeNodesPool *pool, fvec3 center, float radius, Color color, int max_depth);

}

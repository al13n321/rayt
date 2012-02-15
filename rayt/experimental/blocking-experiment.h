#pragma once

#include "common.h"
#include "compact-octree-nodes-pool.h"

namespace rayt {

    void WriteBlockingStatistics(CompactOctreeNode *root, int blocksize, bool multiroot);
    
}

#pragma once

#include "stored-octree-channel-set.h"

namespace rayt {
    
    struct StoredOctreeHeader {
        uint nodes_in_block;
        uint blocks_count;
        uint root_block_index;
        StoredOctreeChannelSet channels;
    };
    
}

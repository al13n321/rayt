#pragma once

#include "buffer.h"

namespace rayt {

    struct StoredOctreeBlockRoot {
        ushort parent_pointer_index; // within parent block
        ushort pointed_child_index; // within this block
    };
    
    struct StoredOctreeBlockHeader {
        uint block_index;
        uint parent_block_index;
        uint roots_count;
        StoredOctreeBlockRoot roots[8];
    };
    
    struct StoredOctreeBlock {
        Buffer data;
        StoredOctreeBlockHeader header;
    };
    
}

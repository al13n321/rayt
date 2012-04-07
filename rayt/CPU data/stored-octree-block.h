#pragma once

#include "buffer.h"

namespace rayt {

    struct StoredOctreeBlockRoot {
        ushort parent_pointer_index; // within parent block
        ushort pointed_child_index; // within this block
        uchar parent_pointer_children_mask; // child mask of node at parent_pointer_index
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

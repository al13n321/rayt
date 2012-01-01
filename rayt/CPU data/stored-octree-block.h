#pragma once

#include "common.h"

namespace rayt {

    struct StoredOctreeBlockRoot {
        ushort parent_pointer_index; // within parent block
        ushort pointed_child_index; // within this block
    };
    
    struct StoredOctreeBlockHeader {
        uint BlockIndex;
        uint ParentBlockIndex;
        uint RootsCount;
        StoredOctreeBlockRoot roots[8];
    };
    
    class StoredOctreeBlock {
    public:
        StoredOctreeBlock();
        ~StoredOctreeBlock();
        
        void resize(int new_bytes_count);
        
        StoredOctreeBlockHeader& header();
        void* data();
        
        const StoredOctreeBlockHeader& header() const;
        const void* data() const;
    private:
        
        DISALLOW_COPY_AND_ASSIGN(StoredOctreeBlock);
    };
    
}

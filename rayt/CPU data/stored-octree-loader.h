#pragma once

#include <string>
#include <vector>
#include "stored-octree-block.h"

namespace rayt {
    
    struct StoredOctreeChannel {
        int bytes_in_node;
        std::string name;
    };

    class StoredOctreeLoader {
    public:
        StoredOctreeLoader(std::string file_name);
        ~StoredOctreeLoader();
        
        int nodes_in_block() const;
        int blocks_count() const;
        int channels_count() const;
        const std::vector<StoredOctreeChannel>& channels() const;
        
        // if out_block has wrong size, it will be resized; typically, you can use the same block for all calls, so that it gets resized only in the first call, avoiding unneeded memory allocations;
        // returns true in case of success
        bool LoadBlock(int index, StoredOctreeBlock &out_block);
    private:
        
        DISALLOW_COPY_AND_ASSIGN(StoredOctreeLoader);
    };
    
}

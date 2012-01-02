#pragma once

#include "common.h"

namespace rayt {
    
    struct StoredOctreeChannel {
        int bytes_in_node;
        std::string name;
        
        StoredOctreeChannel();
        StoredOctreeChannel(int bytes_in_node, std::string name);
        
        bool operator == (const StoredOctreeChannel &c) const;
        bool operator != (const StoredOctreeChannel &c) const;
    };
    
}
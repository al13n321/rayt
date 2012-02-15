#pragma once

#include "cl-buffer.h"
#include "vec.h"

namespace rayt {

    void DebugRayCast(const CLBuffer &node_links_buffer, int root_node, fvec3 origin, fvec3 direction);
    
}

#pragma once

#include "stored-octree-writer.h"
#include "stored-octree-loader.h"
#include "gpu-octree-data.h"
#include "vec.h"

namespace rayt {

    void WriteTestOctreeSphere(fvec3 center, float radius, int level, std::string filename, int nodes_in_block);
    
    // crash if something's wrong
    void CheckTestOctreeSphereWithLoader(fvec3 center, float radius, int level, std::string filename);
    void CheckTestOctreeSphereWithGPUData(fvec3 center, float radius, int level, const GPUOctreeData *data, int root_node_index, bool faultless);
    
}

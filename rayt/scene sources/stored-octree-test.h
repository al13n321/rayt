#pragma once

#include "stored-octree-writer.h"
#include "stored-octree-loader.h"
#include "vec.h"

namespace rayt {

    void WriteTestOctreeSphere(fvec3 center, float radius, int level, std::string filename, int nodes_in_block);
    
    // crashes if something's wrong
    void CheckTestOctreeSphere(fvec3 center, float radius, int level, std::string filename);
    
}

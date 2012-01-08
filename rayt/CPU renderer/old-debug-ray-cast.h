#pragma once

#include "ray-cast.h"

namespace rayt {

void DebugRayFirstIntersection(CompactOctreeNode *root, fvec3 origin, fvec3 direction, OctreeRayIntersection *out);

}

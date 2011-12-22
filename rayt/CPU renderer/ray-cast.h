#pragma once

#include "compact-octree-structures.h"
#include "render-statistics.h"

namespace rayt {

enum OctreeRayIntersectionType {
	kOctreeRayIntersection,
	kOctreeRayNoIntersection,
	kOctreeRayError,
};

struct OctreeRayIntersection {
	fvec3 point;
	fvec3 normal; // must have unit length
	fvec3 color;
	CompactOctreeLeaf *leaf;
	OctreeRayIntersectionType type;
};

void RayFirstIntersection(CompactOctreeNode *root, fvec3 origin, fvec3 direction, OctreeRayIntersection *out, RayStatistics *statistics);

}

#pragma once

#include "vec.h"
#include "color.h"

namespace rayt {

// Structure for fast traversing.

enum CompactOctreeNodeType {
	kCompactOctreeEmptyLeaf = 0, // this one should always be zero for memset to work
	kCompactOctreeFilledLeaf = 1,
	kCompactOctreeInternalNode = 2,
};

struct CompactOctreeLeaf {
	Color color; // with alpha
	fvec3 normal;
};

struct CompactOctreeNode {
	CompactOctreeNodeType type;
	union {
		CompactOctreeNode *first_child; // if type == kCompactOctreeInternalNode; other 7 children go consecutively after the first one
		CompactOctreeLeaf *leaf;        // if type == kCompactOctreeFilledLeaf
	} first_child_or_leaf;
};

}

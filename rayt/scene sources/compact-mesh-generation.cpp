#include "compact-mesh-generation.h"
#include "intersections.h"

namespace rayt {

struct SphereGeneratorState {
	CompactOctreeNodesPool *pool;
	Color color;
	fvec3 center;
	float radius_square;
	int max_depth;
};

static void FillSubtree(SphereGeneratorState *s, CompactOctreeNode *node, fvec3 minp, fvec3 maxp) {
	if (node->type == kCompactOctreeFilledLeaf)
		return;

	if (node->type == kCompactOctreeEmptyLeaf) {
		node->type = kCompactOctreeFilledLeaf;

		CompactOctreeLeaf *leaf = s->pool->AllocateLeaf();
		leaf->color = s->color;
		leaf->normal = ((minp + maxp) - (s->center + s->center)).Normalized();
		node->first_child_or_leaf.leaf = leaf;
	} else {
		fvec3 midp = (minp + maxp) * 0.5;
		CompactOctreeNode *first = node->first_child_or_leaf.first_child;

		FillSubtree(s, first + 0, fvec3(minp.x, minp.y, minp.z), fvec3(midp.x, midp.y, midp.z));
		FillSubtree(s, first + 1, fvec3(midp.x, minp.y, minp.z), fvec3(maxp.x, midp.y, midp.z));
		FillSubtree(s, first + 2, fvec3(minp.x, midp.y, minp.z), fvec3(midp.x, maxp.y, midp.z));
		FillSubtree(s, first + 3, fvec3(midp.x, midp.y, minp.z), fvec3(maxp.x, maxp.y, midp.z));
		FillSubtree(s, first + 4, fvec3(minp.x, minp.y, midp.z), fvec3(midp.x, midp.y, maxp.z));
		FillSubtree(s, first + 5, fvec3(midp.x, minp.y, midp.z), fvec3(maxp.x, midp.y, maxp.z));
		FillSubtree(s, first + 6, fvec3(minp.x, midp.y, midp.z), fvec3(midp.x, maxp.y, maxp.z));
		FillSubtree(s, first + 7, fvec3(midp.x, midp.y, midp.z), fvec3(maxp.x, maxp.y, maxp.z));
	}
}

static void GenerateSphereRecursive(SphereGeneratorState *s, CompactOctreeNode *node, int depth, fvec3 minp, fvec3 maxp) {
	if (node->type == kCompactOctreeFilledLeaf)
		return;

	if (!PointInsideBox(s->center, minp, maxp)) {
		fvec3 verts[8] = {fvec3(minp.x, minp.y, minp.z),
		                  fvec3(maxp.x, minp.y, minp.z),
		                  fvec3(minp.x, maxp.y, minp.z),
		                  fvec3(maxp.x, maxp.y, minp.z),
		                  fvec3(minp.x, minp.y, maxp.z),
		                  fvec3(maxp.x, minp.y, maxp.z),
		                  fvec3(minp.x, maxp.y, maxp.z),
						  fvec3(maxp.x, maxp.y, maxp.z)};

		bool was_inside = false, was_outside = false;

		for (int i = 0; i < 8; ++i) {
			if (s->center.DistanceSquare(verts[i]) < s->radius_square) {
				was_inside = true;
				if (was_outside)
					break;
			} else {
				was_outside = true;
				if (was_inside)
					break;
			}
		}

		if (!was_inside || !was_outside)
			return;
	}

	if (depth >= s->max_depth) {
		if (node->type == kCompactOctreeEmptyLeaf) {
			node->type = kCompactOctreeFilledLeaf;

			CompactOctreeLeaf *leaf = s->pool->AllocateLeaf();
			leaf->color = s->color;
			leaf->normal = ((minp+ maxp) - (s->center + s->center)).Normalized();
			node->first_child_or_leaf.leaf = leaf;
		} else {
			FillSubtree(s, node, minp, maxp);
		}
	} else {
		if (node->type == kCompactOctreeEmptyLeaf) {
			node->type = kCompactOctreeInternalNode;
			node->first_child_or_leaf.first_child = s->pool->AllocateEightEmptyNodes();
		}

		fvec3 midp = (minp + maxp) * 0.5;
		CompactOctreeNode *first = node->first_child_or_leaf.first_child;

		GenerateSphereRecursive(s, first + 0, depth + 1, fvec3(minp.x, minp.y, minp.z), fvec3(midp.x, midp.y, midp.z));
		GenerateSphereRecursive(s, first + 1, depth + 1, fvec3(midp.x, minp.y, minp.z), fvec3(maxp.x, midp.y, midp.z));
		GenerateSphereRecursive(s, first + 2, depth + 1, fvec3(minp.x, midp.y, minp.z), fvec3(midp.x, maxp.y, midp.z));
		GenerateSphereRecursive(s, first + 3, depth + 1, fvec3(midp.x, midp.y, minp.z), fvec3(maxp.x, maxp.y, midp.z));
		GenerateSphereRecursive(s, first + 4, depth + 1, fvec3(minp.x, minp.y, midp.z), fvec3(midp.x, midp.y, maxp.z));
		GenerateSphereRecursive(s, first + 5, depth + 1, fvec3(midp.x, minp.y, midp.z), fvec3(maxp.x, midp.y, maxp.z));
		GenerateSphereRecursive(s, first + 6, depth + 1, fvec3(minp.x, midp.y, midp.z), fvec3(midp.x, maxp.y, maxp.z));
		GenerateSphereRecursive(s, first + 7, depth + 1, fvec3(midp.x, midp.y, midp.z), fvec3(maxp.x, maxp.y, maxp.z));
	}
}

void GenerateSphere(CompactOctreeNodesPool *pool, fvec3 center, float radius, Color color, int max_depth) {
	SphereGeneratorState s;
	s.pool = pool;
	s.color = color;
	s.center = center;
	s.radius_square = radius * radius;
	s.max_depth = max_depth;

	GenerateSphereRecursive(&s, pool->root_node(), 0, fvec3(-0.5f, -0.5f, -0.5f), fvec3(0.5f, 0.5f, 0.5f));
}

}

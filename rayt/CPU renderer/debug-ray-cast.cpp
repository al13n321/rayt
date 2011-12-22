#include "debug-ray-cast.h"
#include "intersections.h"
#include <iostream>
#include <vector>
using namespace std;

namespace rayt {

static const float kStep = 2.5e-7f; // quite close to precision limit; maybe will need to increase it

struct TraverseState {
	CompactOctreeNode *node;
	fvec3 center;
};

static inline float sign(float a) {
	return a < 0 ? -1.0f : 1.0f;
}

// most likely this is not the best raycasting method for CPU, but it's quite close to GPU methods
void DebugRayFirstIntersection(CompactOctreeNode *root, fvec3 origin, fvec3 dir, OctreeRayIntersection *out) {
	// TODO: this array should be thread-local if you want to cast rays from multiple threads
	static TraverseState traverse_stack[100]; // 100 is safe length: no one will ever have an octree of depth 100, I promise

	fvec3 dir_signs(sign(dir.x), sign(dir.y), sign(dir.z));
	//dir *= dir_signs;
	// TODO: case when some dir component is close to zero
	fvec3 dir_inv(1.0f / dir.x, 1.0f / dir.y, 1.0f / dir.z);

	TraverseState *state = traverse_stack;
	state->node = root;
	state->center = fvec3(0.0f, 0.0f, 0.0f);
	float half_size = 0.5f;
	fvec3 point = origin;

	vector<int> st;

	int steps_done = 0;
	do {
		// now point is inside cube state->node
		
		for (int i = 0; i < (int)st.size(); ++i)
			cout << ' ';

		while (state->node->type == kCompactOctreeInternalNode) {
			TraverseState *new_state = state + 1;

			half_size *= 0.5f;

			int index = 0;
			fvec3 signs(-1.0f, -1.0f, -1.0f);
			if (point.x > state->center.x) {
				index |= 1 << 0;
				signs.x = 1.0f;
			}
			if (point.y > state->center.y) {
				index |= 1 << 1;
				signs.y = 1.0f;
			}
			if (point.z > state->center.z) {
				index |= 1 << 2;
				signs.z = 1.0f;
			}

			new_state->node = state->node->first_child_or_leaf.first_child + index;
			new_state->center = state->center + signs * half_size;

			state = new_state;

			st.push_back(index);
			cout<<index;
		}

		cout << endl;

		// now point is still inside state->node, and state->node is a leaf

		if (state->node->type == kCompactOctreeFilledLeaf) {
			cout << "filled leaf discovered!" << endl;
			CompactOctreeLeaf *leaf = state->node->first_child_or_leaf.leaf;
			out->point = point;
			out->normal = leaf->normal;
			out->color = leaf->color.ToVector();
			out->leaf = leaf;
			out->type = kOctreeRayIntersection;
			return;
		}

		fvec3 shifts = (state->center + dir_signs * half_size - point) * dir_inv;
		float sh = shifts.MinComponent();
		if (!(sh == sh)) {
			cout << "NaN!" << endl;
		}
		if (sh < 0) {
			out->type = kOctreeRayError;
			return;
		}
		point += dir * (sh + kStep);

		// TODO: optimize this condition (probably, by conditionally flipping space in the beginning)
		if (PointInsideBox(point, state->center - fvec3(half_size, half_size, half_size), state->center + fvec3(half_size, half_size, half_size))) {
			out->type = kOctreeRayError;
			return;
		}
		do {
			half_size *= 2;
			--state;
			if (state >= traverse_stack)
				st.pop_back();
		} while (state >= traverse_stack && !PointInsideBox(point, state->center - fvec3(half_size, half_size, half_size), state->center + fvec3(half_size, half_size, half_size)));

		++steps_done;
	} while (state >= traverse_stack && steps_done < 100);

	if (steps_done >= 100)
		out->type = kOctreeRayError;
	else
		out->type = kOctreeRayNoIntersection;
}

}

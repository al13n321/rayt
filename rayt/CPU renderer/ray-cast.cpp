#include "ray-cast.h"
#include "intersections.h"
#include <iostream>
using namespace std;

namespace rayt {

// TODO: probably calculate it at runtime as 1 / S, where S is side length of working volume
static const float kStepMultiplier = 1.0f / 1000.0f;

struct TraverseState {
	CompactOctreeNode *node;
	fvec3 center;
};

static inline float sign(float a) {
	return a < 0 ? -1.0f : 1.0f;
}

// most likely this is not the best raycasting method for CPU, but it's quite close to GPU methods
void RayFirstIntersection(CompactOctreeNode *root, fvec3 origin, fvec3 dir, OctreeRayIntersection *out, RayStatistics *statistics) {
	// TODO: this array should be thread-local if you want to cast rays from multiple threads
	static TraverseState traverse_stack[100]; // 100 is safe length: no one will ever have an octree of depth 100, I promise

	fvec3 dir_signs(1.0f, 1.0f, 1.0f);
	int dir_signs_bits = 0;
	if (dir.x < 0) {
		dir_signs.x = -1.0f;
		dir_signs_bits |= 1<<0;
	}
	if (dir.y < 0) {
		dir_signs.y = -1.0f;
		dir_signs_bits |= 1<<1;
	}
	if (dir.z < 0) {
		dir_signs.z = -1.0f;
		dir_signs_bits |= 1<<2;
	}
	dir *= dir_signs;
	// TODO: case when some dir component is close to zero
	fvec3 dir_inv(1.0f / dir.x, 1.0f / dir.y, 1.0f / dir.z);

	TraverseState *state = traverse_stack;
	state->node = root;
	state->center = fvec3(0.0f, 0.0f, 0.0f);
	float half_size = 0.5f;
	fvec3 point = origin * dir_signs;

	fvec3 half_times = dir_inv * half_size;
	fvec3 point_times = dir_inv * point;
	float in_time = (-point_times - half_times).MaxComponent();
	float out_time = (-point_times + half_times).MinComponent();

	if (out_time <= in_time || out_time <= 0) {
		out->type = kOctreeRayNoIntersection;
		return;
	}

	if (in_time > 0)
		point += dir * in_time;

	int steps_done = 0;
	int visited_nodes = 1;
	do {
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

			new_state->node = state->node->first_child_or_leaf.first_child + (index ^ dir_signs_bits);
			new_state->center = state->center + signs * half_size;

			state = new_state;

			++visited_nodes;
		}

		if (state->node->type == kCompactOctreeFilledLeaf) {
			CompactOctreeLeaf *leaf = state->node->first_child_or_leaf.leaf;
			out->point = point * dir_signs;
			out->normal = leaf->normal;
			out->color = leaf->color.ToVector();
			out->leaf = leaf;
			out->type = kOctreeRayIntersection;

			statistics->traverse_steps_count = steps_done;
			statistics->traverse_visited_nodes_count = visited_nodes;

			return;
		}

		fvec3 shifts = (state->center + fvec3(half_size, half_size, half_size) - point) * dir_inv;
		float sh = shifts.MinComponent();
#ifdef _DEBUG
		if (!(sh == sh)) {
			cout << "NaN!" << endl;
		}
		if (sh < 0) {
			// cout << "negative shift!" << endl;
		}
#endif
		float step_len = kStepMultiplier * point.Abs().MaxComponent();
		point += dir * (sh + step_len);

#ifdef _DEBUG
		if (PointInsideBox(point, state->center - fvec3(half_size, half_size, half_size), state->center + fvec3(half_size, half_size, half_size))) {
			cout << "point still in the box after shifting!" << endl;
		}
#endif
		do {
			half_size *= 2;
			--state;
		} while (state >= traverse_stack && !point.AllLessThan(state->center + fvec3(half_size, half_size, half_size)));

		++steps_done;
	} while (state >= traverse_stack && steps_done < 30);

	if (steps_done >= 30)
		out->type = kOctreeRayError;
	else
		out->type = kOctreeRayNoIntersection;

	statistics->traverse_steps_count = steps_done;
	statistics->traverse_visited_nodes_count = visited_nodes;
}

}

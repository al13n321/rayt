#include "common.cl"

#define SHOW_BLOCKS
#define BLOCK_SIZE 4096

// state is in-out
void ProcessHit(float hit_t, int hit_node, TracingState *state, __global char4 *node_normals, __global uchar4 *node_colors) {
	float ambient_light = .1f;
	float4 light_vec = normalize((float4)(1, 2, 3, 0)) * (1.f - ambient_light);
	
	char4 cnormal = node_normals[hit_node];
	float4 normal = ((float4)(cnormal.x, cnormal.y, cnormal.z, 0)) / 127.f;
	
	float light = ambient_light - dot(normal, light_vec);
	
	uchar4 cncol = node_colors[hit_node];
	//float4 color = min((float4)(255, 255, 255, 255), (float4)(cncol.x, cncol.y, cncol.z, 0) * light);
	float4 color = fabs(normal) * 255; // QQQ
	
#ifdef SHOW_BLOCKS
	int id = hit_node / BLOCK_SIZE * 1500;
	state->color = (uchar4)(id % 256, id / 256 % 256, id / 256 / 256, 255);
#else
	state->color = (uchar4)(color.x, color.y, color.z, 255);
#endif
	state->color_multiplier = (uchar4)(0, 0, 0, 0);
}

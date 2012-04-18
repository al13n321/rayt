#include "common.cl"

// state is in-out
void ProcessHit(float hit_t, int hit_node, TracingState *state, __global char4 *node_normals, __global uchar4 *node_colors) {
	// messy temporary lighting

	float4 ambient_light = (float4)(.1f, .1f, .1f, 0);
	float4 light_vec1 = normalize((float4)( 1, 2,  3, 0)) * .8f;
	float4 light_vec2 = normalize((float4)(-1, 2, -3, 0)) * .4f;
	float4 light_col1 = (float4)(1, 1, 1, 0);
	float4 light_col2 = (float4)(99.f/255.f, 1, 145.f/255.f, 0);
	
	char4 cnormal = node_normals[hit_node];
	float4 normal = ((float4)(cnormal.x, cnormal.y, cnormal.z, 0)) / 127.f;
	
	float4 light = ambient_light + max(0.f, dot(-normal, light_vec1)) * light_col1 + max(0.f, dot(-normal, light_vec2)) * light_col2;
	
	uchar4 cncol = node_colors[hit_node];
	float4 color = min((float4)(255, 255, 255, 255), light * 255);
	//float4 color = fabs(normal) * 255; // QQQ
	
#ifdef SHOW_BLOCKS
	int id = hit_node / BLOCK_SIZE * 1499;
	state->color = (uchar4)(id % 256, id / 256 % 256, id / 256 / 256, 255);
#else
	state->color = (uchar4)(color.x, color.y, color.z, 255);
#endif
	state->color_multiplier = (uchar4)(0, 0, 0, 0);
}

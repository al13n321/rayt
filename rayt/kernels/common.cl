#ifndef COMMON_CL_INCLUDED
#define COMMON_CL_INCLUDED

// hacks to work on OpenCL 1.0
typedef float4 float3;
#define make_float3(x, y, z) ((float4)(x, y, z, 0.0f))

// frame dimensions
#if !defined(WIDTH) || !defined(HEIGHT)
#define WIDTH 128
#define HEIGHT 128
#endif

#define NO_HIT (1 << 30) - 1 + (1 << 30);

#define MAX_REFLECTIONS 3

typedef struct TracingState {
	float4 ray_origin; // xyz - ray origin, w - ray size bias
	float4 ray_direction; // xyz - ray direction, w - ray size coefficient
	int fault_parent_node;
	uchar4 color;
	uchar4 color_multiplier;
	int reflections_count; // as well as refractions and any other direction changes
} TracingState;

#if !defined(MAX_ALLOWED_TRACING_STATE_SIZE)
#warning MAX_ALLOWED_TRACING_STATE_SIZE not defined
// TODO: find a way to assert this
//#elif sizeof(TracingState) > MAX_ALLOWED_TRACING_STATE_SIZE 
//#error sizeof(TracingState) > MAX_ALLOWED_TRACING_STATE_SIZE
#endif

int is_zero(uchar4 v) {
	return !v.x && !v.y && !v.z && !v.w;
}

#endif // COMMON_CL_INCLUDED
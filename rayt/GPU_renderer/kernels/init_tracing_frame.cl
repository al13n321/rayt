#include "common.cl"

float4 mul(float16 mat, float3 vec3) {
    float4 vec = (float4)(vec3.x, vec3.y, vec3.z, 1);
    float d = dot(vec, (float4)(mat.sc, mat.sd, mat.se, mat.sf));
    return make_float3(dot(vec, (float4)(mat.s0, mat.s1, mat.s2, mat.s3)),
                       dot(vec, (float4)(mat.s4, mat.s5, mat.s6, mat.s7)),
                       dot(vec, (float4)(mat.s8, mat.s9, mat.sa, mat.sb))) / d;
}

__kernel void InitTracingFrame(__global TracingState *tracing_states,
                               float16 view_proj_inv,
                               float lod_voxel_size)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    
    if(x < WIDTH && y < HEIGHT) {
        int index = y * WIDTH + x;
        
        float3 tmp = make_float3((x + 0.5f) / WIDTH * 2 - 1, (y + 0.5f) / HEIGHT * 2 - 1, -1);
        float3 origin = make_float3(view_proj_inv.s2, view_proj_inv.s6, view_proj_inv.sa) / view_proj_inv.se;
        tmp.z = 0;
        float3 direction = mul(view_proj_inv, tmp) - origin;
        direction = normalize(direction);
        
        tmp = make_float3((float)x / WIDTH * 2 - 1, (float)y / HEIGHT * 2 - 1, 0);
        float3 p1far = mul(view_proj_inv, tmp);
        tmp = make_float3((float)(x + 1) / WIDTH * 2 - 1, (float)(y + 1) / HEIGHT * 2 - 1, 0);
        float3 p2far = mul(view_proj_inv, tmp);
        
        float ray_size_bias = 0;
        float ray_size_coef = lod_voxel_size * distance(p1far, p2far) / distance(origin, p1far);

		TracingState s;
		s.ray_origin = (float4)(origin.x, origin.y, origin.z, ray_size_bias);
		s.ray_direction = (float4)(direction.x, direction.y, direction.z, ray_size_coef);
		s.fault_parent_node = -1;
		s.color = (uchar4)(0, 0, 0, 0);
		s.color_multiplier = (uchar4)(255, 255, 255, 255);
		s.reflections_count = 0;
		
		tracing_states[index] = s;
    }
}

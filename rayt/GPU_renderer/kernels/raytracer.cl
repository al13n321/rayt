#include "raycast.cl"

#if !defined(WIDTH) || !defined(HEIGHT)
#define WIDTH 128
#define HEIGHT 128
#endif

float4 mul(float16 mat, float3 vec3) {
    float4 vec = (float4)(vec3.x, vec3.y, vec3.z, 1);
    float d = dot(vec, (float4)(mat.sc, mat.sd, mat.se, mat.sf));
    return make_float3(dot(vec, (float4)(mat.s0, mat.s1, mat.s2, mat.s3)),
                       dot(vec, (float4)(mat.s4, mat.s5, mat.s6, mat.s7)),
                       dot(vec, (float4)(mat.s8, mat.s9, mat.sa, mat.sb))) / d;
}

__kernel void RaytraceKernel(__global uchar4 *result_colors,
                             __global uint *node_links,
                             __global uint *far_pointers,
                             int root_node_index,
                             float16 view_proj_inv,
                             float lod_voxel_size)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    
    if(x < WIDTH && y < HEIGHT) {
        int index = y * WIDTH + x;
        
        float3 tmp = make_float3((x + 0.5f) / WIDTH * 2 - 1, (y + 0.5f) / HEIGHT * 2 - 1, -1);
        float3 origin = mul(view_proj_inv, tmp);
        tmp.z = 0;
        float3 direction = mul(view_proj_inv, tmp) - origin;
        direction = normalize(direction);
        
        tmp = make_float3((float)x / WIDTH * 2 - 1, (float)y / HEIGHT * 2 - 1, -1);
        float3 p1near = mul(view_proj_inv, tmp);
        tmp.z = 0;
        float3 p1far = mul(view_proj_inv, tmp);
        tmp = make_float3((float)(x + 1) / WIDTH * 2 - 1, (float)(y + 1) / HEIGHT * 2 - 1, -1);
        float3 p2near = mul(view_proj_inv, tmp);
        tmp.z = 0;
        float3 p2far = mul(view_proj_inv, tmp);
        
        float ray_size_bias = lod_voxel_size * distance(p1near, p2near);
        float ray_size_coef = (ray_size_bias < 1e-10f) ? 0 : (lod_voxel_size * (distance(p1far, p2far) - distance(p1near, p2near)) / distance(p1near, p1far));

		struct RayCastResult res;
        
        CastRay(node_links,
		        far_pointers,
                root_node_index,
                origin,
                direction,
                ray_size_coef,
                ray_size_bias,
                &res);
        
        if (res.error_code || res.fault_block != -1) {
            result_colors[index] = (uchar4)(255, 0, 0, 255);
        } else if(res.hit_node == -1) {
			result_colors[index] = (uchar4)(0, 0, 0, 255);
        } else {
			float near = 0.01f;
            float far = 5;
            float d = (log(res.hit_t) - log(near)) / (log(far) - log(near));
            d = (1 - d) * 255;
            float g = res.node_depth * 25;
            result_colors[index] = (uchar4)(d, d, d, 255);
            //result_colors[index] = (uchar4)(g, g, g, 255);
        }
    }
}

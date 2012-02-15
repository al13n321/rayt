#if !defined(WIDTH) || !defined(HEIGHT)
#define WIDTH 128
#define HEIGHT 128
#endif

#define float3 float4
#define make_float3(a,b,c) ((float4){a,b,c,0})

__constant const float infinite_hit_t = 1e30f;

// for some reason apple implementation of opencl doesn't have this function
static int popcount(uchar a) {
    const uchar l[16] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };
    return l[a & 15] + l[a >> 4];
}

static float max_component(float3 v) {
    return fmax(fmax(v.x, v.y), v.z);
}

static float min_component(float3 v) {
    return fmin(fmin(v.x, v.y), v.z);
}

static void CastRay(__global uint *node_links,
                    int root,
                    float3 origin,
                    float3 dir, // unit length
                    float ray_size_coef, // LOD increase along ray
                    float ray_size_bias, // LOD at ray origin
                    float *hit_t, // infinite_hit_t if no hit
                    int *hit_node, // -1 if no hit
                    int *fault_block, // -1 if no fault
                    int *error_code) // 0 - ok, 1 - step limit exceeded
{
    *fault_block = -1;
    *error_code = 0;
    
    const float epsilon = 1e-6f;
    
    if (fabs(dir.x) < epsilon) dir.x = copysign(epsilon, dir.x);
    if (fabs(dir.y) < epsilon) dir.y = copysign(epsilon, dir.y);
    if (fabs(dir.z) < epsilon) dir.z = copysign(epsilon, dir.z);
    
    float3 t_coef = make_float3(1.0f / fabs(dir.x), 1.0f / fabs(dir.y), 1.0f / fabs(dir.z));
    
    volatile float3 t_bias = make_float3(t_coef.x * origin.x, t_coef.y * origin.y, t_coef.z * origin.z);
    
    uchar octant_mask = 0;
    if (dir.x < 0.0f) octant_mask ^= 1, t_bias.x = t_coef.x - t_bias.x;
    if (dir.y < 0.0f) octant_mask ^= 2, t_bias.y = t_coef.y - t_bias.y;
    if (dir.z < 0.0f) octant_mask ^= 4, t_bias.z = t_coef.z - t_bias.z;
    
    float t_max = min_component(t_coef - t_bias);
    float t_min = fmax(0.0f, max_component(-t_bias));
    
    int steps = 0;
    while (t_max > t_min) {
        volatile int node = root;
        volatile float3 pos = make_float3(0, 0, 0);
        volatile float size = 1;
        volatile float ray_size = ray_size_bias + ray_size_coef * t_min;
        while (1) {
            if (++steps > 5000) {
                *error_code = 1;
                return;
            }
            
            if (size < ray_size) {
                *hit_t = t_min;
                *hit_node = node;
                return;
            }
            
            volatile uint link = node_links[node];
            volatile uchar children_mask = link & 255;
            
            if (!children_mask) {
                *hit_t = t_min;
                *hit_node = node;
                return;
            }
            
            size *= 0.5;
            volatile float3 npos = pos + make_float3(size, size, size);
            volatile float3 t_center = npos * t_coef - t_bias;
            volatile uint index = octant_mask;
            if (t_center.x < t_min) index ^= 1, pos.x = npos.x;
            if (t_center.y < t_min) index ^= 2, pos.y = npos.y;
            if (t_center.z < t_min) index ^= 4, pos.z = npos.z;
            
            volatile uchar index_mask = 1 << index;
            
            if (!(children_mask & index_mask)) {
                t_min = min_component((pos + make_float3(size, size, size)) * t_coef - t_bias) + epsilon;
                break;
            }
            
            if (link & (1 << 8)) {
                *hit_t = t_min;
                *hit_node = node;
                *fault_block = link >> 9;
                return;
            }
            
            node = (link >> 9) + popcount(children_mask & (index_mask - 1));
        }
    }

    *hit_t = infinite_hit_t;
    *hit_node = -1;
}

static float4 mul(float16 mat, float3 vec3) {
    float4 vec = (float4)(vec3.x, vec3.y, vec3.z, 1);
    float d = dot(vec, (float4)(mat.sc, mat.sd, mat.se, mat.sf));
    return make_float3(dot(vec, (float4)(mat.s0, mat.s1, mat.s2, mat.s3)),
                       dot(vec, (float4)(mat.s4, mat.s5, mat.s6, mat.s7)),
                       dot(vec, (float4)(mat.s8, mat.s9, mat.sa, mat.sb))) / d;
}

__kernel void RaytraceKernel(__global uchar4 *result,
                             __global read_only uint *node_links,
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
        float ray_size_coef = (ray_size_bias < 1e-10) ? 0 : (lod_voxel_size * (distance(p1far, p2far) - distance(p1near, p2near)) / distance(p1near, p1far));
        
        float hit_t;
        int hit_node;
        int fault_block;
        int error_code = -1;
        
        CastRay(node_links,
                root_node_index,
                origin,
                direction,
                ray_size_coef,
                ray_size_bias,
                &hit_t,
                &hit_node,
                &fault_block,
                &error_code);
        
        if (error_code) {
            result[index] = (uchar4)(255, 0, 0, 255);
        } else {
            float lim = 5;
            hit_t = min(1.0f, hit_t / lim);
            float d = 255 - hit_t * 255;
            result[index] = (uchar4)(d, d, d, 255);
        }
    }
}

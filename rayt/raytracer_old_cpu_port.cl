#if !defined(WIDTH) || !defined(HEIGHT)
#define WIDTH 128
#define HEIGHT 128
#endif

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

static bool all_less(float3 a, float3 b) {
    return all(isless(a, b));
}

struct TraverseState {
    int node;
    float3 center;
};

static void CastRay(__constant uint *node_links,
                    int root,
                    float3 origin,
                    float3 dir, // unit length
                    float ray_size_coef, // LOD increase along ray (currently ignored)
                    float ray_size_bias, // LOD at ray origin (currently ignored)
                    float *hit_t, // infinite_hit_t if no hit
                    int *hit_node, // -1 if no hit
                    int *fault_block, // -1 if no fault
                    int *error_code) // 0 - ok, 1 - step limit exceeded
{
    const float epsilon = 1e-6;

    struct TraverseState traverse_stack[15];
    
    *fault_block = -1;
    *error_code = 0;
    
    float3 dir_signs = make_float3(1.0f, 1.0f, 1.0f);
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
    dir.x = fmax(dir.x, epsilon);
    dir.y = fmax(dir.y, epsilon);
    dir.z = fmax(dir.z, epsilon);

    float3 dir_inv = make_float3(1.0f / dir.x, 1.0f / dir.y, 1.0f / dir.z);
            
    struct TraverseState *state = traverse_stack;
    state->node = root;
    state->center = make_float3(0.0f, 0.0f, 0.0f);
    float half_size = 0.5f;
    float3 point = origin * dir_signs;
            
    float3 half_times = dir_inv * half_size;
    float3 point_times = dir_inv * point;
    float in_time = max_component(-point_times - half_times);
    float out_time = min_component(-point_times + half_times);

    if (out_time <= in_time || out_time <= 0) {
        *hit_t = infinite_hit_t;
        *hit_node = -1;
        return;
    }
    //*
    if (in_time > 0)
        point += dir * in_time;
    
    int steps_done = 0;
    do {
        while (true) {
            if (++steps_done > 100) {
                *error_code = 1;
                return;
            }
            
            if (!(node_links[state->node] & 255)) {
                *hit_t = distance(point * dir_signs, origin);
                *hit_node = state->node;
                return;
            }
            
            struct TraverseState *new_state = state + 1;
            
            half_size *= 0.5f;
            
            int index = 0;
            float3 signs = make_float3(-1.0f, -1.0f, -1.0f);
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
            
            index ^= dir_signs_bits;
            
            new_state->center = state->center + signs * half_size;
            
            if (!(node_links[state->node] & (1 << index))) {
                state = new_state;
                break;
            }
            
            if (node_links[state->node] & (1 << 8)) {
                *hit_t = distance(point * dir_signs, origin);
                *hit_node = state->node;
                *fault_block = node_links[state->node] >> 9;
                return;
            }
            
            new_state->node = (node_links[state->node] >> 9) + popcount(node_links[state->node] & ((1 << index) - 1));
            state = new_state;
        }
                
        float3 shifts = (state->center + make_float3(half_size, half_size, half_size) - point) * dir_inv;
        float sh = min_component(shifts);
        
        point += dir * (sh + epsilon);
        
        do {
            if (++steps_done > 100) {
                *error_code = 1;
                return;
            }
            
            half_size *= 2;
            --state;
        } while (state >= traverse_stack && !all_less(point, state->center + make_float3(half_size, half_size, half_size)));
    } while (state >= traverse_stack);

    *hit_t = infinite_hit_t;
    *hit_node = -1;
     //*/
}

static float3 mul(float16 mat, float3 vec3) {
    float4 vec = (float4)(vec3.x, vec3.y, vec3.z, 1);
    float d = dot(vec, (float4)(mat.sc, mat.sd, mat.se, mat.sf));
    return (float3)(dot(vec, (float4)(mat.s0, mat.s1, mat.s2, mat.s3)),
                    dot(vec, (float4)(mat.s4, mat.s5, mat.s6, mat.s7)),
                    dot(vec, (float4)(mat.s8, mat.s9, mat.sa, mat.sb))) / d;
}

__kernel void RaytraceKernel(__global uchar4 *result,
                             __constant uint *node_links,
                             int root_node_index,
                             float16 view_proj_inv,
                             int test_x, // QQQ
                             int test_y)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    
    if(x < WIDTH && y < HEIGHT) {
        int index = y * WIDTH + x;
        
        if (abs(x - test_x) + abs(y - test_y) < 5) { // QQQ
            result[index] = (uchar4)(0, 0, 255, 255);
            return;
        }
        
        float3 tmp = (float3)((x + 0.5f) / WIDTH * 2 - 1, (y + 0.5f) / HEIGHT * 2 - 1, -1);
        float3 origin = mul(view_proj_inv, tmp);
        tmp.z = 0;
        float3 direction = mul(view_proj_inv, tmp) - origin;
        direction = normalize(direction);
        
        float hit_t;
        int hit_node;
        int fault_block;
        int error_code = -1;
        
        int2 asd[24];
        asd[22] = make_int2(0, as_int(5.13325514f));
        if (asd[22].x != 0) {
            result[index] = (uchar4)(0, 255, 0, 255);
            return;
        }
        
        //*
        CastRay(node_links,
                root_node_index,
                origin,
                direction,
                0, // LOD increase along ray
                0, // LOD at ray origin
                &hit_t,
                &hit_node,
                &fault_block,
                &error_code);//*/
        
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

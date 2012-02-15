// I don't know how to write such copyright-related texts at the top of the file, but here's the point:
// Ray cast function is an adaptation of source code from an extended version ("Efficient Sparse Voxel Octrees – Analysis, Extensions, and Implementation") of a great article [LAINE, S., AND KARRAS, T. 2010. Efficient sparse voxel octrees. In Proceedings of ACM SIGGRAPH 2010 Symposium on Interactive 3D Graphics and Games]

#ifdef __FAST_RELAXED_MATH__
#error it's unlikely to work with __FAST_RELAXED_MATH__
#endif

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

static volatile int highbit(volatile int a) {
    volatile int r = 0;
    while (a > 1) {
        ++r;
        a >>= 1;
    }
    return r;
}

static void CastRay(__constant uint *node_links,
                    int root_node,
                    float3 p, // ray origin
                    float3 d, // ray direction
                    float ray_size_coef, // LOD increase along ray
                    float ray_size_bias, // LOD at ray origin
                    float *hit_t,
                    int *hit_node,
                    int *fault_block)
{
    p += make_float3(1, 1, 1); // move root node from [0, 1] to [1, 2]
    *fault_block = -1;
    
    volatile const int s_max = 23; // Maximum scale (number of float mantissa bits).
    volatile const float epsilon = exp2((float)-s_max);
    
    volatile __private int4 stack[s_max + 1]; // Stack of parent voxels (local mem).
    
    // Get rid of small ray direction components to avoid division by zero.
    
    if (fabs(d.x) < epsilon) d.x = copysign(epsilon, d.x);
    if (fabs(d.y) < epsilon) d.y = copysign(epsilon, d.y);
    if (fabs(d.z) < epsilon) d.z = copysign(epsilon, d.z);
    
    // Precompute the coefficients of tx(x), ty(y), and tz(z).
    // The octree is assumed to reside at coordinates [1, 2].
    
    volatile float tx_coef = 1.0f / -fabs(d.x);
    volatile float ty_coef = 1.0f / -fabs(d.y);
    volatile float tz_coef = 1.0f / -fabs(d.z);
    
    volatile float tx_bias = tx_coef * p.x;
    volatile float ty_bias = ty_coef * p.y;
    volatile float tz_bias = tz_coef * p.z;
    
    // Select octant mask to mirror the coordinate system so
    // that ray direction is negative along each axis.
    
    volatile int octant_mask = 0;
    if (d.x > 0.0f) octant_mask ^= 1, tx_bias = 3.0f * tx_coef - tx_bias;
    if (d.y > 0.0f) octant_mask ^= 2, ty_bias = 3.0f * ty_coef - ty_bias;
    if (d.z > 0.0f) octant_mask ^= 4, tz_bias = 3.0f * tz_coef - tz_bias;
    
    // Initialize the active span of t-values.
    
    volatile float t_min = fmax(fmax(2.0f * tx_coef - tx_bias, 2.0f * ty_coef - ty_bias), 2.0f * tz_coef - tz_bias);
    volatile float t_max = fmin(fmin(tx_coef - tx_bias, ty_coef - ty_bias), tz_coef - tz_bias);
    volatile float h = t_max;
    t_min = fmax(t_min, 0.0f);
    //t_max = fmin(t_max, 1.0f);
    
    if (t_max <= t_min) { // QQQ
        *hit_t = infinite_hit_t;
        *hit_node = -1;
        return;
    }
    
    // Initialize the current voxel to the first child of the root.
    
    volatile int parent = root_node;
    volatile uint child_descriptor = 0; // invalid until fetched
    volatile int idx = 0;
    volatile float3 pos = make_float3(1.0f, 1.0f, 1.0f);
    volatile int scale = s_max - 1;
    volatile float scale_exp2 = 0.5f; // exp2f(scale - s_max)
    
    if (1.5f * tx_coef - tx_bias > t_min) idx ^= 1, pos.x = 1.5f;
    if (1.5f * ty_coef - ty_bias > t_min) idx ^= 2, pos.y = 1.5f;
    if (1.5f * tz_coef - tz_bias > t_min) idx ^= 4, pos.z = 1.5f;
    
    volatile int iter = 0; // QQQ
    
    int assigned = 0;
    
    // Traverse voxels along the ray as long as the current voxel
    // stays within the octree.
    
    while (scale < s_max && ++iter < 100)
    {   
        // Fetch child descriptor unless it is already valid.
        
        if (child_descriptor == 0)
            child_descriptor = node_links[parent];
        
        // Determine maximum t-value of the cube by evaluating
        // tx(), ty(), and tz() at its corner.
        
        volatile float tx_corner = pos.x * tx_coef - tx_bias;
        volatile float ty_corner = pos.y * ty_coef - ty_bias;
        volatile float tz_corner = pos.z * tz_coef - tz_bias;
        volatile float tc_max = fmin(fmin(tx_corner, ty_corner), tz_corner);
        
        // Process voxel if the corresponding bit in valid mask is set
        // and the active t-span is non-empty.
        
        volatile int child_shift = idx ^ octant_mask; // permute child slots based on the mirroring
        volatile int child_masks = 1 << child_shift;
        if ((child_descriptor & child_masks) != 0 && t_min <= t_max)
        {
            // Terminate if children are not loaded.
            
            if ((child_descriptor & (1 << 8)) != 0) {
                *hit_node = parent;
                *hit_t = t_min;
                *fault_block = child_descriptor >> 9;
                return;
            }
            
            // Child node pointer.
            
            volatile int ofs = child_descriptor >> 9; // child pointer
            ofs += popcount(child_descriptor & (child_masks - 1));
            
            // Terminate if the voxel is small enough.
            
            if (tc_max * ray_size_coef + ray_size_bias >= scale_exp2) {
                *hit_t = t_min;
                *hit_node = ofs;
                return;
            }
            
            // Fetch descriptor of child.
            
            child_descriptor = node_links[ofs];
            
            // Terminate if child is a leaf.
            
            if (!(child_descriptor & 255)) {
                *hit_t = t_min;
                *hit_node = ofs;
                return;
            }
            
            // INTERSECT
            // Intersect active t-span with the cube and evaluate
            // tx(), ty(), and tz() at the center of the voxel.
            
            volatile float half = scale_exp2 * 0.5f;
            volatile float tx_center = half * tx_coef + tx_corner;
            volatile float ty_center = half * ty_coef + ty_corner;
            volatile float tz_center = half * tz_coef + tz_corner;
            
            // QQQ
            if (t_min > tc_max) {
                *fault_block = 1;
                return;
            }
            
            // Descend to the first child.
            
            // PUSH
            // Write current parent to the stack.
            
            if (tc_max < h) {
                stack[scale] = make_int4(parent, as_int(t_max), 0, 0);
                if (scale == 22 && parent == 0) assigned = 1;
            }
            h = tc_max;
            
            parent = ofs;
            
            // Select child voxel that the ray enters first.
            
            idx = 0;
            scale--;
            scale_exp2 = half;
            if (tx_center > t_min) idx ^= 1, pos.x += scale_exp2;
            if (ty_center > t_min) idx ^= 2, pos.y += scale_exp2;
            if (tz_center > t_min) idx ^= 4, pos.z += scale_exp2;
            
            // Update active t-span and invalidate cached child descriptor.
            
            t_max = tc_max;
            continue;
        }
        
        // ADVANCE
        // Step along the ray.
        
        volatile int step_mask = 0;
        if (tx_corner <= tc_max) step_mask ^= 1, pos.x -= scale_exp2;
        if (ty_corner <= tc_max) step_mask ^= 2, pos.y -= scale_exp2;
        if (tz_corner <= tc_max) step_mask ^= 4, pos.z -= scale_exp2;
        
        // Update active t-span and flip bits of the child slot index.
        
        t_min = tc_max;
        idx ^= step_mask;
        
        // Proceed with pop if the bit flips disagree with the ray direction.
        
        if ((idx & step_mask) != 0)
        {   
            // POP
            // Find the highest differing bit between the two positions.
        
            volatile unsigned int differing_bits = 0;
            if ((step_mask & 1) != 0) differing_bits |= as_int(pos.x) ^ as_int(pos.x + scale_exp2);
            if ((step_mask & 2) != 0) differing_bits |= as_int(pos.y) ^ as_int(pos.y + scale_exp2);
            if ((step_mask & 4) != 0) differing_bits |= as_int(pos.z) ^ as_int(pos.z + scale_exp2);
            scale = (as_int((float)differing_bits) >> 23) - 127; // position of the highest bit
            //scale = highbit(differing_bits);
            //if (scale == 22) { *fault_block = 1; return; } // QQQ
            scale_exp2 = as_float((scale - s_max + 127) << 23); // exp2f(scale - s_max)
            if (scale_exp2 != exp2((float)(scale - s_max))) { *fault_block = 1; return; } // QQQ
            
            // Restore parent voxel from the stack.
            
            volatile int4 stackEntry = stack[scale];
            parent = stackEntry.x;
            t_max = as_float(stackEntry.y);
            
            // Round cube position and extract child slot index.
            
            volatile int shx = as_int(pos.x) >> scale;
            volatile int shy = as_int(pos.y) >> scale;
            volatile int shz = as_int(pos.z) >> scale;
            pos.x = as_float(shx << scale);
            pos.y = as_float(shy << scale);
            pos.z = as_float(shz << scale);
            idx = (shx & 1) | ((shy & 1) << 1) | ((shz & 1) << 2);
            
            if (scale == 22) { // QQQ
                //if (!assigned) *fault_block = 1;
                //if (stack[scale].x > 1071199000) *fault_block = 1;
                //if (parent == 23) *fault_block = 1;
                parent = 0;
                idx = 1;
                pos = make_float3(1.5, 1, 1);
                //return;
            }
            
            // Prevent same parent from being stored again and invalidate cached child descriptor.
            
            h = 0.0f;
            child_descriptor = 0;
        }
    }
    
    if (iter >= 100)
        *fault_block = 1;
    
    // Indicate miss if we are outside the octree.
    
    *hit_t = infinite_hit_t;
    *hit_node = -1;
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
                &fault_block);//*/
        
        if (fault_block != -1) {
            result[index] = (uchar4)(255, 0, 0, 255);
        } else {
            float lim = 5;
            hit_t = min(1.0f, hit_t / lim);
            float d = 255 - hit_t * 255;
            result[index] = (uchar4)(d, d, d, 255);
        }
    }
}

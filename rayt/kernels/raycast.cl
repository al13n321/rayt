// I don't know how to write such copyright-related texts at the top of the file, but here's the point:
// Ray cast function is an adaptation of source code from an extended version ("Efficient Sparse Voxel Octrees – Analysis, Extensions, and Implementation") of a great article [LAINE, S., AND KARRAS, T. 2010. Efficient sparse voxel octrees. In Proceedings of ACM SIGGRAPH 2010 Symposium on Interactive 3D Graphics and Games]

#include "common.cl"

#ifdef __FAST_RELAXED_MATH__
#error it's unlikely to work with __FAST_RELAXED_MATH__
#endif

#define S_MAX 23 // Maximum scale (number of float mantissa bits).
#define EPSILON 0.00000011920928955078125f // exp2(-S_MAX)

#define infinite_hit_t 1e30f;

#define ERROR_ITERATION_LIMIT 1;

int popcount(uchar a) {
    const uchar l[16] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };
    return l[a & 15] + l[a >> 4];
}

typedef struct RayCastResult
{
	float hit_t;
	int hit_node;
	int fault_block;
	int error_code;
	
	int node_depth; // in tree
} RayCastResult;

void CastRay(__global uint *node_links,
             __global uint *far_pointers,
             int root_node,
             float3 p, // ray origin
             float3 d, // ray direction
             float ray_size_coef, // LOD increase along ray
             float ray_size_bias, // LOD at ray origin
             struct RayCastResult *res)
{
    p += make_float3(1, 1, 1); // move root node from [0, 1] to [1, 2]
    res->fault_block = -1;
    res->error_code = 0;
    
    __private uint2 stack[S_MAX + 1]; // Stack of parent voxels (local mem).
    
    // Get rid of small ray direction components to avoid division by zero.
    
    if (fabs(d.x) < EPSILON) d.x = copysign(EPSILON, d.x);
    if (fabs(d.y) < EPSILON) d.y = copysign(EPSILON, d.y);
    if (fabs(d.z) < EPSILON) d.z = copysign(EPSILON, d.z);
    
    // Precompute the coefficients of tx(x), ty(y), and tz(z).
    // The octree is assumed to reside at coordinates [1, 2].
    
    float tx_coef = 1.0f / -fabs(d.x);
    float ty_coef = 1.0f / -fabs(d.y);
    float tz_coef = 1.0f / -fabs(d.z);
    
    float tx_bias = tx_coef * p.x;
    float ty_bias = ty_coef * p.y;
    float tz_bias = tz_coef * p.z;
    
    // Select octant mask to mirror the coordinate system so
    // that ray direction is negative along each axis.
    
    uchar octant_mask = 0;
    if (d.x > 0.0f) octant_mask ^= 1, tx_bias = 3.0f * tx_coef - tx_bias;
    if (d.y > 0.0f) octant_mask ^= 2, ty_bias = 3.0f * ty_coef - ty_bias;
    if (d.z > 0.0f) octant_mask ^= 4, tz_bias = 3.0f * tz_coef - tz_bias;
    
    // Initialize the active span of t-values.
    
    float t_min = fmax(fmax(2.0f * tx_coef - tx_bias, 2.0f * ty_coef - ty_bias), 2.0f * tz_coef - tz_bias);
    float t_max = fmin(fmin(tx_coef - tx_bias, ty_coef - ty_bias), tz_coef - tz_bias);
    float h = t_max;
    t_min = fmax(t_min, 0.0f);
    
    // Initialize the current voxel to the first child of the root.
    
    uint parent = root_node;
    uint child_descriptor = 0; // invalid until fetched
    uchar idx = 0;
    float3 pos = make_float3(1.0f, 1.0f, 1.0f);
    uchar scale = S_MAX - 1;
    float scale_exp2 = 0.5f; // exp2f(scale - S_MAX)
    
    if (1.5f * tx_coef - tx_bias > t_min) idx ^= 1, pos.x = 1.5f;
    if (1.5f * ty_coef - ty_bias > t_min) idx ^= 2, pos.y = 1.5f;
    if (1.5f * tz_coef - tz_bias > t_min) idx ^= 4, pos.z = 1.5f;
    
    ushort iter = 0;
    const ushort iter_limit = 500;
    
    // Traverse voxels along the ray as long as the current voxel
    // stays within the octree.
    
    while (scale < S_MAX)
    {   
		// Check for iteration limit
		
		if (++iter > iter_limit) {
			res->error_code = ERROR_ITERATION_LIMIT;
			res->hit_t = t_min;
			res->hit_node = parent;
			res->node_depth = S_MAX - scale - 1;
			return;
		}
    
        // Fetch child descriptor unless it is already valid.
        
        if (child_descriptor == 0)
            child_descriptor = node_links[parent];
        
        // Determine maximum t-value of the cube by evaluating
        // tx(), ty(), and tz() at its corner.
        
        float tx_corner = pos.x * tx_coef - tx_bias;
        float ty_corner = pos.y * ty_coef - ty_bias;
        float tz_corner = pos.z * tz_coef - tz_bias;
        float tc_max = fmin(fmin(tx_corner, ty_corner), tz_corner);
        
        // Process voxel if the corresponding bit in valid mask is set
        // and the active t-span is non-empty.
        
        uchar child_shift = idx ^ octant_mask; // permute child slots based on the mirroring
        uchar child_masks = 1 << child_shift;
        if ((child_descriptor & child_masks) != 0 && t_min <= t_max)
        {
            // Terminate if children are not loaded.
            
            if ((child_descriptor & (1 << 8)) != 0) {
                res->hit_node = parent;
                res->hit_t = t_min;
                res->fault_block = child_descriptor >> 9;
				res->node_depth = S_MAX - scale - 1;
                return;
            }
            
            // Child node pointer.
            
            uint ofs;
            if (child_descriptor & (1 << 9)) { // far pointer
				ofs = far_pointers[child_descriptor >> 10];
            } else {
				ofs = (child_descriptor >> 10) + parent - (1 << 21); // child pointer
			}
            ofs += popcount(child_descriptor & (child_masks - 1));
            
            // Terminate if the voxel is small enough.
            
            if (tc_max * ray_size_coef + ray_size_bias >= scale_exp2) {
                res->hit_t = t_min;
                res->hit_node = ofs;
				res->node_depth = S_MAX - scale;
                return;
            }
            
            // Fetch descriptor of child.
            
            child_descriptor = node_links[ofs];
            
            // Terminate if child is a leaf.
            
            if (!(child_descriptor & 255)) {
                res->hit_t = t_min;
                res->hit_node = ofs;
				res->node_depth = S_MAX - scale;
                return;
            }
            
            // INTERSECT
            // Intersect active t-span with the cube and evaluate
            // tx(), ty(), and tz() at the center of the voxel.
            
            float half = scale_exp2 * 0.5f;
            float tx_center = half * tx_coef + tx_corner;
            float ty_center = half * ty_coef + ty_corner;
            float tz_center = half * tz_coef + tz_corner;
            
            // Descend to the first child.
            
            // PUSH
            // Write current parent to the stack.
            
            if (tc_max < h)
                stack[scale] = (uint2)(parent, as_uint(t_max));
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
        
        uchar step_mask = 0;
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
        
            uint differing_bits = 0;
            if ((step_mask & 1) != 0) differing_bits |= as_int(pos.x) ^ as_int(pos.x + scale_exp2);
            if ((step_mask & 2) != 0) differing_bits |= as_int(pos.y) ^ as_int(pos.y + scale_exp2);
            if ((step_mask & 4) != 0) differing_bits |= as_int(pos.z) ^ as_int(pos.z + scale_exp2);
            scale = (as_int((float)differing_bits) >> 23) - 127; // position of the highest bit
            scale_exp2 = as_float((scale - S_MAX + 127) << 23); // exp2f(scale - S_MAX)
            
            // Restore parent voxel from the stack.
            
            uint2 stackEntry = stack[scale];
            parent = stackEntry.x;
            t_max = as_float(stackEntry.y);
            
            // Round cube position and extract child slot index.
            
            uint shx = as_uint(pos.x) >> scale;
            uint shy = as_uint(pos.y) >> scale;
            uint shz = as_uint(pos.z) >> scale;
            pos.x = as_float(shx << scale);
            pos.y = as_float(shy << scale);
            pos.z = as_float(shz << scale);
            idx = (shx & 1) | ((shy & 1) << 1) | ((shz & 1) << 2);
            
            // Prevent same parent from being stored again and invalidate cached child descriptor.
            
            h = 0.0f;
            child_descriptor = 0;
        }
    }
    
    // Indicate miss if we are outside the octree.
    
    res->hit_t = infinite_hit_t;
    res->hit_node = -1;
}

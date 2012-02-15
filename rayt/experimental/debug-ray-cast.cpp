#include "debug-ray-cast.h"
using namespace std;
using namespace boost;

namespace rayt {

    // to make OpenCL code work without modifications
#define __constant
#define float3 fvec3
//#define make_float3 float3
//
//    struct int2 {
//        int x, y;
//    };
//    
//    static int2 make_int2(int x, int y) {
//        return (int2) { x, y };
//    }
//    
//    static int as_int(float a) {
//        return *reinterpret_cast<int*>(&a);
//    }
//    
//    static float as_float(int a) {
//        return *reinterpret_cast<float*>(&a);
//    }
//    
//    __constant const float infinite_hit_t = 1e30f;
//    
//    // for some reason my implementation of opencl doesn't have this function
//    static int popcount(uchar a) {
//        const uchar l[16] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };
//        return l[a & 15] + l[a >> 4];
//    }
//    
//    static float max_component(float3 v) {
//        return fmax(fmax(v.x, v.y), v.z);
//    }
//    
//    static float min_component(float3 v) {
//        return fmin(fmin(v.x, v.y), v.z);
//    }
//    
//    static float distance(float3 a, float3 b) {
//        return (a - b).Length();
//    }
//    
//    static bool all_less(float3 a, float3 b) {
//        return a.AllLessThan(b);
//    }
//    
//    struct TraverseState {
//        int node;
//        float3 center;
//    };
    
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
        /**fault_block = -1;
        *error_code = 0;
        
        const float epsilon = 1e-6f;
        
        if (fabs(dir.x) < epsilon) dir.x = copysign(epsilon, dir.x);
        if (fabs(dir.y) < epsilon) dir.y = copysign(epsilon, dir.y);
        if (fabs(dir.z) < epsilon) dir.z = copysign(epsilon, dir.z);
        
        float3 t_coef = make_float3(1.0f / fabs(dir.x), 1.0f / fabs(dir.y), 1.0f / fabs(dir.z));
        
        float3 t_bias = make_float3(t_coef.x * origin.x, t_coef.y * origin.y, t_coef.z * origin.z);
        
        int octant_mask = 0;
        if (dir.x < 0.0f) octant_mask ^= 1, t_bias.x = t_coef.x - t_bias.x;
        if (dir.y < 0.0f) octant_mask ^= 2, t_bias.y = t_coef.y - t_bias.y;
        if (dir.z < 0.0f) octant_mask ^= 4, t_bias.z = t_coef.z - t_bias.z;
        
        float t_max = min_component(t_coef - t_bias);
        float t_min = fmax(0.0f, max_component(-t_bias));
        
        while (t_max > t_min) {
            int node = root;
            float3 pos = make_float3(0, 0, 0);
            float size = 1;
            while (1) {
                uint link = node_links[node];
                uchar children_mask = link & 255;
                
                if (!children_mask) {
                    *hit_t = t_min;
                    *hit_node = node;
                    return;
                }
                
                size *= 0.5;
                float3 npos = pos + make_float3(size, size, size);
                float3 t_center = npos * t_coef - t_bias;
                uint index = octant_mask;
                if (t_center.x < t_min) index ^= 1, pos.x = npos.x;
                if (t_center.y < t_min) index ^= 2, pos.y = npos.y;
                if (t_center.z < t_min) index ^= 4, pos.z = npos.z;
                
                uchar index_mask = 1 << index;
                
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
        *hit_node = -1;*/
    }
    
    void DebugRayCast(const CLBuffer &node_links_buffer, int root_node, fvec3 origin, fvec3 direction) {
        uint *data = new uint[node_links_buffer.size() / 4];
        node_links_buffer.Read(0, node_links_buffer.size(), data, true);
        float hit_t;
        int hit_node;
        int fault_block;
        int error_code = -1;
        CastRay(data, root_node, origin, direction, 0, 0, &hit_t, &hit_node, &fault_block, &error_code);
        if (error_code) {
            cout << "ray cast error" << endl;
        } else if (hit_node != -1) {
            cout << "ray hit at distance " << hit_t << endl;
        } else {
            cout << "ray miss" << endl;
        }
        delete[] data;
    }
    
}

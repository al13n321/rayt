#include "raycast.cl"
#include "shading.cl"

__kernel void RaytracingPass(__global int *faults_and_hits, // out
							 __global TracingState *tracing_states, // in-out
                             __global uint *node_links,
                             __global uint *far_pointers,
                             int root_node_index)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    
    if(x < WIDTH && y < HEIGHT) {
        int index = y * WIDTH + x;
        
        TracingState s = tracing_states[index];
        
		int done;
		do {
			done = true;
        
			if (s.color_multiplier == (uchar4)(0, 0, 0, 0)) {
				faults_and_hits[index] = NO_HIT;
			} else {
				RayCastResult res;
				
				CastRay(node_links,
						far_pointers,
						root_node_index,
						s.ray_origin.xyz,
						s.ray_direction.xyz,
						s.ray_direction.w,
						s.ray_origin.w,
						&res);
		        
				if (res.error_code) {
					s.color = (uchar4)(255, 0, 0, 255);
					s.color_multiplier = (uchar4)(0, 0, 0, 0);
					faults_and_hits[index] = NO_HIT;
				} else if (res.fault_block != -1) {
					s.ray_origin += s.ray_direction * res.hit_t;
					s.fault_parent_node = res.hit_node;
					faults_and_hits[index] = -res.fault_block - 1;
				} else if(res.hit_node == -1) {
					s.color_multiplier = (uchar4)(0, 0, 0, 0);
					faults_and_hits[index] = NO_HIT;
				} else {
					ProcessHit(res.hit_t, res.hit_node, &s);
					if (s.color_multiplier == (uchar4)(0, 0, 0, 0)) {
						faults_and_hits[index] = s.hit_node + 1;
					} else {
						done = false;
					}
				}
			}
        } while(!done);
        
        tracing_states[index] = s;
    }
}

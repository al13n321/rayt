#include "shading.cl"

__kernel void FinishTracingFrame(__global uchar4 *result_colors,
                                 __global TracingState *tracing_states,
                                 float4 background_color,
								 __global char4 *node_normals,
								 __global uchar4 *node_colors)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    
    if(x < WIDTH && y < HEIGHT) {
        int index = y * WIDTH + x;
        
        TracingState s = tracing_states[index];
        
        if (!is_zero(s.color_multiplier))
			ProcessHit(0, s.fault_parent_node, &s, node_normals, node_colors);
			//s.color = (uchar4)(0, 255, 0, 255); // QQQ
        
        float4 t = (float4)(s.color.x, s.color.y, s.color.z, s.color.w) + background_color * (255.f - s.color.w) + (float4)(.5f, .5f, .5f, .5f);
        result_colors[index] = (uchar4)((uchar)max(0.f, min(255.f, t.x)), (uchar)max(0.f, min(255.f, t.y)), (uchar)max(0.f, min(255.f, t.z)), (uchar)max(0.f, min(255.f, t.w)));
    }
}

#include "shading.cl"

__kernel void FinishTracingFrame(__global uchar4 *result_colors,
								 __global TracingState *tracing_states,
								 __constant uchar4 background_color)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    
    if(x < WIDTH && y < HEIGHT) {
        int index = y * WIDTH + x;
        
        TracingState s = tracing_states[index];
        
        if (s.color_multiplier != (uchar4)(0, 0, 0, 0))
			ProcessHit(0, fault_parent_node, &s);
        
        result_colors[index] = s.color + (uchar4)(background_color * (s.color.w / 255.f) + (float4)(.5f, .5f, .5f, .5f));
    }
}

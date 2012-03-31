#include "common.cl"

// state is in-out
void ProcessHit(float hit_t, int hit_node, TracingState *state) {
	float near = 0.01f;
    float far = 5;
    float d = (log(hit_t) - log(near)) / (log(far) - log(near));
    d = (1 - d) * 255;
	state->color = (uchar4)(d, d, d, 255);
	state->color_multiplier = (uchar4)(0, 0, 0, 0);
}

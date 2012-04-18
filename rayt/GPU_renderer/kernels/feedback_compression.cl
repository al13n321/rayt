
#define NO_HIT ((1 << 30) - 1 + (1 << 30))

__kernel void NodeToBlockKernel(int N, int block_size, __global int *data) {
	int i = get_global_id(0);
	if (i >= N)
		return;
	
	int d = data[i];
	if (d > 0 && d != NO_HIT) {
		data[i] = ((((d >> 1) - 1) / block_size + 1) << 1) | (d & 1);
	}
}

__kernel void DifferenceFlagKernel(int N, __global int *in, __global int *out) {
	int i = get_global_id(0);
	if (i >= N)
		return;
		
	out[i] = (i != 0 && in[i-1] != in[i]) ? 1 : 0;
}

__kernel void CompressKernel(int N, __global int *index, __global int *in, __global int *out) {
	int i = get_global_id(0);
	if (i >= N)
		return;
	
	// workaround of what I believe is AMD OpenCL compiler bug
	if (i == 0) {
		out[0] = in[0];
	} else if (in[i-1] != in[i]) {
		out[index[i]] = in[i];
	}
}

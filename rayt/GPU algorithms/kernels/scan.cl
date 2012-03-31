
__kernel void ScanPassKernel(int N, int length, __global int *in, __global int *out) {
	int i = get_global_id(0);
	
	if (i >= N)
		return;
		
	int v = in[i];
	if (i - length >= 0)
		v += in[i - length];
	out[i] = v;
}

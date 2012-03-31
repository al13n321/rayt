
__kernel void LocalMergeSortKernel(int N, __global int *data, __local int *aux) {
	int gi = get_global_id(0);
	
	if (gi >= N)
		return;
	
	int i = get_local_id(0);
	int wg = get_local_size(0); // workgroup size = block size
	
	int offset = gi - i;
	data += offset;
	
	int n = min(wg, N - offset);

	aux[i] = data[i];
	barrier(CLK_LOCAL_MEM_FENCE);

	// merge sub-sequences of length 1, 2, ..., n/2
	for (int length = 1; length < n; length <<= 1) {
		int iKey = aux[i];
		int ii = i & (length - 1);  // index in our sequence in 0..length-1
		int sibling = (i - ii) ^ length; // beginning of the sibling sequence
		
		int pos;
		if (sibling >= n || (aux[sibling] > iKey) || (aux[sibling] == iKey && sibling > i)) {
			pos = -1;
		} else {
			pos = 0;
			for (int inc = length >> 1; inc > 0;inc >>= 1) { // increment for dichotomic search
				int j = sibling + pos + inc;
				int jKey = aux[j];
				int smaller = (j < n) && ((jKey < iKey) || (jKey == iKey && j < i));
				pos += (smaller) ? inc : 0;
			}
		}
		
		int dest = (ii + pos + 1) | (i & ~((length << 1) - 1));
		barrier(CLK_LOCAL_MEM_FENCE);
		aux[dest] = iKey;
		barrier(CLK_LOCAL_MEM_FENCE);
	}
		
	data[i] = aux[i];
}

__kernel void GlobalMergeSortPassKernel(int N, int length, __global int *in, __global int *out) {
	int i = get_global_id(0);
	
	if (i >= N)
		return;
	
	int iKey = in[i];
	int ii = i & (length - 1);  // index in our sequence in 0..length-1
	int sibling = (i - ii) ^ length; // beginning of the sibling sequence
	
	int pos;
	if (sibling >= N || (in[sibling] > iKey) || (in[sibling] == iKey && sibling > i)) {
		pos = -1;
	} else {
		pos = 0;
		for (int inc = length >> 1; inc > 0;inc >>= 1) { // increment for dichotomic search
			int j = sibling + pos + inc;
			int jKey = in[j];
			int smaller = (j < N) && ((jKey < iKey) || ( jKey == iKey && j < i ));
			pos += (smaller) ? inc : 0;
		}
	}
	
	int dest = (ii + pos + 1) | (i & ~((length << 1) - 1));

	out[dest] = iKey;
}


__kernel void updateMask(__global uchar* depthPixels,
						 __global uchar* maskPixels,
						 __global ushort* maskDetailPixels,
						 int nearThreshold,
						 int farThreshold,
						 int fadeRate) {
	const int i = get_global_id(0);
	int p = depthPixels[i];
	
	// Normalize between near and far thresholds.
	p = (clamp(p, farThreshold, nearThreshold) - farThreshold) * 255 / (nearThreshold - farThreshold);
	
	// Multiply by another 255 for the higher-res maskDetailPixels[].
	p *= 255;
	
	// Take the brighter pixel and fade back to black slowly.
	maskDetailPixels[i] = p > maskDetailPixels[i] ? p : max(0, maskDetailPixels[i] - fadeRate);
	
	// Copy low-fi value to maskPixels[].
	maskPixels[i] = maskDetailPixels[i] / 255;
}
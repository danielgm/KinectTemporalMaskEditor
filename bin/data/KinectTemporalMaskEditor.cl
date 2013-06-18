__constant sampler_t sampler =
	CLK_NORMALIZED_COORDS_FALSE
	| CLK_ADDRESS_CLAMP_TO_EDGE
	| CLK_FILTER_NEAREST;

__kernel void updateMask(read_only image2d_t depthmap,
						 read_only image2d_t prevMask,
						 write_only image2d_t mask,
						 write_only image2d_t nextMask,
						 float nearThreshold,
						 float farThreshold,
						 float fadeRate) {
	
	int2 coords = (int2)(get_global_id(0), get_global_id(1));
	float4 d = read_imagef(depthmap, sampler, coords);
	float4 p = read_imagef(prevMask, sampler, coords);

	// Normalize between near and far thresholds.
	d = (clamp(d, farThreshold, nearThreshold) - farThreshold) / (nearThreshold - farThreshold);
	
	d = d > p ? d : max(0, p - fadeRate);
	write_imagef(mask, coords, d);
	write_imagef(nextMask, coords, d);
}
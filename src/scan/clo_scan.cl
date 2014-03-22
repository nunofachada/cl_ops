/*   
 * This file is part of CL-Ops.
 * 
 * CL-Ops is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * CL-Ops is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with CL-Ops.  If not, see <http://www.gnu.org/licenses/>.
 * */

/** 
 * @file
 * @brief Parallel prefix sum (scan) implementation.
 * 
 * This implementation is based on 
 * [GPU Gems 3 - Chapter 39. Parallel Prefix Sum (Scan) with CUDA](http://http.developer.nvidia.com/GPUGems3/gpugems3_ch39.html).
 */

#ifndef CLO_SCAN_ELEM_TYPE
	#define CLO_SCAN_ELEM_TYPE uint
#endif

/**
 * @brief Performs a workgroup-wise scan.
 * 
 * @param data_in Vector to scan.
 * @param data_out Location where to place scan results.
 * @param aux Auxiliary local memory.
 * @param n Size of vector to scan.
 */
__kernel void workgroupScan(
			__global CLO_SCAN_ELEM_TYPE *data_in,
			__global CLO_SCAN_ELEM_TYPE *data_out,
			__global CLO_SCAN_ELEM_TYPE *data_wgsum,
			__local CLO_SCAN_ELEM_TYPE *aux) 
{

	uint gid = get_global_id(0);
	uint lid = get_local_id(0);
	uint lws = get_local_size(0);
	uint offset = 1;

	/* Load input data into local memory. */
	aux[2 * lid] = data_in[2 * gid];
	aux[2 * lid + 1] = data_in[2 * gid + 1];
	
	/* Upsweep: build sum in place up the tree. */
	for (uint d = lws >> 1; d > 0; d >>= 1) {
		barrier(CLK_LOCAL_MEM_FENCE);
		if (lid < d) {
			uint ai = offset * (2 * lid + 1) - 1;  
			uint bi = offset * (2 * lid + 2) - 1;  
			aux[bi] += aux[ai];
		}  
		offset *= 2;  
	}

	barrier(CLK_LOCAL_MEM_FENCE);
	if (lid == 0) { 
		/* Store the last element in workgroup sums. */
		data_wgsum[get_group_id(0)] = aux[lws - 1];
		/* Clear the last element. */
		aux[lws - 1] = 0; 
	}
	barrier(CLK_LOCAL_MEM_FENCE);              
	
 	/* Downsweep: traverse down tree and build scan. */
	for (uint d = 1; d < lws; d *= 2) {
		offset >>= 1;
		barrier(CLK_LOCAL_MEM_FENCE);
		if (lid < d) {
			uint ai = offset * (2 * lid + 1) - 1;
			uint bi = offset * (2 * lid + 2) - 1;
			CLO_SCAN_ELEM_TYPE t = aux[ai];
			aux[ai] = aux[bi];
			aux[bi] += t;
		}  
	}
	barrier(CLK_LOCAL_MEM_FENCE);

	/* Save scan result to global memory. */
	data_out[2 * gid] = aux[2 * lid];
	data_out[2 * gid + 1] = aux[2 * lid + 1];  
} 

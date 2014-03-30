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
 * This implementation is based on the design described in:
 * Blelloch, G. E. "Prefix Sums and Their Applications.", Technical 
 * Report CMU-CS-90-190, School of Computer Science, Carnegie Mellon 
 * University, 1990.
 * 
 * A maximum of three kernels are called for problems of any size, with
 * the first kernel serializing the scan operation when the array size
 * is larger than the squared local worksize.
 * 
 */

#ifndef CLO_SCAN_ELEM_TYPE
	#define CLO_SCAN_ELEM_TYPE uint
#endif

#ifndef CLO_SCAN_SUM_TYPE
	#define CLO_SCAN_SUM_TYPE ulong
#endif

/**
 * @brief Performs a workgroup-wise scan.
 * 
 * @param data_in Vector to scan.
 * @param data_out Location where to place scan results.
 * @param data_wgsum Workgroup-wise sums.
 * @param aux Auxiliary local memory.
 */
__kernel void workgroupScan(
			__global CLO_SCAN_ELEM_TYPE *data_in,
			__global CLO_SCAN_SUM_TYPE *data_out,
			__global CLO_SCAN_SUM_TYPE *data_wgsum,
			__local CLO_SCAN_SUM_TYPE *aux,
			uint numel,
			uint blocks_per_wg) 
{

	uint gid = get_global_id(0);
	uint lid = get_local_id(0);
	uint lsize = get_local_size(0);
	uint block_size =  lsize * 2;
	uint wgid = get_group_id(0);
	
	__local CLO_SCAN_SUM_TYPE in_sum[1];

	if (lid == 0) { 
		in_sum[0] = 0; 
	}

	for (uint b = 0; (b < blocks_per_wg) && ((wgid * blocks_per_wg + b) < (numel / block_size)); b++) {
		
		/* These global memory offsets improve memory coalescing. */
		uint goffset1 = (blocks_per_wg * wgid + b) * block_size + lid;
		uint goffset2 = goffset1 + lsize;

		uint offset = 1;
		
		/* Load input data into local memory. */
		aux[lid] = data_in[goffset1];
		aux[lid + lsize] = data_in[goffset2];
		
		/* Upsweep: build sum in place up the tree. */
		for (uint d = block_size >> 1; d > 0; d >>= 1) {
			barrier(CLK_LOCAL_MEM_FENCE);
			if (lid < d) {
				uint ai = offset * (2 * lid + 1) - 1;  
				uint bi = offset * (2 * lid + 2) - 1;  
				aux[bi] += aux[ai];
			}  
			offset *= 2;  
		}

		CLO_SCAN_SUM_TYPE in_sum_prev = in_sum[0];
		barrier(CLK_LOCAL_MEM_FENCE);
		if (lid == 0) { 
			/* Store the last element in intermediate sum. */
			in_sum[0] += aux[block_size - 1];
			/* Clear the last element. */
			aux[block_size - 1] = 0; 
		}
		barrier(CLK_LOCAL_MEM_FENCE);              
		
		/* Downsweep: traverse down tree and build scan. */
		for (uint d = 1; d < block_size; d *= 2) {
			offset >>= 1;
			barrier(CLK_LOCAL_MEM_FENCE);
			if (lid < d) {
				uint ai = offset * (2 * lid + 1) - 1;
				uint bi = offset * (2 * lid + 2) - 1;
				CLO_SCAN_SUM_TYPE t = aux[ai];
				aux[ai] = aux[bi];
				aux[bi] += t;
			}  
		}
		barrier(CLK_LOCAL_MEM_FENCE);
		
		/* Save scan result to global memory, adding the intermediate sum. */
		data_out[goffset1] = aux[lid] + in_sum_prev;
		data_out[goffset2] = aux[lid + lsize] + in_sum_prev;
	}

	if (lid == 0) { 
		/* Store the last element in workgroup sums. */
		data_wgsum[wgid] = in_sum[0];
	}
} 

/**
 * @brief Performs a scan on the workgroup sums vector.
 * 
 * @param data_wgsum Workgroup-wise sums.
 * @param aux Auxiliary local memory.
 */
__kernel void workgroupSumsScan(
			__global CLO_SCAN_SUM_TYPE *data_wgsum,
			__local CLO_SCAN_SUM_TYPE *aux) 
{
	
	uint lid = get_local_id(0);
	uint lsize = get_local_size(0);
	uint block_size = lsize * 2;
	uint offset = 1;

    /* Load input data into local memory. */
	aux[lid] = data_wgsum[lid];
	aux[lid + lsize] = data_wgsum[lid + lsize];	

    /* Upsweep: build sum in place up the tree. */
	for (uint d = block_size >> 1; d > 0; d >>= 1) {
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
		/* Clear the last element. */
		aux[block_size - 1] = 0; 
	}

 	/* Downsweep: traverse down tree and build scan. */
	for (uint d = 1; d < block_size; d *= 2) {
		offset >>= 1;
		barrier(CLK_LOCAL_MEM_FENCE);
		if (lid < d) {
			uint ai = offset * (2 * lid + 1) - 1;
			uint bi = offset * (2 * lid + 2) - 1;
			CLO_SCAN_SUM_TYPE t = aux[ai];
			aux[ai] = aux[bi];
			aux[bi] += t;
		}  
	}
	barrier(CLK_LOCAL_MEM_FENCE);

	/* Save scan result to global memory. */
	data_wgsum[lid] = aux[lid];
	data_wgsum[lid + lsize] = aux[lid + lsize]; 
}

/**
 * @brief Adds the workgroup-wise sums to the respective workgroup
 * elements.
 * 
 * @param data_wgsum Workgroup-wise sums.
 * @param data_out Location where to place scan results.
 */
__kernel void addWorkgroupSums(
	__global CLO_SCAN_SUM_TYPE *data_wgsum, 
	__global CLO_SCAN_SUM_TYPE *data_out,
	uint blocks_per_wg)
{	
	__local CLO_SCAN_SUM_TYPE wgsum[1];
	uint gid = get_global_id(0);

	/* The first workitem loads the respective workgroup sum. */
	if(get_local_id(0) == 0) {
		wgsum[0] = data_wgsum[get_group_id(0) / (2 * blocks_per_wg)];
	}
	barrier(CLK_LOCAL_MEM_FENCE);

	/* Then each workitem adds the sum to their respective array element. */
	data_out[gid] += wgsum[0];

}

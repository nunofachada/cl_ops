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
 * @brief Satish radix sort implementation.
 */

/**
 * @brief Calculates partial histogram with radix = 256
 * 
 * @param data Array of elements to sort.
 * @param bins Global histogram bins.	
 * @param shift Shift required to obtain current digit.
 * @param local_bins Local histogram bins.
  */
__kernel void satradixHistogram(
	__global const CLO_SORT_ELEM_TYPE* data,
	__global uint* bins,
	__local uint* local_bins,
	uint shiftCount) {
		
	size_t lid = get_local_id(0);
	size_t gid = get_global_id(0);
	size_t wgid = get_group_id(0);
	size_t wgsize = get_local_size(0);
	size_t num_wg = get_num_groups(0);
	
	/* Initialize local bins to zero */
	local_bins[lid] = 0;

	barrier(CLK_LOCAL_MEM_FENCE);
	
	/* Calculate local histograms with a radix of 256. */
	CLO_SORT_ELEM_TYPE value = data[gid];
	value = (value >> shiftCount) & 0xFFU;
	atomic_inc(local_bins + value);
		
	barrier(CLK_LOCAL_MEM_FENCE);
	
	/* Copy local histogram bins to global memory. */
	bins[num_wg * lid + wgid] = local_bins[lid];
	
}

_kernel void satradixPermute(
	__global const CLO_SORT_ELEM_TYPE* data_in,
	__global CLO_SORT_ELEM_TYPE* data_out,
	__global uint* bins,
	__local uint* local_bins,
	uint shiftCount) {
		
	size_t lid = get_local_id(0);
	size_t gid = get_global_id(0);
	size_t wgid = get_group_id(0);
	size_t wgsize = get_local_size(0);
	size_t num_wg = get_num_groups(0);
	
	/* Load bin data locally. */
	local_bins[lid] = bins[num_wg * lid + wgid];
		
	barrier(CLK_LOCAL_MEM_FENCE);

	/* Permute. */
	
	/* 1. Obtain current digit. */
	CLO_SORT_ELEM_TYPE value = data[gid];
	CLO_SORT_ELEM_TYPE digit = (value >> shiftCount) & 0xFFU;
	
	/* 2. Obtain histogram count for current digit */
	uint count = local_bins[value];
	
	/* 3. Copy value to global memory out */
	data_out[count] = value;
	
	/* 4. Increment histogram count for current digit */
	/// This wont work, must make a pre-scan or something
}

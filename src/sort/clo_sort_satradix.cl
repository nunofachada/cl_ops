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
 * 
 * Requires definition of:
 * 
 * * CLO_SORT_NUM_BITS - Number of bits in digit 
 */

#define CLO_SORT_RADIX (1 << CLO_SORT_NUM_BITS)
#define CLO_SORT_RADIX1 (CLO_SORT_RADIX - 1)

__kernel void satradixLocalSort(
	__global CLO_SORT_ELEM_TYPE* data_global,
	__global CLO_SORT_ELEM_TYPE* data_global_tmp,
	__local CLO_SORT_ELEM_TYPE* data_local,
	__local uint* scan_local,
	uint start_bit) {
		
	/// @todo Currently only sorts power of 2 sized arrays
	
	uint lid = get_local_id(0);
	uint gid = get_global_id(0);
	
	__local uint total_zeros[1];
	
	/* Load data locally. */
	data_local[lid] = data_global[gid];
	
	/* Perform local sort. */
	for (uint b = start_bit; b < start_bit + CLO_SORT_NUM_BITS; b++) {

		/* *** Perform split (sort by current bit) *** */

		/* Get current value. */
		CLO_SORT_ELEM_TYPE value = data_local[lid];
		CLO_SORT_KEY_TYPE key = CLO_SORT_KEY_GET(value);
		
		/* Get current bit */
		bool bit = (key >> b) & 0x1;
		
		/* Invert it and put it in vector to scan. */
		scan_local[lid] = (uint) !bit;
		
		/* Perform the scan. */
		/// @todo This scan is not very efficient, only half of the threads do any work, and there are bank conflicts
		
		uint offset = 1;
		
		/* Upsweep: build sum in place up the tree. */
		for (uint d = get_local_size(0) >> 1; d > 0; d >>= 1) {
			barrier(CLK_LOCAL_MEM_FENCE);
			if (lid < d) {
				uint ai = offset * (2 * lid + 1) - 1;  
				uint bi = offset * (2 * lid + 2) - 1;  
				scan_local[bi] += scan_local[ai];
			}  
			offset *= 2;  
		}

		barrier(CLK_LOCAL_MEM_FENCE);
		if (lid == 0) { 
			/* Clear the last element. */
			scan_local[get_local_size(0) - 1] = 0; 
		}

		/* Downsweep: traverse down tree and build scan. */
		for (uint d = 1; d < get_local_size(0); d *= 2) {
			offset >>= 1;
			barrier(CLK_LOCAL_MEM_FENCE);
			if (lid < d) {
				uint ai = offset * (2 * lid + 1) - 1;
				uint bi = offset * (2 * lid + 2) - 1;
				uint t = scan_local[ai];
				scan_local[ai] = scan_local[bi];
				scan_local[bi] += t;
			}  
		}
		barrier(CLK_LOCAL_MEM_FENCE);

		/* Get the total number of 0's */
		if (lid == get_local_size(0) - 1) {
			total_zeros[0] = ((uint) (!bit)) + scan_local[lid];
		}
		
		/* Synchronize work-items. */
		barrier(CLK_LOCAL_MEM_FENCE);

		/* Get output position. */
		uint pos = bit ? (lid - scan_local[lid] + total_zeros[0]) : scan_local[lid];
		
		/* Scatter input using output position. */
		data_local[pos] = value;

		/* Synchronize work-items. */
		barrier(CLK_LOCAL_MEM_FENCE);
	}
	
	/* Store sorted data in global memory. */
	data_global_tmp[gid] = data_local[lid];
	
}

__kernel void satradixHistogram(
	__global CLO_SORT_ELEM_TYPE* data_global_tmp,
	__global uint *offsets,
	__global uint *counters,
	__local uint *offsets_local,
	__local uint *counters_local,
	__local CLO_SORT_KEY_TYPE *digits_local,
	uint start_bit,
	uint array_len) {
		
	uint lid = get_local_id(0);
	uint gid = get_global_id(0);
	uint wgid = get_group_id(0);
	
	/* Get current digit. */
	digits_local[lid] = CLO_SORT_RADIX1 & 
		(CLO_SORT_KEY_GET(data_global_tmp[gid]) >> start_bit);
		
	if (lid < CLO_SORT_RADIX) {
		offsets_local[lid] = UINT_MAX;
	}
	
	/* Synchronize work-items. */
	barrier(CLK_LOCAL_MEM_FENCE);
	
	/* Determine offsets where contiguous regions of same value digits
	 * start. */
	if ((lid > 0) && (lid < array_len)) {
		if (digits_local[lid] != digits_local[lid - 1]) {
			offsets_local[digits_local[lid]] = lid;
		}
	} else {
		offsets_local[digits_local[0]] = 0; 
	}
			
	/* Synchronize work-items. */
	barrier(CLK_LOCAL_MEM_FENCE);
	
	/* Make sure last offset is the array lenght, if not set. */
	if (lid == CLO_SORT_RADIX1) {
		if (offsets_local[lid] == UINT_MAX) {
			offsets_local[lid] = array_len;
		}
	}	
	
	/* Populate uninitialized starting offsets with 0's. */
	if (lid == 0) {
		uint i = 0;
		/// @todo Do this in parallel
		while ((i < CLO_SORT_RADIX) && (offsets_local[i] == UINT_MAX)) {
			offsets_local[i] = 0;
			i++;
		}
	}

	/* Synchronize work-items. */
	barrier(CLK_LOCAL_MEM_FENCE);
	
	/* Fill uninitialized offsets. */
	if ((lid > 0) && /* First thread won't do anything here. */
		(offsets_local[lid] > 0) && /* Current offset must be higher than zero. */
		(offsets_local[lid] != UINT_MAX) && /* Current offset cannot be uninitialized. */
		(offsets_local[lid - 1] == UINT_MAX)) /* Offset bellow current one must be uninitialized. */
	{ 
		/* Current offset will replace uninitialized offsets. */
		uint curr_offset = offsets_local[lid];
		/* Get index of offset bellow current one. */
		uint i = lid - 1;
		/* Initialize uninitialized offsets bellow current one until we 
		 * reach an initialized offset. */
		while ((i > 0) && (offsets_local[i] == UINT_MAX)) {
			offsets_local[i] = curr_offset;
			i--;
		}
	}

	/* Synchronize work-items. */
	barrier(CLK_LOCAL_MEM_FENCE);
	
	/* Determine the histogram proper. */
	if (lid < CLO_SORT_RADIX1) {
		counters_local[lid] = offsets_local[lid + 1] - offsets_local[lid];
	} else if (lid == CLO_SORT_RADIX1) {
		counters_local[lid] = array_len - offsets_local[lid];
	}

	/* Synchronize work-items. */
	barrier(CLK_LOCAL_MEM_FENCE);
	
	/* Store results in global memory. */
	if (lid < CLO_SORT_RADIX) {
		offsets[(CLO_SORT_RADIX * wgid) + lid] = offsets_local[lid];
		counters[(get_num_groups(0) * lid) + wgid] = counters_local[lid];
	}

}

__kernel void satradixScatter(
	__global CLO_SORT_ELEM_TYPE* data_global,
	__global CLO_SORT_ELEM_TYPE* data_global_tmp,
	__global uint *offsets,
	__global uint *counters_sum,
	__local CLO_SORT_ELEM_TYPE* data_local,
	__local uint *offsets_local,
	__local uint *counters_sum_local,
	uint start_bit) {
		
	uint lid = get_local_id(0);
	uint gid = get_global_id(0);
	uint wgid = get_group_id(0);
	
	/* Load data into local memory. */
	data_local[lid] = data_global_tmp[gid];
	if (lid < CLO_SORT_RADIX) {
		offsets_local[lid] = offsets[(CLO_SORT_RADIX * wgid) + lid];
		counters_sum_local[lid] = counters_sum[(get_num_groups(0) * lid) + wgid];
	}
	
	/* Synchronize work-items. */
	barrier(CLK_LOCAL_MEM_FENCE);
	
	/* Get digit. */
	CLO_SORT_KEY_TYPE digit = CLO_SORT_RADIX1 & 
		(CLO_SORT_KEY_GET(data_local[lid]) >> start_bit);			
		
	/* Determine output address. */
	uint out_idx = counters_sum_local[digit] + lid - offsets_local[digit];
	
	/* Scatter! */
	data_global[out_idx] = data_local[lid];
}

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
 * @brief Advanced bitonic sort implementation.
 */

/**
 * @brief This kernel can perform any step of any stage of a bitonic
 * sort.
 * 
 * @param data Array of data to sort.
 * @param stage
 * @param step
 */
__kernel void abitonic_steps_any(
			__global CLO_SORT_ELEM_TYPE *data,
			uint stage,
			uint step)
{
	/* Global id for this work-item. */
	uint gid = get_global_id(0);
	
	/* Determine if ascending or descending */
	bool desc = (bool) (0x1 & (gid >> (stage - 1)));

	/* Determine stride. */
	uint pair_stride = (uint) (1 << (step - 1)); 
	
	/* Block of which this thread is part of. */
	uint block = gid / pair_stride;
	
	/* ID of thread in block. */
	uint bid = gid % pair_stride;

	/* Determine what to compare and possibly swap. */
	uint index1 = block * pair_stride * 2 + bid;
	uint index2 = index1 + pair_stride;
	
	/* Get hashes from global memory. */
	CLO_SORT_ELEM_TYPE data1 = data[index1];
	CLO_SORT_ELEM_TYPE data2 = data[index2];
		
	/* Determine it is required to swap the agents. */
	bool swap = CLO_SORT_COMPARE(data1, data2) ^ desc; 
		
	/* Perform swap if needed */ 
	if (swap) {
		data[index1] = data2; 
		data[index2] = data1; 
	}
		
}

/**
 * @brief This kernel can perform the two last steps of a stage in a
 * bitonic sort.
 * 
 * @param data Array of data to sort.
 * @param stage
 * @param step
 */
__kernel void abitonic_steps_2_1(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local)
{
	
	/* Global and local ids for this work-item. */
	uint gid = get_global_id(0);
	uint lid = get_local_id(0);
	uint local_size = get_local_size(0);
	uint group_id = get_group_id(0);
		
	/* Load data locally */
	data_local[lid] = data_global[group_id * local_size * 2 + lid];
	data_local[local_size + lid] = data_global[local_size * (group_id * 2 + 1) + lid];
	
	barrier(CLK_LOCAL_MEM_FENCE);

	/* Determine if ascending or descending */
	bool desc = (bool) (0x1 & (gid >> (stage - 1)));
	
	/* Swap or not to swap? */
	bool swap;
	
	/* Index of values to possibly swap. */
	uint index1, index2;
	/* Data elements to possibly swap. */
	CLO_SORT_ELEM_TYPE data1, data2;
	
	/* ********** STEP 2 ************** */

	/* Determine what to compare and possibly swap. */
	index1 = (lid / 2) * 4 + (lid % 2);
	index2 = index1 + 2;
		
	/* Get elements from global memory. */
	data1 = data_local[index1];
	data2 = data_local[index2];
		
	/* Determine if it's required to swap the elements. */
	swap = CLO_SORT_COMPARE(data1, data2) ^ desc; 
		
	/* Perform swap if needed */ 
	if (swap) {
		data_local[index1] = data2; 
		data_local[index2] = data1; 
	}
		
	barrier(CLK_LOCAL_MEM_FENCE);
	
	/* ********** STEP 1 ************** */
		
	/* Determine what to compare and possibly swap. */
	index1 = lid * 2;
	index2 = index1 + 1;
		
	/* Get elements from global memory. */
	data1 = data_local[index1];
	data2 = data_local[index2];
		
	/* Determine if it's required to swap the elements. */
	swap = CLO_SORT_COMPARE(data1, data2) ^ desc; 
		
	/* Perform swap if needed */ 
	if (swap) {
		data_local[index1] = data2; 
		data_local[index2] = data1; 
	}

	barrier(CLK_LOCAL_MEM_FENCE);

	/* Store data globally */
	data_global[group_id * local_size * 2 + lid] = data_local[lid];
	data_global[local_size * (group_id * 2 + 1) + lid] = data_local[local_size + lid];
		
}

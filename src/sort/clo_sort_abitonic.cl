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

//~ /**
 //~ * @brief A advanced bitonic sort kernel.
 //~ * 
 //~ * @param data Array of agent data.
 //~ * @param stride
 //~ * @param alternate
 //~ */
//~ __kernel void abitonic_B2(
			//~ __global CLO_SORT_ELEM_TYPE *data,
			//~ const uint stride,
			//~ const uint alternate)
//~ {
	//~ /* Global id for this work-item. */
	//~ uint gid = get_global_id(0);
	//~ 
	//~ /* Block of which this thread is part of. */
	//~ uint block = gid / stride;
	//~ 
	//~ /* ID of thread in block. */
	//~ uint bid = gid % stride;
//~ 
	//~ /* Determine what to compare and possibly swap. */
	//~ uint index1 = block * stride * 2 + bid;
	//~ uint index2 = index1 + stride;
	//~ 
	//~ /* Get hashes from global memory. */
	//~ CLO_SORT_ELEM_TYPE data1 = data[index1];
	//~ CLO_SORT_ELEM_TYPE data2 = data[index2];
	//~ 
	//~ /* Determine if ascending or descending */
	//~ bool desc = (bool) (alternate) ? (0x1 & (block % 2) : 0;
	//~ 
	//~ /* Determine it is required to swap the agents. */
	//~ bool swap = CLO_SORT_COMPARE(data1, data2) ^ desc; 
	//~ 
	//~ /* Perform swap if needed */ 
	//~ if (swap) {
		//~ data[index1] = data2; 
		//~ data[index2] = data1; 
	//~ }
//~ 
//~ 
//~ }

__kernel void abitonicSort(
			__global CLO_SORT_ELEM_TYPE *data,
			uint stage,
			uint start_step,
			uint end_step)
{
	/* Global id for this work-item. */
	uint gid = get_global_id(0);
	
	/* Determine if ascending or descending */
	bool desc = (bool) (0x1 & (gid >> (stage - 1)));

	for (uint step = start_step; step >= end_step; step--) { 
		
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
		
		barrier(CLK_GLOBAL_MEM_FENCE);
	}
}

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
 * @brief Global memory selection sort implementation.
 */

/**
 * @brief A global memory selection sort kernel.
 * 
 * @param data_in Array of unsorted elements.
 * @param data_out Array of sorted elements.
 */
__kernel void gselectSort(
			__global CLO_SORT_ELEM_TYPE *data_in,
			__global CLO_SORT_ELEM_TYPE *data_out,
			uint size)
{
	/* Global id for this work-item. */
	size_t gid = get_global_id(0);
	
	size_t pos = 0;
	uint count = 0;
	CLO_SORT_ELEM_TYPE data2 = data_in[gid];
	
	if (gid < size) {
		for (uint i = 0; i < size; i++) {
			CLO_SORT_ELEM_TYPE data1 = data_in[i];
			if (CLO_SORT_COMPARE(data1, data2) || ((data1 == data2) && (i < gid))) {
				pos++;
			}
		}
		data_out[pos] = data2;
	}
}

__kernel void gselectCopy(
			__global CLO_SORT_ELEM_TYPE *data_in,
			__global CLO_SORT_ELEM_TYPE *data_out,
			uint size)
{
	size_t gid = get_global_id(0);
	if (gid < size) {
		data_in[gid] = data_out[gid];
	}
}
	

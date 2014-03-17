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
 * @brief OpenCL kernels for a radix sort implementation based on 
 * Satish, N., Harris, M., and Garland, M. "Designing Efficient Sorting 
 * Algorithms for Manycore GPUs". In Proceedings of IEEE International 
 * Parallel & Distributed Processing Symposium 2009 (IPDPS 2009).
 */

/**
 * @brief 
 * 
 * @param data Array of elements to sort.
 * @param stage Current bitonic sort step.
 * @param step Current bitonic sort stage.
 */
__kernel void srsLocalSort(
			__global CLO_SORT_ELEM_TYPE *data,
			__local CLO_SORT_ELEM_TYPE *local_data,
			const uint start_bit,
			const uint b)
{
	/* Global and local ids for this work-item. */
	uint gid = get_global_id(0);
	uint lid = get_local_id(0);
	
	



}

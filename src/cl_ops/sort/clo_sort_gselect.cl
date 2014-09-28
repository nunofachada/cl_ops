/*
 * This file is part of CL-Ops.
 *
 * CL-Ops is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * CL-Ops is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with CL-Ops. If not, see
 * <http://www.gnu.org/licenses/>.
 * */

/**
 * @file
 * @brief Global memory selection sort implementation.
 */

#ifndef CLO_SORT_ELEM_TYPE
	#define CLO_SORT_ELEM_TYPE uint
#endif

#ifndef CLO_SORT_COMPARE
	#define CLO_SORT_COMPARE(a, b) ((a) > (b))
#endif

#ifndef CLO_SORT_KEY_GET
	#define CLO_SORT_KEY_GET(x) (x)
#endif

#ifndef CLO_SORT_KEY_TYPE
	#define CLO_SORT_KEY_TYPE uint
#endif

/**
 * @brief A global memory selection sort kernel.
 *
 * @param[in] data_in Array of unsorted elements.
 * @param[out] data_out Array of sorted elements.
 * @param[in] size Number of elements to sort.
 */
__kernel void clo_sort_gselect(__global CLO_SORT_ELEM_TYPE *data_in,
	__global CLO_SORT_ELEM_TYPE *data_out, ulong size) {

	/* Global id for this work-item. */
	size_t gid = get_global_id(0);

	size_t pos = 0;
	uint count = 0;
	CLO_SORT_ELEM_TYPE data_gid = data_in[gid];

	if (gid < size) {
		for (uint i = 0; i < size; i++) {
			CLO_SORT_ELEM_TYPE data_i = data_in[i];
			if (CLO_SORT_COMPARE(data_gid, data_i) || ((data_i == data_gid) && (i < gid))) {
				pos++;
			}
		}
		data_out[pos] = data_gid;
	}
}




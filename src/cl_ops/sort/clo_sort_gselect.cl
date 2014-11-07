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
 * Global memory selection sort implementation.
 *
 * Requires definition of:
 *
 * * CLO_SORT_ELEM_TYPE - Type of element to sort
 * * CLO_SORT_COMPARE(a,b) - Compare macro or function
 * * CLO_SORT_KEY_GET(x) - Get key macro or function
 * * CLO_SORT_KEY_TYPE - Type of key
 */

/**
 * A global memory selection sort kernel.
 *
 * @param[in] data_in Array of unsorted elements.
 * @param[out] data_out Array of sorted elements.
 * @param[in] size Number of elements to sort.
 */
__kernel void gselect(__global CLO_SORT_ELEM_TYPE *data_in,
	__global CLO_SORT_ELEM_TYPE *data_out, ulong size) {

	/* Global id for this work-item. */
	size_t gid = get_global_id(0);

	size_t pos = 0;
	CLO_SORT_ELEM_TYPE data_gid = data_in[gid];
	CLO_SORT_KEY_TYPE key_gid = CLO_SORT_KEY_GET(data_gid);

	if (gid < size) {
		for (uint i = 0; i < size; i++) {
			CLO_SORT_KEY_TYPE key_i = CLO_SORT_KEY_GET(data_in[i]);
			if (CLO_SORT_COMPARE(key_gid, key_i) || ((key_i == key_gid) && (i < gid))) {
				pos++;
			}
		}
		data_out[pos] = data_gid;
	}
}




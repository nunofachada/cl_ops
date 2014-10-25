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
 * Simple bitonic sort implementation.
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
 * A simple bitonic sort kernel.
 *
 * @param[in,out] data Array of elements to sort.
 * @param[in] stage Current bitonic sort step.
 * @param[in] step Current bitonic sort stage.
 */
__kernel void clo_sort_sbitonic(__global CLO_SORT_ELEM_TYPE *data,
	const uint stage, const uint step) {

	/* Global id for this work-item. */
	uint gid = get_global_id(0);

	/* Determine what to compare and possibly swap. */
	uint pair_stride = (uint) (1 << (step - 1));
	uint index1 = gid + (gid / pair_stride) * pair_stride;
	uint index2 = index1 + pair_stride;

	/* Get values from global memory. */
	CLO_SORT_ELEM_TYPE data1 = data[index1];
	CLO_SORT_ELEM_TYPE data2 = data[index2];

	/* Determine keys. */
	CLO_SORT_KEY_TYPE key1 = CLO_SORT_KEY_GET(data1);
	CLO_SORT_KEY_TYPE key2 = CLO_SORT_KEY_GET(data2);

	/* Determine if ascending or descending */
	bool desc = (bool) (0x1 & (gid >> (stage - 1)));

	/* Determine it is required to swap the agents. */
	bool swap = CLO_SORT_COMPARE(key1, key2) ^ desc;

	/* Perform swap if needed */
	if (swap) {
		data[index1] = data2;
		data[index2] = data1;
	}

}


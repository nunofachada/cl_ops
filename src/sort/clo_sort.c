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
 * @brief Sorting algorithms host implementation.
 */

#include "clo_sort.h"
#include "clo_sort_sbitonic.h"

/** Available sorting algorithms and respective properties. */
CloSortInfo sort_infos[] = {
	{"s-bitonic", "CLO_SORT_SBITONIC", 1, 
		clo_sort_sbitonic_sort, 
		clo_sort_sbitonic_kernels_create, clo_sort_sbitonic_kernelargs_set, 
		clo_sort_sbitonic_localmem_usage, clo_sort_sbitonic_kernels_free, 
		clo_sort_sbitonic_events_create, clo_sort_sbitonic_events_free, 
		clo_sort_sbitonic_events_profile}, 
	{NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};


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
 * @brief Sorting algorithms host implementation.
 */

#include "clo_sort.h"
#include "clo_sort_sbitonic.h"
#include "clo_sort_abitonic.h"

/** Available sorting algorithms and respective properties. */
CloSortInfo sort_infos[] = {
	{"s-bitonic", "CLO_SORT_SBITONIC", 
		CLO_SORT_SBITONIC_NUMQUEUES,
		CLO_SORT_SBITONIC_NUMKERNELS, 
		clo_sort_sbitonic_sort, clo_sort_sbitonic_kernelname_get,
		clo_sort_sbitonic_kernels_create, clo_sort_sbitonic_localmem_usage,
		clo_sort_sbitonic_kernelargs_set, clo_sort_sbitonic_kernels_free}, 
	{"a-bitonic", "CLO_SORT_ABITONIC", 
		CLO_SORT_ABITONIC_NUMQUEUES,
		CLO_SORT_ABITONIC_NUMKERNELS, 
		clo_sort_abitonic_sort, clo_sort_abitonic_kernelname_get,
		clo_sort_abitonic_kernels_create, clo_sort_abitonic_localmem_usage,
		clo_sort_abitonic_kernelargs_set, clo_sort_abitonic_kernels_free}, 
	{"select", "CLO_SORT_GSELECT", 
		CLO_SORT_GSELECT_NUMQUEUES,
		CLO_SORT_GSELECT_NUMKERNELS, 
		clo_sort_gselect_sort, clo_sort_gselect_kernelname_get,
		clo_sort_gselect_kernels_create, clo_sort_gselect_localmem_usage,
		clo_sort_gselect_kernelargs_set, clo_sort_gselect_kernels_free}, 
	{NULL, NULL, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL}
};


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
 * @brief Global memory selection sort header file.
 */

#ifndef CLO_SORTGSELECT_H
#define CLO_SORTGSELECT_H

#include "clo_sort.h"

/** @brief Name of single selection sort kernel. */
#define CLO_SORT_GSELECT_KNAME_0 "gselectSort"
#define CLO_SORT_GSELECT_KNAME_1 "gselectCopy"
 
/** @brief Number of command queues used by the selection sort. */
#define CLO_SORT_GSELECT_NUMQUEUES 1

/** @brief Number of kernels used by the selection sort. */
#define CLO_SORT_GSELECT_NUMKERNELS 2

/** @brief Array of strings containing names of the kernels used by the selection sort strategy. */
#define CLO_SORT_GSELECT_KERNELNAMES {CLO_SORT_GSELECT_KNAME_0, CLO_SORT_GSELECT_KNAME_1}

/** @brief Sort elements using the selection sort. */
int clo_sort_gselect_sort(cl_command_queue *queues, cl_kernel *krnls, 
	size_t lws_max, size_t len, unsigned int numel, const char* options, 
	GArray *evts, gboolean profile, GError **err);

/** @brief Returns the name of the kernel identified by the given
 * index. */
const char* clo_sort_gselect_kernelname_get(unsigned int index);

/** @brief Create kernels for the selection sort. */
int clo_sort_gselect_kernels_create(cl_kernel **krnls, cl_program program, GError **err);

/** @brief Get local memory usage for the selection sort kernels. */
size_t clo_sort_gselect_localmem_usage(const char* kernel_name, size_t lws_max, size_t len, unsigned int numel);

/** @brief Set kernels arguments for the selection sort. */
int clo_sort_gselect_kernelargs_set(cl_kernel **krnls, cl_mem data, size_t lws, size_t len, GError **err);

/** @brief Free the selection sort kernels. */
void clo_sort_gselect_kernels_free(cl_kernel **krnls);

#endif

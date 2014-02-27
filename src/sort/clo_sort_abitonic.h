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
 * @brief Advanced bitonic sort header file.
 */

#ifndef CLO_SORTABITONIC_H
#define CLO_SORTABITONIC_H

#include "clo_sort.h"

/**
 * @defgroup CLO_SORT_ABITONIC_K Id/index of the advanced bitonic sort kernels.
 *
 * @{
 */
#define CLO_SORT_ABITONIC_K_ANY 0
#define CLO_SORT_ABITONIC_K_2 1
#define CLO_SORT_ABITONIC_K_3 2
#define CLO_SORT_ABITONIC_K_4 3
#define CLO_SORT_ABITONIC_K_5 4
#define CLO_SORT_ABITONIC_K_6 5
#define CLO_SORT_ABITONIC_K_7 6
#define CLO_SORT_ABITONIC_K_8 7
#define CLO_SORT_ABITONIC_K_9 8
#define CLO_SORT_ABITONIC_K_10 9
#define CLO_SORT_ABITONIC_K_11 10
#define CLO_SORT_ABITONIC_K_S8 11
#define CLO_SORT_ABITONIC_K_S16 12
#define CLO_SORT_ABITONIC_K_4_S4 13
#define CLO_SORT_ABITONIC_K_4_S6 14
#define CLO_SORT_ABITONIC_K_4_S8 15
#define CLO_SORT_ABITONIC_K_4_S10 16
#define CLO_SORT_ABITONIC_K_4_S12 17
/** @} */ 

/**
 * @defgroup CLO_SORT_SBITONIC_KERNELNAME Name of the advanced bitonic sort kernels.
 *
 * @{
 */
#define CLO_SORT_SBITONIC_KERNELNAME_ANY "abitonic_any"
#define CLO_SORT_SBITONIC_KERNELNAME_2 "abitonic_2"
#define CLO_SORT_SBITONIC_KERNELNAME_3 "abitonic_3"
#define CLO_SORT_SBITONIC_KERNELNAME_4 "abitonic_4"
#define CLO_SORT_SBITONIC_KERNELNAME_5 "abitonic_5"
#define CLO_SORT_SBITONIC_KERNELNAME_6 "abitonic_6"
#define CLO_SORT_SBITONIC_KERNELNAME_7 "abitonic_7"
#define CLO_SORT_SBITONIC_KERNELNAME_8 "abitonic_8"
#define CLO_SORT_SBITONIC_KERNELNAME_9 "abitonic_9"
#define CLO_SORT_SBITONIC_KERNELNAME_10 "abitonic_10"
#define CLO_SORT_SBITONIC_KERNELNAME_11 "abitonic_11"
#define CLO_SORT_SBITONIC_KERNELNAME_S8 "abitonic_s8"
#define CLO_SORT_SBITONIC_KERNELNAME_S16 "abitonic_s16"
#define CLO_SORT_SBITONIC_KERNELNAME_4_S4 "abitonic_4_s4"
#define CLO_SORT_SBITONIC_KERNELNAME_4_S6 "abitonic_4_s6"
#define CLO_SORT_SBITONIC_KERNELNAME_4_S8 "abitonic_4_s8"
#define CLO_SORT_SBITONIC_KERNELNAME_4_S10 "abitonic_4_s10"
#define CLO_SORT_SBITONIC_KERNELNAME_4_S12 "abitonic_4_s12"
/** @} */ 
 
/** @brief Number of command queues used by the advanced bitonic sort. */
#define CLO_SORT_ABITONIC_NUMQUEUES 1

/** @brief Number of kernels used by the advanced bitonic sort. */
#define CLO_SORT_ABITONIC_NUMKERNELS 18

/** @brief Array of strings containing names of the kernels used by the advanced bitonic sort strategy. */
#define CLO_SORT_ABITONIC_KERNELNAMES {CLO_SORT_SBITONIC_KERNELNAME_ANY, \
	CLO_SORT_SBITONIC_KERNELNAME_2, CLO_SORT_SBITONIC_KERNELNAME_3, \
	CLO_SORT_SBITONIC_KERNELNAME_4, CLO_SORT_SBITONIC_KERNELNAME_5, \
	CLO_SORT_SBITONIC_KERNELNAME_6, CLO_SORT_SBITONIC_KERNELNAME_7, \
	CLO_SORT_SBITONIC_KERNELNAME_8, CLO_SORT_SBITONIC_KERNELNAME_9, \
	CLO_SORT_SBITONIC_KERNELNAME_10, CLO_SORT_SBITONIC_KERNELNAME_11, \
	CLO_SORT_SBITONIC_KERNELNAME_S8, CLO_SORT_SBITONIC_KERNELNAME_S16, \
	CLO_SORT_SBITONIC_KERNELNAME_4_S4, CLO_SORT_SBITONIC_KERNELNAME_4_S6, \
	CLO_SORT_SBITONIC_KERNELNAME_4_S8, CLO_SORT_SBITONIC_KERNELNAME_4_S10, \
	CLO_SORT_SBITONIC_KERNELNAME_4_S12}
	
/** @brief Sort agents using the advanced bitonic sort. */
int clo_sort_abitonic_sort(cl_command_queue *queues, cl_kernel *krnls, cl_event **evts, size_t lws_max, unsigned int numel, gboolean profile, GError **err);

/** @brief Returns the name of the kernel identified by the given
 * index. */
const char* clo_sort_abitonic_kernelname_get(unsigned int index);

/** @brief Create kernels for the advanced bitonic sort. */
int clo_sort_abitonic_kernels_create(cl_kernel **krnls, cl_program program, GError **err);

/** @brief Get local memory usage for the advanced bitonic sort kernels. */
size_t clo_sort_abitonic_localmem_usage(const char* kernel_name, size_t lws_max, size_t len, unsigned int numel);

/** @brief Set kernels arguments for the advanced bitonic sort. */
int clo_sort_abitonic_kernelargs_set(cl_kernel **krnls, cl_mem data, size_t lws, size_t len, GError **err);

/** @brief Free the advanced bitonic sort kernels. */
void clo_sort_abitonic_kernels_free(cl_kernel **krnls);

/** @brief Create events for the advanced bitonic sort kernels. */
int clo_sort_abitonic_events_create(cl_event ***evts, unsigned int iters, size_t numel, size_t lws_max, GError **err);

/** @brief Free the advanced bitonic sort events. */
void clo_sort_abitonic_events_free(cl_event ***evts);

/** @brief Add bitonic sort events to the profiler object. */
int clo_sort_abitonic_events_profile(cl_event **evts, ProfCLProfile *profile, GError **err);

#endif


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

#define CLO_SORT_ABITONIC_NUMKRNLS 2

#define CLO_SORT_ABITONIC_K_ANY 0
#define CLO_SORT_ABITONIC_K_2_1 1

/** @brief Sort agents using the advanced bitonic sort. */
int clo_sort_abitonic_sort(cl_command_queue *queues, cl_kernel *krnls, cl_event **evts, size_t lws_max, unsigned int numel, gboolean profile, GError **err);

/** @brief Create kernels for the advanced bitonic sort. */
int clo_sort_abitonic_kernels_create(cl_kernel **krnls, cl_program program, GError **err);

/** @brief Get local memory usage for the advanced bitonic sort kernels. */
size_t clo_sort_abitonic_localmem_usage(gchar* kernel_name, size_t lws_max, size_t len, unsigned int numel);

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


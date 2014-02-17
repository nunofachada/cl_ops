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
 * @brief Ocl-Ops sorting algorithms header file.
 */

#ifndef CLO_SORT_H
#define CLO_SORT_H

#include "clo_common.h"

/**
 * @brief Available sorting algorithms.
 * */
#define CLO_SORT_ALGS "s-bitonic"

/**
 * @brief Default sorting algorithm.
 * */
#define CLO_DEFAULT_SORT "s-bitonic"

/**
 * @brief Sort array.
 * 
 * @param queues Available command queues.
 * @param krnls Sort kernels.
 * @param evts Associated events.
 * @param lws_max Maximum local worksize.
 * @param numel Number of elements to sort.
 * @param profile TRUE if profiling is to be performed, FALSE otherwise.
 * @param err GError error reporting object.
 * @return @link clo_error_codes::CLO_SUCCESS @endlink if function 
 * terminates successfully, or an error code otherwise.
 */
typedef int (*clo_sort_sort)(cl_command_queue *queues, cl_kernel *krnls, cl_event **evts, size_t lws_max, unsigned int numel, gboolean profile, GError **err);

/**
 * @brief Create sort kernels.
 * 
 * @param krnls Sort kernels.
 * @param program OpenCL program from where kernels are created.
 * @param err GError error reporting object.
 * @return @link clo_error_codes::CLO_SUCCESS @endlink if function 
 * terminates successfully, or an error code otherwise.
 */
typedef int (*clo_sort_kernels_create)(cl_kernel **krnls, cl_program program, GError **err);

/** 
 * @brief Get local memory usage for the specified kernel. 
 * 
 * @param kernel_name Name of kernel to get the local memory usage from.
 * @param lws_max Maximum local worksize for the specified kernel.
 * @param numel Number of elements to sort.
 * @return Local memory usage (in bytes) for the specified kernel.
 * */
typedef size_t (*clo_sort_localmem_usage)(gchar* kernel_name, size_t lws_max, unsigned int numel);

/**
 * @brief Set sort kernels arguments.
 * 
 * @param krnls Sort kernels.
 * @param buffersDevice Device buffers (agents, cells, etc.).
 * @param err GError error reporting object.
 * @param lws Local work size.
 * @param len Element width, 1, 2, 4 or 8 bytes.
 * @return @link clo_error_codes::CLO_SUCCESS @endlink if function 
 * terminates successfully, or an error code otherwise.
 */
typedef int (*clo_sort_kernelargs_set)(cl_kernel **krnls, cl_mem data, size_t lws, size_t len, GError **err);

/**
 * @brief Free sort kernels.
 * 
 * @param krnls Sort kernels.
 */
typedef void (*clo_sort_kernels_free)(cl_kernel **krnls);

/**
 * @brief Create sort events.
 * 
 * @param evts Sort events.
 * @param iters Number of iterations.
 * @param err GError error reporting object.
 * @param max_agents Maximum number of agents for this simulation.
 * @param lws_max Maximum local worksize for sort kernels.
 * @return @link clo_error_codes::CLO_SUCCESS @endlink if function 
 * terminates successfully, or an error code otherwise.
 */
typedef int (*clo_sort_events_create)(cl_event ***evts, unsigned int iters, size_t numel, size_t lws_max, GError **err);

/**
 * @brief Free sort events.
 * 
 * @param evts Sort events.
 */
typedef void (*clo_sort_events_free)(cl_event ***evts);

/**
 * @brief Add sort events to profiler object.
 * 
 * @param evts Sort events.
 * @param profile Profiler object.
 * @param err GError error reporting object.
 * @return @link clo_error_codes::CLO_SUCCESS @endlink if function 
 * terminates successfully, or an error code otherwise.
 */
typedef int (*clo_sort_events_profile)(cl_event **evts, ProfCLProfile *profile, GError **err);

/**
 * @brief Object which represents an agent sorting algorithm.
 * */	
typedef struct clo_sort_info {
	char* tag;            /**< Tag identifying the algorithm. */
	char* compiler_const; /**< OpenCL compiler constant to include the proper CL file. */
	unsigned int num_queues;  /**< Number of OpenCL command queues required for the algorithm. */
	clo_sort_sort sort;   /**< The sorting function. */
	clo_sort_kernels_create kernels_create; /**< Create sort kernels function. */
	clo_sort_localmem_usage localmem_usage; /**< Function which returns the local memory usage for the specified kernel. */
	clo_sort_kernelargs_set kernelargs_set; /**< Set sort kernels arguments function. */
	clo_sort_kernels_free kernels_free;     /**< Free sort kernels function. */
	clo_sort_events_create events_create;   /**< Create sort events function.*/
	clo_sort_events_free events_free;       /**< Free sort events function. */
	clo_sort_events_profile events_profile; /**< Add sort events to profiler object function. */
} CloSortInfo;

/** @brief Information about the available agent sorting algorithms. */
extern CloSortInfo sort_infos[];

#endif

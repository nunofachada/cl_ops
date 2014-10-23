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
 * @brief Abstract declarations for a sort algorithm.
 * */

#ifndef _CLO_SORT_ABSTRACT_H_
#define _CLO_SORT_ABSTRACT_H_

#include "common/clo_common.h"

/* Sort class. */
typedef struct clo_sort CloSort;

/**
 * Definition of a sort implementation.
 * */
typedef struct clo_sort_impl_def {

	/**
	 * Sort algorithm name.
	 * */
	const char* name;

	/**
	 * Sort algorithm initializer function.
	 *
	 * @param[in] sorter
	 * @param[in] options
	 * @param[out] err
	 * @return Sort algorithm source code.
	 * */
	const char* (*init)(CloSort* sorter, const char* options,
		GError** err);

	/**
	 * Sort algorithm finalizer function.
	 *
	 * @param[in] sorter
	 * */
	void (*finalize)(CloSort* sorter);

	/**
	 * Perform sort using device data.
	 *
	 * @param[in] sorter Sorter object.
	 * @param[in] cq_exec A valid command queue wrapper for kernel
	 * execution, cannot be `NULL`.
	 * @param[in] cq_comm A command queue wrapper for data transfers.
	 * If `NULL`, `cq_exec` will be used for data transfers.
	 * @param[in] data_in Data to be sorted.
	 * @param[out] data_out Location where to place sorted data. If
	 * `NULL`, data will be sorted in-place or copied back from auxiliar
	 * device buffer, depending on the sort implementation.
	 * @param[in] numel Number of elements in `data_in`.
	 * @param[in] lws_max Max. local worksize. If 0, the local worksize
	 * will be automatically determined.
	 * @param[out] err Return location for a GError, or `NULL` if error
	 * reporting is to be ignored.
	 * @return An event wait list which contains events which must
	 * terminate before sorting is considered complete.
	 * */
	CCLEventWaitList (*sort_with_device_data)(CloSort* sorter,
		CCLQueue* cq_exec, CCLQueue* cq_comm, CCLBuffer* data_in,
		CCLBuffer* data_out, size_t numel, size_t lws_max,
		GError** err);

	/**
	 * Perform sort using host data. Device buffers will be created and
	 * destroyed by sort implementation.
	 *
	 * @param[in] sorter Sorter object.
	 * @param[in] cq_exec Command queue wrapper for kernel execution. If
	 * `NULL` a queue will be created.
	 * @param[in] cq_comm A command queue wrapper for data transfers.
	 * If `NULL`, `cq_exec` will be used for data transfers.
	 * @param[in] data_in Data to be sorted.
	 * @param[out] data_out Location where to place sorted data.
	 * @param[in] numel Number of elements in `data_in`.
	 * @param[in] lws_max Max. local worksize. If 0, the local worksize
	 * will be automatically determined.
	 * @param[out] err Return location for a GError, or `NULL` if error
	 * reporting is to be ignored.
	 * @return `CL_TRUE` if sort was successfully performed, or
	 * `CL_FALSE` otherwise.
	 * */
	cl_bool (*sort_with_host_data)(CloSort* sorter, CCLQueue* cq_exec,
		CCLQueue* cq_comm, void* data_in, void* data_out, size_t numel,
		size_t lws_max, GError** err);

} CloSortImplDef;

/* Generic sorter object constructor. The exact type is given in the
 * first parameter. */
CloSort* clo_sort_new(const char* type, const char* options,
	CCLContext* ctx, CloType* elem_type, CloType* key_type,
	const char* compare, const char* get_key, const char* compiler_opts,
	GError** err);

/* Destroy a sorter object. */
void clo_sort_destroy(CloSort* sorter);

/* Perform sort using device data. */
CCLEventWaitList clo_sort_with_device_data(CloSort* sorter,
	CCLQueue* cq_exec, CCLQueue* cq_comm, CCLBuffer* data_in,
	CCLBuffer* data_out, size_t numel, size_t lws_max, GError** err);

/* Perform sort using host data. Device buffers will be created and
 * destroyed by sort implementation. */
cl_bool clo_sort_with_host_data(CloSort* sorter, CCLQueue* cq_exec,
	CCLQueue* cq_comm, void* data_in, void* data_out, size_t numel,
	size_t lws_max, GError** err);

/* Get the context wrapper associated with the given sorter object. */
CCLContext* clo_sort_get_context(CloSort* sorter);

/* Get the program wrapper associated with the given sorter object. */
CCLProgram* clo_sort_get_program(CloSort* sorter);

/* Get the element type associated with the given sorter object. */
CloType clo_sort_get_element_type(CloSort* sorter);

/* Get the size in bytes of each element to be sorted. */
size_t clo_sort_get_element_size(CloSort* sorter);

/* Get sort specific data. */
void* clo_sort_get_data(CloSort* sorter);

/* Set sort specific data. */
void clo_sort_set_data(CloSort* sorter, void* data);

#endif




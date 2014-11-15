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
 * Abstract declarations for a sort algorithm.
 * */

#ifndef _CLO_SORT_ABSTRACT_H_
#define _CLO_SORT_ABSTRACT_H_

#include "cl_ops/clo_common.h"

/* Available sort algoritms. */
#define CLO_SORT_IMPLS "sbitonic, abitonic, gselect, satradix"

/**
 * @defgroup CLO_SORT Sorting algorithms
 *
 * This module provides several sorting algorithms.
 *
 * @{
 */

/**
 * Definition of a sort implementation.
 * */
typedef struct clo_sort_impl_def {

	/**
	 * Sort algorithm name.
	 * */
	const char* name;

	/**
	 * Does the algorithm sort values in-place?
	 * */
	cl_bool in_place;

	/**
	 * Sort algorithm initializer function.
	 *
	 * @param[in] sorter Sorter object.
	 * @param[in] options Algorithm options.
	 * @param[out] err Return location for a GError, or `NULL` if error
	 * reporting is to be ignored.
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
	 * @copydetails clo_sort::clo_sort_with_device_data()
	 * */
	CCLEvent* (*sort_with_device_data)(CloSort* sorter,
		CCLQueue* cq_exec, CCLQueue* cq_comm, CCLBuffer* data_in,
		CCLBuffer* data_out, size_t numel, size_t lws_max,
		GError** err);

	/**
	 * Get the maximum number of kernels used by the sort
	 * implementation.
	 *
	 * @copydetails clo_sort::clo_sort_get_num_kernels()
	 * */
	cl_uint (*get_num_kernels)(CloSort* sorter, GError** err);

	/**
	 * Get name of the i^th kernel used by the sort implementation.
	 *
	 * @copydetails clo_sort::clo_sort_get_kernel_name()
	 * */
	const char* (*get_kernel_name)(CloSort* sorter, cl_uint i,
		GError** err);

	/**
	 * Get local memory usage of i^th kernel used by the sort
	 * implementation for the given maximum local worksize and number
	 * of elements to sort.
	 *
	 * @copydetails clo_sort::clo_sort_get_localmem_usage()
	 * */
	size_t (*get_localmem_usage)(CloSort* sorter, cl_uint i,
		size_t lws_max, size_t numel, GError** err);

} CloSortImplDef;

/** @} */

/* Generic sorter object constructor. The exact type is given in the
 * first parameter. */
CloSort* clo_sort_new(const char* type, const char* options,
	CCLContext* ctx, CloType* elem_type, CloType* key_type,
	const char* compare, const char* get_key, const char* compiler_opts,
	GError** err);

/* Destroy a sorter object. */
void clo_sort_destroy(CloSort* sorter);

/* Perform sort using device data. */
CCLEvent* clo_sort_with_device_data(CloSort* sorter, CCLQueue* cq_exec,
	CCLQueue* cq_comm, CCLBuffer* data_in, CCLBuffer* data_out,
	size_t numel, size_t lws_max, GError** err);

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

/* Get the key type associated with the given sorter object. */
CloType clo_sort_get_key_type(CloSort* sorter);

/* Get the size in bytes of each key to be sorted. */
size_t clo_sort_get_key_size(CloSort* sorter);

/* Get sort specific data. */
void* clo_sort_get_data(CloSort* sorter);

/* Set sort specific data. */
void clo_sort_set_data(CloSort* sorter, void* data);

/* Get the maximum number of kernels used by the sort implementation. */
cl_uint clo_sort_get_num_kernels(CloSort* sorter, GError** err);

/* Get name of the i^th kernel used by the sort implementation. */
const char* clo_sort_get_kernel_name(
	CloSort* sorter, cl_uint i, GError** err);

/* Get local memory usage of i^th kernel used by the sort implementation
 * for the given maximum local worksize and number of elements to
 * sort. */
size_t clo_sort_get_localmem_usage(CloSort* sorter, cl_uint i,
	size_t lws_max, size_t numel, GError** err);

#endif

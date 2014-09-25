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

/**
 * Abstract sort class.
 * */
typedef struct clo_sort {

	/**
	 * Perform sort using device data.
	 *
	 * @param[in] sorter Sorter object.
	 * @param[in] queue A valid command queue wrapper, cannot be `NULL`.
	 * @param[in] data_in Data to be sorted.
	 * @param[out] data_out Location where to place sorted data. If
	 * `NULL`, data will be sorted in-place or copied back from auxiliar
	 * device buffer, depending on the sort implementation.
	 * @param[in] numel Number of elements in `data_in`.
	 * @param[in] lws_max Max. local worksize. If 0, the local worksize
	 * will be automatically determined.
	 * @param[out] duration Location where to place the duration in
	 * seconds of the sort. If `NULL` it will be ignored.
	 * @param[out] err Return location for a GError, or `NULL` if error
	 * reporting is to be ignored.
	 * @return `CL_TRUE` if sort was successfully performed, or
	 * `CL_FALSE` otherwise.
	 * */
	cl_bool (*sort_with_device_data)(struct clo_sort* sorter,
		CCLQueue* queue, CCLBuffer* data_in, CCLBuffer* data_out,
		size_t numel, size_t lws_max, double* duration, GError** err);

	/**
	 * Perform sort using host data. Device buffers will be created and
	 * destroyed by sort implementation.
	 *
	 * @param[in] sorter Sorter object.
	 * @param[in] queue Command queue wrapper. If `NULL` a queue will
	 * be created for the sort.
	 * @param[in] data_in Data to be sorted.
	 * @param[out] data_out Location where to place sorted data.
	 * @param[in] numel Number of elements in `data_in`.
	 * @param[in] lws_max Max. local worksize. If 0, the local worksize
	 * will be automatically determined.
	 * @param[out] duration Location where to place the duration in
	 * seconds of the sort. If `NULL` it will be ignored.
	 * @param[out] err Return location for a GError, or `NULL` if error
	 * reporting is to be ignored.
	 * @return `CL_TRUE` if sort was successfully performed, or
	 * `CL_FALSE` otherwise.
	 * */
	cl_bool (*sort_with_host_data)(struct clo_sort* sorter,
		CCLQueue* queue, void* data_in, void* data_out, size_t numel,
		size_t lws_max, double* duration, GError** err);

	/**
	 * Destroy sorter object.
	 *
	 * @param[in] sorter Sorter object to destroy.
	 * */
	void (*destroy)(struct clo_sort* sorter);

	/**
	 * @internal
	 * Private sorter data.
	 * */
	void* _data;

} CloSort;

/* Generic sorter object constructor. The exact type is given in the
 * first parameter. */
CloSort* clo_sort_new(const char* type, const char* options,
	CCLContext* ctx, CloType elem_type, const char* compiler_opts,
	GError** err);

#endif




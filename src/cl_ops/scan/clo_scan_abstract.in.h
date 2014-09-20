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
 * @brief Parallel prefix sum (scan) abstract declarations.
 * */

#ifndef _CLO_SCAN_ABSTRACT_H_
#define _CLO_SCAN_ABSTRACT_H_

#include "common/clo_common.h"

/**
 * Abstract scan class.
 * */
typedef struct clo_scan {

	/**
	 * Perform scan using device data.
	 *
	 * @param[in] scanner Scan object.
	 * @param[in] queue A valid command queue wrapper, cannot be `NULL`.
	 * @param[in] data_in Data to be scanned.
	 * @param[out] data_out Location where to place scanned data.
	 * @param[in] numel Number of elements in `data_in`.
	 * @param[in] lws_max Max. local worksize. If 0, the local worksize
	 * will be automatically determined.
	 * @param[out] duration Location where to place the duration of the
	 * scan. If `NULL` it will be ignored.
	 * @param[out] err Return location for a GError, or `NULL` if error
	 * reporting is to be ignored.
	 * @return `CL_TRUE` if scan was successfully perform, or `CL_FALSE`
	 * otherwise.
	 * */
	cl_bool (*scan_with_device_data)(struct clo_scan* scanner,
		CCLQueue* queue, CCLBuffer* data_in, CCLBuffer* data_out,
		size_t numel, size_t lws_max, double* duration, GError** err);

	/**
	 * Perform scan using host data. Device buffers will be created and
	 * destroyed by scan implementation.
	 *
	 * @param[in] scanner Scan object.
	 * @param[in] queue Command queue wrapper. If `NULL` a queue will
	 * be created for the scan.
	 * @param[in] data_in Data to be scanned.
	 * @param[out] data_out Location where to place scanned data.
	 * @param[in] numel Number of elements in `data_in`.
	 * @param[in] lws_max Max. local worksize. If 0, the local worksize
	 * will be automatically determined.
	 * @param[out] duration Location where to place the duration of the
	 * scan. If `NULL` it will be ignored.
	 * @param[out] err Return location for a GError, or `NULL` if error
	 * reporting is to be ignored.
	 * @return `CL_TRUE` if scan was successfully perform, or `CL_FALSE`
	 * otherwise.
	 * */
	cl_bool (*scan_with_host_data)(struct clo_scan* scanner,
		CCLQueue* queue, void* data_in, void* data_out, size_t numel,
		size_t lws_max, double* duration, GError** err);

	/**
	 * Destroy scan object.
	 *
	 * @param[in] scan Scan object to destroy.
	 * */
	void (*destroy)(struct clo_scan* scan);

	/**
	 * @internal
	 * Private scan data.
	 * */
	void* _data;

} CloScan;

/* Generic scan object constructor. The exact type is given in the
 * first parameter. */
CloScan* clo_scan_new(const char* type, const char* options,
	CCLContext* ctx, CloType elem_type, CloType sum_type,
	const char* compiler_opts, GError** err);

#endif




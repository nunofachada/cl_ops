/*
 * This file is part of CL_Ops.
 *
 * CL_Ops is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * CL_Ops is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with CL_Ops. If not, see
 * <http://www.gnu.org/licenses/>.
 * */

/**
 * @file
 * Parallel prefix sum (scan) abstract declarations.
 * */

#ifndef _CLO_SCAN_ABSTRACT_H_
#define _CLO_SCAN_ABSTRACT_H_

#include "cl_ops/clo_common.h"

/**
 * @defgroup CLO_SCAN Parallel prefix sum (scan)
 *
 * This module provides several parallel prefix sum (scan)
 * implementations.
 *
 * @{
 */

/**
 * Definition of a scan implementation.
 * */
typedef struct clo_scan_impl_def {

	/**
	 * Scan implementation name.
	 * */
	const char* name;

	/**
	 * Initialize specific scan implementation.
	 *
	 * @param[in] scanner Scanner object.
	 * @param[in] options Algorithm options.
	 * @param[out] err Return location for a GError, or `NULL` if error
	 * reporting is to be ignored.
	 * @return Scan algorithm source code.
	 * */
	const char* (*init)(CloScan* scanner, const char* options,
		GError** err);

	/**
	 * Finalize specific scan implementation.
	 *
	 * @param[in] scan Scanner object.
	 * */
	void (*finalize)(CloScan* scan);

	/**
	 * Perform scan using device data.
	 *
	 * @copydetails clo_scan::clo_scan_with_device_data()
	 * */
	CCLEvent* (*scan_with_device_data)(CloScan* scanner,
		CCLQueue* cq_exec, CCLQueue* cq_comm, CCLBuffer* data_in,
		CCLBuffer* data_out, size_t numel, size_t lws_max,
		GError** err);

	/**
	 * Get the maximum number of kernels used by the scan
	 * implementation.
	 *
	 * @copydetails clo_scan::clo_scan_get_num_kernels()
	 * */
	cl_uint (*get_num_kernels)(CloScan* scanner, GError** err);

	/**
	 * Get name of the i^th kernel used by the scan implementation.
	 *
	 * @copydetails clo_scan::clo_scan_get_kernel_name()
	 * */
	const char* (*get_kernel_name)(
		CloScan* scanner, cl_uint i, GError** err);

	/**
	 * Get local memory usage of i^th kernel used by the scan
	 * implementation for the given maximum local worksize and number
	 * of elements to scan.
	 *
	 * @copydetails clo_scan::clo_scan_get_localmem_usage()
	 * */
	size_t (*get_localmem_usage)(CloScan* scanner, cl_uint i,
		size_t lws_max, size_t numel, GError** err);

} CloScanImplDef;

/** @} */

/* Generic scan object constructor. The exact type is given in the
 * first parameter. */
CloScan* clo_scan_new(const char* type, const char* options,
	CCLContext* ctx, CloType elem_type, CloType sum_type,
	const char* compiler_opts, GError** err);

/* Destroy scanner object. */
void clo_scan_destroy(CloScan* scan);

/* Perform scan using device data. */
CCLEvent* clo_scan_with_device_data(
	CloScan* scanner, CCLQueue* cq_exec, CCLQueue* cq_comm,
	CCLBuffer* data_in, CCLBuffer* data_out, size_t numel,
	size_t lws_max, GError** err);

/* Perform scan using host data. */
cl_bool clo_scan_with_host_data(CloScan* scanner,
	CCLQueue* cq_exec, CCLQueue* cq_comm, void* data_in, void* data_out,
	size_t numel, size_t lws_max, GError** err);

/* Get context wrapper associated with scanner object. */
CCLContext* clo_scan_get_context(CloScan* scanner);

/* Get program wrapper associated with scanner object. */
CCLProgram* clo_scan_get_program(CloScan* scanner);

/* Get type of elements to scan. */
CloType clo_scan_get_elem_type(CloScan* scanner);

/* Get the size in bytes of each element to be scanned. */
size_t clo_scan_get_element_size(CloScan* scanner);

/* Get type of elements in scan sum. */
CloType clo_scan_get_sum_type(CloScan* scanner);

/* Get the size in bytes of element in scan sum. */
size_t clo_scan_get_sum_size(CloScan* scanner);

/* Get data associated with specific scan implementation. */
void* clo_scan_get_data(CloScan* scanner);

/* Set scan specific data. */
void clo_scan_set_data(CloScan* scanner, void* data);

/* Get the maximum number of kernels used by the scan implementation. */
cl_uint clo_scan_get_num_kernels(CloScan* scanner, GError** err);

/* Get name of the i^th kernel used by the scan implementation. */
const char* clo_scan_get_kernel_name(
	CloScan* scanner, cl_uint i, GError** err);

/* Get local memory usage of i^th kernel used by the scan implementation
 * for the given maximum local worksize and number of elements to
 * scan. */
size_t clo_scan_get_localmem_usage(CloScan* scanner, cl_uint i,
	size_t lws_max, size_t numel, GError** err);

#endif




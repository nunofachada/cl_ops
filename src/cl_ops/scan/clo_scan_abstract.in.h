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

/* Abstract scan class. */
typedef struct clo_scan CloScan;

/**
 * Definition of a scan implementation.
 * */
typedef struct clo_scan_impl_def {

	/**
	 * Scan implementation name.
	 * */
	const char* name;

	/**
	 * Initialize specific scan implementation stuff.
	 *
	 * @param[in] options Algorithm options.
	 * @param[in] ctx Context wrapper object.
	 * @param[in] elem_type Type of elements to scan.
	 * @param[in] sum_type Type of scanned elements.
	 * @param[in] compiler_opts Compiler options.
	 * @param[out] err Return location for a GError, or `NULL` if error
	 * reporting is to be ignored.
	 * @return A program wrapper for the scan program.
	 * */
	CCLProgram* (*init)(CloScan* scanner, const char* options,
		const char* compiler_opts, GError** err);

	/**
	 * Finalize specific scan implementation stuff.
	 *
	 * @param[in] scan Scanner object.
	 * */
	void (*finalize)(CloScan* scan);

	/**
	 * Perform scan using device data.
	 *
	 * @copydetails ::clo_scan_with_device_data()
	 * */
	CCLEventWaitList (*scan_with_device_data)(CloScan* scanner,
		CCLQueue* cq_exec, CCLQueue* cq_comm, CCLBuffer* data_in,
		CCLBuffer* data_out, size_t numel, size_t lws_max,
		GError** err);

} CloScanImplDef;

/* Generic scan object constructor. The exact type is given in the
 * first parameter. */
CloScan* clo_scan_new(const char* type, const char* options,
	CCLContext* ctx, CloType elem_type, CloType sum_type,
	const char* compiler_opts, GError** err);

/* Destroy scanner object. */
void clo_scan_destroy(CloScan* scan);

/* Perform scan using device data. */
CCLEventWaitList clo_scan_with_device_data(
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

/* Get type of elements in scan sum. */
CloType clo_scan_get_sum_type(CloScan* scanner);

/* Get data associated with specific scan implementation. */
void* clo_scan_get_data(CloScan* scanner);

#endif




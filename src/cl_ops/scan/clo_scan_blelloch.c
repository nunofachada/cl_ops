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
 * Blelloch scan definitions.
 * */

#include "cl_ops/clo_scan_blelloch.h"
#include "common/_g_err_macros.h"

/**
 * @internal
 * Initializes the Blelloch scan object and returns the appropriate
 * program wrapper.
 * */
static const char* clo_scan_blelloch_init(CloScan* scanner,
	const char* options, GError** err) {

	/* Blelloch scan source code. */
	const char* src;

	/* Avoid compiler warnings. */
	(void)scanner;

	/* For now ignore specific blelloch scan options and throw error
	 * if any option is given. */
	g_if_err_create_goto(*err, CLO_ERROR,
		(options != NULL) && (strlen(options) > 0), CLO_ERROR_ARGS,
		error_handler, "Invalid options for blelloch scan.");

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	src = CLO_SCAN_BLELLOCH_SRC;
	goto finish;

error_handler:

	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	src = NULL;

finish:

	/* Return Blelloch source code. */
	return src;

}

/**
 * @internal
 * Finalize blelloch scan object.
 * */
static void clo_scan_blelloch_finalize(CloScan* scan) {
	(void)scan;
	return;
}

/**
 * @internal
 * Perform scan using device data.
 * */
static CCLEvent* clo_scan_blelloch_scan_with_device_data(
	CloScan* scanner, CCLQueue* cq_exec, CCLQueue* cq_comm,
	CCLBuffer* data_in, CCLBuffer* data_out, size_t numel,
	size_t lws_max, GError** err) {

	/* Local worksize. */
	size_t lws;

	/* OpenCL object wrappers. */
	CCLContext* ctx = NULL;
	CCLProgram* prg = NULL;
	CCLDevice* dev = NULL;
	CCLKernel* krnl_wgscan = NULL;
	CCLKernel* krnl_wgsumsscan = NULL;
	CCLKernel* krnl_addwgsums = NULL;
	CCLBuffer* dev_wgsums = NULL;
	CCLEvent* evt = NULL;

	/* Internal error reporting object. */
	GError* err_internal = NULL;

	/* Number of blocks to be processed per workgroup. */
	cl_uint blocks_per_wg;
	cl_uint numel_cl;

	/* Global worksizes. */
	size_t gws_wgscan, ws_wgsumsscan, gws_addwgsums;

	/* If data transfer queue is NULL, use exec queue for data
	 * transfers. */
	if (cq_comm == NULL) cq_comm = cq_exec;

	/* Get program wrapper. */
	prg = clo_scan_get_program(scanner);

	/* Size in bytes of sum scalars. */
	size_t size_sum = clo_scan_get_sum_size(scanner);

	/* Get device where scan will occurr. */
	dev = ccl_queue_get_device(cq_exec, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Get the context wrapper. */
	ctx = ccl_queue_get_context(cq_exec, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Get the wgscan kernel wrapper. */
	krnl_wgscan = ccl_program_get_kernel(
		prg, CLO_SCAN_BLELLOCH_KNAME_WGSCAN, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Determine worksizes. */
	lws = lws_max;
	size_t realws = numel / 2;
	ccl_kernel_suggest_worksizes(krnl_wgscan, dev, 1, &realws,
		&gws_wgscan, &lws, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);
	gws_wgscan = MIN(gws_wgscan, lws * lws);
	ws_wgsumsscan = (gws_wgscan / lws) / 2;
	gws_addwgsums = CLO_GWS_MULT(numel, lws);

	/* Determine number of blocks to be processed per workgroup. */
	blocks_per_wg = CLO_DIV_CEIL(numel / 2, gws_wgscan);
	numel_cl = numel;

	/* Create temporary buffer. */
	dev_wgsums = ccl_buffer_new(ctx, CL_MEM_READ_WRITE,
		(gws_wgscan / lws) * size_sum, NULL, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Set wgscan kernel arguments. */
	ccl_kernel_set_args(krnl_wgscan, data_in, data_out, dev_wgsums,
		ccl_arg_full(NULL, size_sum * lws * 2),
		ccl_arg_priv(numel_cl, cl_uint),
		ccl_arg_priv(blocks_per_wg, cl_uint), NULL);

	/* Perform workgroup-wise scan on complete array. */
	evt = ccl_kernel_enqueue_ndrange(krnl_wgscan, cq_exec, 1, NULL,
		&gws_wgscan, &lws, NULL, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);
	ccl_event_set_name(evt, "clo_scan_blelloch_wgscan");

	g_debug("N: %d, GWS1: %d, WS2: %d, GWS3: %d | LWS: %d | BPWG=%d | Enter? %s",
		(int) numel, (int) gws_wgscan, (int) ws_wgsumsscan,
		(int) gws_addwgsums, (int) lws, blocks_per_wg,
		gws_wgscan > lws ? "YES!" : "NO!");

	if (gws_wgscan > lws) {

		/* Get the remaining kernel wrappers. */
		krnl_wgsumsscan = ccl_program_get_kernel(
			prg, CLO_SCAN_BLELLOCH_KNAME_WGSUMSSCAN, &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
		krnl_addwgsums = ccl_program_get_kernel(
			prg, CLO_SCAN_BLELLOCH_KNAME_ADDWGSUMS, &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);

		/* Perform scan on workgroup sums array. */
		evt = ccl_kernel_set_args_and_enqueue_ndrange(krnl_wgsumsscan,
			cq_exec, 1, NULL, &ws_wgsumsscan, &ws_wgsumsscan, NULL,
			&err_internal,
			/* Argument list. */
			dev_wgsums, ccl_arg_full(NULL, size_sum * lws * 2),
			NULL);
		g_if_err_propagate_goto(err, err_internal, error_handler);
		ccl_event_set_name(evt, "clo_scan_blelloch_wgsumsscan");

		/* Add the workgroup-wise sums to the respective workgroup
		 * elements.*/
		evt = ccl_kernel_set_args_and_enqueue_ndrange(krnl_addwgsums,
			cq_exec, 1, NULL, &gws_addwgsums, &lws, NULL, &err_internal,
			/* Argument list. */
			dev_wgsums, data_out, ccl_arg_priv(blocks_per_wg, cl_uint),
			NULL);
		g_if_err_propagate_goto(err, err_internal, error_handler);
		ccl_event_set_name(evt, "clo_scan_blelloch_addwgsums");

	}

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	evt = NULL;

finish:

	/* Release temporary buffer. */
	if (dev_wgsums) ccl_buffer_destroy(dev_wgsums);

	/* Return event wait list. */
	return evt;

}

/**
 * @internal
 * Get the maximum number of kernels used by the scan implementation.
 * */
static cl_uint clo_scan_blelloch_get_num_kernels(
	CloScan* scanner, GError** err) {

	/* Avoid compiler warnings. */
	(void)scanner;
	(void)err;

	/* Return number of kernels. */
	return CLO_SCAN_BLELLOCH_NUM_KERNELS;

}

/**
 * @internal
 * Get name of the i^th kernel used by the scan implementation.
 * */
static const char* clo_scan_blelloch_get_kernel_name(
	CloScan* scanner, cl_uint i, GError** err) {

	/* Check that i is within bounds. */
	g_return_val_if_fail(i < CLO_SCAN_BLELLOCH_NUM_KERNELS, NULL);

	/* Avoid compiler warnings. */
	(void)scanner;

	/* Avoid compiler warnings. */
	(void)err;

	/* Kernel name. */
	const char* kernel_name = NULL;

	/* Determine kernel name. */
	switch (i) {
		case CLO_SCAN_BLELLOCH_KIDX_WGSCAN:
			kernel_name = CLO_SCAN_BLELLOCH_KNAME_WGSCAN;
			break;
		case CLO_SCAN_BLELLOCH_KIDX_WGSUMSSCAN:
			kernel_name = CLO_SCAN_BLELLOCH_KNAME_WGSUMSSCAN;
			break;
		case CLO_SCAN_BLELLOCH_KIDX_ADDWGSUMS:
			kernel_name = CLO_SCAN_BLELLOCH_KNAME_ADDWGSUMS;
			break;
		default:
			g_assert_not_reached();
	}

	/* Return kernel name. */
	return kernel_name;

}

/**
 * @internal
 * Get local memory usage of i^th kernel used by the scan implementation
 * for the given maximum local worksize and number of elements to scan.
 * */
static size_t clo_scan_blelloch_get_localmem_usage(CloScan* scanner,
	cl_uint i, size_t lws_max, size_t numel, GError** err) {

	/* Check that i is within bounds. */
	g_return_val_if_fail(i < CLO_SCAN_BLELLOCH_NUM_KERNELS, 0);

	/* Internal error handling object. */
	GError* err_internal = NULL;
	/* Local memory usage. */
	size_t local_mem;
	/* Worksizes. */
	size_t realws, gws;
	/* Device where scan will take place. */
	CCLDevice* dev = NULL;

	/* Get device where sort will take place (it is assumed to be the
	 * first device in the context). */
	dev = ccl_context_get_device(
		clo_scan_get_context(scanner), 0, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Determine worksizes. */
	realws = numel / 2;
	ccl_kernel_suggest_worksizes(
		NULL, dev, 1, &realws, &gws, &lws_max, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Determine local mem usage. */
	switch (i) {
		case CLO_SCAN_BLELLOCH_KIDX_WGSCAN:
			local_mem = clo_scan_get_sum_size(scanner) * lws_max * 2;
			break;
		case CLO_SCAN_BLELLOCH_KIDX_WGSUMSSCAN:
			local_mem = clo_scan_get_sum_size(scanner) * lws_max * 2;
			break;
		case CLO_SCAN_BLELLOCH_KIDX_ADDWGSUMS:
			local_mem = 0;
			break;
		default:
			g_assert_not_reached();
	}

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	local_mem = 0;

finish:
	/* Return kernel name. */
	return local_mem;


}

/* Definition of the Blelloch scan implementation. */
const CloScanImplDef clo_scan_blelloch_def = {
	"blelloch",
	clo_scan_blelloch_init,
	clo_scan_blelloch_finalize,
	clo_scan_blelloch_scan_with_device_data,
	clo_scan_blelloch_get_num_kernels,
	clo_scan_blelloch_get_kernel_name,
	clo_scan_blelloch_get_localmem_usage
};

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

#include "clo_scan_blelloch.h"

/**
 * Initializes the Blelloch scan object and returns the appropriate
 * program wrapper.
 *
 * @copydetails ::clo_scan_impl::init()
 * */
static const char* clo_scan_blelloch_init(CloScan* scanner,
	const char* options, GError** err) {

	/* For now ignore specific blelloch scan options. */
	(void)scanner;
	(void)options;
	(void)err;

	/* Return Blelloch source code. */
	return CLO_SCAN_BLELLOCH_SRC;

}

/**
 * Finalize blelloch scan object.
 *
 * @copydetails ::clo_scan_impl::finalize()
 * */
static void clo_scan_blelloch_finalize(CloScan* scan) {
	(void)scan;
	return;
}

/**
 * Perform scan using device data.
 *
 * @copydetails ::clo_scan_impl::scan_with_device_data()
 * */
static CCLEventWaitList clo_scan_blelloch_scan_with_device_data(
	CloScan* scanner, CCLQueue* cq_exec, CCLQueue* cq_comm,
	CCLBuffer* data_in, CCLBuffer* data_out, size_t numel,
	size_t lws_max, GError** err) {

	/* Local worksize. */
	size_t lws;

	/* OpenCL object wrappers. */
	CCLContext* ctx;
	CCLProgram* prg;
	CCLDevice* dev;
	CCLKernel* krnl_wgscan;
	CCLKernel* krnl_wgsumsscan;
	CCLKernel* krnl_addwgsums;
	CCLBuffer* dev_wgsums;
	CCLEvent* evt;

	/* Event wait list. */
	CCLEventWaitList ewl = NULL;

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
	size_t size_sum = clo_type_sizeof(clo_scan_get_sum_type(scanner));

	/* Get device where scan will occurr. */
	dev = ccl_queue_get_device(cq_exec, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Get the context wrapper. */
	ctx = ccl_queue_get_context(cq_exec, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Get the wgscan kernel wrapper. */
	krnl_wgscan = ccl_program_get_kernel(
		prg, "workgroupScan", &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Determine worksizes. */
	if (lws_max != 0) {
		lws = MIN(lws_max, clo_nlpo2(numel) / 2);
		gws_wgscan = MIN(CLO_GWS_MULT(numel / 2, lws), lws * lws);
	} else {
		size_t realws = numel / 2;
		ccl_kernel_suggest_worksizes(krnl_wgscan, dev, 1, &realws,
			&gws_wgscan, &lws, &err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
		gws_wgscan = MIN(gws_wgscan, lws * lws);
	}
	ws_wgsumsscan = (gws_wgscan / lws) / 2;
	gws_addwgsums = CLO_GWS_MULT(numel, lws);

	/* Determine number of blocks to be processed per workgroup. */
	blocks_per_wg = CLO_DIV_CEIL(numel / 2, gws_wgscan);
	numel_cl = numel;

	/* Create temporary buffer. */
	dev_wgsums = ccl_buffer_new(ctx, CL_MEM_READ_WRITE,
		(gws_wgscan / lws) * size_sum, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Set wgscan kernel arguments. */
	ccl_kernel_set_args(krnl_wgscan, data_in, data_out, dev_wgsums,
		ccl_arg_full(NULL, size_sum * lws * 2),
		ccl_arg_priv(numel_cl, cl_uint),
		ccl_arg_priv(blocks_per_wg, cl_uint), NULL);

	/* Perform workgroup-wise scan on complete array. */
	evt = ccl_kernel_enqueue_ndrange(krnl_wgscan, cq_exec, 1, NULL,
		&gws_wgscan, &lws, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	ccl_event_set_name(evt, "clo_scan_blelloch_wgscan");

	g_debug("N: %d, GWS1: %d, WS2: %d, GWS3: %d | LWS: %d | BPWG=%d | Enter? %s",
		(int) numel, (int) gws_wgscan, (int) ws_wgsumsscan,
		(int) gws_addwgsums, (int) lws, blocks_per_wg,
		gws_wgscan > lws ? "YES!" : "NO!");

	if (gws_wgscan > lws) {

		/* Get the remaining kernel wrappers. */
		krnl_wgsumsscan = ccl_program_get_kernel(
			prg, "workgroupSumsScan", &err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
		krnl_addwgsums = ccl_program_get_kernel(
			prg, "addWorkgroupSums", &err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);

		/* Perform scan on workgroup sums array. */
		evt = ccl_kernel_set_args_and_enqueue_ndrange(krnl_wgsumsscan,
			cq_exec, 1, NULL, &ws_wgsumsscan, &ws_wgsumsscan, NULL,
			&err_internal,
			/* Argument list. */
			dev_wgsums, ccl_arg_full(NULL, size_sum * lws * 2),
			NULL);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
		ccl_event_set_name(evt, "clo_scan_blelloch_wgsumsscan");

		/* Add the workgroup-wise sums to the respective workgroup
		 * elements.*/
		evt = ccl_kernel_set_args_and_enqueue_ndrange(krnl_addwgsums,
			cq_exec, 1, NULL, &gws_addwgsums, &lws, NULL, &err_internal,
			/* Argument list. */
			dev_wgsums, data_out, ccl_arg_priv(blocks_per_wg, cl_uint),
			NULL);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
		ccl_event_set_name(evt, "clo_scan_blelloch_addwgsums");

	}

	/* Add last event to wait list to return. */
	ccl_event_wait_list_add(&ewl, evt, NULL);

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);

finish:

	/* Release temporary buffer. */
	if (dev_wgsums) ccl_buffer_destroy(dev_wgsums);

	/* Return event wait list. */
	return ewl;

}

/* Definition of the Blelloch scan implementation. */
const CloScanImplDef clo_scan_blelloch_def = {
	"blelloch",
	clo_scan_blelloch_init,
	clo_scan_blelloch_finalize,
	clo_scan_blelloch_scan_with_device_data
};

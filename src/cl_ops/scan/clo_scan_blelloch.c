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
 * @internal
 * Blelloch scanner internal data.
 * */
struct clo_scan_blelloch_data {
	CCLContext* ctx;
	CCLProgram* prg;
	CloType elem_type;
	CloType sum_type;
};


/**
 * @internal
 * Perform scan using device data.
 *
 * @copydetails ::CloScan::scan_with_device_data()
 * */
static cl_bool clo_scan_blelloch_scan_with_device_data(CloScan* scanner,
	CCLQueue* queue, CCLBuffer* data_in, CCLBuffer* data_out,
	size_t numel, size_t lws_max, double* duration, GError** err) {

	/* Function return status. */
	cl_bool status;

	/* Local worksize. */
	size_t lws;

	/* OpenCL object wrappers. */
	CCLContext* ctx;
	CCLDevice* dev;
	CCLKernel* krnl_wgscan;
	CCLKernel* krnl_wgsumsscan;
	CCLKernel* krnl_addwgsums;
	CCLBuffer* dev_wgsums;

	/* Timer object. */
	GTimer* timer = NULL;

	/* Internal error reporting object. */
	GError* err_internal = NULL;

	/* Number of blocks to be processed per workgroup. */
	cl_uint blocks_per_wg;
	cl_uint numel_cl;

	/* Global worksizes. */
	size_t gws_wgscan, ws_wgsumsscan, gws_addwgsums;

	/* Scan data. */
	struct clo_scan_blelloch_data* data = scanner->_data;

	/* Size in bytes of sum scalars. */
	size_t size_sum = clo_type_sizeof(data->sum_type);

	/* Get device where scan will occurr. */
	dev = ccl_queue_get_device(queue, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Get the context wrapper. */
	ctx = ccl_queue_get_context(queue, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Get the wgscan kernel wrapper. */
	krnl_wgscan = ccl_program_get_kernel(
		data->prg, "workgroupScan", &err_internal);
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

	/* Create and start timer, if required. */
	if (duration) { timer = g_timer_new(); g_timer_start(timer); }

	/* Perform workgroup-wise scan on complete array. */
	ccl_kernel_enqueue_ndrange(krnl_wgscan, queue, 1, NULL, &gws_wgscan,
		&lws, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	g_debug("N: %d, GWS1: %d, WS2: %d, GWS3: %d | LWS: %d | BPWG=%d | Enter? %s",
		(int) numel, (int) gws_wgscan, (int) ws_wgsumsscan,
		(int) gws_addwgsums, (int) lws, blocks_per_wg,
		gws_wgscan > lws ? "YES!" : "NO!");

	if (gws_wgscan > lws) {

		/* Get the remaining kernel wrappers. */
		krnl_wgsumsscan = ccl_program_get_kernel(
			data->prg, "workgroupSumsScan", &err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
		krnl_addwgsums = ccl_program_get_kernel(
			data->prg, "addWorkgroupSums", &err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);

		/* Perform scan on workgroup sums array. */
		ccl_kernel_set_args_and_enqueue_ndrange(krnl_wgsumsscan, queue,
			1, NULL, &ws_wgsumsscan, &ws_wgsumsscan, NULL, &err_internal,
			/* Argument list. */
			dev_wgsums, ccl_arg_full(NULL, size_sum * lws * 2),
			NULL);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);

		/* Add the workgroup-wise sums to the respective workgroup
		 * elements.*/
		ccl_kernel_set_args_and_enqueue_ndrange(krnl_addwgsums, queue,
			1, NULL, &gws_addwgsums, &lws, NULL, &err_internal,
			/* Argument list. */
			dev_wgsums, data_out, ccl_arg_priv(blocks_per_wg, cl_uint),
			NULL);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);

	}

	/* Stop timer and keep time, if required. */
	if (duration) {
		g_timer_stop(timer);
		*duration = g_timer_elapsed(timer, NULL);
	}

	/* If we got here, everything is OK. */
	status = CL_TRUE;
	g_assert(err == NULL || *err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	status = CL_FALSE;

finish:

	/* Release temporary buffer. */
	if (dev_wgsums) ccl_buffer_destroy(dev_wgsums);

	/* Free timer, if required.. */
	if (timer) g_timer_destroy(timer);

	/* Return. */
	return status;

}

/**
 * @internal
 * Perform scan using host data.
 *
 * @copydetails ::CloScan::scan_with_host_data()
 * */
static cl_bool clo_scan_blelloch_scan_with_host_data(CloScan* scanner,
	CCLQueue* queue, void* data_in, void* data_out, size_t numel,
	size_t lws_max, double* duration, GError** err) {

	/* Function return status. */
	cl_bool status;

	/* OpenCL wrapper objects. */
	CCLBuffer* data_in_dev = NULL;
	CCLBuffer* data_out_dev = NULL;
	CCLQueue* intern_queue = NULL;
	CCLDevice* dev = NULL;

	/* Internal error object. */
	GError* err_internal = NULL;

	/* Blelloch scan internal data. */
	struct clo_scan_blelloch_data* data =
		(struct clo_scan_blelloch_data*) scanner->_data;

	/* Determine data sizes. */
	size_t data_in_size = numel * clo_type_sizeof(data->elem_type);
	size_t data_out_size = numel * clo_type_sizeof(data->sum_type);

	/* If queue is NULL, create own queue using first device in
	 * context. */
	if (queue == NULL) {
		/* Get first device in context. */
		dev = ccl_context_get_device(data->ctx, 0, &err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
		/* Create queue. */
		intern_queue = ccl_queue_new(data->ctx, dev, 0, &err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
		queue = intern_queue;
	}

	/* Create device buffers. */
	data_in_dev = ccl_buffer_new(
		data->ctx, CL_MEM_READ_ONLY, data_in_size, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	data_out_dev = ccl_buffer_new(
		data->ctx, CL_MEM_READ_WRITE, data_out_size, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Transfer data to device. */
	ccl_buffer_enqueue_write(data_in_dev, queue, CL_TRUE, 0,
		data_in_size, data_in, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Perform scan with device data. */
	clo_scan_blelloch_scan_with_device_data(scanner, queue, data_in_dev,
		data_out_dev, numel, lws_max, duration, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Transfer data back to host. */
	ccl_buffer_enqueue_read(data_out_dev, queue, CL_TRUE, 0,
		data_out_size, data_out, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	status = CL_TRUE;
	goto finish;

error_handler:

	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	status = CL_FALSE;

finish:

	/* Free stuff. */
	if (data_in_dev) ccl_buffer_destroy(data_in_dev);
	if (data_out_dev) ccl_buffer_destroy(data_out_dev);
	if (intern_queue) ccl_queue_destroy(intern_queue);

	/* Return function status. */
	return status;

}


/**
 * @internal
 * Destroy scan object.
 *
 * @copydetails ::CloScan::destroy()
 * */
static void clo_scan_blelloch_destroy(CloScan* scan) {
	struct clo_scan_blelloch_data* data =
		(struct clo_scan_blelloch_data*) scan->_data;
	ccl_context_unref(data->ctx);
	if (data->prg) ccl_program_destroy(data->prg);
	g_slice_free(struct clo_scan_blelloch_data, scan->_data);
	g_slice_free(CloScan, scan);
}

/**
 * Creates a new blelloch scan object.
 *
 * @param[in] options Algorithm options.
 * @param[in] ctx Context wrapper object.
 * @param[in] elem_type Type of elements to scan.
 * @param[in] sum_type Type of scanned elements.
 * @param[in] compiler_opts Compiler options.
 * @param[out] err Return location for a GError, or `NULL` if error
 * reporting is to be ignored.
 * @return A new blelloch scan object.
 * */
CloScan* clo_scan_blelloch_new(const char* options, CCLContext* ctx,
	CloType elem_type, CloType sum_type, const char* compiler_opts,
	GError** err) {

	/* Internal error management object. */
	GError *err_internal = NULL;

	/* Final compiler options. */
	gchar* compiler_opts_final = NULL;

	/* Allocate memory for scan object. */
	CloScan* scan = g_slice_new0(CloScan);

	/* Allocate data for private scan data. */
	struct clo_scan_blelloch_data* data =
		g_slice_new0(struct clo_scan_blelloch_data);

	/* Keep data in scan private data. */
	ccl_context_ref(ctx);
	data->ctx = ctx;
	data->elem_type = elem_type;
	data->sum_type = sum_type;

	/* Set object methods. */
	scan->destroy = clo_scan_blelloch_destroy;
	scan->scan_with_host_data = clo_scan_blelloch_scan_with_host_data;
	scan->scan_with_device_data = clo_scan_blelloch_scan_with_device_data;
	scan->_data = data;

	/* For now ignore specific blelloch scan options. */
	options = options;

	/* Determine final compiler options. */
	compiler_opts_final = g_strconcat(
		" -DCLO_SCAN_ELEM_TYPE=", clo_type_get_name(elem_type),
		" -DCLO_SCAN_SUM_TYPE=", clo_type_get_name(sum_type),
		compiler_opts, NULL);

	/* Create and build program. */
	data->prg = ccl_program_new_from_source(
		ctx, CLO_SCAN_BLELLOCH_SRC, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	ccl_program_build(data->prg, compiler_opts_final, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	clo_scan_blelloch_destroy(scan);
	scan = NULL;

finish:

	/* Free stuff. */
	if (compiler_opts_final) g_free(compiler_opts_final);

	/* Return scan object. */
	return scan;

}


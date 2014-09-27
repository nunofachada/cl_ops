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

#include "clo_sort_sbitonic.h"

/**
 * @internal
 * Perform sort using device data.
 *
 * @copydetails ::CloSort::sort_with_device_data()
 * */
static cl_bool clo_sort_sbitonic_sort_with_device_data(CloSort* sorter,
	CCLQueue* queue, CCLBuffer* data_in, CCLBuffer* data_out,
	size_t numel, size_t lws_max, double* duration, GError** err) {

	/* Function return status. */
	cl_bool status;

	/* Worksizes. */
	size_t lws, gws;

	/* Number of bitonic sort stages. */
	cl_uint tot_stages;

	/* OpenCL object wrappers. */
	//~ CCLContext* ctx;
	CCLDevice* dev;
	CCLKernel* krnl;

	/* Timer object. */
	GTimer* timer = NULL;

	/* Internal error reporting object. */
	GError* err_internal = NULL;

	/* Get device where sort will occurr. */
	dev = ccl_queue_get_device(queue, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Get the kernel wrapper. */
	krnl = ccl_program_get_kernel(clo_sort_get_program(sorter),
		"clo_sort_sbitonic", &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Determine worksizes. */
	if (lws_max != 0) {
		lws = MIN(lws_max, clo_nlpo2(numel) / 2);
		gws = CLO_GWS_MULT(numel / 2, lws);
	} else {
		size_t realws = numel / 2 + numel % 2;
		ccl_kernel_suggest_worksizes(
			krnl, dev, 1, &realws, &gws, &lws, &err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
	}

	/* Determine number of bitonic sort stages. */
	tot_stages = (cl_uint) clo_tzc(gws * 2);

	/* Determine which buffer to use. */
	if (data_out == NULL) {
		/* Sort directly in original data. */
		data_out = data_in;
	} else {
		/* Copy data_in to data_out first, and then sort on copied
		 * data. */
		ccl_buffer_enqueue_copy(data_in, data_out, queue, 0, 0,
			clo_sort_get_element_size(sorter) * numel, NULL,
			&err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
	}

	/* Set first kernel argument. */
	ccl_kernel_set_arg(krnl, 0, data_out);

	/* Create and start timer, if required. */
	if (duration) { timer = g_timer_new(); g_timer_start(timer); }

	/* Perform simple bitonic sort. */
	for (cl_uint curr_stage = 1; curr_stage <= tot_stages; curr_stage++) {

		ccl_kernel_set_arg(krnl, 1, ccl_arg_priv(curr_stage, cl_uint));

		cl_uint step = curr_stage;

		for (cl_uint curr_step = step; curr_step > 0; curr_step--) {

			ccl_kernel_set_arg(krnl, 2, ccl_arg_priv(curr_step, cl_uint));

			ccl_kernel_enqueue_ndrange(krnl, queue, 1, NULL, &gws, &lws,
				NULL, &err_internal);
			ccl_if_err_propagate_goto(err, err_internal, error_handler);

		}
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

	/* Free timer, if required.. */
	if (timer) g_timer_destroy(timer);

	/* Return. */
	return status;

}

/**
 * @internal
 * Perform sort using host data.
 *
 * @copydetails ::CloSort::sort_with_host_data()
 * */
static cl_bool clo_sort_sbitonic_sort_with_host_data(CloSort* sorter,
	CCLQueue* queue, void* data_in, void* data_out, size_t numel,
	size_t lws_max, double* duration, GError** err) {

	/* Function return status. */
	cl_bool status;

	/* OpenCL wrapper objects. */
	CCLContext* ctx = NULL;
	CCLBuffer* data_in_dev = NULL;
	CCLQueue* intern_queue = NULL;
	CCLDevice* dev = NULL;

	/* Internal error object. */
	GError* err_internal = NULL;

	/* Determine data size. */
	size_t data_size = numel * clo_sort_get_element_size(sorter);

	/* Get context wrapper. */
	ctx = clo_sort_get_context(sorter);

	/* If queue is NULL, create own queue using first device in
	 * context. */
	if (queue == NULL) {
		/* Get first device in context. */
		dev = ccl_context_get_device(ctx, 0, &err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
		/* Create queue. */
		intern_queue = ccl_queue_new(ctx, dev, 0, &err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
		queue = intern_queue;
	}

	/* Create device buffer. */
	data_in_dev = ccl_buffer_new(
		ctx, CL_MEM_READ_WRITE, data_size, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Transfer data to device. */
	ccl_buffer_enqueue_write(data_in_dev, queue, CL_TRUE, 0,
		data_size, data_in, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Perform sort with device data. */
	clo_sort_sbitonic_sort_with_device_data(sorter, queue, data_in_dev,
		NULL, numel, lws_max, duration, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Transfer data back to host. */
	ccl_buffer_enqueue_read(data_in_dev, queue, CL_TRUE, 0,
		data_size, data_out, NULL, &err_internal);
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
	if (intern_queue) ccl_queue_destroy(intern_queue);

	/* Return function status. */
	return status;

}

/**
 * Initializes a simple bitonic sorter object and returns the
 * respective source code.
 *
 * @param[in] Sorter object to initialize.
 * @param[in] options Algorithm options.
 * @param[out] err Return location for a GError, or `NULL` if error
 * reporting is to be ignored.
 * @return Simple bitonic sorter source code.
 * */
const char* clo_sort_sbitonic_init(
	CloSort* sorter, const char* options, GError** err) {

	/* Set object methods. */
	sorter->sort_with_host_data = clo_sort_sbitonic_sort_with_host_data;
	sorter->sort_with_device_data = clo_sort_sbitonic_sort_with_device_data;

	/* Ignore specific sbitonic sort options and error handling. */
	options = options;
	err = err;

	/* Return source to be compiled. */
	return CLO_SORT_SBITONIC_SRC;

}


/**
 * Finalizes a bitonic sorter object.
 *
 * @param[in] sorter Sorter object to finalize.
 * */
void clo_sort_sbitonic_finalize(CloSort* sorter) {
	/* Nothing to finalize. */
	sorter = sorter;
	return;
}

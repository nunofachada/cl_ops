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
 * @brief Simple bitonic sort host implementation.
 */

#include "clo_sort_sbitonic.h"

/**
 * @internal
 * Perform sort using device data.
 *
 * @copydetails ::CloSort::sort_with_device_data()
 * */
static CCLEventWaitList clo_sort_sbitonic_sort_with_device_data(
	CloSort* sorter, CCLQueue* cq_exec, CCLQueue* cq_comm,
	CCLBuffer* data_in, CCLBuffer* data_out, size_t numel,
	size_t lws_max, GError** err) {

	/* Make sure err is NULL or it is not set. */
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	/* Make sure cq_exec is not NULL. */
	g_return_val_if_fail(cq_exec != NULL, NULL);

	/* Worksizes. */
	size_t lws, gws;

	/* Number of bitonic sort stages. */
	cl_uint tot_stages;

	/* OpenCL object wrappers. */
	CCLDevice* dev;
	CCLKernel* krnl;
	CCLEvent* evt;

	/* Event wait list. */
	CCLEventWaitList ewl = NULL;

	/* Internal error reporting object. */
	GError* err_internal = NULL;

	/* If data transfer queue is NULL, use exec queue for data
	 * transfers. */
	if (cq_comm == NULL) cq_comm = cq_exec;

	/* Get device where sort will occurr. */
	dev = ccl_queue_get_device(cq_exec, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Get the kernel wrapper. */
	krnl = ccl_program_get_kernel(clo_sort_get_program(sorter),
		"clo_sort_sbitonic", &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Determine worksizes. */
	gws = clo_nlpo2(numel) / 2;
	lws = clo_get_lws(krnl, dev, gws, lws_max, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Determine number of bitonic sort stages. */
	tot_stages = (cl_uint) clo_tzc(gws * 2);

	/* Determine which buffer to use. */
	if (data_out == NULL) {
		/* Sort directly in original data. */
		data_out = data_in;
	} else {
		/* Copy data_in to data_out first, and then sort on copied
		 * data. */
		evt = ccl_buffer_enqueue_copy(data_in, data_out, cq_comm, 0, 0,
			clo_sort_get_element_size(sorter) * numel, NULL,
			&err_internal);

		ccl_if_err_propagate_goto(err, err_internal, error_handler);
		ccl_event_set_name(evt, "copy_sbitonic");
		ccl_event_wait_list_add(&ewl, evt, NULL);
	}

	/* Set first kernel argument. */
	ccl_kernel_set_arg(krnl, 0, data_out);

	/* Perform simple bitonic sort. */
	for (cl_uint curr_stage = 1; curr_stage <= tot_stages; curr_stage++) {

		ccl_kernel_set_arg(krnl, 1, ccl_arg_priv(curr_stage, cl_uint));

		cl_uint step = curr_stage;

		for (cl_uint curr_step = step; curr_step > 0; curr_step--) {

			ccl_kernel_set_arg(krnl, 2, ccl_arg_priv(curr_step, cl_uint));

			evt = ccl_kernel_enqueue_ndrange(krnl, cq_exec, 1, NULL,
				&gws, &lws, &ewl, &err_internal);
			ccl_if_err_propagate_goto(err, err_internal, error_handler);
			ccl_event_set_name(evt, "ndrange_sbitonic");

		}
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

	/* Return. */
	return ewl;

}

/**
 * @internal
 * Perform sort using host data.
 *
 * @copydetails ::CloSort::sort_with_host_data()
 * */
static cl_bool clo_sort_sbitonic_sort_with_host_data(CloSort* sorter,
	CCLQueue* cq_exec, CCLQueue* cq_comm, void* data_in, void* data_out,
	size_t numel, size_t lws_max, GError** err) {

	/* Make sure err is NULL or it is not set. */
	g_return_val_if_fail(err == NULL || *err == NULL, CL_FALSE);

	/* Function return status. */
	cl_bool status;

	/* OpenCL wrapper objects. */
	CCLContext* ctx = NULL;
	CCLBuffer* data_in_dev = NULL;
	CCLQueue* intern_queue = NULL;
	CCLDevice* dev = NULL;
	CCLEvent* evt;

	/* Event wait list. */
	CCLEventWaitList ewl = NULL;

	/* Internal error object. */
	GError* err_internal = NULL;

	/* Determine data size. */
	size_t data_size = numel * clo_sort_get_element_size(sorter);

	/* Get context wrapper. */
	ctx = clo_sort_get_context(sorter);

	/* If execution queue is NULL, create own queue using first device
	 * in context. */
	if (cq_exec == NULL) {
		/* Get first device in context. */
		dev = ccl_context_get_device(ctx, 0, &err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
		/* Create queue. */
		intern_queue = ccl_queue_new(ctx, dev, 0, &err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
		cq_exec = intern_queue;
	}

	/* If data transfer queue is NULL, use exec queue for data
	 * transfers. */
	if (cq_comm == NULL) cq_comm = cq_exec;

	/* Create device buffer. */
	data_in_dev = ccl_buffer_new(
		ctx, CL_MEM_READ_WRITE, data_size, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Transfer data to device. */
	evt = ccl_buffer_enqueue_write(data_in_dev, cq_comm, CL_FALSE, 0,
		data_size, data_in, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	ccl_event_set_name(evt, "write_sbitonic");

	/* Explicitly wait for transfer (some OpenCL implementations don't
	 * respect CL_TRUE in data transfers). */
	ccl_event_wait_list_add(&ewl, evt, NULL);
	ccl_event_wait(&ewl, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Perform sort with device data. */
	ewl = clo_sort_sbitonic_sort_with_device_data(sorter, cq_exec,
		cq_comm, data_in_dev, NULL, numel, lws_max, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Transfer data back to host. */
	evt = ccl_buffer_enqueue_read(data_in_dev, cq_comm, CL_FALSE, 0,
		data_size, data_out, &ewl, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	ccl_event_set_name(evt, "read_sbitonic");

	/* Explicitly wait for transfer (some OpenCL implementations don't
	 * respect CL_TRUE in data transfers). */
	ccl_event_wait_list_add(&ewl, evt, NULL);
	ccl_event_wait(&ewl, &err_internal);
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
static const char* clo_sort_sbitonic_init(
	CloSort* sorter, const char* options, GError** err) {

	/* Make sure err is NULL or it is not set. */
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	/* Ignore specific sbitonic sort options and error handling. */
	(void)options;
	(void)err;
	(void)sorter;

	/* Return source to be compiled. */
	return CLO_SORT_SBITONIC_SRC;

}


/**
 * Finalizes a bitonic sorter object.
 *
 * @param[in] sorter Sorter object to finalize.
 * */
static void clo_sort_sbitonic_finalize(CloSort* sorter) {
	/* Nothing to finalize. */
	sorter = sorter;
	return;
}

/* Definition of the sbitonic sort implementation. */
const CloSortImplDef clo_sort_sbitonic_def = {
	"sbitonic",
	clo_sort_sbitonic_init,
	clo_sort_sbitonic_finalize,
	clo_sort_sbitonic_sort_with_device_data,
	clo_sort_sbitonic_sort_with_host_data
};

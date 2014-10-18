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
 * @brief Global memory selection sort host implementation.
 */

#include "clo_sort_gselect.h"

/**
 * @internal
 * Perform sort using device data.
 *
 * @copydetails ::CloSort::sort_with_device_data()
 * */
static CCLEventWaitList clo_sort_gselect_sort_with_device_data(
	CloSort* sorter, CCLQueue* cq_exec, CCLQueue* cq_comm,
	CCLBuffer* data_in, CCLBuffer* data_out, size_t numel,
	size_t lws_max, GError** err) {
		
	/* Make sure err is NULL or it is not set. */
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	/* Worksizes. */
	size_t lws, gws;

	/* OpenCL object wrappers. */
	CCLContext* ctx;
	CCLDevice* dev;
	CCLKernel* krnl;
	CCLEvent* evt;

	/* Event wait list. */
	CCLEventWaitList ewl = NULL;

	/* Internal error reporting object. */
	GError* err_internal = NULL;

	/* Flag indicating if sorted data is to be copied back to original
	 * buffer, simulating an in-place sort. */
	cl_bool copy_back;

	/* If data transfer queue is NULL, use exec queue for data
	 * transfers. */
	if (cq_comm == NULL) cq_comm = cq_exec;

	/* Get device where sort will occurr. */
	dev = ccl_queue_get_device(cq_exec, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Get the kernel wrapper. */
	krnl = ccl_program_get_kernel(clo_sort_get_program(sorter),
		"clo_sort_gselect", &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Determine worksizes. */
	gws = numel;
	lws = clo_get_lws(krnl, dev, gws, lws_max, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Check if data_out is set. */
	if (data_out == NULL) {
		/* If not create it and set the copy back flag to TRUE. */

		/* Get context. */
		ctx = ccl_queue_get_context(cq_comm, &err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);

		/* Set copy-back flag to true. */
		copy_back = CL_TRUE;

		/* Create output buffer. */
		data_out = ccl_buffer_new(ctx, CL_MEM_WRITE_ONLY,
			numel * clo_sort_get_element_size(sorter), NULL,
			&err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);

	} else {

		/* Set copy back flag to FALSE. */
		copy_back = CL_FALSE;
	}

	/* Set kernel arguments. */
	cl_ulong numel_l = numel;
	ccl_kernel_set_args(
		krnl, data_in, data_out, ccl_arg_priv(numel_l, cl_ulong), NULL);

	/* Perform global memory selection sort. */
	evt = ccl_kernel_enqueue_ndrange(
		krnl, cq_exec, 1, NULL, &gws, &lws, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	ccl_event_set_name(evt, "ndrange_gselect");

	/* If copy-back flag is set, copy sorted data back to original
	 * buffer. */
	if (copy_back) {
		ccl_event_wait_list_add(&ewl, evt, NULL);
		evt = ccl_buffer_enqueue_copy(data_out, data_in, cq_comm, 0, 0,
			numel * clo_sort_get_element_size(sorter), &ewl,
			&err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
		ccl_event_set_name(evt, "copy_gselect");
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

	/* Free data out buffer if copy-back flag is set. */
	if ((copy_back) && (data_out != NULL)) ccl_buffer_destroy(data_out);

	/* Return. */
	return ewl;

}

/**
 * @internal
 * Perform sort using host data.
 *
 * @copydetails ::CloSort::sort_with_host_data()
 * */
static cl_bool clo_sort_gselect_sort_with_host_data(CloSort* sorter,
	CCLQueue* cq_exec, CCLQueue* cq_comm, void* data_in, void* data_out,
	size_t numel, size_t lws_max, GError** err) {
		
	/* Make sure err is NULL or it is not set. */
	g_return_val_if_fail(err == NULL || *err == NULL, CL_FALSE);

	/* Function return status. */
	cl_bool status;

	/* OpenCL wrapper objects. */
	CCLContext* ctx = NULL;
	CCLBuffer* data_in_dev = NULL;
	CCLBuffer* data_out_dev = NULL;
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

	/* Create device data in buffer. */
	data_in_dev = ccl_buffer_new(
		ctx, CL_MEM_READ_ONLY, data_size, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Create device data out buffer. */
	data_out_dev = ccl_buffer_new(
		ctx, CL_MEM_WRITE_ONLY, data_size, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Transfer data to device. */
	evt = ccl_buffer_enqueue_write(data_in_dev, cq_comm, CL_FALSE, 0,
		data_size, data_in, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	ccl_event_set_name(evt, "write_gselect");

	/* Explicitly wait for transfer (some OpenCL implementations don't
	 * respect CL_TRUE in data transfers). */
	ccl_event_wait_list_add(&ewl, evt, NULL);
	ccl_event_wait(&ewl, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Perform sort with device data. */
	ewl = clo_sort_gselect_sort_with_device_data(sorter, cq_exec,
		cq_comm, data_in_dev, data_out_dev, numel, lws_max,
		&err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Transfer data back to host. */
	evt = ccl_buffer_enqueue_read(data_out_dev, cq_comm, CL_FALSE, 0,
		data_size, data_out, &ewl, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	ccl_event_set_name(evt, "read_gselect");

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
	if (data_out_dev) ccl_buffer_destroy(data_out_dev);
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
const char* clo_sort_gselect_init(
	CloSort* sorter, const char* options, GError** err) {
		
	/* Make sure err is NULL or it is not set. */
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	/* Set object methods. */
	sorter->sort_with_host_data = clo_sort_gselect_sort_with_host_data;
	sorter->sort_with_device_data = clo_sort_gselect_sort_with_device_data;

	/* Ignore specific sbitonic sort options and error handling. */
	options = options;
	err = err;

	/* Return source to be compiled. */
	return CLO_SORT_GSELECT_SRC;

}


/**
 * Finalizes a bitonic sorter object.
 *
 * @param[in] sorter Sorter object to finalize.
 * */
void clo_sort_gselect_finalize(CloSort* sorter) {
	/* Nothing to finalize. */
	sorter = sorter;
	return;
}

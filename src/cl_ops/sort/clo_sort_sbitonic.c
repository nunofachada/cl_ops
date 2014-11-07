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
 * Simple bitonic sort host implementation.
 */

#include "clo_sort_sbitonic.h"

/**
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
		"sbitonic", &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Determine worksizes. */
	gws = clo_nlpo2(numel) / 2;
	lws = lws_max;
	ccl_kernel_suggest_worksizes(
		krnl, dev, 1, &gws, NULL, &lws, &err_internal);
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
		ccl_event_set_name(evt, "sbitonic_copy");
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
			ccl_event_set_name(evt, "sbitonic_ndrange");

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
 * Initializes a simple bitonic sorter object and returns the
 * respective source code.
 *
 * @copydetails ::CloSort::init()
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
 * @copydetails ::CloSort::finalize()
 * */
static void clo_sort_sbitonic_finalize(CloSort* sorter) {
	/* Nothing to finalize. */
	(void)sorter;
	return;
}

/**
 * Get the maximum number of kernels used by the sort implementation.
 *
 * @copydetails ::CloSort::get_num_kernels()
 * */
static cl_uint clo_sort_sbitonic_get_num_kernels(CloSort* sorter) {

	/* Avoid compiler warnings. */
	(void)sorter;

	/* Return number of kernels. */
	return 1;

}

/**
 * Get name of the i^th kernel used by the sort implementation.
 *
 * @copydetails ::CloSort::get_kernel_name()
 * */
const char* clo_sort_sbitonic_get_kernel_name(
	CloSort* sorter, cl_uint i) {

	/* i must be zero because there is only one kernel. */
	g_return_val_if_fail(i == 0, NULL);

	/* Avoid compiler warnings. */
	(void)sorter;

	/* Return kernel name. */
	return CLO_SORT_SBITONIC_KNAME;
}

/**
 * Get local memory usage of i^th kernel used by the sort implementation
 * for the given maximum local worksize and number of elements to sort.
 *
 * @copydetails ::CloSort::get_localmem_usage()
 * */
size_t clo_sort_sbitonic_get_localmem_usage(CloSort* sorter, cl_uint i,
	size_t lws_max, size_t numel, GError** err) {

	/* i must be zero because there is only one kernel. */
	g_return_val_if_fail(i == 0, NULL);

	/* Avoid compiler warnings. */
	(void)sorter;
	(void)lws_max;
	(void)numel;
	(void)err;

	/* Simple bitonic sort doesn't use local memory. */
	return 0;

}

/* Definition of the sbitonic sort implementation. */
const CloSortImplDef clo_sort_sbitonic_def = {
	"sbitonic",
	CL_TRUE,
	clo_sort_sbitonic_init,
	clo_sort_sbitonic_finalize,
	clo_sort_sbitonic_sort_with_device_data,
	clo_sort_sbitonic_get_num_kernels,
	clo_sort_sbitonic_get_kernel_name,
	clo_sort_sbitonic_get_localmem_usage
};

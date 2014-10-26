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
 * Satish et al Radix Sort host implementation.
 */

#include "clo_sort_satradix.h"

typedef struct {

	/** Radix. */
	cl_uint radix;

} clo_sort_satradix_data;

/**
 * Perform sort using device data.
 *
 * @copydetails ::CloSort::sort_with_device_data()
 * */
static CCLEventWaitList clo_sort_satradix_sort_with_device_data(
	CloSort* sorter, CCLQueue* cq_exec, CCLQueue* cq_comm,
	CCLBuffer* data_in, CCLBuffer* data_out, size_t numel,
	size_t lws_max, GError** err) {

	/* Make sure err is NULL or it is not set. */
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	/* Make sure cq_exec is not NULL. */
	g_return_val_if_fail(cq_exec != NULL, NULL);

	/* Required OpenCL object wrappers. */
	CCLContext* ctx = NULL;
	CCLDevice* dev = NULL;
	CCLBuffer* data_aux = NULL;
	CCLBuffer* offsets = NULL;
	CCLBuffer* counters = NULL;
	CCLBuffer* counters_sum = NULL;

	/* Effective local worksize for the several radix sort kernels. */
	size_t lws_sort;

	/* Number of workgroups for the several radix sort kernels. */
	size_t num_wgs;

	/* Size of aux. buffers. */
	size_t aux_buf_size;

	/* Event wait list. */
	CCLEventWaitList ewl = NULL;

	/* Internal error reporting object. */
	GError* err_internal = NULL;

	/* Get radix sort parameters. */
	clo_sort_satradix_data data =
		*((clo_sort_satradix_data*) clo_sort_get_data(sorter));

	/* Get device where sort will occurr. */
	dev = ccl_queue_get_device(cq_exec, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Determine the effective local worksize for the several radix
	 * sort kernels... */
	lws_sort = clo_get_lws(NULL, dev, numel, lws_max, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	/* ...and it can't be smaller than the radix itself. */
	lws_sort = MAX(lws_sort, data.radix);

	/* Determine the number of workgroups for the several radix sort
	 * kernels. */
	num_wgs = numel / lws_sort;

	/* Get context. */
	ctx = clo_sort_get_context(sorter);

	/* Determine size of aux. buffers. */
	aux_buf_size = num_wgs * data.radix * sizeof(cl_uint);

	/* Allocate auxiliary device buffers. */
	data_aux = ccl_buffer_new(
		ctx, CL_MEM_READ_WRITE, numel * clo_sort_get_element_size(sorter),
		NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	offsets = ccl_buffer_new(
		ctx, CL_MEM_READ_WRITE, aux_buf_size, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	counters = ccl_buffer_new(
		ctx, CL_MEM_READ_WRITE, aux_buf_size, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	counters_sum = ccl_buffer_new(
		ctx, CL_MEM_READ_WRITE, aux_buf_size, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Perform sort. */
	for (cl_uint i = 0; i < 10 /* total_digits */; ++i) {

	}

	//~ /* Add last event to wait list to return. */
	//~ ccl_event_wait_list_add(&ewl, evt, NULL);

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);

finish:

	/* Free stuff. */
	ccl_buffer_destroy(data_aux);
	ccl_buffer_destroy(offsets);
	ccl_buffer_destroy(counters);
	ccl_buffer_destroy(counters_sum);

	/* Return. */
	return ewl;

}

/**
 * Initializes a SatRadix sorter object and returns the
 * respective source code.
 *
 * @copydetails ::CloSort::init()
 * */
static const char* clo_sort_satradix_init(
	CloSort* sorter, const char* options, GError** err) {

	/* Make sure err is NULL or it is not set. */
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	const char* satradix_src;
	clo_sort_satradix_data* data;
	data = g_slice_new0(clo_sort_satradix_data);

	/* Set internal data default values. */
	data->radix = 16;

	/* Number of tokens. */
	int num_toks;

	/* Tokenized options. */
	gchar** opts = NULL;
	gchar** opt = NULL;

	/* Value to read from current option. */
	guint value;

	/* Check options. */
	if (options) {
		opts = g_strsplit_set(options, ",", -1);
		for (guint i = 0; opts[i] != NULL; i++) {

			/* Ignore empty tokens. */
			if (opts[i][0] == '\0') continue;

			/* Parse current option, get key and value. */
			opt = g_strsplit_set(opts[i], "=", 2);

			/* Count number of tokens. */
			for (num_toks = 0; opt[num_toks] != NULL; num_toks++);

			/* If number of tokens is not 2 (key and value), throw error. */
			ccl_if_err_create_goto(*err, CLO_ERROR, num_toks != 2,
				CLO_ERROR_ARGS, error_handler,
				"Invalid option '%s' for abitonic sort.", opts[i]);

			/* Get option value. */
			value = atoi(opt[1]);

			/* Check key/value option. */
			if (g_strcmp0("radix", opt[0]) == 0) {
				/* Radix. */
				ccl_if_err_create_goto(*err, CLO_ERROR,
					clo_ones32(value) != 1, CLO_ERROR_ARGS,
					error_handler,
					"Radix must be a power of 2.");
				data->radix = value;
			} else {
				ccl_if_err_create_goto(*err, CLO_ERROR, TRUE,
					CLO_ERROR_ARGS, error_handler,
					"Invalid option key '%s' for satradix sort.",
					opt[0]);
			}

			/* Free token. */
			g_strfreev(opt);
			opt = NULL;

		}

	}

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	satradix_src = CLO_SORT_SATRADIX_SRC;
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	satradix_src = NULL;

finish:

	/* Free parsed a-bitonic options. */
	g_strfreev(opts);
	g_strfreev(opt);

	/* Set object methods and internal data. */
	clo_sort_set_data(sorter, data);

	/* Return source to be compiled. */
	return satradix_src;

}


/**
 * Finalizes a SatRadix sorter object.
 *
 * @copydetails ::CloSort::finalize()
 * */
static void clo_sort_satradix_finalize(CloSort* sorter) {

	/* Release internal data. */
	g_slice_free(clo_sort_satradix_data, clo_sort_get_data(sorter));

	return;
}

/* Definition of the satradix sort implementation. */
const CloSortImplDef clo_sort_satradix_def = {
	"satradix",
	CL_TRUE,
	clo_sort_satradix_init,
	clo_sort_satradix_finalize,
	clo_sort_satradix_sort_with_device_data
};

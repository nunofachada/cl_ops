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
#include "clo_scan_abstract.h"

/* The default scan implementation. */
#define CLO_SORT_SATRADIX_SCAN_DEFAULT "blelloch"

typedef struct {

	/** Radix. */
	cl_uint radix;

	/** Source code. */
	char* src;

	/** Scanner type. */
	char* scan_type;

	/** Scanner options. */
	char* scan_opts;

	/** Scanner object. */
	CloScan* scanner;

} clo_sort_satradix_data;


/**
 * @internal
 * Get scanner object used for radix sort.
 *
 * @param[in] sorter Sorter object (radix sort).
 * @return Scanner object used for radix sort.
 * */
static CloScan* clo_sort_satradix_get_scanner(
	CloSort* sorter, GError** err) {

	/* Variables. */
	CCLContext* ctx = NULL;
	CCLProgram* prg = NULL;
	CCLDevice* dev = NULL;
	char* compiler_opts = NULL;
	GError* err_internal = NULL;

	/* Get radix sort parameters. */
	clo_sort_satradix_data* data =
		(clo_sort_satradix_data*) clo_sort_get_data(sorter);

	/* Check if scanner object was already created. */
	if (data->scanner == NULL) {

		/* If not, create it. */

		/* Get context, program and device. */
		ctx = clo_sort_get_context(sorter);
		prg = clo_sort_get_program(sorter);
		dev = ccl_program_get_device(prg, 0, &err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);

		/* Get sorter compiler options, which will also be used for
		 * the scanner. */
		compiler_opts = ccl_program_get_build_info_array(
			prg, dev, CL_PROGRAM_BUILD_OPTIONS, char*, &err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);

		/* Create scanner object. */
		data->scanner = clo_scan_new(data->scan_type, data->scan_opts,
			ctx, CLO_UINT, CLO_UINT, compiler_opts, &err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
	}

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);

finish:

	/* Return. */
	return data->scanner;
}

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
	CCLProgram* prg = NULL;
	CCLDevice* dev = NULL;
	CCLBuffer* data_aux = NULL;
	CCLBuffer* offsets = NULL;
	CCLBuffer* counters = NULL;
	CCLBuffer* counters_sum = NULL;
	CCLEvent* evt = NULL;
	CCLKernel* krnl_lsrt = NULL;
	CCLKernel* krnl_hist = NULL;
	CCLKernel* krnl_scat = NULL;

	/* Scanner object, required for part of the radix sort. */
	CloScan* scanner = NULL;

	/* Effective local worksize for the several radix sort kernels. */
	size_t lws_sort;
	/* Number of workgroups for the several radix sort kernels. */
	size_t num_wgs;
	/* Size of aux. buffers. */
	size_t aux_buf_size;
	/* Bits in digit and total digits, depend on the radix. */
	cl_uint bits_in_digit, total_digits;

	/* Event wait list. */
	CCLEventWaitList ewl = NULL;

	/* Internal error reporting object. */
	GError* err_internal = NULL;

	/* Get radix sort parameters. */
	clo_sort_satradix_data data =
		*((clo_sort_satradix_data*) clo_sort_get_data(sorter));

	/* Determine bits in digit and total digits. */
	bits_in_digit = clo_tzc(data.radix);
	total_digits =
		clo_sort_get_element_size(sorter) * 8 / bits_in_digit;

	/* If data transfer queue is NULL, use exec queue for data
	 * transfers. */
	if (cq_comm == NULL) cq_comm = cq_exec;

	/* Get device where sort will occurr. */
	dev = ccl_queue_get_device(cq_exec, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Determine the effective local worksize for the several radix
	 * sort kernels... */
	lws_sort = lws_max;
	ccl_kernel_suggest_worksizes(
		NULL, dev, 1, &numel, NULL, &lws_sort, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	/* ...and it can't be smaller than the radix itself. */
	lws_sort = MAX(lws_sort, data.radix);

	/* Determine the number of workgroups for the several radix sort
	 * kernels. */
	num_wgs = numel / lws_sort + numel % lws_sort;

	/* Get context and program. */
	ctx = clo_sort_get_context(sorter);
	prg = clo_sort_get_program(sorter);

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

		ccl_event_set_name(evt, "copy_satradix");
		ccl_event_wait_list_add(&ewl, evt, NULL);
	}

	/* Get kernels. */
	krnl_lsrt = ccl_program_get_kernel(
		prg, "satradix_localsort", &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	krnl_hist = ccl_program_get_kernel(
		prg, "satradix_histogram", &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	krnl_scat = ccl_program_get_kernel(
		prg, "satradix_scatter", &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

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

	/* Get scanner object. */
	scanner = clo_sort_satradix_get_scanner(sorter, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Perform sort. */
	for (cl_uint i = 0; i < total_digits; ++i) {

		cl_uint start_bit = i * bits_in_digit;
		cl_uint array_len = numel / num_wgs;

		/* Local sort. */
		evt = ccl_kernel_set_args_and_enqueue_ndrange(krnl_lsrt,
			cq_exec, 1, NULL, &numel, &lws_sort, NULL, &err_internal,
			data_out, data_aux,
			ccl_arg_full(NULL, array_len * clo_sort_get_element_size(sorter)),
			ccl_arg_local(array_len, cl_uint),
			ccl_arg_priv(start_bit, cl_uint),
			NULL);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
		ccl_event_set_name(evt, "localsort_satradix");

		/* Histogram. */
		evt = ccl_kernel_set_args_and_enqueue_ndrange(krnl_hist, cq_exec, 1,
			NULL, &numel, &lws_sort, NULL, &err_internal,
			data_aux, offsets, counters,
			ccl_arg_local(data.radix, cl_uint),
			ccl_arg_local(data.radix, cl_uint),
			ccl_arg_full(NULL, array_len * clo_sort_get_key_size(sorter)),
			ccl_arg_priv(start_bit, cl_uint),
			ccl_arg_priv(array_len, cl_uint),
			NULL);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
		ccl_event_set_name(evt, "histogram_satradix");

		/* Scan. */
		clo_scan_with_device_data(scanner, cq_exec, cq_comm, counters,
			counters_sum, num_wgs * data.radix, lws_max, &err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);

		/* Scatter. */
		evt = ccl_kernel_set_args_and_enqueue_ndrange(krnl_scat,
			cq_exec, 1, NULL, &numel, &lws_sort, NULL, &err_internal,
			data_in, data_aux, offsets, counters_sum,
			ccl_arg_full(NULL, array_len * clo_sort_get_element_size(sorter)),
			ccl_arg_local(data.radix, cl_uint),
			ccl_arg_local(data.radix, cl_uint),
			ccl_arg_priv(start_bit, cl_uint),
			NULL);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
		ccl_event_set_name(evt, "scatter_satradix");
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

	char* satradix_src = NULL;
	clo_sort_satradix_data* data = NULL;
	data = g_slice_new0(clo_sort_satradix_data);

	/* Set internal data default values. */
	data->radix = 16;

	/* Number of tokens. */
	int num_toks;

	/* Tokenized options. */
	gchar** opts = NULL;
	gchar** opt = NULL;

	/* Scan options. */
	GString* scan_opts = g_string_new("");

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

			/* Check key/value option. */
			if (g_strcmp0("radix", opt[0]) == 0) {
				/* Get option value. */
				data->radix = atoi(opt[1]);
				/* Radix. */
				ccl_if_err_create_goto(*err, CLO_ERROR,
					clo_ones32(data->radix) != 1, CLO_ERROR_ARGS,
					error_handler,
					"Radix must be a power of 2.");
			} else if (g_ascii_strncasecmp("scan", opt[0], 4) == 0) {
				/* Its a scanner option, analyse it. */

				/// @todo Test this code better.

				if (strlen(opt[0]) == 4) {
					/* It's the type of scan. */
					data->scan_type = g_strdup(opt[1]);
				} else {
					/* It's some other scan option. */
					g_string_append_printf(
						scan_opts, "%s,", opts[i] + 4);
				}


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
	satradix_src = g_strdup_printf("#define CLO_SORT_NUM_BITS %d\n%s",
		clo_tzc(data->radix), CLO_SORT_SATRADIX_SRC);
	data->src = satradix_src;
	if (data->scan_type == NULL)
		data->scan_type = g_strdup(CLO_SORT_SATRADIX_SCAN_DEFAULT);
	data->scan_opts = scan_opts->str;
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	satradix_src = NULL;
	data->src = NULL;

finish:

	/* Free parsed a-bitonic options. */
	g_strfreev(opts);
	g_strfreev(opt);
	g_string_free(scan_opts, FALSE);

	/* Set object methods and internal data. */
	clo_sort_set_data(sorter, data);

	/* Return source to be compiled. */
	return (const char*) satradix_src;

}

/**
 * Finalizes a SatRadix sorter object.
 *
 * @copydetails ::CloSort::finalize()
 * */
static void clo_sort_satradix_finalize(CloSort* sorter) {

	/* Get internal data. */
	clo_sort_satradix_data* data =
		(clo_sort_satradix_data*) clo_sort_get_data(sorter);

	/* Release internal data. */
	g_free(data->src);
	g_free(data->scan_type);
	if (data->scan_opts) g_free(data->scan_opts);
	if (data->scanner) clo_scan_destroy(data->scanner);
	g_slice_free(clo_sort_satradix_data, data);

	return;
}

/**
 * Get the maximum number of kernels used by the sort implementation.
 *
 * @copydetails ::CloSort::get_num_kernels()
 * */
static cl_uint clo_sort_satradix_get_num_kernels(CloSort* sorter) {

	/* Avoid compiler warnings. */
	(void)sorter;

	/* Return number of kernels. */
	return 3;

	/** @todo Take into account the number of scan kernel.
	 * We have to ask the scan implementation about it. */

}

/**
 * Get name of the i^th kernel used by the sort implementation.
 *
 * @copydetails ::CloSort::get_kernel_name()
 * */
const char* clo_sort_satradix_get_kernel_name(
	CloSort* sorter, cl_uint i) {

	/* Avoid compiler warnings. */
	(void)sorter;
	(void)i;

	/* Return kernel name. */
	return NULL;
}

/**
 * Get local memory usage of i^th kernel used by the sort implementation
 * for the given maximum local worksize and number of elements to sort.
 *
 * @copydetails ::CloSort::get_localmem_usage()
 * */
size_t clo_sort_satradix_get_localmem_usage(CloSort* sorter, cl_uint i,
	size_t lws_max, size_t numel, GError** err) {

	/* Avoid compiler warnings. */
	(void)sorter;
	(void)i;
	(void)lws_max;
	(void)numel;
	(void)err;

	/* Return local memory usage. */
	return 0;

}

/* Definition of the satradix sort implementation. */
const CloSortImplDef clo_sort_satradix_def = {
	"satradix",
	CL_TRUE,
	clo_sort_satradix_init,
	clo_sort_satradix_finalize,
	clo_sort_satradix_sort_with_device_data,
	clo_sort_satradix_get_num_kernels,
	clo_sort_satradix_get_kernel_name,
	clo_sort_satradix_get_localmem_usage
};

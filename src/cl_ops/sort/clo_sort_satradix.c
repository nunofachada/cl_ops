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
 * Satish et al Radix Sort host implementation.
 */

#include "cl_ops/clo_sort_satradix.h"
#include "cl_ops/clo_scan_abstract.h"
#include "common/_g_err_macros.h"

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


/* Array of kernel names. */
static const char* clo_sort_satradix_knames[] =
	CLO_SORT_SATRADIX_KERNELNAMES;

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
		g_if_err_propagate_goto(err, err_internal, error_handler);

		/* Get sorter compiler options, which will also be used for
		 * the scanner. */
		compiler_opts = ccl_program_get_build_info_array(
			prg, dev, CL_PROGRAM_BUILD_OPTIONS, char*, &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);

		/* Create scanner object. */
		data->scanner = clo_scan_new(data->scan_type, data->scan_opts,
			ctx, CLO_UINT, CLO_UINT, compiler_opts, &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);
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
 * @internal
 * Perform sort using device data.
 * */
static CCLEvent* clo_sort_satradix_sort_with_device_data(
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

	/* Effective number of elements to sort, appropriate for the
	 * radix sort. */
	 size_t numel_eff;
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

	g_debug("SATRADIX: radix=%d (bits_in_digit=%d)",
		data.radix, bits_in_digit);

	/* If data transfer queue is NULL, use exec queue for data
	 * transfers. */
	if (cq_comm == NULL) cq_comm = cq_exec;

	/* Get device where sort will occurr. */
	dev = ccl_queue_get_device(cq_exec, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Determine the effective local worksize for the several radix
	 * sort kernels... */
	lws_sort = lws_max;
	numel_eff = clo_nlpo2(numel);
	ccl_kernel_suggest_worksizes(
		NULL, dev, 1, &numel_eff, NULL, &lws_sort, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);
	/* ...and it can't be smaller than the radix itself. */
	lws_sort = MAX(lws_sort, data.radix);

	g_debug("SATRADIX: numel=%d,  gws=%d, lws=%d",
		(int) numel, (int) numel_eff, (int) lws_sort);

	/* Determine the number of workgroups for the several radix sort
	 * kernels. */
	num_wgs = numel_eff / lws_sort + numel_eff % lws_sort;

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
			clo_sort_get_element_size(sorter) * numel_eff, NULL,
			&err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);

		ccl_event_set_name(evt, "satradix_copy");
		ccl_event_wait_list_add(&ewl, evt, NULL);
	}

	/* Get kernels. */

	/// @todo Maybe set and get kernels in satradix data so were
	/// not always searching for them in a hash table

	krnl_lsrt = ccl_program_get_kernel(
		prg, CLO_SORT_SATRADIX_KNAME_LOCALSORT, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);
	krnl_hist = ccl_program_get_kernel(
		prg, CLO_SORT_SATRADIX_KNAME_HISTOGRAM, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);
	krnl_scat = ccl_program_get_kernel(
		prg, CLO_SORT_SATRADIX_KNAME_SCATTER, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Determine size of aux. buffers. */
	aux_buf_size = num_wgs * data.radix * sizeof(cl_uint);

	/* Allocate auxiliary device buffers. */

	/// @todo Somehow reuse the aux. buffers between calls so that
	/// they aren't constantly being created and destroyed.

	data_aux = ccl_buffer_new(
		ctx, CL_MEM_READ_WRITE, numel_eff * clo_sort_get_element_size(sorter),
		NULL, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	offsets = ccl_buffer_new(
		ctx, CL_MEM_READ_WRITE, aux_buf_size, NULL, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	counters = ccl_buffer_new(
		ctx, CL_MEM_READ_WRITE, aux_buf_size, NULL, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	counters_sum = ccl_buffer_new(
		ctx, CL_MEM_READ_WRITE, aux_buf_size, NULL, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Get scanner object. */
	scanner = clo_sort_satradix_get_scanner(sorter, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Perform sort. */
	for (cl_uint i = 0; i < total_digits; ++i) {

		cl_uint start_bit = i * bits_in_digit;
		cl_uint array_len = numel_eff / num_wgs;

		/// @todo I don't need to be setting kernel arguments over and
		/// over again, so set the constant arguments before entering
		/// the loop.

		/* Local sort. */
		evt = ccl_kernel_set_args_and_enqueue_ndrange(krnl_lsrt,
			cq_exec, 1, NULL, &numel_eff, &lws_sort, NULL, &err_internal,
			data_out, data_aux,
			ccl_arg_full(NULL, array_len * clo_sort_get_element_size(sorter)),
			ccl_arg_local(array_len, cl_uint),
			ccl_arg_priv(start_bit, cl_uint),
			NULL);
		g_if_err_propagate_goto(err, err_internal, error_handler);
		ccl_event_set_name(evt, "satradix_localsort");

		/* Histogram. */
		evt = ccl_kernel_set_args_and_enqueue_ndrange(krnl_hist, cq_exec, 1,
			NULL, &numel_eff, &lws_sort, NULL, &err_internal,
			data_aux, offsets, counters,
			ccl_arg_local(data.radix, cl_uint),
			ccl_arg_local(data.radix, cl_uint),
			ccl_arg_full(NULL, array_len * clo_sort_get_key_size(sorter)),
			ccl_arg_priv(start_bit, cl_uint),
			ccl_arg_priv(array_len, cl_uint),
			NULL);
		g_if_err_propagate_goto(err, err_internal, error_handler);
		ccl_event_set_name(evt, "satradix_histogram");

		/* Scan. */
		clo_scan_with_device_data(scanner, cq_exec, cq_comm, counters,
			counters_sum, num_wgs * data.radix, lws_max, &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);

		/* Scatter. */
		evt = ccl_kernel_set_args_and_enqueue_ndrange(krnl_scat,
			cq_exec, 1, NULL, &numel_eff, &lws_sort, NULL, &err_internal,
			data_in, data_aux, offsets, counters_sum,
			ccl_arg_full(NULL, array_len * clo_sort_get_element_size(sorter)),
			ccl_arg_local(data.radix, cl_uint),
			ccl_arg_local(data.radix, cl_uint),
			ccl_arg_priv(start_bit, cl_uint),
			NULL);
		g_if_err_propagate_goto(err, err_internal, error_handler);
		ccl_event_set_name(evt, "satradix_scatter");
	}

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	evt = NULL;

finish:

	/* Free stuff. */
	ccl_buffer_destroy(data_aux);
	ccl_buffer_destroy(offsets);
	ccl_buffer_destroy(counters);
	ccl_buffer_destroy(counters_sum);

	/* Return. */
	return evt;

}

/**
 * @internal
 * Initializes a SatRadix sorter object and returns the
 * respective source code.
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
			g_if_err_create_goto(*err, CLO_ERROR, num_toks != 2,
				CLO_ERROR_ARGS, error_handler,
				"Invalid option '%s' for abitonic sort.", opts[i]);

			/* Check key/value option. */
			if (g_strcmp0("radix", opt[0]) == 0) {
				/* Get option value. */
				data->radix = atoi(opt[1]);
				/* Radix. */
				g_if_err_create_goto(*err, CLO_ERROR,
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
				g_if_err_create_goto(*err, CLO_ERROR, TRUE,
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
 * @internal
 * Get the maximum number of kernels used by the sort implementation.
 * */
static cl_uint clo_sort_satradix_get_num_kernels(
	CloSort* sorter, GError** err) {

	/* Scanner object associated with the satradix sort. */
	CloScan* scanner = NULL;
	/* Internal error handling object. */
	GError* err_internal = NULL;
	/* Number of kernels. */
	cl_uint num_kernels = 0;

	/* Get associated scan implementation. */
	scanner = clo_sort_satradix_get_scanner(sorter, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Determine number of kernels: satradix kernels + scan kernels. */
	num_kernels = CLO_SORT_SATRADIX_NUM_KERNELS
		+ clo_scan_get_num_kernels(scanner, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);

finish:

	/* Return number of kernels. */
	return num_kernels;

}

/**
 * @internal
 * Get name of the i^th kernel used by the sort implementation.
 * */
static const char* clo_sort_satradix_get_kernel_name(
	CloSort* sorter, cl_uint i, GError** err) {

	/* Scanner object associated with the satradix sort. */
	CloScan* scanner = NULL;
	/* Number of kernels. */
	cl_uint num_kernels = 0;
	/* Kernel name. */
	const char* kernel_name = NULL;
	/* Internal error handling object. */
	GError* err_internal = NULL;

	/* Get number of kernels. */
	num_kernels =
		clo_sort_satradix_get_num_kernels(sorter, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Check that it's within bounds. */
	g_return_val_if_fail(i < num_kernels, NULL);

	/* Get associated scan implementation. */
	scanner = clo_sort_satradix_get_scanner(sorter, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Check if the requested kernel name is from the satradix sort or
	 * from the scan implementation. */
	if (i < CLO_SORT_SATRADIX_NUM_KERNELS) {

		/* It's a satradix kernel. */
		kernel_name = clo_sort_satradix_knames[i];

	} else {

		/* It's a scan kernel. */
		kernel_name = clo_scan_get_kernel_name(scanner,
			i - CLO_SORT_SATRADIX_NUM_KERNELS, &err_internal);
		g_if_err_propagate_goto(err, err_internal, error_handler);

	}

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	kernel_name = NULL;

finish:
	/* Return kernel name. */
	return kernel_name;
}

/**
 * @internal
 * Get local memory usage of i^th kernel used by the sort implementation
 * for the given maximum local worksize and number of elements to sort.
 * */
static size_t clo_sort_satradix_get_localmem_usage(CloSort* sorter,
	cl_uint i, size_t lws_max, size_t numel, GError** err) {

	/* Scanner object associated with the satradix sort. */
	CloScan* scanner = NULL;
	/* Number of kernels. */
	cl_uint num_kernels = 0;
	/* Local memory usage. */
	size_t local_mem_usage;
	/* Internal error handling object. */
	GError* err_internal = NULL;
	/* Work sizes. */
	size_t lws_sort, numel_eff, num_wgs;
	/* Device where sort will occur. */
	CCLDevice* dev = NULL;

	/* Get number of kernels. */
	num_kernels =
		clo_sort_satradix_get_num_kernels(sorter, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Check that it's within bounds. */
	g_return_val_if_fail(i < num_kernels, 0);

	/* Get device where sort will occurr. */
	dev = ccl_context_get_device(
		clo_sort_get_context(sorter), 0, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* Get radix sort parameters. */
	clo_sort_satradix_data data =
		*((clo_sort_satradix_data*) clo_sort_get_data(sorter));

	/* Determine the effective local worksize for the several radix
	 * sort kernels... */
	lws_sort = lws_max;
	numel_eff = clo_nlpo2(numel);
	ccl_kernel_suggest_worksizes(
		NULL, dev, 1, &numel_eff, NULL, &lws_sort, &err_internal);
	g_if_err_propagate_goto(err, err_internal, error_handler);

	/* ...and it can't be smaller than the radix itself. */
	lws_sort = MAX(lws_sort, data.radix);

	/* Determine the number of workgroups for the several radix sort
	 * kernels. */
	num_wgs = numel_eff / lws_sort + numel_eff % lws_sort;

	/* Check if the local memory usage is for a satradix kernel or
	 * for a scan kernel. */
	switch (i) {

		case CLO_SORT_SATRADIX_KIDX_LOCALSORT:
			/* It's for the satradix localsort kernel. */
			local_mem_usage =
				(numel_eff / num_wgs) * clo_sort_get_element_size(sorter)
				+
				(numel_eff / num_wgs) * sizeof(cl_uint);
			break;
		case CLO_SORT_SATRADIX_KIDX_HISTOGRAM:
			/* It's for the satradix histogram kernel. */
			local_mem_usage = 2 * data.radix * sizeof(cl_uint)
				+
				(numel_eff / num_wgs) * clo_sort_get_key_size(sorter);
			break;
		case CLO_SORT_SATRADIX_KIDX_SCATTER:
			/* It's for the satradix scatter kernel. */
			local_mem_usage =
				(numel_eff / num_wgs) * clo_sort_get_element_size(sorter)
				+
				2 * data.radix * sizeof(cl_uint);
			break;
		default:
			/* It's for a scan kernel. */
			/* Get associated scan implementation. */
			scanner = clo_sort_satradix_get_scanner(
				sorter, &err_internal);
			g_if_err_propagate_goto(err, err_internal, error_handler);
			/* Determine scan kernel local memory usage. */
			local_mem_usage = clo_scan_get_localmem_usage(scanner,
				i - CLO_SORT_SATRADIX_NUM_KERNELS, lws_sort,
				num_wgs * data.radix, &err_internal);
			g_if_err_propagate_goto(err, err_internal, error_handler);
	}

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	local_mem_usage = 0;

finish:

	/* Return local memory usage. */
	return local_mem_usage;

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

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
 * Advanced bitonic sort host implementation.
 */

#include "clo_sort_abitonic.h"

typedef struct {

	/** Maximum number of in-kernel steps for private memory kernels. */
	cl_uint max_inkrnl_stps;

	/** Minimum number of in-kernel steps for private memory kernels. */
	cl_uint min_inkrnl_stps;

	/** Maximum in-kernel "stage finish" step. */
	cl_uint max_inkrnl_sfs;

} clo_sort_abitonic_data;

typedef struct {
	CCLKernel* krnl;
	const char* krnl_name;
	size_t local_mem;
	size_t gws;
	size_t lws;
	gboolean set_step;
	guint num_steps;
} clo_sort_abitonic_step;

/**
 * @internal
 * Determine abitonic sort strategy for the given parameters.
 *
 * */
static clo_sort_abitonic_step* clo_sort_abitonic_get_strategy(
	CCLProgram* prg, CCLDevice* dev, clo_sort_abitonic_data data,
	size_t lws_max, cl_uint numel, GError** err) {

	/* Make sure err is NULL or it is not set. */
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	/* Kernels applicable to each step. */
	static const char* lookup[11][4] = {
		/* Step 2 */
		{
			CLO_SORT_ABITONIC_KNAME_LOCAL_S2,
			NULL, NULL, NULL
		},
		/* Step 3 */
		{
			CLO_SORT_ABITONIC_KNAME_HYB_S3_3S8V,
			CLO_SORT_ABITONIC_KNAME_LOCAL_S3,
			NULL, NULL
		},
		/* Step 4 */
		{
			CLO_SORT_ABITONIC_KNAME_HYB_S4_4S16V,
			CLO_SORT_ABITONIC_KNAME_HYB_S4_2S4V,
			CLO_SORT_ABITONIC_KNAME_LOCAL_S4,
			NULL
		},
		/* Step 5 */
		{
			CLO_SORT_ABITONIC_KNAME_LOCAL_S5,
			NULL, NULL, NULL
		},
		/* Step 6 */
		{
			CLO_SORT_ABITONIC_KNAME_HYB_S6_3S8V,
			CLO_SORT_ABITONIC_KNAME_HYB_S6_2S4V,
			CLO_SORT_ABITONIC_KNAME_LOCAL_S6,
			NULL
		},
		/* Step 7 */
		{
			CLO_SORT_ABITONIC_KNAME_LOCAL_S7,
			NULL, NULL, NULL
		},
		/* Step 8 */
		{
			CLO_SORT_ABITONIC_KNAME_HYB_S8_4S16V,
			CLO_SORT_ABITONIC_KNAME_HYB_S8_2S4V,
			CLO_SORT_ABITONIC_KNAME_LOCAL_S8,
			NULL
		},
		/* Step 9 */
		{
			CLO_SORT_ABITONIC_KNAME_HYB_S9_3S8V,
			CLO_SORT_ABITONIC_KNAME_LOCAL_S9,
			NULL, NULL
		},
		/* Step 10 */
		{
			CLO_SORT_ABITONIC_KNAME_HYB_S10_2S4V,
			CLO_SORT_ABITONIC_KNAME_LOCAL_S10,
			NULL, NULL
		},
		/* Step 11 */
		{
			CLO_SORT_ABITONIC_KNAME_LOCAL_S11,
			NULL, NULL, NULL
		},
		/* Step 12 */
		{
			CLO_SORT_ABITONIC_KNAME_HYB_S12_4S16V,
			CLO_SORT_ABITONIC_KNAME_HYB_S12_3S8V,
			CLO_SORT_ABITONIC_KNAME_HYB_S12_2S4V,
			NULL
		},
	};

	/* Next power of two of the number of elements. */
	cl_uint numel_nlpo2 = (cl_uint) clo_nlpo2(numel);
	/* Total stages. */
	cl_uint tot_stages = (cl_uint) clo_tzc(numel_nlpo2);
	/* Array of steps. */
	clo_sort_abitonic_step* steps =
		g_new(clo_sort_abitonic_step, tot_stages);
	/* Internal error handling object. */
	GError* err_internal = NULL;

	/* Get a maximum lws for "private" kernels if step > max_inkrnl_sfs.
	 * The 1 << 20 is just a "high enough" gws value, i.e. higher than
	 * the max lws supported by the device. */
	size_t lws_max_sfs = clo_get_lws(NULL, dev, 1 << 20, lws_max, &err_internal);

	/* Determine effective maximum in-kernel "stage finish" step. */
	data.max_inkrnl_sfs = MIN(MIN(12, data.max_inkrnl_sfs), clo_tzc(lws_max_sfs) + data.max_inkrnl_stps);

	/* Build strategy. */
	for (guint step = 1; step <= tot_stages; step++) {
		if (step == 1) {
			/* Step 1 requires the "any" kernel. */
			steps[step - 1].krnl_name = CLO_SORT_ABITONIC_KNAME_ANY;
			steps[step - 1].krnl = ccl_program_get_kernel(
				prg, CLO_SORT_ABITONIC_KNAME_ANY, &err_internal);
			ccl_if_err_propagate_goto(err, err_internal, error_handler);
			steps[step - 1].gws = numel_nlpo2 / 2;
			steps[step - 1].lws = clo_get_lws(steps[step - 1].krnl, dev,
				steps[step - 1].gws, lws_max, &err_internal);
			ccl_if_err_propagate_goto(err, err_internal, error_handler);
			steps[step - 1].set_step = TRUE;
			steps[step - 1].num_steps = 1;
		} else if (step > data.max_inkrnl_sfs) {
			/* For steps higher than the maximum in-kernel "stage
			 * finish" step, e.g., max_inkrnl_sfs. */

			/* No local memory required by "any" and "private" kernels. */
			steps[step - 1].local_mem = 0;

			/* This may be required when max_inkrnl_sfs < 4 in order to
			 * avoid overshooting the target step 0. */
			cl_uint step_margin = MIN(step, data.max_inkrnl_stps);

			/* Chose proper kernel. */
			switch (step_margin) {
				case 4:
					steps[step - 1].krnl_name =
						CLO_SORT_ABITONIC_KNAME_PRIV_4S16V;
					steps[step - 1].krnl =
						ccl_program_get_kernel(
							prg, CLO_SORT_ABITONIC_KNAME_PRIV_4S16V,
							&err_internal);
					break;
				case 3:
					steps[step - 1].krnl_name =
						CLO_SORT_ABITONIC_KNAME_PRIV_3S8V;
					steps[step - 1].krnl =
						ccl_program_get_kernel(
							prg, CLO_SORT_ABITONIC_KNAME_PRIV_3S8V,
							&err_internal);
					break;
				case 2:
					steps[step - 1].krnl_name =
						CLO_SORT_ABITONIC_KNAME_PRIV_2S4V;
					steps[step - 1].krnl =
						ccl_program_get_kernel(
							prg, CLO_SORT_ABITONIC_KNAME_PRIV_2S4V,
							&err_internal);
					break;
				case 1:
					steps[step - 1].krnl_name =
						CLO_SORT_ABITONIC_KNAME_ANY;
					steps[step - 1].krnl =
						ccl_program_get_kernel(
							prg, CLO_SORT_ABITONIC_KNAME_ANY,
							&err_internal);
					break;
				default:
					g_assert_not_reached();
			}
			ccl_if_err_propagate_goto(err, err_internal, error_handler);
			steps[step - 1].gws = numel_nlpo2 / (1 << step_margin);
			steps[step - 1].lws = MIN(lws_max_sfs, steps[step - 1].gws);
			ccl_if_err_propagate_goto(err, err_internal, error_handler);
			steps[step - 1].set_step = TRUE;
			steps[step - 1].num_steps = step_margin;
		} else {
			/* For steps equal or smaller than the maximum in-kernel
			 * "stage finish" step. */
			const char** possible_krnls = lookup[step - 2];
			cl_uint priv_steps;
			gboolean found = FALSE;
			/* Find proper kernel for current step by looking at
			 * kernel lookup table. */
			for (cl_uint i = 0; possible_krnls[i] != NULL; ++i) {

				/* Get current possible kernel name. */
				const char* krnl_name = possible_krnls[i];

				/* Check if it's a hybrid or local kernel. */
				if (g_strrstr(krnl_name, CLO_SORT_ABITONIC_KNAME_HYB_MARK)) {
					/* It's a hybrid kernel, determine how many steps
					 * does it perform on private memory each time. */
					priv_steps = CLO_SORT_ABITONIC_KPARSE_S(krnl_name);
					steps[step - 1].local_mem =
						CLO_SORT_ABITONIC_KPARSE_V(krnl_name);
				} else {
					/* It's a local kernel, thus only one step is
					 * performed in private memory. */
					priv_steps = 1;
					steps[step - 1].local_mem = 2;
				}
				/* IF within interval of acceptable in-kernel private
				 * memory steps AND IF required LWS within limits of
				 * maximum LWS, THEN chose this kernel. */
				steps[step - 1].gws =
					clo_nlpo2(numel) / (1 << priv_steps);
				steps[step - 1].lws = clo_get_lws(NULL, dev,
					steps[step - 1].gws, lws_max, &err_internal);
				ccl_if_err_propagate_goto(
					err, err_internal, error_handler);

				if ((priv_steps <= data.max_inkrnl_stps)
					&& (priv_steps >= data.min_inkrnl_stps)
					&& (((int) steps[step - 1].lws)
						>= (1 << (step - priv_steps))))
				{
					steps[step - 1].krnl_name = krnl_name;
					steps[step - 1].krnl = ccl_program_get_kernel(
						prg, krnl_name, &err_internal);
					ccl_if_err_propagate_goto(
						err, err_internal, error_handler);
					steps[step - 1].set_step = FALSE;
					steps[step - 1].num_steps = step;
					found = TRUE;
					break;
				}
			}
			/* If no kernel was selected, use the "any" kernel to advance one step. */
			if (!found) {
				steps[step - 1].krnl_name = CLO_SORT_ABITONIC_KNAME_ANY;
				steps[step - 1].krnl = ccl_program_get_kernel(
					prg, CLO_SORT_ABITONIC_KNAME_ANY, &err_internal);
				ccl_if_err_propagate_goto(
					err, err_internal, error_handler);
				steps[step - 1].gws = clo_nlpo2(numel) / 2;
				steps[step - 1].lws = clo_get_lws(steps[step - 1].krnl,
					dev, steps[step - 1].gws, lws_max, &err_internal);
				ccl_if_err_propagate_goto(err, err_internal, error_handler);
				steps[step - 1].set_step = TRUE;
				steps[step - 1].num_steps = 1;
				steps[step - 1].local_mem = 0;
			}
		}
	}

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);

finish:

	return steps;
}

/**
 * @internal
 * Perform sort using device data.
 *
 * @copydetails ::CloSort::sort_with_device_data()
 * */
static CCLEventWaitList clo_sort_abitonic_sort_with_device_data(
	CloSort* sorter, CCLQueue* cq_exec, CCLQueue* cq_comm,
	CCLBuffer* data_in, CCLBuffer* data_out, size_t numel,
	size_t lws_max, GError** err) {

	/* Make sure err is NULL or it is not set. */
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	/* Make sure cq_exec is not NULL. */
	g_return_val_if_fail(cq_exec != NULL, NULL);

	/* Number of bitonic sort stages. */
	cl_uint tot_stages;

	/* Implementation of the strategy to follow on each step. */
	clo_sort_abitonic_step* steps = NULL;

	clo_sort_abitonic_data data =
		*((clo_sort_abitonic_data*) clo_sort_get_data(sorter));

	/* OpenCL object wrappers. */
	CCLDevice* dev;
	CCLEvent* evt;
	CCLProgram* prg;

	/* Event wait list. */
	CCLEventWaitList ewl = NULL;

	/* Internal error reporting object. */
	GError* err_internal = NULL;

	/* Get the program. */
	prg = clo_sort_get_program(sorter);

	/* If data transfer queue is NULL, use exec queue for data
	 * transfers. */
	if (cq_comm == NULL) cq_comm = cq_exec;

	/* Get device where sort will occurr. */
	dev = ccl_queue_get_device(cq_exec, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

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

		ccl_event_set_name(evt, "copy_abitonic");
		ccl_event_wait_list_add(&ewl, evt, NULL);
	}

	/* Determine number of bitonic sort stages. */
	tot_stages = (cl_uint) clo_tzc(clo_nlpo2(numel));

	/* Obtain sorting strategy, e.g., which kernels to use in each step. */
	steps = clo_sort_abitonic_get_strategy(
		prg, dev, data, lws_max, numel, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Set kernel arguments. */
	for (cl_uint i = 0; i < tot_stages; ++i) {

		ccl_kernel_set_arg(steps[i].krnl, 0, data_in);

		if (steps[i].local_mem > 0) {
			ccl_kernel_set_arg(steps[i].krnl, 2,
				ccl_arg_full(NULL, clo_sort_get_element_size(sorter)
					* steps[i].lws * steps[i].local_mem));
		}
	}


	g_debug("********* NUMEL = %d *******", (int) numel);

	/* Perform sorting. */
	for (cl_uint curr_stage = 1; curr_stage <= tot_stages; curr_stage++) {
		for (cl_uint curr_step = curr_stage; curr_step >= 1; ) {

			/* Get strategy for current step. */
			clo_sort_abitonic_step stp_strat = steps[curr_step - 1];

			g_debug("Stage %d, Step %d | %s [GWS=%d, LWS=%d, NSTEPS=%d]",
				curr_stage, curr_step, stp_strat.krnl_name,
				(int) stp_strat.gws, (int) stp_strat.lws, stp_strat.num_steps);

			/* Set kernel arguments. */
			/* Current stage. */
			ccl_kernel_set_arg(
				stp_strat.krnl, 1, ccl_arg_priv(curr_stage, cl_uint));

			/* Current step (for some kernels only). */
			if (stp_strat.set_step)
				ccl_kernel_set_arg(stp_strat.krnl, 2,
					ccl_arg_priv(curr_step, cl_uint));

			/* Execute kernel. */
			evt = ccl_kernel_enqueue_ndrange(stp_strat.krnl, cq_exec, 1,
				NULL, &stp_strat.gws, &stp_strat.lws, NULL,
				&err_internal);
			ccl_if_err_propagate_goto(err, err_internal, error_handler);

			/* Update step. */
			curr_step -= stp_strat.num_steps;

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

	/* Free the steps array. */
	g_free(steps);

	/* Return. */
	return ewl;

}

/**
 * Initializes an advanced bitonic sorter object and returns the
 * respective source code.
 *
 * @param[in] Sorter object to initialize.
 * @param[in] options Algorithm options.
 * @param[out] err Return location for a GError, or `NULL` if error
 * reporting is to be ignored.
 * @return Advanced bitonic sorter source code or `NULL` if an error
 * occurs.
 * */
static const char* clo_sort_abitonic_init(
	CloSort* sorter, const char* options, GError** err) {

	/* Make sure err is NULL or it is not set. */
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	const char* abitonic_src;

	clo_sort_abitonic_data* data;

	data = g_slice_new0(clo_sort_abitonic_data);

	/* Set internal data default values. */
	data->max_inkrnl_stps = 4;
	data->min_inkrnl_stps = 1;
	data->max_inkrnl_sfs = UINT_MAX;

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
			if (g_strcmp0("minps", opt[0]) == 0) {
				/* Minimum in-kernel private memory steps key option. */
				ccl_if_err_create_goto(*err, CLO_ERROR,
					(value > 4) || (value < 1), CLO_ERROR_ARGS,
					error_handler,
					"Option 'minps' must be between 1 and 4.");
				data->min_inkrnl_stps = value;
			} else if (g_strcmp0("maxps", opt[0]) == 0) {
				/* Maximum in-kernel private memory steps key option. */
				ccl_if_err_create_goto(*err, CLO_ERROR,
					(value > 4) || (value < 1), CLO_ERROR_ARGS,
					error_handler,
					"Option 'maxps' must be between 1 and 4.");
				data->max_inkrnl_stps = value;
			} else if (g_strcmp0("maxsfs", opt[0]) == 0) {
				/* Maximum in-kernel "stage finish" step. */
				data->max_inkrnl_sfs = value;
			} else {
				ccl_if_err_create_goto(*err, CLO_ERROR, TRUE,
					CLO_ERROR_ARGS, error_handler,
					"Invalid option key '%s' for a-bitonic sort.", opt[0]);
			}

			/* Free token. */
			g_strfreev(opt);
			opt = NULL;

		}
		ccl_if_err_create_goto(*err, CLO_ERROR,
			data->max_inkrnl_stps < data->min_inkrnl_stps,
			CLO_ERROR_ARGS, error_handler,
			"'minps' (%d) must be less or equal than 'maxps' (%d).",
			data->min_inkrnl_stps, data->max_inkrnl_stps);

	}

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	abitonic_src = CLO_SORT_ABITONIC_SRC;
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	abitonic_src = NULL;

finish:

	/* Free parsed a-bitonic options. */
	g_strfreev(opts);
	g_strfreev(opt);

	/* Set object methods and internal data. */
	clo_sort_set_data(sorter, data);

	/* Return source to be compiled. */
	return abitonic_src;

}


/**
 * Finalizes an advanced bitonic sorter object.
 *
 * @param[in] sorter Sorter object to finalize.
 * */
static void clo_sort_abitonic_finalize(CloSort* sorter) {

	/* Release internal data. */
	g_slice_free(clo_sort_abitonic_data, clo_sort_get_data(sorter));

	return;
}

/* Definition of the abitonic sort implementation. */
const CloSortImplDef clo_sort_abitonic_def = {
	"abitonic",
	CL_TRUE,
	clo_sort_abitonic_init,
	clo_sort_abitonic_finalize,
	clo_sort_abitonic_sort_with_device_data
};

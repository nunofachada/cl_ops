/*   
 * This file is part of CL-Ops.
 * 
 * CL-Ops is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * CL-Ops is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with CL-Ops.  If not, see <http://www.gnu.org/licenses/>.
 * */

 
/** 
 * @file
 * @brief Advanced bitonic sort host implementation.
 */

#include "clo_sort_abitonic.h"

/* Event index for advanced bitonic sort kernel. */
static unsigned int abitonic_evt_idx[CLO_SORT_ABITONIC_NUMKERNELS];

/* Array of kernel names. */
static const char* const kernel_names[] = CLO_SORT_ABITONIC_KERNELNAMES;

/**
 * @brief Sort agents using the advanced bitonic sort.
 * 
 * @see clo_sort_sort()
 */
int clo_sort_abitonic_sort(cl_command_queue *queues, cl_kernel *krnls, size_t lws_max, unsigned int numel, const char* options, cl_event **evts, gboolean profile, GError **err) {
	
	/* Aux. var. */
	int status, ocl_status;
		
	/* Event pointer, in case profiling is on. */
	cl_event* evt;

	/* Number of bitonic sort stages. */
	unsigned int totalStages;
	
	/* Maximum number of in-kernel steps for private memory kernels. */
	unsigned int max_inkrnl_stps = 4;

	/* Minimum number of in-kernel steps for private memory kernels. */
	unsigned int min_inkrnl_stps = 1;

	/* Maximum in-kernel "stage finish" step. */
	unsigned int max_inkrnl_sfs = UINT_MAX;
	
	/* Implementation of the strategy to follow on each step. */
	clo_sort_abitonic_step *steps = NULL;

	/* Parse options. */
	status = clo_sort_abitonic_options_parse(options, &max_inkrnl_stps, &min_inkrnl_stps, &max_inkrnl_sfs, err);
	gef_if_error_goto(*err, GEF_USE_STATUS, status, error_handler);
	
	/* Determine effective maximum in-kernel "stage finish" step. */
	max_inkrnl_sfs = MIN(MIN(12, max_inkrnl_sfs), clo_tzc(lws_max) + max_inkrnl_stps);

	/* Determine total number of bitonic stages. */
	totalStages = (cl_uint) clo_tzc(clo_nlpo2(numel));
	
	/* Allocate memory for sorting strategy. */
	steps = (clo_sort_abitonic_step*) malloc(
		sizeof(clo_sort_abitonic_step) * totalStages);
	gef_if_error_create_goto(*err, CLO_ERROR, 
		steps == NULL, status = CLO_ERROR_NOALLOC, error_handler, 
		"Unable to allocate memory for a-bitonic sorting strategy.");
	
	/* Obtain sorting strategy, e.g., which kernels to use in each step. */
	clo_sort_abitonic_strategy_get(steps, lws_max, totalStages, numel, 
		min_inkrnl_stps, max_inkrnl_stps, max_inkrnl_sfs);

	fprintf(stderr, "\n********* NUMEL = %d *******\n", numel);
	
	/* Perform sorting. */
	for (cl_uint currentStage = 1; currentStage <= totalStages; currentStage++) {
		for (cl_uint currentStep = currentStage; currentStep >= 1; ) {

			/* Get strategy for current step. */
			clo_sort_abitonic_step stp_strat = steps[currentStep - 1];

			fprintf(stderr, "Stage %d, Step %d | %s [GWS=%d, LWS=%d, NSTEPS=%d] \n", currentStage, currentStep, stp_strat.krnl_name, stp_strat.gws, stp_strat.lws, stp_strat.num_steps);
			
			/* Set kernel arguments. */
			/* Current stage. */
			ocl_status = clSetKernelArg(krnls[stp_strat.krnl_idx], 1, sizeof(cl_uint), (void *) &currentStage);
			gef_if_error_create_goto(*err, CLO_ERROR, 
				ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, error_handler, 
				"arg 1 of %s kernel, OpenCL error %d: %s", stp_strat.krnl_name, ocl_status, clerror_get(ocl_status));
			/* Current step (for some kernels only). */
			if (stp_strat.set_step) {
				ocl_status = clSetKernelArg(krnls[stp_strat.krnl_idx], 2, sizeof(cl_uint), (void *) &currentStep);
				gef_if_error_create_goto(*err, CLO_ERROR, 
					ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, 
					error_handler, "arg 2 of %s kernel, OpenCL error %d: %s", 
					stp_strat.krnl_name, ocl_status, clerror_get(ocl_status));
			}
			
			/* Determine wether to profile the kernel or not. */
			evt = profile ? &evts[stp_strat.krnl_idx][abitonic_evt_idx[stp_strat.krnl_idx]] : NULL;
					
			/* Execute kernel. */
			ocl_status = clEnqueueNDRangeKernel(
				queues[0], 
				krnls[stp_strat.krnl_idx], 
				1, 
				NULL, 
				&stp_strat.gws, 
				&stp_strat.lws, 
				0, 
				NULL,
				evt
			);
				
			gef_if_error_create_goto(*err, CLO_ERROR, 
				ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, error_handler, 
				"Executing %s kernel, OpenCL error %d: %s", 
				stp_strat.krnl_name, ocl_status, clerror_get(ocl_status));
					
			/* Increment event index for this kernel. */
			abitonic_evt_idx[stp_strat.krnl_idx]++;
				
			/* Update step. */
			currentStep -= stp_strat.num_steps;
			
		}
	}
				
	/* If we got here, everything is OK. */
	status = CLO_SUCCESS;
	g_assert(err == NULL || *err == NULL);
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	
finish:
	
	/* Free strategy implementation. */
	if (steps) free(steps);
	
	/* Return. */
	return status;	
}

/** 
 * @brief Returns the name of the kernel identified by the given
 * index.
 * 
 * @see clo_sort_kernelname_get()
 * */
const char* clo_sort_abitonic_kernelname_get(unsigned int index) {
	g_assert_cmpuint(index, <, CLO_SORT_ABITONIC_NUMKERNELS);
	return kernel_names[index];
}

/** 
 * @brief Create kernels for the advanced bitonic sort. 
 * 
 * @see clo_sort_kernels_create()
 * */
int clo_sort_abitonic_kernels_create(cl_kernel **krnls, cl_program program, GError **err) {
	
	/* Aux. var. */
	int status, ocl_status;
	
	/* Allocate memory for required for advanced bitonic sort kernels. */
	*krnls = (cl_kernel*) calloc(CLO_SORT_ABITONIC_NUMKERNELS, sizeof(cl_kernel));
	gef_if_error_create_goto(*err, CLO_ERROR, *krnls == NULL, status = CLO_ERROR_NOALLOC, error_handler, "Unable to allocate memory for advanced bitonic sort kernels.");	
	
	/* Create kernels. */
	for (unsigned int i = 0; i < CLO_SORT_ABITONIC_NUMKERNELS; i++) {
		const char* kernel_name = clo_sort_abitonic_kernelname_get(i);
		(*krnls)[i] = clCreateKernel(program, kernel_name, &ocl_status);
		gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Create %s kernel, OpenCL error %d: %s", kernel_name, ocl_status, clerror_get(ocl_status));
	}

	/* If we got here, everything is OK. */
	status = CLO_SUCCESS;
	g_assert(err == NULL || *err == NULL);
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	
finish:
	
	/* Return. */
	return status;

}

/** 
 * @brief Get local memory usage for the advanced bitonic sort kernels.
 * 
 * @see clo_sort_localmem_usage()
 * */
size_t clo_sort_abitonic_localmem_usage(const char* kernel_name, size_t lws_max, size_t len, unsigned int numel) {
	
	/* Local memory usage. */
	size_t local_mem_usage;
	
	/* Avoid compiler warnings. */
	numel = numel;
	
	if ((g_strcmp0(kernel_name, CLO_SORT_ABITONIC_KNAME_ANY) == 0) ||
		(g_strrstr(kernel_name, CLO_SORT_ABITONIC_KNAME_PRIV_MARK))) {
		/* No local memory for private memory kernels and for kernel "any" */
		local_mem_usage = 0;
	} else if (g_strrstr(kernel_name, CLO_SORT_ABITONIC_KNAME_LOCAL_MARK)) {
		/* Local memory kernels use local memory. */
		local_mem_usage = len * lws_max * 2;
	} else if (g_strrstr(kernel_name, CLO_SORT_ABITONIC_KNAME_HYB_MARK)) {
		/* Hybrid memory kernels use local memory, but it depends on
		 * how many elements each thread sorts. */
		local_mem_usage = len * lws_max * CLO_SORT_ABITONIC_KPARSE_V(kernel_name);
	} else {
		g_assert_not_reached();
	}
	
	/* Advanced bitonic sort doesn't use local memory. */
	return local_mem_usage;
}

/** 
 * @brief Set kernels arguments for the advanced bitonic sort. 
 * 
 * @see clo_sort_kernelargs_set()
 * */
int clo_sort_abitonic_kernelargs_set(cl_kernel **krnls, cl_mem data, size_t lws, size_t len, GError **err) {
	
	/* Aux. var. */
	int status, ocl_status;
	
	/* Set kernel arguments. */
	ocl_status = clSetKernelArg((*krnls)[CLO_SORT_ABITONIC_KIDX_ANY], 0, sizeof(cl_mem), &data);
	gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Set arg 0 of " CLO_SORT_ABITONIC_KNAME_ANY " kernel. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	for (unsigned int i = CLO_SORT_ABITONIC_KIDX_LOCAL_S2;  i <= CLO_SORT_ABITONIC_KIDX_LOCAL_S11; i++) {
		
		ocl_status = clSetKernelArg((*krnls)[i], 0, sizeof(cl_mem), &data);
		gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Set arg 0 of %s kernel. OpenCL error %d: %s", clo_sort_abitonic_kernelname_get(i), ocl_status, clerror_get(ocl_status));

		ocl_status = clSetKernelArg((*krnls)[i], 2, len * lws * 2, NULL);
		gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Set arg 2 of %s kernel. OpenCL error %d: %s", clo_sort_abitonic_kernelname_get(i), ocl_status, clerror_get(ocl_status));
	}

	for (unsigned int i = CLO_SORT_ABITONIC_KIDX_PRIV_2S4V;  i <= CLO_SORT_ABITONIC_KIDX_PRIV_4S16V; i++) {

		ocl_status = clSetKernelArg((*krnls)[i], 0, sizeof(cl_mem), &data);
		gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Set arg 0 of %s kernel. OpenCL error %d: %s", clo_sort_abitonic_kernelname_get(i), ocl_status, clerror_get(ocl_status));

	}
	
	for (unsigned int i = CLO_SORT_ABITONIC_KIDX_HYB_S4_2S4V;  i <= CLO_SORT_ABITONIC_KIDX_HYB_S12_2S4V; i++) {

		ocl_status = clSetKernelArg((*krnls)[i], 0, sizeof(cl_mem), &data);
		gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Set arg 0 of %s kernel. OpenCL error %d: %s", clo_sort_abitonic_kernelname_get(i), ocl_status, clerror_get(ocl_status));

		ocl_status = clSetKernelArg((*krnls)[i], 2, len * lws * 4, NULL);
		gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Set arg 2 of %s kernel. OpenCL error %d: %s", clo_sort_abitonic_kernelname_get(i), ocl_status, clerror_get(ocl_status));
	}

	for (unsigned int i = CLO_SORT_ABITONIC_KIDX_HYB_S3_3S8V;  i <= CLO_SORT_ABITONIC_KIDX_HYB_S12_3S8V; i++) {

		ocl_status = clSetKernelArg((*krnls)[i], 0, sizeof(cl_mem), &data);
		gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Set arg 0 of %s kernel. OpenCL error %d: %s", clo_sort_abitonic_kernelname_get(i), ocl_status, clerror_get(ocl_status));

		ocl_status = clSetKernelArg((*krnls)[i], 2, len * lws * 8, NULL);
		gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Set arg 2 of %s kernel. OpenCL error %d: %s", clo_sort_abitonic_kernelname_get(i), ocl_status, clerror_get(ocl_status));
	}
	
	for (unsigned int i = CLO_SORT_ABITONIC_KIDX_HYB_S4_4S16V;  i <= CLO_SORT_ABITONIC_KIDX_HYB_S12_4S16V; i++) {

		ocl_status = clSetKernelArg((*krnls)[i], 0, sizeof(cl_mem), &data);
		gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Set arg 0 of %s kernel. OpenCL error %d: %s", clo_sort_abitonic_kernelname_get(i), ocl_status, clerror_get(ocl_status));

		ocl_status = clSetKernelArg((*krnls)[i], 2, len * lws * 16, NULL);
		gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Set arg 2 of %s kernel. OpenCL error %d: %s", clo_sort_abitonic_kernelname_get(i), ocl_status, clerror_get(ocl_status));
	}
	

	/* If we got here, everything is OK. */
	status = CLO_SUCCESS;
	g_assert(err == NULL || *err == NULL);
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	
finish:
	
	/* Return. */
	return status;
	
}

/** 
 * @brief Free the advanced bitonic sort kernels. 
 * 
 * @see clo_sort_kernels_free()
 * */
void clo_sort_abitonic_kernels_free(cl_kernel **krnls) {
	if (*krnls) {
		for (int i = 0; i < CLO_SORT_ABITONIC_NUMKERNELS; i++) {
			if ((*krnls)[i]) clReleaseKernel((*krnls)[i]);
		}
		free(*krnls);
	}
}

/** 
 * @brief Create events for the advanced bitonic sort kernels. 
 * 
 * @see clo_sort_events_create()
 * */
int clo_sort_abitonic_events_create(cl_event ***evts, unsigned int iters, size_t numel, size_t lws_max, GError **err) {

	/* Aux. var. */
	int status;
	
	/* LWS not used for advanced bitonic sort. */
	lws_max = lws_max;
	
	/* Required number of events, worst case usage scenario. The instruction 
	 * below sums all numbers from 0 to x, where is is the log2 (clo_tzc) of
	 * the next larger power of 2 of the maximum possible agents. */
	int num_evts = iters * clo_sum(clo_tzc(clo_nlpo2(numel)));  /// @todo Not so many events required
	
	/* Two types of event required for the advanced bitonic sort kernel. */
	*evts = (cl_event**) calloc(CLO_SORT_ABITONIC_NUMKERNELS, sizeof(cl_event*));
	gef_if_error_create_goto(*err, CLO_ERROR, *evts == NULL, status = CLO_ERROR_NOALLOC, error_handler, "Unable to allocate memory for advanced bitonic sort events.");	
	
	/* Allocate memory for all occurrences of the event (i.e. executions of the advanced bitonic sort kernel). */
	for (unsigned int i = 0; i < CLO_SORT_ABITONIC_NUMKERNELS; i++) {
		
		(*evts)[i] = (cl_event*) calloc(num_evts, sizeof(cl_event)); /// @todo Adjust according to kernel, find some formula
		gef_if_error_create_goto(*err, CLO_ERROR, (*evts)[i] == NULL, status = CLO_ERROR_NOALLOC, error_handler, "Unable to allocate memory for %s kernel events.", clo_sort_abitonic_kernelname_get(i));	
		
		/* Set current kernel event index to zero. */
		abitonic_evt_idx[i] = 0;
	}
	
	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	status = CLO_SUCCESS;
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);

finish:
	
	/* Return. */
	return status;	
}

/** 
 * @brief Free the advanced bitonic sort events. 
 * 
 * @see clo_sort_events_free()
 * */
void clo_sort_abitonic_events_free(cl_event ***evts) {
	if (*evts) {
		for (int k = 0; k < CLO_SORT_ABITONIC_NUMKERNELS; k++) {
			if ((*evts)[k]) {
				for (unsigned int i = 0; i < abitonic_evt_idx[k]; i++) {
					if (((*evts)[k])[i]) {
						clReleaseEvent(((*evts)[k])[i]);
					}
				}
				free((*evts)[k]);
			}
		}
		free(*evts);
	}
}

/** 
 * @brief Add bitonic sort events to the profiler object. 
 * 
 * @see clo_sort_events_profile()
 * */
int clo_sort_abitonic_events_profile(cl_event **evts, ProfCLProfile *profile, GError **err) {
	
	int status;

	for (unsigned int j = 0; j < CLO_SORT_ABITONIC_NUMKERNELS; j++) {
		const char* kernel_name = clo_sort_abitonic_kernelname_get(j);
		for (unsigned int i = 0; i < abitonic_evt_idx[j]; i++) {
			profcl_profile_add(profile, kernel_name, evts[j][i], err);
			gef_if_error_goto(*err, CLO_ERROR_LIBRARY, status, error_handler);
		}
	}
	
	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	status = CLO_SUCCESS;
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	
finish:
	
	/* Return. */
	return status;	
	
}

void clo_sort_abitonic_strategy_get(clo_sort_abitonic_step *steps, size_t lws_max,
	unsigned int totalStages, unsigned int numel, unsigned int min_inkrnl_stps, 
	unsigned int max_inkrnl_stps, unsigned int max_inkrnl_sfs) {
	
	/* Kernels applicable to each step. */
	static const int lookup[11][4] = {
		/* Step 2 */
		{
			CLO_SORT_ABITONIC_KIDX_LOCAL_S2, 
			CLO_SORT_ABITONIC_NUMKERNELS, CLO_SORT_ABITONIC_NUMKERNELS, CLO_SORT_ABITONIC_NUMKERNELS
		},
		/* Step 3 */
		{
			CLO_SORT_ABITONIC_KIDX_HYB_S3_3S8V, 
			CLO_SORT_ABITONIC_KIDX_LOCAL_S3, 
			CLO_SORT_ABITONIC_NUMKERNELS, CLO_SORT_ABITONIC_NUMKERNELS
		},
		/* Step 4 */
		{
			CLO_SORT_ABITONIC_KIDX_HYB_S4_4S16V, 
			CLO_SORT_ABITONIC_KIDX_HYB_S4_2S4V, 
			CLO_SORT_ABITONIC_KIDX_LOCAL_S4, 
			CLO_SORT_ABITONIC_NUMKERNELS
		},
		/* Step 5 */
		{
			CLO_SORT_ABITONIC_KIDX_LOCAL_S5, 
			CLO_SORT_ABITONIC_NUMKERNELS, CLO_SORT_ABITONIC_NUMKERNELS, CLO_SORT_ABITONIC_NUMKERNELS
		},
		/* Step 6 */
		{
			CLO_SORT_ABITONIC_KIDX_HYB_S6_3S8V,
			CLO_SORT_ABITONIC_KIDX_HYB_S6_2S4V,
			CLO_SORT_ABITONIC_KIDX_LOCAL_S6, 
			CLO_SORT_ABITONIC_NUMKERNELS
		},
		/* Step 7 */
		{
			CLO_SORT_ABITONIC_KIDX_LOCAL_S7, 
			CLO_SORT_ABITONIC_NUMKERNELS, CLO_SORT_ABITONIC_NUMKERNELS, CLO_SORT_ABITONIC_NUMKERNELS
		},
		/* Step 8 */
		{
			CLO_SORT_ABITONIC_KIDX_HYB_S8_4S16V,
			CLO_SORT_ABITONIC_KIDX_HYB_S8_2S4V,
			CLO_SORT_ABITONIC_KIDX_LOCAL_S8, 
			CLO_SORT_ABITONIC_NUMKERNELS
		},
		/* Step 9 */
		{
			CLO_SORT_ABITONIC_KIDX_HYB_S9_3S8V,
			CLO_SORT_ABITONIC_KIDX_LOCAL_S9, 
			CLO_SORT_ABITONIC_NUMKERNELS, CLO_SORT_ABITONIC_NUMKERNELS
		},
		/* Step 10 */
		{
			CLO_SORT_ABITONIC_KIDX_HYB_S10_2S4V,
			CLO_SORT_ABITONIC_KIDX_LOCAL_S10, 
			CLO_SORT_ABITONIC_NUMKERNELS, CLO_SORT_ABITONIC_NUMKERNELS
		},
		/* Step 11 */
		{
			CLO_SORT_ABITONIC_KIDX_LOCAL_S11, 
			CLO_SORT_ABITONIC_NUMKERNELS, CLO_SORT_ABITONIC_NUMKERNELS, CLO_SORT_ABITONIC_NUMKERNELS
		},
		/* Step 12 */
		{
			CLO_SORT_ABITONIC_KIDX_HYB_S12_4S16V,
			CLO_SORT_ABITONIC_KIDX_HYB_S12_3S8V,
			CLO_SORT_ABITONIC_KIDX_HYB_S12_2S4V,
			CLO_SORT_ABITONIC_NUMKERNELS
		},
	};

	/* Build strategy. */
	for (unsigned int step = 1; step <= totalStages; step++) {
		if (step == 1) {
			/* Step 1 requires the "any" kernel. */
			steps[step - 1].krnl_name = CLO_SORT_ABITONIC_KNAME_ANY;
			steps[step - 1].krnl_idx = CLO_SORT_ABITONIC_KIDX_ANY;
			steps[step - 1].gws = clo_nlpo2(numel) / 2;
			steps[step - 1].lws = MIN(lws_max, steps[step - 1].gws);
			steps[step - 1].set_step = TRUE;
			steps[step - 1].num_steps = 1;
		} else if (step > max_inkrnl_sfs) {
			/* For steps higher than the maximum in-kernel "stage finish" 
			 * step, e.g., max_inkrnl_sfs. */
			
			/* This may be required when max_inkrnl_sfs < 4 in order to
			 * avoid overshooting the target step 0. */
			unsigned int step_margin = MIN(step, max_inkrnl_stps);
			
			/* Chose proper kernel. */
			switch (step_margin) {
				case 4:
					steps[step - 1].krnl_name = CLO_SORT_ABITONIC_KNAME_PRIV_4S16V;
					steps[step - 1].krnl_idx = CLO_SORT_ABITONIC_KIDX_PRIV_4S16V;
					break;
				case 3:
					steps[step - 1].krnl_name = CLO_SORT_ABITONIC_KNAME_PRIV_3S8V;
					steps[step - 1].krnl_idx = CLO_SORT_ABITONIC_KIDX_PRIV_3S8V;
					break;
				case 2:
					steps[step - 1].krnl_name = CLO_SORT_ABITONIC_KNAME_PRIV_2S4V;
					steps[step - 1].krnl_idx = CLO_SORT_ABITONIC_KIDX_PRIV_2S4V;
					break;
				case 1:
					steps[step - 1].krnl_name = CLO_SORT_ABITONIC_KNAME_ANY;
					steps[step - 1].krnl_idx = CLO_SORT_ABITONIC_KIDX_ANY;
					break;
				default:
					g_assert_not_reached();
			}
			steps[step - 1].gws = clo_nlpo2(numel) / (1 << step_margin);
			steps[step - 1].lws = MIN(lws_max, steps[step - 1].gws);
			steps[step - 1].set_step = TRUE;
			steps[step - 1].num_steps = step_margin;
		} else {
			/* For steps equal or smaller than the maximum in-kernel 
			 * "stage finish" step. */
			const int *possible_krnls = lookup[step - 2];
			unsigned int priv_steps;
			gboolean found = FALSE;
			/* Find proper kernel for current step by looking at 
			 * kernel lookup table. */
			for (unsigned int i = 0; possible_krnls[i] != CLO_SORT_ABITONIC_NUMKERNELS; i++) {
				/* Get current possible kernel name. */
				const char* krnl_name = clo_sort_abitonic_kernelname_get(possible_krnls[i]);
				/* Check if it's a hybrid or local kernel. */
				if (g_strrstr(krnl_name, CLO_SORT_ABITONIC_KNAME_HYB_MARK)) {
					/* It's a hybrid kernel, determine how many steps 
					 * does it perform on private memory each time. */
					priv_steps = CLO_SORT_ABITONIC_KPARSE_S(krnl_name);
				} else {
					/* It's a local kernel, thus only one step is performed in
					 * private memory. */
					priv_steps = 1;
				}
				/* IF within interval of acceptable in-kernel private 
				 * memory steps AND IF required LWS within limits of 
				 * maximum LWS, THEN chose this kernel. */
				if ((priv_steps <= max_inkrnl_stps) && (priv_steps >= min_inkrnl_stps) &&
					(lws_max >= (1 << (step - priv_steps)))) {
					steps[step - 1].krnl_name = krnl_name;
					steps[step - 1].krnl_idx = possible_krnls[i];
					steps[step - 1].gws = clo_nlpo2(numel) / (1 << priv_steps);
					steps[step - 1].lws = MIN(lws_max, steps[step - 1].gws);
					steps[step - 1].set_step = FALSE;
					steps[step - 1].num_steps = step;
					found = TRUE;
					break;
				}
			}
			/* If no kernel was selected, use the "any" kernel to advance one step. */
			if (!found) {
				steps[step - 1].krnl_name = CLO_SORT_ABITONIC_KNAME_ANY;
				steps[step - 1].krnl_idx = CLO_SORT_ABITONIC_KIDX_ANY;
				steps[step - 1].gws = clo_nlpo2(numel) / 2;
				steps[step - 1].lws = MIN(lws_max, steps[step - 1].gws);
				steps[step - 1].set_step = TRUE;
				steps[step - 1].num_steps = 1;				
			}
		}
	}
	
}

int clo_sort_abitonic_options_parse(const char* options, unsigned int *max_inkrnl_stps, 
	unsigned int *min_inkrnl_stps, unsigned int *max_inkrnl_sfs, GError **err) {

	/* Aux. var. */
	int status, num_toks;
		
	/* Tokenized options. */
	gchar** opts = NULL;
	gchar** opt = NULL;

	/* Value to read from current option. */
	unsigned int value;
	
	/* Check options. */
	if (options) {
		opts = g_strsplit_set(options, ",", -1);
		for (unsigned int i = 0; opts[i] != NULL; i++) {
			
			/* Ignore empty tokens. */
			if (opts[i][0] == '\0') continue;
			
			/* Parse current option, get key and value. */
			opt = g_strsplit_set(opts[i], "=", 2);
			
			/* Count number of tokens. */
			for (num_toks = 0; opt[num_toks] != NULL; num_toks++);
			
			/* If number of tokens is not 2 (key and value), throw error. */
			gef_if_error_create_goto(*err, CLO_ERROR, 
				num_toks != 2, status = CLO_ERROR_ARGS, error_handler, 
						"Invalid option '%s' for a-bitonic sort.", opts[i]);
			
			/* Get option value. */
			value = atoi(opt[1]);

			/* Check key/value option. */
			if (g_strcmp0("minps", opt[0]) == 0) {
				/* Minimum in-kernel private memory steps key option. */
				gef_if_error_create_goto(*err, CLO_ERROR, 
					(value > 4) || (value < 1), 
					status = CLO_ERROR_ARGS, error_handler, 
					"Option 'minps' must be between 1 and 4.");
				*min_inkrnl_stps = value;
			} else if (g_strcmp0("maxps", opt[0]) == 0) {
				/* Maximum in-kernel private memory steps key option. */
				gef_if_error_create_goto(*err, CLO_ERROR, 
					(value > 4) || (value < 1), 
					status = CLO_ERROR_ARGS, error_handler, 
					"Option 'maxps' must be between 1 and 4.");
				*max_inkrnl_stps = value;
			} else if (g_strcmp0("maxsfs", opt[0]) == 0) {
				/* Maximum in-kernel "stage finish" step. */
				*max_inkrnl_sfs = value;
			} else {
				gef_if_error_create_goto(*err, CLO_ERROR, TRUE, 
					status = CLO_ERROR_ARGS, error_handler, 
					"Invalid option key '%s' for a-bitonic sort.", opt[0]);
			}
			
			/* Free token. */
			g_strfreev(opt);
			opt = NULL;
			
		}
		gef_if_error_create_goto(*err, CLO_ERROR, 
			*max_inkrnl_stps < *min_inkrnl_stps, 
			status = CLO_ERROR_ARGS, error_handler, 
			"'minps' (%d) must be less or equal than 'maxps' (%d).", 
			*min_inkrnl_stps, *max_inkrnl_stps);
		
	}

	/* If we got here, everything is OK. */
	status = CLO_SUCCESS;
	g_assert(err == NULL || *err == NULL);
	goto finish;
	
error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	
finish:
	
	/* Free parsed a-bitonic options. */
	g_strfreev(opts);
	g_strfreev(opt);
	
	/* Return. */
	return status;
	
}

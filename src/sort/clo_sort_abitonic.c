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
int clo_sort_abitonic_sort(cl_command_queue *queues, cl_kernel *krnls, cl_event **evts, size_t lws_max, unsigned int numel, gboolean profile, GError **err) {
	
	/* Aux. var. */
	int status, ocl_status;
		
	/* Event pointer, in case profiling is on. */
	cl_event* evt;

	/* Number of bitonic sort stages. */
	cl_uint totalStages = (cl_uint) clo_tzc(numel);

	/* ****** Unrolled and any kernels ******** */

	/* Global worksize. */
	size_t gws = clo_nlpo2(numel) / 2;
	/* Local worksize. */
	size_t lws = MIN(lws_max, gws);
	/* Maximum step for unrolled kernels. */
	cl_uint maxStep = clo_tzc(lws) + 1;
	
	/* ***** Three step kernels ****** */
	
	/* Global worksize. */
	size_t gws8 = clo_nlpo2(numel) / 8;
	/* Local worksize. */
	size_t lws8 = lws / 2; /// @todo This is more or less, I should adjust it better or try to make it dependent on the device local memory
		
	/* Perform sorting. */
	for (cl_uint currentStage = 1; currentStage <= totalStages; currentStage++) {
		
		for (cl_uint currentStep = currentStage; currentStep >= 1; ) {
			
			/* Use an unrolled kernel. */
			if ((currentStep <= maxStep) && (currentStep >= 2)) {

				unsigned int krnl_idx = currentStep - 1;
				const char* krnl_name = clo_sort_abitonic_kernelname_get(krnl_idx);

				ocl_status = clSetKernelArg(krnls[krnl_idx], 1, sizeof(cl_uint), (void *) &currentStage);
				gef_if_error_create_goto(*err, CLO_ERROR, ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, error_handler, "arg 1 of %s kernel, OpenCL error %d: %s", krnl_name, ocl_status, clerror_get(ocl_status));
					
				evt = profile ? &evts[krnl_idx][abitonic_evt_idx[krnl_idx]] : NULL;
					
				ocl_status = clEnqueueNDRangeKernel(
					queues[0], 
					krnls[krnl_idx], 
					1, 
					NULL, 
					&gws, 
					&lws, 
					0, 
					NULL,
					evt
				);
				
				gef_if_error_create_goto(*err, CLO_ERROR, ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, error_handler, "Executing %s kernel, OpenCL error %d: %s", krnl_name, ocl_status, clerror_get(ocl_status));
				abitonic_evt_idx[krnl_idx]++;
				
				/* Break out of the loop. */
				break;
				
			} else if (currentStep > maxStep) {
				
				/* Use 3-step kernel (each thread completely sorts 8 values). */
				ocl_status = clSetKernelArg(krnls[CLO_SORT_ABITONIC_K_S8], 1, sizeof(cl_uint), (void *) &currentStage);
				gef_if_error_create_goto(*err, CLO_ERROR, ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, error_handler, "arg 1 of " CLO_SORT_SBITONIC_KERNELNAME_S8 " kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));
					
				ocl_status = clSetKernelArg(krnls[CLO_SORT_ABITONIC_K_S8], 2, sizeof(cl_uint), (void *) &currentStep);
				gef_if_error_create_goto(*err, CLO_ERROR, ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, error_handler, "arg 2 of " CLO_SORT_SBITONIC_KERNELNAME_S8 " kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

				evt = profile ? &evts[CLO_SORT_ABITONIC_K_S8][abitonic_evt_idx[CLO_SORT_ABITONIC_K_S8]] : NULL;
					
				ocl_status = clEnqueueNDRangeKernel(
					queues[0], 
					krnls[CLO_SORT_ABITONIC_K_S8], 
					1, 
					NULL, 
					&gws8, 
					&lws8, 
					0, 
					NULL,
					evt
				);
				
				/* Advance three steps. */
				currentStep -= 3;
				
				gef_if_error_create_goto(*err, CLO_ERROR, ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, error_handler, "Executing " CLO_SORT_SBITONIC_KERNELNAME_S8 " kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));
				abitonic_evt_idx[CLO_SORT_ABITONIC_K_S8]++;
			
			} else { /* Step = 1 */
			
				/* Use "any" kernel. */
				ocl_status = clSetKernelArg(krnls[CLO_SORT_ABITONIC_K_ANY], 1, sizeof(cl_uint), (void *) &currentStage);
				gef_if_error_create_goto(*err, CLO_ERROR, ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, error_handler, "arg 1 of " CLO_SORT_SBITONIC_KERNELNAME_ANY " kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));
					
				ocl_status = clSetKernelArg(krnls[CLO_SORT_ABITONIC_K_ANY], 2, sizeof(cl_uint), (void *) &currentStep);
				gef_if_error_create_goto(*err, CLO_ERROR, ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, error_handler, "arg 2 of " CLO_SORT_SBITONIC_KERNELNAME_ANY " kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));
					
				evt = profile ? &evts[CLO_SORT_ABITONIC_K_ANY][abitonic_evt_idx[CLO_SORT_ABITONIC_K_ANY]] : NULL;
					
				ocl_status = clEnqueueNDRangeKernel(
					queues[0], 
					krnls[CLO_SORT_ABITONIC_K_ANY], 
					1, 
					NULL, 
					&gws, 
					&lws, 
					0, 
					NULL,
					evt
				);
				
				/* Advance one step. */
				currentStep--;
				
				gef_if_error_create_goto(*err, CLO_ERROR, ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, error_handler, "Executing " CLO_SORT_SBITONIC_KERNELNAME_ANY " kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));
				abitonic_evt_idx[CLO_SORT_ABITONIC_K_ANY]++;
			}
			
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
	
	if (g_strcmp0(kernel_name, CLO_SORT_SBITONIC_KERNELNAME_ANY) == 0) {
		/* No local memory for kernel "any" */
		local_mem_usage = 0;
	} else {
		/* Other kernels use local memory. */
		local_mem_usage = len * lws_max * 2;
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
	
	/* LWS and len not used here. */
	lws = lws;
	len = len;
	
	/* Set kernel arguments. */
	ocl_status = clSetKernelArg((*krnls)[CLO_SORT_ABITONIC_K_ANY], 0, sizeof(cl_mem), &data);
	gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Set arg 0 of " CLO_SORT_SBITONIC_KERNELNAME_ANY " kernel. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	for (unsigned int i = 1;  i < CLO_SORT_ABITONIC_NUMKERNELS - 1; i++) {
		
		ocl_status = clSetKernelArg((*krnls)[i], 0, sizeof(cl_mem), &data);
		gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Set arg 0 of %s kernel. OpenCL error %d: %s", clo_sort_abitonic_kernelname_get(i), ocl_status, clerror_get(ocl_status));

		ocl_status = clSetKernelArg((*krnls)[i], 2, len * lws * 2, NULL);
		gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Set arg 2 of %s kernel. OpenCL error %d: %s", clo_sort_abitonic_kernelname_get(i), ocl_status, clerror_get(ocl_status));
	}

	ocl_status = clSetKernelArg((*krnls)[CLO_SORT_ABITONIC_K_S8], 0, sizeof(cl_mem), &data);
	gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Set arg 0 of " CLO_SORT_SBITONIC_KERNELNAME_S8 " kernel. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	ocl_status = clSetKernelArg((*krnls)[CLO_SORT_ABITONIC_K_S8], 3, len * lws * 8, NULL);
	gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Set arg 3 of " CLO_SORT_SBITONIC_KERNELNAME_S8 " kernel. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

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


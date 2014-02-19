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
static unsigned int abitonic_evt_idx[CLO_SORT_ABITONIC_NUMKRNLS];

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
		
	/* Global worksize. */
	size_t gws = clo_nlpo2(numel) / 2;
	
	/* Local worksize. */
	size_t lws = lws_max;
	
	/* Number of bitonic sort stages. */
	cl_uint totalStages = (cl_uint) clo_tzc(gws * 2);
	
	/* Event pointer, in case profiling is on. */
	cl_event* evt;

	/* Adjust LWS so that its equal or smaller than GWS, making sure that
	 * GWS is still a multiple of it. */
	while (gws % lws != 0)
		lws = lws / 2;
		
	/* Perform sorting. */
	for (cl_uint currentStage = 1; currentStage <= totalStages; currentStage++) {
		
		for (cl_uint currentStep = currentStage; currentStep >= 1; currentStep--) {
			
			/* Use steps_21 kernel. */
			if (currentStep == 2) {
				
				ocl_status = clSetKernelArg(krnls[CLO_SORT_ABITONIC_K_21], 1, sizeof(cl_uint), (void *) &currentStage);
				gef_if_error_create_goto(*err, CLO_ERROR, ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, error_handler, "arg 1 of " CLO_SORT_SBITONIC_KERNELNAME_21 " kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));
					
				evt = profile ? &evts[CLO_SORT_ABITONIC_K_21][abitonic_evt_idx[CLO_SORT_ABITONIC_K_21]] : NULL;
					
				ocl_status = clEnqueueNDRangeKernel(
					queues[0], 
					krnls[CLO_SORT_ABITONIC_K_21], 
					1, 
					NULL, 
					&gws, 
					&lws, 
					0, 
					NULL,
					evt
				);
				
				gef_if_error_create_goto(*err, CLO_ERROR, ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, error_handler, "Executing " CLO_SORT_SBITONIC_KERNELNAME_21 " kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));
				abitonic_evt_idx[CLO_SORT_ABITONIC_K_21]++;
				
				
				/* Break out of the loop. */
				break;
			}
			
			/* Use steps_any kernel. */
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
			
			gef_if_error_create_goto(*err, CLO_ERROR, ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, error_handler, "Executing " CLO_SORT_SBITONIC_KERNELNAME_ANY " kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));
			abitonic_evt_idx[CLO_SORT_ABITONIC_K_ANY]++;
			
			
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
	*krnls = (cl_kernel*) calloc(CLO_SORT_ABITONIC_NUMKRNLS, sizeof(cl_kernel));
	gef_if_error_create_goto(*err, CLO_ERROR, *krnls == NULL, status = CLO_ERROR_NOALLOC, error_handler, "Unable to allocate memory for advanced bitonic sort kernels.");	
	
	/* Create kernels. */
	(*krnls)[CLO_SORT_ABITONIC_K_ANY] = clCreateKernel(program, CLO_SORT_SBITONIC_KERNELNAME_ANY, &ocl_status);
	gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Create " CLO_SORT_SBITONIC_KERNELNAME_ANY " kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	(*krnls)[CLO_SORT_ABITONIC_K_21] = clCreateKernel(program, CLO_SORT_SBITONIC_KERNELNAME_21, &ocl_status);
	gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Create " CLO_SORT_SBITONIC_KERNELNAME_21 " kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

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
	} else if (g_strcmp0(kernel_name, CLO_SORT_SBITONIC_KERNELNAME_21) == 0) {
		/* Kernel 21 uses local memory. */
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

	ocl_status = clSetKernelArg((*krnls)[CLO_SORT_ABITONIC_K_21], 0, sizeof(cl_mem), &data);
	gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Set arg 0 of " CLO_SORT_SBITONIC_KERNELNAME_21 " kernel. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	ocl_status = clSetKernelArg((*krnls)[CLO_SORT_ABITONIC_K_21], 2, len * lws * 2, NULL);
	gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Set arg 2 of " CLO_SORT_SBITONIC_KERNELNAME_21 " kernel. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

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
		for (int i = 0; i < CLO_SORT_ABITONIC_NUMKRNLS; i++) {
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
	int num_evts = iters * clo_sum(clo_tzc(clo_nlpo2(numel))); 
	/// @todo Not so many events required
	
	/* Set advanced bitonic sort event index to zero (global var) */
	for (int i = 0; i < CLO_SORT_ABITONIC_NUMKRNLS; i++)
		abitonic_evt_idx[i] = 0;
	
	/* Two types of event required for the advanced bitonic sort kernel. */
	*evts = (cl_event**) calloc(CLO_SORT_ABITONIC_NUMKRNLS, sizeof(cl_event*));
	gef_if_error_create_goto(*err, CLO_ERROR, *evts == NULL, status = CLO_ERROR_NOALLOC, error_handler, "Unable to allocate memory for advanced bitonic sort events (0).");	
	
	/* Allocate memory for all occurrences of the event (i.e. executions of the advanced bitonic sort kernel). */
	(*evts)[CLO_SORT_ABITONIC_K_ANY] = (cl_event*) calloc(num_evts, sizeof(cl_event));
	gef_if_error_create_goto(*err, CLO_ERROR, (*evts)[0] == NULL, status = CLO_ERROR_NOALLOC, error_handler, "Unable to allocate memory for advanced bitonic sort events (1).");	
	
	(*evts)[CLO_SORT_ABITONIC_K_21] = (cl_event*) calloc(num_evts, sizeof(cl_event));
	gef_if_error_create_goto(*err, CLO_ERROR, (*evts)[1] == NULL, status = CLO_ERROR_NOALLOC, error_handler, "Unable to allocate memory for advanced bitonic sort events (2).");	
	
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
		for (int k = 0; k < CLO_SORT_ABITONIC_NUMKRNLS; k++) {
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

	for (unsigned int i = 0; i < abitonic_evt_idx[CLO_SORT_ABITONIC_K_ANY]; i++) {
		profcl_profile_add(profile, CLO_SORT_SBITONIC_KERNELNAME_ANY, evts[CLO_SORT_ABITONIC_K_ANY][i], err);
		gef_if_error_goto(*err, CLO_ERROR_LIBRARY, status, error_handler);
	}

	for (unsigned int i = 0; i < abitonic_evt_idx[CLO_SORT_ABITONIC_K_21]; i++) {
		profcl_profile_add(profile, CLO_SORT_SBITONIC_KERNELNAME_21, evts[CLO_SORT_ABITONIC_K_21][i], err);
		gef_if_error_goto(*err, CLO_ERROR_LIBRARY, status, error_handler);
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


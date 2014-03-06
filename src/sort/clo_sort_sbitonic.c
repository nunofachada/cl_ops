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
 * @brief Simple bitonic sort host implementation.
 */

#include "clo_sort_sbitonic.h"

/* Array of kernel names. */
static const char* const kernel_names[] = CLO_SORT_SBITONIC_KERNELNAMES;

/**
 * @brief Sort agents using the simple bitonic sort.
 * 
 * @see clo_sort_sort()
 */
int clo_sort_sbitonic_sort(cl_command_queue *queues, cl_kernel *krnls, 
	size_t lws_max, unsigned int numel, const char* options, 
	GArray *evts, gboolean profile, GError **err) {
	
	/* Aux. var. */
	int status, ocl_status;
		
	/* Global worksize. */
	size_t gws = clo_nlpo2(numel) / 2;
	
	/* Local worksize. */
	size_t lws = MIN(lws_max, gws);
	
	/* Number of bitonic sort stages. */
	cl_uint totalStages = (cl_uint) clo_tzc(gws * 2);
	
	/* OpenCL event, in case profiling is on. */
	cl_event evt;
	
	/* Avoid compiler warnings. */
	options = options;
	
	/* Perform sorting. */
	for (cl_uint currentStage = 1; currentStage <= totalStages; currentStage++) {
		cl_uint step = currentStage;
		for (cl_uint currentStep = step; currentStep > 0; currentStep--) {
			
			ocl_status = clSetKernelArg(krnls[0], 1, sizeof(cl_uint), (void *) &currentStage);
			gef_if_error_create_goto(*err, CLO_ERROR, ocl_status != CL_SUCCESS, 
				status = CLO_ERROR_LIBRARY, error_handler, 
				"arg 1 of " CLO_SORT_SBITONIC_KNAME_0 " kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));
			
			ocl_status = clSetKernelArg(krnls[0], 2, sizeof(cl_uint), (void *) &currentStep);
			gef_if_error_create_goto(*err, CLO_ERROR, ocl_status != CL_SUCCESS, 
				status = CLO_ERROR_LIBRARY, error_handler, 
				"arg 2 of " CLO_SORT_SBITONIC_KNAME_0 " kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

			ocl_status = clEnqueueNDRangeKernel(
				queues[0], 
				krnls[0], 
				1, 
				NULL, 
				&gws, 
				&lws, 
				0, 
				NULL,
				profile ? &evt : NULL
			);
			gef_if_error_create_goto(*err, CLO_ERROR, ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, error_handler, "Executing " CLO_SORT_SBITONIC_KNAME_0 " kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

			/* If profilling is on, add event to array. */
			if (profile) {
				ProfCLEvName ev_name = { .eventName = CLO_SORT_SBITONIC_KNAME_0, .event = evt};
				g_array_append_val(evts, ev_name);
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
const char* clo_sort_sbitonic_kernelname_get(unsigned int index) {
	g_assert_cmpuint(index, <, CLO_SORT_SBITONIC_NUMKERNELS);
	return kernel_names[index];
}

/** 
 * @brief Create kernels for the simple bitonic sort. 
 * 
 * @see clo_sort_kernels_create()
 * */
int clo_sort_sbitonic_kernels_create(cl_kernel **krnls, cl_program program, GError **err) {
	
	/* Aux. var. */
	int status, ocl_status;
	
	/* Allocate memory for single kernel required for simple bitonic sort. */
	*krnls = (cl_kernel*) calloc(1, sizeof(cl_kernel));
	gef_if_error_create_goto(*err, CLO_ERROR, *krnls == NULL, status = CLO_ERROR_NOALLOC, error_handler, "Unable to allocate memory for " CLO_SORT_SBITONIC_KNAME_0 " kernel.");	
	
	/* Create kernel. */
	(*krnls)[0] = clCreateKernel(program, CLO_SORT_SBITONIC_KNAME_0, &ocl_status);
	gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Create " CLO_SORT_SBITONIC_KNAME_0 " kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

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
 * @brief Get local memory usage for the simple bitonic sort kernels.
 * 
 * Simple bitonic sort only uses one kernel which doesn't require
 * local memory. 
 * 
 * @see clo_sort_localmem_usage()
 * */
size_t clo_sort_sbitonic_localmem_usage(const char* kernel_name, size_t lws_max, size_t len, unsigned int numel) {
	
	/* Avoid compiler warnings. */
	kernel_name = kernel_name;
	lws_max = lws_max;
	numel = numel;
	len = len;
	
	/* Simple bitonic sort doesn't use local memory. */
	return 0;
}

/** 
 * @brief Set kernels arguments for the simple bitonic sort. 
 * 
 * @see clo_sort_kernelargs_set()
 * */
int clo_sort_sbitonic_kernelargs_set(cl_kernel **krnls, cl_mem data, size_t lws, size_t len, GError **err) {
	
	/* Aux. var. */
	int status, ocl_status;
	
	/* LWS and len not used here. */
	lws = lws;
	len = len;
	
	/* Set kernel arguments. */
	ocl_status = clSetKernelArg(*krnls[0], 0, sizeof(cl_mem), &data);
	gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Set arg 0 of " CLO_SORT_SBITONIC_KNAME_0 " kernel. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

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
 * @brief Free the simple bitonic sort kernels. 
 * 
 * @see clo_sort_kernels_free()
 * */
void clo_sort_sbitonic_kernels_free(cl_kernel **krnls) {
	if (*krnls) {
		if ((*krnls)[0]) clReleaseKernel((*krnls)[0]);
		free(*krnls);
	}
}

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
	cl_uint totalStages = (cl_uint) clo_tzc(clo_nlpo2(numel));

	/* ****** Local memory kernels and any kernel ******** */

	/* Global worksize. */
	size_t gws = clo_nlpo2(numel) / 2;
	/* Local worksize. */
	size_t lws = MIN(lws_max, gws);
	/* Maximum step for unrolled kernels. */
	cl_uint maxStep = clo_tzc(lws) + 1;
	
	/* ***** Private memory kernels ****** */

	/* Global worksize. */
	size_t gws4 = clo_nlpo2(numel) / 4;
	/* Local worksize. */
	size_t lws4 = MIN(lws_max, gws4);

	/* Global worksize. */
	size_t gws8 = clo_nlpo2(numel) / 8;
	/* Local worksize. */
	size_t lws8 = MIN(lws_max, gws8);
		
	/* Global worksize. */
	size_t gws16 = clo_nlpo2(numel) / 16;
	/* Local worksize. */
	size_t lws16 = MIN(lws_max, gws16);

	/* ***** Hybrid kernels ****** */

	/* Global worksize. */
	size_t gws_h4 = clo_nlpo2(numel) / 4;
	/* Local worksize. */
	size_t lws_h4 = MIN(lws_max, gws_h4); /// @todo This is more or less, I should adjust it better or try to make it dependent on the device local memory

	/* Global worksize. */
	size_t gws_h8 = clo_nlpo2(numel) / 8;
	/* Local worksize. */
	size_t lws_h8 = MIN(lws_max, gws_h8); /// @todo This is more or less, I should adjust it better or try to make it dependent on the device local memory

	/* Global worksize. */
	size_t gws_h16 = clo_nlpo2(numel) / 16;
	/* Local worksize. */
	size_t lws_h16 = MIN(lws_max, gws_h16); /// @todo This is more or less, I should adjust it better or try to make it dependent on the device local memory

	fprintf(stderr, "----------------\n");
	/* Perform sorting. */
	for (cl_uint currentStage = 1; currentStage <= totalStages; currentStage++) {
		
		for (cl_uint currentStep = currentStage; currentStep >= 1; ) {
			
			if ((currentStep % 4 == 0) 
				&& (currentStep >= 4) 
				&& (currentStep <= 12)
				&& (((cl_uint) (1 << (currentStep - 2))) <= lws_h16)) {
				
				unsigned int krnl_idx;
				switch (currentStep) {
					case 4: krnl_idx = CLO_SORT_ABITONIC_KIDX_HYB_S4_4S16V; break;
					case 8: krnl_idx = CLO_SORT_ABITONIC_KIDX_HYB_S8_4S16V; break;
					case 12: krnl_idx = CLO_SORT_ABITONIC_KIDX_HYB_S12_4S16V; break;
				}
				
				const char* krnl_name = clo_sort_abitonic_kernelname_get(krnl_idx);

				fprintf(stderr, " %d (HYB_S%d_4S16V): Stage: %d, Step: %d,Gws: %d, Lws: %d\n", numel, currentStep, currentStage, currentStep, gws_h4, lws_h4);

				ocl_status = clSetKernelArg(krnls[krnl_idx], 1, sizeof(cl_uint), (void *) &currentStage);
				gef_if_error_create_goto(*err, CLO_ERROR, 
					ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, error_handler, 
					"arg 1 of %s kernel, OpenCL error %d: %s", krnl_name, ocl_status, clerror_get(ocl_status));
					
				evt = profile ? &evts[krnl_idx][abitonic_evt_idx[krnl_idx]] : NULL;
					
				ocl_status = clEnqueueNDRangeKernel(
					queues[0], 
					krnls[krnl_idx], 
					1, 
					NULL, 
					&gws_h16, 
					&lws_h16, 
					0, 
					NULL,
					evt
				);
				
				gef_if_error_create_goto(*err, CLO_ERROR, 
					ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, error_handler, 
					"Executing %s kernel, OpenCL error %d: %s", krnl_name, ocl_status, clerror_get(ocl_status));
					
				abitonic_evt_idx[krnl_idx]++;
				
				/* Break out of the loop. */
				break;
				
				
			} else 
			if ((currentStep % 3 == 0) 
				&& (currentStep >= 3) 
				&& (currentStep <= 12)
				&& (((cl_uint) (1 << (currentStep - 2))) <= lws_h8)) {
				
				unsigned int krnl_idx;
				switch (currentStep) {
					case 3: krnl_idx = CLO_SORT_ABITONIC_KIDX_HYB_S3_3S8V; break;
					case 6: krnl_idx = CLO_SORT_ABITONIC_KIDX_HYB_S6_3S8V; break;
					case 9: krnl_idx = CLO_SORT_ABITONIC_KIDX_HYB_S9_3S8V; break;
					case 12: krnl_idx = CLO_SORT_ABITONIC_KIDX_HYB_S12_3S8V; break;
				}
				
				const char* krnl_name = clo_sort_abitonic_kernelname_get(krnl_idx);

				fprintf(stderr, " %d (HYB_S%d_3S8V): Stage: %d, Step: %d,Gws: %d, Lws: %d\n", numel, currentStep, currentStage, currentStep, gws_h4, lws_h4);

				ocl_status = clSetKernelArg(krnls[krnl_idx], 1, sizeof(cl_uint), (void *) &currentStage);
				gef_if_error_create_goto(*err, CLO_ERROR, 
					ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, error_handler, 
					"arg 1 of %s kernel, OpenCL error %d: %s", krnl_name, ocl_status, clerror_get(ocl_status));
					
				evt = profile ? &evts[krnl_idx][abitonic_evt_idx[krnl_idx]] : NULL;
					
				ocl_status = clEnqueueNDRangeKernel(
					queues[0], 
					krnls[krnl_idx], 
					1, 
					NULL, 
					&gws_h8, 
					&lws_h8, 
					0, 
					NULL,
					evt
				);
				
				gef_if_error_create_goto(*err, CLO_ERROR, 
					ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, error_handler, 
					"Executing %s kernel, OpenCL error %d: %s", krnl_name, ocl_status, clerror_get(ocl_status));
					
				abitonic_evt_idx[krnl_idx]++;
				
				/* Break out of the loop. */
				break;
				
				
			} else 
			if ((currentStep % 2 == 0) 
				&& (currentStep >= 4) 
				&& (currentStep <= 12)
				&& (((cl_uint) (1 << (currentStep - 2))) <= lws_h4)) {
				
				unsigned int krnl_idx;
				switch (currentStep) {
					case 4: krnl_idx = CLO_SORT_ABITONIC_KIDX_HYB_S4_2S4V; break;
					case 6: krnl_idx = CLO_SORT_ABITONIC_KIDX_HYB_S6_2S4V; break;
					case 8: krnl_idx = CLO_SORT_ABITONIC_KIDX_HYB_S8_2S4V; break;
					case 10: krnl_idx = CLO_SORT_ABITONIC_KIDX_HYB_S10_2S4V; break;
					case 12: krnl_idx = CLO_SORT_ABITONIC_KIDX_HYB_S12_2S4V; break;
				}
				
				const char* krnl_name = clo_sort_abitonic_kernelname_get(krnl_idx);

				fprintf(stderr, " %d (HYB_S%d_2S4V): Stage: %d, Step: %d,Gws: %d, Lws: %d\n", numel, currentStep, currentStage, currentStep, gws_h4, lws_h4);

				ocl_status = clSetKernelArg(krnls[krnl_idx], 1, sizeof(cl_uint), (void *) &currentStage);
				gef_if_error_create_goto(*err, CLO_ERROR, 
					ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, error_handler, 
					"arg 1 of %s kernel, OpenCL error %d: %s", krnl_name, ocl_status, clerror_get(ocl_status));
					
				evt = profile ? &evts[krnl_idx][abitonic_evt_idx[krnl_idx]] : NULL;
					
				ocl_status = clEnqueueNDRangeKernel(
					queues[0], 
					krnls[krnl_idx], 
					1, 
					NULL, 
					&gws_h4, 
					&lws_h4, 
					0, 
					NULL,
					evt
				);
				
				gef_if_error_create_goto(*err, CLO_ERROR, 
					ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, error_handler, 
					"Executing %s kernel, OpenCL error %d: %s", krnl_name, ocl_status, clerror_get(ocl_status));
					
				abitonic_evt_idx[krnl_idx]++;
				
				/* Break out of the loop. */
				break;
				
				
			} else 
			if ((currentStep <= maxStep) && (currentStep >= 2)) {
				/* Use a local memory kernel. */

				fprintf(stderr, " %d (LOCAL_S%d): Stage: %d, Step: %d,Gws: %d, Lws: %d\n", numel, currentStep, currentStage, currentStep, gws, lws);

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
				
			} else if (currentStep == maxStep + 2) {

				fprintf(stderr, " %d (PRIV_2S4V): Stage: %d, Step: %d,Gws: %d, Lws: %d\n", numel, currentStage, currentStep, gws4, lws4);
			
				/* Use 2-step kernel (each thread completely sorts 4 values). */
				ocl_status = clSetKernelArg(krnls[CLO_SORT_ABITONIC_KIDX_PRIV_2S4V], 1, sizeof(cl_uint), (void *) &currentStage);
				gef_if_error_create_goto(*err, CLO_ERROR, 
					ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, 
					error_handler, 
					"arg 1 of " CLO_SORT_SBITONIC_KNAME_PRIV_2S4V " kernel, OpenCL error %d: %s", 
					ocl_status, clerror_get(ocl_status));
						
				ocl_status = clSetKernelArg(krnls[CLO_SORT_ABITONIC_KIDX_PRIV_2S4V], 2, sizeof(cl_uint), (void *) &currentStep);
				gef_if_error_create_goto(*err, CLO_ERROR, 
					ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, 
					error_handler, 
					"arg 2 of " CLO_SORT_SBITONIC_KNAME_PRIV_2S4V " kernel, OpenCL error %d: %s", 
					ocl_status, clerror_get(ocl_status));

				evt = profile ? &evts[CLO_SORT_ABITONIC_KIDX_PRIV_2S4V][abitonic_evt_idx[CLO_SORT_ABITONIC_KIDX_PRIV_2S4V]] : NULL;
						
				ocl_status = clEnqueueNDRangeKernel(
					queues[0], 
					krnls[CLO_SORT_ABITONIC_KIDX_PRIV_2S4V], 
					1, 
					NULL, 
					&gws4, 
					&lws4, 
					0, 
					NULL,
					evt
				);
					
				/* Advance two steps. */
				currentStep -= 2;
					
				gef_if_error_create_goto(*err, CLO_ERROR, 
					ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, 
					error_handler, 
					"Executing " CLO_SORT_SBITONIC_KNAME_PRIV_2S4V " kernel, OpenCL error %d: %s", 
					ocl_status, clerror_get(ocl_status));
				abitonic_evt_idx[CLO_SORT_ABITONIC_KIDX_PRIV_2S4V]++;

			} else if (currentStep == maxStep + 3) {
				


				fprintf(stderr, " %d (PRIV_3S8V): Stage: %d, Step: %d,Gws: %d, Lws: %d\n", numel, currentStage, currentStep, gws8, lws8);
			
				/* Use 3-step kernel (each thread completely sorts 8 values). */
				ocl_status = clSetKernelArg(krnls[CLO_SORT_ABITONIC_KIDX_PRIV_3S8V], 1, sizeof(cl_uint), (void *) &currentStage);
				gef_if_error_create_goto(*err, CLO_ERROR, 
					ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, 
					error_handler, 
					"arg 1 of " CLO_SORT_SBITONIC_KNAME_PRIV_3S8V " kernel, OpenCL error %d: %s", 
					ocl_status, clerror_get(ocl_status));
						
				ocl_status = clSetKernelArg(krnls[CLO_SORT_ABITONIC_KIDX_PRIV_3S8V], 2, sizeof(cl_uint), (void *) &currentStep);
				gef_if_error_create_goto(*err, CLO_ERROR, 
					ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, 
					error_handler, 
					"arg 2 of " CLO_SORT_SBITONIC_KNAME_PRIV_3S8V " kernel, OpenCL error %d: %s", 
					ocl_status, clerror_get(ocl_status));

				evt = profile ? &evts[CLO_SORT_ABITONIC_KIDX_PRIV_3S8V][abitonic_evt_idx[CLO_SORT_ABITONIC_KIDX_PRIV_3S8V]] : NULL;
						
				ocl_status = clEnqueueNDRangeKernel(
					queues[0], 
					krnls[CLO_SORT_ABITONIC_KIDX_PRIV_3S8V], 
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
					
				gef_if_error_create_goto(*err, CLO_ERROR, 
					ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, 
					error_handler, 
					"Executing " CLO_SORT_SBITONIC_KNAME_PRIV_3S8V " kernel, OpenCL error %d: %s", 
					ocl_status, clerror_get(ocl_status));
				abitonic_evt_idx[CLO_SORT_ABITONIC_KIDX_PRIV_3S8V]++;
			
				
			} else if (currentStep > maxStep + 3) {

				fprintf(stderr, " %d (PRIV_4S16V): Stage: %d, Step: %d, Gws: %d, Lws: %d\n", numel, currentStage, currentStep, gws16, lws16);

				/* Use 4-step kernel (each thread completely sorts 16 values). */
				ocl_status = clSetKernelArg(krnls[CLO_SORT_ABITONIC_KIDX_PRIV_4S16V], 1, sizeof(cl_uint), (void *) &currentStage);
				gef_if_error_create_goto(*err, CLO_ERROR, 
					ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, 
					error_handler, 
					"arg 1 of " CLO_SORT_SBITONIC_KNAME_PRIV_4S16V " kernel, OpenCL error %d: %s", 
					ocl_status, clerror_get(ocl_status));
					
				ocl_status = clSetKernelArg(krnls[CLO_SORT_ABITONIC_KIDX_PRIV_4S16V], 2, sizeof(cl_uint), (void *) &currentStep);
				gef_if_error_create_goto(*err, CLO_ERROR, 
					ocl_status != CL_SUCCESS, 
					status = CLO_ERROR_LIBRARY, 
					error_handler, 
					"arg 2 of " CLO_SORT_SBITONIC_KNAME_PRIV_4S16V " kernel, OpenCL error %d: %s", 
					ocl_status, clerror_get(ocl_status));

				evt = profile ? &evts[CLO_SORT_ABITONIC_KIDX_PRIV_4S16V][abitonic_evt_idx[CLO_SORT_ABITONIC_KIDX_PRIV_4S16V]] : NULL;
						
				ocl_status = clEnqueueNDRangeKernel(
					queues[0], 
					krnls[CLO_SORT_ABITONIC_KIDX_PRIV_4S16V], 
					1, 
					NULL, 
					&gws16, 
					&lws16, 
					0, 
					NULL,
					evt
				);
				/* Advance four steps. */
				currentStep -= 4;
				
				gef_if_error_create_goto(*err, CLO_ERROR, 
					ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, 
					error_handler, 
					"Executing " CLO_SORT_SBITONIC_KNAME_PRIV_4S16V " kernel, OpenCL error %d: %s", 
					ocl_status, clerror_get(ocl_status));
				abitonic_evt_idx[CLO_SORT_ABITONIC_KIDX_PRIV_4S16V]++;
			
			} else { /* Step = 1 */
			
				fprintf(stderr, " %d (ANY): Stage: %d, Step: %d,Gws: %d, Lws: %d\n", numel, currentStage, currentStep, gws, lws);

				/* Use "any" kernel. */
				ocl_status = clSetKernelArg(krnls[CLO_SORT_ABITONIC_KIDX_ANY], 1, sizeof(cl_uint), (void *) &currentStage);
				gef_if_error_create_goto(*err, CLO_ERROR, ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, error_handler, "arg 1 of " CLO_SORT_SBITONIC_KNAME_ANY " kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));
					
				ocl_status = clSetKernelArg(krnls[CLO_SORT_ABITONIC_KIDX_ANY], 2, sizeof(cl_uint), (void *) &currentStep);
				gef_if_error_create_goto(*err, CLO_ERROR, ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, error_handler, "arg 2 of " CLO_SORT_SBITONIC_KNAME_ANY " kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));
					
				evt = profile ? &evts[CLO_SORT_ABITONIC_KIDX_ANY][abitonic_evt_idx[CLO_SORT_ABITONIC_KIDX_ANY]] : NULL;
					
				ocl_status = clEnqueueNDRangeKernel(
					queues[0], 
					krnls[CLO_SORT_ABITONIC_KIDX_ANY], 
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
				
				gef_if_error_create_goto(*err, CLO_ERROR, ocl_status != CL_SUCCESS, status = CLO_ERROR_LIBRARY, error_handler, "Executing " CLO_SORT_SBITONIC_KNAME_ANY " kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));
				abitonic_evt_idx[CLO_SORT_ABITONIC_KIDX_ANY]++;
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
	
	if (g_strcmp0(kernel_name, CLO_SORT_SBITONIC_KNAME_ANY) == 0) {
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
	
	/* Set kernel arguments. */
	ocl_status = clSetKernelArg((*krnls)[CLO_SORT_ABITONIC_KIDX_ANY], 0, sizeof(cl_mem), &data);
	gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Set arg 0 of " CLO_SORT_SBITONIC_KNAME_ANY " kernel. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

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


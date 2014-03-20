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
 * @brief Parallel prefix sum (scan) implementation.
 */

#include "clo_scan.h"


/**
 * @brief Perform scan.
 * 
 * @see clo_sort_sort()
 */
int clo_scan(cl_command_queue *queues, cl_kernel *krnls, 
	size_t lws_max, size_t len, unsigned int numel, const char* options, 
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
	len = len;
	
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
 * */
const char* clo_scan_kernelname_get(unsigned int index) {
	g_assert_cmpuint(index, <, CLO_SCAN_NUMKERNELS);
	return kernel_names[index];
}

/** 
 * @brief Create kernels for the scan implementation. 
 * 
 * @param krnls
 * @param program
 * @param err
 * @return
 * */
int clo_scan_kernels_create(cl_kernel **krnls, cl_program program, GError **err) {
	
	/* Aux. var. */
	int status, ocl_status;
	
	/* Allocate memory for the kernels required for scan. */
	*krnls = (cl_kernel*) calloc(CLO_SCAN_NUMKERNELS, sizeof(cl_kernel));
	gef_if_error_create_goto(*err, CLO_ERROR, *krnls == NULL, status = CLO_ERROR_NOALLOC, error_handler, "Unable to allocate memory for Scan kernels.");	
	
	/* Create kernel. */
	(*krnls)[0] = clCreateKernel(program, CLO_SCAN_KNAME_WGSCAN, &ocl_status);
	gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Create " CLO_SCAN_KNAME_WGSCAN " kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

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
 * @brief Get local memory usage for the scan kernels.
 * 
 * @param kernel_name
 * @param lws_max
 * @param len
 * @return
 * */
size_t clo_scan_localmem_usage(const char* kernel_name, size_t lws_max, size_t len) {
	
	/* Local memory usage. */
	size_t lmu = 0;
	
	/*  Return local memory usage depending on given kernel. */
	if (g_strcmp(CLO_SCAN_KNAME_WGSCAN, kernel_name) == 0) {
		/* Workgroup scan kernel. */
		lmu = 2 * lws_max * len;
	} else if (g_strcmp(CLO_SCAN_KNAME_WGSUMSSCAN, kernel_name) == 0) {
		/// @todo
	} else if (g_strcmp(CLO_SCAN_KNAME_ADDWGSUMS, kernel_name) == 0) {
		/// @todo
	} else {
		g_assert_not_reached();
	}
	
	/* Return local memory usage. */
	return lmu;
}

/** 
 * @brief Set kernels arguments for the scan implemenation. 
 * */
int clo_scan_kernelargs_set(cl_kernel **krnls, cl_mem data2scan, cl_mem scanned_data, size_t lws, size_t len, GError **err) {
	
	/* Aux. var. */
	int status, ocl_status;
	
	/* Set CLO_SCAN_KNAME_WGSCAN arguments. */
	ocl_status = clSetKernelArg(*krnls[CLO_SCAN_KIDX_WGSCAN], 0, sizeof(cl_mem), &data2scan);
	gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Set arg 0 of " CLO_SCAN_KNAME_WGSCAN " kernel. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	ocl_status = clSetKernelArg(*krnls[CLO_SCAN_KIDX_WGSCAN], 1, sizeof(cl_mem), &scanned_data);
	gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Set arg 1 of " CLO_SCAN_KNAME_WGSCAN " kernel. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	ocl_status = clSetKernelArg((*krnls)[CLO_SCAN_KIDX_WGSCAN], 2, len * lws * 2, NULL);
	gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_ERROR_LIBRARY, error_handler, "Set arg 2 of " CLO_SCAN_KNAME_WGSCAN " kernel. OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

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
 * @brief Free the scan kernels. 
 *
 * */
void clo_scan_kernels_free(cl_kernel **krnls) {
	if (*krnls) {
		for (int i = 0; i < CLO_SCAN_NUMKERNELS; i++) {
			if ((*krnls)[i]) clReleaseKernel((*krnls)[i]);
		}
		free(*krnls);
	}
}

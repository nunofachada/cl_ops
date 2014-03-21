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
int clo_scan(cl_command_queue queue, cl_kernel *krnls, 
	size_t lws_max, size_t len, unsigned int numel, const char* options, 
	GArray *evts, gboolean profile, GError **err) {
	
	/* Aux. var. */
	int status, ocl_status;
		
	/* Local worksize. */
	size_t lws = MIN(lws_max, clo_nlpo2(numel));

	/* Global worksize. */
	size_t gws = CLO_GWS_MULT(numel, lws);
	
	/* OpenCL events, in case profiling is set to true. */
	cl_event evt_wgscan, evt_wgsumscan, evt_addwgsums;
	
	//~ /* Avoid compiler warnings. */
	//~ options = options;
	//~ len = len;
	
	/* Perform scan. */
	ocl_status = clSetKernelArg(krnls[CLO_SCAN_KIDX_WGSCAN], 3, sizeof(cl_uint), (void *) &numel);
	gef_if_error_create_goto(*err, CLO_ERROR, ocl_status != CL_SUCCESS, 
		status = CLO_ERROR_LIBRARY, error_handler, 
		"arg 3 of " CLO_SCAN_KNAME_WGSCAN " kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));
			
	ocl_status = clEnqueueNDRangeKernel(
		queue, 
		krnls[CLO_SCAN_KIDX_WGSCAN], 
		1, 
		NULL, 
		&gws, 
		&lws, 
		0, 
		NULL,
		profile ? &evt_wgscan : NULL
	);
	gef_if_error_create_goto(*err, CLO_ERROR, ocl_status != CL_SUCCESS, 
		status = CLO_ERROR_LIBRARY, error_handler, 
		"Executing " CLO_SCAN_KNAME_WGSCAN " kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));
		
	/// @todo WG SUM SCANS
	
	/// @todo ADD WG SUMS

	/* If profilling is on, add events to array. */
	if (profile) {
		ProfCLEvName ev_name = { .eventName = CLO_SCAN_KNAME_WGSCAN, .event = evt_wgscan};
		g_array_append_val(evts, ev_name);
		/// @todo Events from remaining two kernels
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
	const char* kernel_name;
	switch (index) {
		case CLO_SCAN_KIDX_WGSCAN:
			kernel_name = CLO_SCAN_KNAME_WGSCAN; break;
		case CLO_SCAN_KIDX_WGSUMSSCAN:
			kernel_name = CLO_SCAN_KNAME_WGSUMSSCAN; break;
		case CLO_SCAN_KIDX_ADDWGSUMS:
			kernel_name = CLO_SCAN_KNAME_ADDWGSUMS; break;
		default:
			g_assert_not_reached();
	}
	return kernel_name;
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
	if (g_strcmp0(CLO_SCAN_KNAME_WGSCAN, kernel_name) == 0) {
		/* Workgroup scan kernel. */
		lmu = 2 * lws_max * len;
	} else if (g_strcmp0(CLO_SCAN_KNAME_WGSUMSSCAN, kernel_name) == 0) {
		/// @todo
	} else if (g_strcmp0(CLO_SCAN_KNAME_ADDWGSUMS, kernel_name) == 0) {
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

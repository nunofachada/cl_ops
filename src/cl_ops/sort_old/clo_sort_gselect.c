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
 * Global memory selection sort host implementation.
 */

#include "clo_sort_gselect.h"

/* Array of kernel names. */
static const char* const kernel_names[] = CLO_SORT_GSELECT_KERNELNAMES;

/**
 * Sort agents using the selection sort.
 *
 * @see clo_sort_sort()
 */
int clo_sort_gselect_sort(cl_command_queue *queues, cl_kernel *krnls,
	size_t lws_max, size_t len, unsigned int numel, const char* options,
	GArray *evts, gboolean profile, GError **err) {

	/* Aux. var. */
	int status, ocl_status;

	/* Local worksize. */
	size_t lws = lws_max;

	/* Global worksize. */
	size_t gws = lws * ((numel + lws - 1) / lws);

	/* OpenCL event, in case profiling is on. */
	cl_event evt0, evt1;

	/* Output buffer. */
	cl_mem outbuf = NULL;

	/* OpenCL context. */
	cl_context context;

	/* Avoid compiler warnings. */
	options = options;

	/* Get the OpenCL context. */
	ocl_status = clGetCommandQueueInfo(queues[0], CL_QUEUE_CONTEXT,
		sizeof(cl_context), &context, NULL);
	gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status,
		status = CLO_ERROR_LIBRARY, error_handler,
		"Get context from command queue. OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	/* Allocate memory for the output buffer. */
	outbuf = clCreateBuffer(context, CL_MEM_READ_WRITE, len * numel, NULL, &ocl_status);
	gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status,
		status = CLO_ERROR_LIBRARY, error_handler,
		"Create buffer: gselection sort output buffer. OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	/* Set sort time kernel arguments. */
	for (guint32 i = 0; i < CLO_SORT_GSELECT_NUMKERNELS; i++) {
		ocl_status = clSetKernelArg(krnls[i], 1, sizeof(cl_mem), &outbuf);
		gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status,
			status = CLO_ERROR_LIBRARY, error_handler,
			"Set arg 1 of %s kernel. OpenCL error %d: %s", clo_sort_gselect_kernelname_get(i), ocl_status, clerror_get(ocl_status));

		ocl_status = clSetKernelArg(krnls[i], 2, sizeof(cl_uint), (void *) &numel);
		gef_if_error_create_goto(*err, CLO_ERROR, ocl_status != CL_SUCCESS,
			status = CLO_ERROR_LIBRARY, error_handler,
			"arg 2 of %s kernel, OpenCL error %d: %s", clo_sort_gselect_kernelname_get(i), ocl_status, clerror_get(ocl_status));
	}

	/* Perform sorting. */
	ocl_status = clEnqueueNDRangeKernel(
		queues[0],
		krnls[0],
		1,
		NULL,
		&gws,
		&lws,
		0,
		NULL,
		profile ? &evt0 : NULL
	);
	gef_if_error_create_goto(*err, CLO_ERROR, ocl_status != CL_SUCCESS,
		status = CLO_ERROR_LIBRARY, error_handler,
		"Executing " CLO_SORT_GSELECT_KNAME_0 " kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	/* Perform copy. */
	ocl_status = clEnqueueNDRangeKernel(
		queues[0],
		krnls[1],
		1,
		NULL,
		&gws,
		&lws,
		0,
		NULL,
		profile ? &evt1 : NULL
	);
	gef_if_error_create_goto(*err, CLO_ERROR, ocl_status != CL_SUCCESS,
		status = CLO_ERROR_LIBRARY, error_handler,
		"Executing " CLO_SORT_GSELECT_KNAME_1 " kernel, OpenCL error %d: %s", ocl_status, clerror_get(ocl_status));

	/* If profilling is on, add event to array. */
	if (profile) {
		ProfCLEvName ev_name0 = { .eventName = CLO_SORT_GSELECT_KNAME_0, .event = evt0};
		ProfCLEvName ev_name1 = { .eventName = CLO_SORT_GSELECT_KNAME_1, .event = evt1};
		g_array_append_val(evts, ev_name0);
		g_array_append_val(evts, ev_name1);
	}

	/* If we got here, everything is OK. */
	status = CLO_SUCCESS;
	g_assert(err == NULL || *err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);

finish:

	/* Release helper memory buffer. */
	if (outbuf) clReleaseMemObject(outbuf);

	/* Return. */
	return status;
}

/**
 * Returns the name of the kernel identified by the given
 * index.
 *
 * @see clo_sort_kernelname_get()
 * */
const char* clo_sort_gselect_kernelname_get(unsigned int index) {
	g_assert_cmpuint(index, <, CLO_SORT_GSELECT_NUMKERNELS);
	return kernel_names[index];
}

/**
 * Create kernels for the selection sort.
 *
 * @see clo_sort_kernels_create()
 * */
int clo_sort_gselect_kernels_create(cl_kernel **krnls, cl_program program, GError **err) {

	/* Aux. var. */
	int status, ocl_status;

	/* Allocate memory for single kernel required for selection sort. */
	*krnls = (cl_kernel*) calloc(CLO_SORT_GSELECT_NUMKERNELS, sizeof(cl_kernel));
	gef_if_error_create_goto(*err, CLO_ERROR, *krnls == NULL,
		status = CLO_ERROR_NOALLOC, error_handler,
		"Unable to allocate memory for global memory selection sort kernels.");

	/* Create kernels. */
	for (guint32 i = 0; i < CLO_SORT_GSELECT_NUMKERNELS; i++) {
		(*krnls)[i] = clCreateKernel(program, clo_sort_gselect_kernelname_get(i), &ocl_status);
		gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status,
			status = CLO_ERROR_LIBRARY, error_handler,
			"Create %s kernel. OpenCL error %d: %s", clo_sort_gselect_kernelname_get(i), ocl_status, clerror_get(ocl_status));
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
 * Get local memory usage for the selection sort kernels.
 *
 * Global memory selection sort only uses one kernel which doesn't require
 * local memory.
 *
 * @see clo_sort_localmem_usage()
 * */
size_t clo_sort_gselect_localmem_usage(const char* kernel_name, size_t lws_max, size_t len, unsigned int numel) {

	/* Avoid compiler warnings. */
	kernel_name = kernel_name;
	lws_max = lws_max;
	numel = numel;
	len = len;

	/* Global memory selection sort doesn't use local memory. */
	return 0;
}

/**
 * Set kernels arguments for the selection sort.
 *
 * @see clo_sort_kernelargs_set()
 * */
int clo_sort_gselect_kernelargs_set(cl_kernel **krnls, cl_mem data, size_t lws, size_t len, GError **err) {

	/* Aux. var. */
	int status, ocl_status;

	/* LWS and len not used here. */
	lws = lws;
	len = len;

	/* Set sort time kernel arguments. */
	for (guint32 i = 0; i < CLO_SORT_GSELECT_NUMKERNELS; i++) {
		ocl_status = clSetKernelArg((*krnls)[i], 0, sizeof(cl_mem), &data);
		gef_if_error_create_goto(*err, CLO_ERROR, CL_SUCCESS != ocl_status,
			status = CLO_ERROR_LIBRARY, error_handler,
			"Set arg 0 of %s kernel. OpenCL error %d: %s", clo_sort_gselect_kernelname_get(i), ocl_status, clerror_get(ocl_status));
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
 * Free the selection sort kernels.
 *
 * @see clo_sort_kernels_free()
 * */
void clo_sort_gselect_kernels_free(cl_kernel **krnls) {
	if (*krnls) {
		for (guint32 i = 0; i < CLO_SORT_GSELECT_NUMKERNELS; i++) {
			if ((*krnls)[i]) clReleaseKernel((*krnls)[i]);
		}
		free(*krnls);
	}
}

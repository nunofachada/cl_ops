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
 *
 * This implementation is based on the design described in:
 * Blelloch, G. E. "Prefix Sums and Their Applications.", Technical
 * Report CMU-CS-90-190, School of Computer Science, Carnegie Mellon
 * University, 1990.
 *
 * A maximum of three kernels are called for problems of any size, with
 * the first kernel serializing the scan operation when the array size
 * is larger than the squared local worksize.
 * */

#include "clo_scan.h"

/**
 * @brief Perform scan.
 *
 * @see clo_sort_sort()
 */
cl_bool clo_scan(CCLQueue* queue, CCLKernel** krnls, size_t lws_max,
	size_t size_elem, size_t size_sum, unsigned int numel,
	const char* options, GError **err) {

	/* Aux. vars. */
	int status;

	/* Temporary buffer. */
	CCLBuffer* dev_wgsums;

	/* Local worksize. */
	size_t lws = MIN(lws_max, clo_nlpo2(numel) / 2);

	/* OpenCL context wrapper. */
	CCLContext* ctx;

	/* Internal error reporting object. */
	GError* err_internal = NULL;

	/* Global worksizes. */
	size_t gws_wgscan, ws_wgsumsscan, gws_addwgsums;

	/* Avoid compiler warnings. */
	options = options;
	size_elem = size_elem;

	/* Determine worksizes. */
	gws_wgscan = MIN(CLO_GWS_MULT(numel / 2, lws), lws * lws);
	ws_wgsumsscan = (gws_wgscan / lws) / 2;
	gws_addwgsums = CLO_GWS_MULT(numel, lws);

	/* Determine number of blocks to be processed per workgroup. */
	cl_uint blocks_per_wg = CLO_DIV_CEIL(numel / 2, gws_wgscan);
	cl_uint numel_cl = numel;

	/* Get the context wrapper. */
	ctx = ccl_queue_get_context(queue, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	dev_wgsums = ccl_buffer_new(ctx, CL_MEM_READ_WRITE,
		(gws_wgscan / lws) * size_sum, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Set kernel arguments. */
	ccl_kernel_set_arg(krnls[CLO_SCAN_KIDX_WGSCAN], 2, dev_wgsums);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	ccl_kernel_set_arg(krnls[CLO_SCAN_KIDX_WGSCAN], 4, ccl_arg_priv(numel_cl, cl_uint));
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	ccl_kernel_set_arg(krnls[CLO_SCAN_KIDX_WGSCAN], 5, ccl_arg_priv(blocks_per_wg, cl_uint));
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Perform workgroup-wise scan on complete array. */
	ccl_kernel_enqueue_ndrange(krnls[CLO_SCAN_KIDX_WGSCAN], queue, 1,
		NULL, &gws_wgscan, &lws, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	g_debug("N: %d, GWS1: %d, WS2: %d, GWS3: %d | LWS: %d | BPWG=%d | Enter? %s", numel, (int) gws_wgscan, (int) ws_wgsumsscan, (int) gws_addwgsums, (int) lws, blocks_per_wg, gws_wgscan > lws ? "YES!" : "NO!");
	if (gws_wgscan > lws) {

		/* Perform scan on workgroup sums array. */
		ccl_kernel_set_arg(krnls[CLO_SCAN_KIDX_WGSUMSSCAN], 0, dev_wgsums);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);

		ccl_kernel_enqueue_ndrange(krnls[CLO_SCAN_KIDX_WGSUMSSCAN],
			queue, 1, NULL, &ws_wgsumsscan, &ws_wgsumsscan, NULL,
			&err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);

		/* Add the workgroup-wise sums to the respective workgroup elements.*/
		ccl_kernel_set_arg(krnls[CLO_SCAN_KIDX_ADDWGSUMS], 0, dev_wgsums);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);

		ccl_kernel_set_arg(krnls[CLO_SCAN_KIDX_ADDWGSUMS], 2, ccl_arg_priv(blocks_per_wg, cl_uint));
		ccl_if_err_propagate_goto(err, err_internal, error_handler);

		ccl_kernel_enqueue_ndrange(krnls[CLO_SCAN_KIDX_ADDWGSUMS],
			queue, 1, NULL, &gws_addwgsums, &lws, NULL,
			&err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);

	}

	/* If we got here, everything is OK. */
	status = CL_TRUE;
	g_assert(err == NULL || *err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	status = CL_FALSE;

finish:

	/* Release temporary buffer. */
	if (dev_wgsums) ccl_buffer_destroy(dev_wgsums);

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
CCLKernel** clo_scan_kernels_create(CCLProgram* prg, GError **err) {

	/* Kernels to create. */
	CCLKernel** krnls = NULL;

	/* Internal error reporting object. */
	GError* err_internal = NULL;

	/* Allocate memory for the kernels required for scan. */
	krnls = g_new0(CCLKernel*, CLO_SCAN_NUMKERNELS);

	/* Create kernels. */
	krnls[CLO_SCAN_KIDX_WGSCAN] = ccl_program_get_kernel(
		prg, CLO_SCAN_KNAME_WGSCAN, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	krnls[CLO_SCAN_KIDX_WGSUMSSCAN] = ccl_program_get_kernel(
		prg, CLO_SCAN_KNAME_WGSUMSSCAN, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	krnls[CLO_SCAN_KIDX_ADDWGSUMS] = ccl_program_get_kernel(
		prg, CLO_SCAN_KNAME_ADDWGSUMS, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	if (krnls) clo_scan_kernels_free(krnls);
	krnls = NULL;

finish:

	/* Return. */
	return krnls;

}

/**
 * @brief Get local memory usage for the scan kernels.
 *
 * @param kernel_name
 * @param lws_max
 * @param len
 * @return
 * */
size_t clo_scan_localmem_usage(const char* kernel_name, size_t lws_max,
	size_t size_elem, size_t size_sum) {

	/* Local memory usage. */
	size_t lmu = 0;

	/* Avoid compiler warnings. */
	size_elem = size_elem;

	/*  Return local memory usage depending on given kernel. */
	if (g_strcmp0(CLO_SCAN_KNAME_WGSCAN, kernel_name) == 0) {
		/* Workgroup scan kernel. */
		lmu = 2 * lws_max * size_sum + 1;
	} else if (g_strcmp0(CLO_SCAN_KNAME_WGSUMSSCAN, kernel_name) == 0) {
		/* Workgroup sums scan kernel. */
		lmu = 2 * lws_max * size_sum;
	} else if (g_strcmp0(CLO_SCAN_KNAME_ADDWGSUMS, kernel_name) == 0) {
		/* Add workgroup sums kernel. */
		lmu = size_sum;
	} else {
		g_assert_not_reached();
	}

	/* Return local memory usage. */
	return lmu;
}

/**
 * @brief Set kernels arguments for the scan implemenation.
 * */
cl_bool clo_scan_kernelargs_set(CCLKernel** krnls, CCLBuffer* data2scan,
	CCLBuffer* scanned_data, size_t lws, size_t size_elem,
	size_t size_sum, GError **err) {

	/* Aux. var. */
	cl_bool status;

	/* Internal error reporting object. */
	GError* err_internal = NULL;

	/* Avoid compiler warnings. */
	size_elem = size_elem;

	/* Set CLO_SCAN_KNAME_WGSCAN arguments. */
	ccl_kernel_set_arg(krnls[CLO_SCAN_KIDX_WGSCAN], 0, data2scan);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	ccl_kernel_set_arg(krnls[CLO_SCAN_KIDX_WGSCAN], 1, scanned_data);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	ccl_kernel_set_arg(krnls[CLO_SCAN_KIDX_WGSCAN], 3, ccl_arg_full(NULL, size_sum * lws * 2));
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Set CLO_SCAN_KNAME_WGSUMSSCAN arguments. */
	ccl_kernel_set_arg(krnls[CLO_SCAN_KIDX_WGSUMSSCAN], 1, ccl_arg_full(NULL, size_sum * lws * 2));
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Set CLO_SCAN_KNAME_ADDWGSUMS arguments. */
	ccl_kernel_set_arg(krnls[CLO_SCAN_KIDX_ADDWGSUMS], 1, scanned_data);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	status = CL_TRUE;
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	status = CL_FALSE;

finish:

	/* Return. */
	return status;

}

/**
 * @brief Free the scan kernels.
 *
 * */
void clo_scan_kernels_free(CCLKernel** krnls) {
	if (krnls) g_free(krnls);
}

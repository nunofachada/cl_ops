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
 * Parallel prefix sum (scan) abstract definitions.
 * */

#include "clo_scan_abstract.h"
#include "clo_scan_blelloch.h"

/**
 * @addtogroup CLO_SCAN
 * @{
 */

/**
 * Scanner class.
 * */
struct clo_scan {

	/** @private Scan implementation. */
	CloScanImplDef impl_def;

	/** @private Context wrapper. */
	CCLContext* ctx;

	/** @private Program wrapper. */
	CCLProgram* prg;

	/** @private Type of elements to scan. */
	CloType elem_type;

	/** @private Type of elements in scan sum. */
	CloType sum_type;

	/** @private Scan implementation data. */
	void* data;

};

/**
 * Generic scan object constructor. The exact type is given in the
 * first parameter.
 *
 * @public @memberof clo_scan
 *
 * @param[in] type Name of scan algorithm class to create.
 * @param[in] options Algorithm options.
 * @param[in] ctx OpenCL context wrapper.
 * @param[in] elem_type Type of elements to scan.
 * @param[in] sum_type Type of scanned elements.
 * @param[in] compiler_opts OpenCL Compiler options.
 * @param[out] err Return location for a GError, or `NULL` if error
 * reporting is to be ignored.
 * @return A new scan object of the specified type or `NULL` if an
 * error occurs.
 * */
CloScan* clo_scan_new(const char* type, const char* options,
	CCLContext* ctx, CloType elem_type, CloType sum_type,
	const char* compiler_opts, GError** err) {

	/* Make sure type is not NULL. */
	g_return_val_if_fail(type != NULL, NULL);
	/* Make sure context is not NULL. */
	g_return_val_if_fail(ctx != NULL, NULL);
	/* Make sure err is NULL or it is not set. */
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	/* The list of known scan implementations. */
	CloScanImplDef scan_impl_defs[] = {
		clo_scan_blelloch_def,
		{ NULL, NULL, NULL, NULL, NULL, NULL, NULL }
	};

	/* Scanner object. */
	CloScan* scanner = NULL;

	/* Final compiler options. */
	gchar* compiler_opts_final = NULL;

	/* Internal error management object. */
	GError *err_internal = NULL;

	/* Scan program. */
	CCLProgram* prg = NULL;

	/* Scan source code. */
	const char* src;

	/* Search in the list of known scan classes. */
	for (guint i = 0; scan_impl_defs[i].name != NULL; ++i) {
		if (g_strcmp0(type, scan_impl_defs[i].name) == 0) {
			/* If found, create a new instance and initialize it.*/

			/* Allocate memory for scan object. */
			scanner = g_slice_new0(CloScan);

			/* Keep data in scanner object. */
			scanner->impl_def = scan_impl_defs[i];
			ccl_context_ref(ctx);
			scanner->ctx = ctx;
			scanner->elem_type = elem_type;
			scanner->sum_type = sum_type;

			/* Determine final compiler options. */
			compiler_opts_final = g_strconcat(
				" -DCLO_SCAN_ELEM_TYPE=", clo_type_get_name(elem_type),
				" -DCLO_SCAN_SUM_TYPE=", clo_type_get_name(sum_type),
				compiler_opts, NULL);

			/* Initialize scanner implementation. */
			src = scanner->impl_def.init(
				scanner, options, &err_internal);
			ccl_if_err_propagate_goto(err, err_internal, error_handler);

			/* Create and build scanner program. */
			prg = ccl_program_new_from_source(
				scanner->ctx, src, &err_internal);
			ccl_if_err_propagate_goto(err, err_internal, error_handler);

			ccl_program_build(prg, compiler_opts_final, &err_internal);
			ccl_if_err_propagate_goto(err, err_internal, error_handler);

			/* Set scanner program. */
			scanner->prg = prg;
		}
	}

	/* Check if an implementation was indeed found. */
	ccl_if_err_create_goto(*err, CLO_ERROR, scanner == NULL,
		CLO_ERROR_IMPL_NOT_FOUND, error_handler,
		"The requested scan implementation, '%s', was not found.",
		type);

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);

	if (scanner) { clo_scan_destroy(scanner); scanner = NULL; }

finish:

	/* Free stuff. */
	if (compiler_opts_final) g_free(compiler_opts_final);

	/* Return scanner object. */
	return scanner;
}

/**
 * Destroy scanner object.
 *
 * @public @memberof clo_scan
 *
 * @param[in] scan Scanner object to destroy.
 * */
void clo_scan_destroy(CloScan* scan) {

	/* Check scanner object is not NULL. */
	g_return_if_fail(scan != NULL);

	/* Finalize specific scan implementation stuff. */
	scan->impl_def.finalize(scan);

	/* Unreference context. */
	if (scan->ctx) ccl_context_unref(scan->ctx);

	/* Destroy program. */
	if (scan->prg) ccl_program_destroy(scan->prg);

	/* Free scanner object memory. */
	g_slice_free(CloScan, scan);

}

/**
 * Perform scan using device data.
 *
 * @public @memberof clo_scan
 *
 * @param[in] scanner Scanner object.
 * @param[in] cq_exec A valid command queue wrapper for kernel
 * execution, cannot be `NULL`.
 * @param[in] cq_comm A command queue wrapper for data transfers.
 * If `NULL`, `cq_exec` will be used for data transfers.
 * @param[in] data_in Data to be scanned.
 * @param[out] data_out Location where to place scanned data.
 * @param[in] numel Number of elements in `data_in`.
 * @param[in] lws_max Max. local worksize. If 0, the local worksize
 * will be automatically determined.
 * @param[out] err Return location for a GError, or `NULL` if error
 * reporting is to be ignored.
 * @return An event wait list which contains events which must
 * terminate before scanning is considered complete.
 * */
CCLEventWaitList clo_scan_with_device_data(CloScan* scanner,
	CCLQueue* cq_exec, CCLQueue* cq_comm, CCLBuffer* data_in,
	CCLBuffer* data_out, size_t numel, size_t lws_max, GError** err) {

	/* Make sure scanner object is not NULL. */
	g_return_val_if_fail(scanner != NULL, NULL);

	/* Make sure err is NULL or it is not set. */
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	/* Make sure cq_exec is not NULL. */
	g_return_val_if_fail(cq_exec != NULL, NULL);

	/* Use specific implementation. */
	return scanner->impl_def.scan_with_device_data(scanner, cq_exec,
		cq_comm, data_in, data_out, numel, lws_max, err);

}

/**
 * Perform scan using host data.
 *
 * @public @memberof clo_scan
 *
 * @param[in] scanner Scanner object.
 * @param[in] cq_exec Command queue wrapper for kernel execution. If
 * `NULL` a queue will be created.
 * @param[in] cq_comm A command queue wrapper for data transfers.
 * If `NULL`, `cq_exec` will be used for data transfers.
 * @param[in] data_in Data to be scanned.
 * @param[out] data_out Location where to place scanned data.
 * @param[in] numel Number of elements in `data_in`.
 * @param[in] lws_max Max. local worksize. If 0, the local worksize
 * will be automatically determined.
 * @param[out] err Return location for a GError, or `NULL` if error
 * reporting is to be ignored.
 * @return `CL_TRUE` if scan was successfully performed, `CL_FALSE`
 * otherwise.
 * */
cl_bool clo_scan_with_host_data(CloScan* scanner,
	CCLQueue* cq_exec, CCLQueue* cq_comm, void* data_in, void* data_out,
	size_t numel, size_t lws_max, GError** err) {

	/* Make sure scanner object is not NULL. */
	g_return_val_if_fail(scanner != NULL, CL_FALSE);

	/* Make sure err is NULL or it is not set. */
	g_return_val_if_fail(err == NULL || *err == NULL, CL_FALSE);

	/* Function return status. */
	cl_bool status;

	/* OpenCL wrapper objects. */
	CCLEvent* evt = NULL;
	CCLBuffer* data_in_dev = NULL;
	CCLBuffer* data_out_dev = NULL;
	CCLQueue* intern_queue = NULL;
	CCLDevice* dev = NULL;

	/* Event wait list. */
	CCLEventWaitList ewl = NULL;

	/* Internal error object. */
	GError* err_internal = NULL;

	/* Determine data sizes. */
	size_t data_in_size = numel * clo_type_sizeof(scanner->elem_type);
	size_t data_out_size = numel * clo_type_sizeof(scanner->sum_type);

	/* If execution queue is NULL, create own queue using first device
	 * in context. */
	if (cq_exec == NULL) {
		/* Get first device in queue. */
		dev = ccl_context_get_device(scanner->ctx, 0, &err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
		/* Create queue. */
		intern_queue = ccl_queue_new(
			scanner->ctx, dev, 0, &err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
		cq_exec = intern_queue;
	}

	/* If data transfer queue is NULL, use exec queue for data
	 * transfers. */
	if (cq_comm == NULL) cq_comm = cq_exec;

	/* Create device buffers. */
	data_in_dev = ccl_buffer_new(
		scanner->ctx, CL_MEM_READ_ONLY, data_in_size, NULL,
		&err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	data_out_dev = ccl_buffer_new(
		scanner->ctx, CL_MEM_READ_WRITE, data_out_size, NULL,
		&err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Transfer data to device. */
	evt = ccl_buffer_enqueue_write(data_in_dev, cq_comm, CL_FALSE, 0,
		data_in_size, data_in, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	ccl_event_set_name(evt, "clo_scan_write");

	/* Explicitly wait for transfer (some OpenCL implementations don't
	 * respect CL_TRUE in data transfers). */
	ccl_event_wait_list_add(&ewl, evt, NULL);
	ccl_event_wait(&ewl, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Perform scan with device data. */
	ewl = scanner->impl_def.scan_with_device_data(scanner, cq_exec,
		cq_comm, data_in_dev, data_out_dev, numel, lws_max,
		&err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Transfer data back to host. */
	evt = ccl_buffer_enqueue_read(data_out_dev, cq_comm, CL_FALSE, 0,
		data_out_size, data_out, &ewl, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	ccl_event_set_name(evt, "clo_scan_read");

	/* Explicitly wait for transfer (some OpenCL implementations don't
	 * respect CL_TRUE in data transfers). */
	ccl_event_wait_list_add(&ewl, evt, NULL);
	ccl_event_wait(&ewl, &err_internal);
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

	/* Free stuff. */
	if (data_in_dev) ccl_buffer_destroy(data_in_dev);
	if (data_out_dev) ccl_buffer_destroy(data_out_dev);
	if (intern_queue) ccl_queue_destroy(intern_queue);

	/* Return function status. */
	return status;

}

/**
 * Get context wrapper associated with scanner object.
 *
 * @public @memberof clo_scan
 *
 * @param[in] scanner Scanner object.
 * @return Context wrapper associated with scanner object.
 * */
CCLContext* clo_scan_get_context(CloScan* scanner) {

	/* Make sure scanner object is not NULL. */
	g_return_val_if_fail(scanner != NULL, NULL);

	return scanner->ctx;
}

/**
 * Get program wrapper associated with scanner object.
 *
 * @public @memberof clo_scan
 *
 * @param[in] scanner Scanner object.
 * @return Program wrapper associated with scanner object.
 * */
CCLProgram* clo_scan_get_program(CloScan* scanner) {

	/* Make sure scanner object is not NULL. */
	g_return_val_if_fail(scanner != NULL, NULL);

	return scanner->prg;
}
/**
 * Get type of elements to scan.
 *
 * @public @memberof clo_scan
 *
 * @param[in] scanner Scanner object.
 * @return Type of elements to scan.
 * */
CloType clo_scan_get_elem_type(CloScan* scanner) {

	/* Make sure scanner object is not NULL. */
	g_return_val_if_fail(scanner != NULL, -1);

	return scanner->elem_type;
}


/**
 * Get the size in bytes of each element to be scanned.
 *
 * @public @memberof clo_scan
 *
 * @param[in] scanner Scanner object.
 * @return Size in bytes of each element to be scanned.
 * */
size_t clo_scan_get_element_size(CloScan* scanner) {

	/* Make sure scanner object is not NULL. */
	g_return_val_if_fail(scanner != NULL, 0);

	/* Return element size. */
	return clo_type_sizeof(scanner->elem_type);

}

/**
 * Get type of elements in scan sum.
 *
 * @public @memberof clo_scan
 *
 * @param[in] scanner Scanner object.
 * @return Type of elements in scan sum.
 * */
CloType clo_scan_get_sum_type(CloScan* scanner) {

	/* Make sure scanner object is not NULL. */
	g_return_val_if_fail(scanner != NULL, -1);

	return scanner->sum_type;
}

/**
 * Get the size in bytes of element in scan sum.
 *
 * @public @memberof clo_scan
 *
 * @param[in] scanner Scanner object.
 * @return Size in bytes of of element in scan sum.
 * */
size_t clo_scan_get_sum_size(CloScan* scanner) {

	/* Make sure scanner object is not NULL. */
	g_return_val_if_fail(scanner != NULL, 0);

	/* Return element size. */
	return clo_type_sizeof(scanner->sum_type);

}


/**
 * Get data associated with specific scan implementation.
 *
 * @public @memberof clo_scan
 *
 * @param[in] scanner Scanner object.
 * @return Data associated with specific scan implementation.
 * */
void* clo_scan_get_data(CloScan* scanner) {

	/* Make sure scanner object is not NULL. */
	g_return_val_if_fail(scanner != NULL, NULL);

	return scanner->data;
}

/**
 * Set scan specific data.
 *
 * @public @memberof clo_scan
 *
 * @param[in] scanner Scanner object.
 * @param[in] data Sort specific data.
 * */
void clo_scan_set_data(CloScan* scanner, void* data) {

	/* Make sure scanner object is not NULL. */
	g_return_if_fail(scanner != NULL);

	/* Set scan specific data. */
	scanner->data = data;

}

/**
 * Get the maximum number of kernels used by the scan implementation.
 *
 * @public @memberof clo_scan
 *
 * @param[in] scanner Scanner object.
 * @param[out] err Return location for a GError, or `NULL` if error
 * reporting is to be ignored.
 * @return Maximum number of kernels used by the scan implementation.
 * */
cl_uint clo_scan_get_num_kernels(CloScan* scanner, GError** err) {

	/* Make sure scanner object is not NULL. */
	g_return_val_if_fail(scanner != NULL, 0);

	/* Return number of kernels. */
	return scanner->impl_def.get_num_kernels(scanner, err);
}

/**
 * Get name of the i^th kernel used by the scan implementation.
 *
 * @public @memberof clo_scan
 *
 * @param[in] scanner Scanner object.
 * @param[in] i i^th kernel used by the scan implementation.
 * @param[out] err Return location for a GError, or `NULL` if error
 * reporting is to be ignored.
 * @return The name of the i^th kernel used by the scan implementation.
 * */
const char* clo_scan_get_kernel_name(
	CloScan* scanner, cl_uint i, GError** err) {

	/* Make sure scanner object is not NULL. */
	g_return_val_if_fail(scanner != NULL, NULL);

	/* Return kernel name. */
	return scanner->impl_def.get_kernel_name(scanner, i, err);
}

/**
 * Get local memory usage of i^th kernel used by the scan implementation
 * for the given maximum local worksize and number of elements to scan.
 *
 * @public @memberof clo_scan
 *
 * @param[in] scanner Scanner object.
 * @param[in] i i^th kernel used by the scan implementation.
 * @param[in] lws_max Max. local worksize. If 0, the local worksize
 * is automatically determined and the returned memory usage corresponds
 * to this value.
 * @param[in] numel Number of elements to scan.
 * @param[out] err Return location for a GError, or `NULL` if error
 * reporting is to be ignored.
 * @return The local memory usage of i^th kernel used by the scan
 * implementation for the given maximum local worksize and number of
 * elements to scan.
 * */
size_t clo_scan_get_localmem_usage(CloScan* scanner, cl_uint i,
	size_t lws_max, size_t numel, GError** err) {

	/* Make sure scanner object is not NULL. */
	g_return_val_if_fail(scanner != NULL, 0);

	/* Return local memory usage. */
	return scanner->impl_def.get_localmem_usage(
		scanner, i, lws_max, numel, err);

}

/** @} */

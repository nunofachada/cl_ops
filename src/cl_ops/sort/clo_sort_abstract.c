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
 * Abstract definitions for a sort algorithm.
 * */

#include "clo_sort_abstract.h"
#include "clo_sort_sbitonic.h"
#include "clo_sort_abitonic.h"
#include "clo_sort_gselect.h"
#include "clo_sort_satradix.h"

/**
 * @addtogroup CLO_SORT
 * @{
 */

/**
 * Sorter class.
 * */
struct clo_sort {

	/** @private Sort implementation. */
	CloSortImplDef impl_def;

	/** @private Context wrapper. */
	CCLContext* ctx;

	/** @private Program wrapper. */
	CCLProgram* prg;

	/** @private Type of elements to sort. */
	CloType elem_type;

	/** @private Type of keys to sort. */
	CloType key_type;

	/** @private Scan implementation data. */
	void* data;

};


/**
 * Generic sort object constructor. The exact type is given in the
 * first parameter.
 *
 * @public @memberof clo_sort
 *
 * @param[in] type Name of sort algorithm class to create.
 * @param[in] options Algorithm options.
 * @param[in] ctx OpenCL context wrapper.
 * @param[in] elem_type Type of elements from which to get the keys to
 * sort.
 * @param[in] key_type Type of keys to sort (if NULL, defaults to the
 * element type).
 * @param[in] compare One-liner OpenCL C code string which compares two
 * keys, a and b, yielding a boolean; e.g. `((a) < (b))` will sort
 * element in descendent order. If NULL, this defaults to
 * `((a) > (b))`, i.e. sort in ascendent order.
 * @param[in] get_key One-liner OpenCL C code string which obtains a
 * value of type `key_type` from the element (of type `elem_type`)
 * defined in the `x` variable; e.g. `((x) & 0xF)` will obtain a key
 * which consists of four LSBs of the corresponding element. If NULL,
 * this defaults to `(type_of_key) (x)`, which is simply the element
 * cast to the key type.
 * @param[in] compiler_opts OpenCL Compiler options.
 * @param[out] err Return location for a GError, or `NULL` if error
 * reporting is to be ignored.
 * @return A new sort object of the specified type or `NULL` if an
 * error occurs.
 * */
CloSort* clo_sort_new(const char* type, const char* options,
	CCLContext* ctx, CloType* elem_type, CloType* key_type,
	const char* compare, const char* get_key, const char* compiler_opts,
	GError** err) {

	/* Make sure err is NULL or it is not set. */
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	/* Sorter object to create. */
	CloSort* sorter = NULL;
	/* Sort algorithm source code. */
	const char* src;
	/* Sort macros builder. */
	GString* ocl_macros = NULL;
	/* Complete source (macros + algorithm source). */
	const char* src_full[2];
	/* Internal error handling object. */
	GError* err_internal = NULL;

	/* Known sort implementations. */
	CloSortImplDef sort_impls[] = {
		clo_sort_sbitonic_def,
		clo_sort_abitonic_def,
		clo_sort_gselect_def,
		clo_sort_satradix_def,
		{ NULL, CL_FALSE, NULL, NULL, NULL, NULL }
	};

	/* Search in the list of known sort classes. */
	for (guint i = 0; sort_impls[i].name != NULL; ++i) {
		if (g_strcmp0(type, sort_impls[i].name) == 0) {

			/* Allocate memory for sorter object. */
			sorter = g_slice_new0(CloSort);

			/* Set implementation definition. */
			sorter->impl_def = sort_impls[i];

			/* Keep context, program and element type. */
			ccl_context_ref(ctx);
			sorter->ctx  = ctx;
			sorter->elem_type = *elem_type;
			sorter->key_type =
				(key_type != NULL) ? *key_type : *elem_type;

			/* Initialize specific sort implementation and get source
			 * code. */
			src = sorter->impl_def.init(sorter, options, &err_internal);
			ccl_if_err_propagate_goto(err, err_internal, error_handler);

			/* Build sort macros which define element and key types,
			 * comparison type and how to obtain a key from the
			 * corresponding element. */
			ocl_macros = g_string_new("");

			/* Element type. */
			g_string_append_printf(ocl_macros,
				"#define CLO_SORT_ELEM_TYPE %s\n",
				clo_type_get_name(sorter->elem_type));

			/* Key type. */
			g_string_append_printf(ocl_macros,
				"#define CLO_SORT_KEY_TYPE %s\n",
				clo_type_get_name(sorter->key_type));

			/* Comparison type. */
			g_string_append_printf(ocl_macros,
				"#define CLO_SORT_COMPARE(a, b) %s\n",
				compare != NULL
					? compare
					: "((a) > (b))");

			/* Getting a key from the element. */
			g_string_append_printf(ocl_macros,
				"#define CLO_SORT_KEY_GET(x) %s\n",
				get_key != NULL
					? get_key
					: "(x)");

			/* Create and build program. */
			src_full[0] = (const char*) ocl_macros->str;
			src_full[1] = src;
			sorter->prg = ccl_program_new_from_sources(
				ctx, 2, src_full, NULL, &err_internal);
			ccl_if_err_propagate_goto(err, err_internal, error_handler);

			ccl_program_build(
				sorter->prg, compiler_opts, &err_internal);
			ccl_if_err_propagate_goto(err, err_internal, error_handler);

		}
	}

	ccl_if_err_create_goto(*err, CLO_ERROR, sorter == NULL,
		CLO_ERROR_IMPL_NOT_FOUND, error_handler,
		"The requested sort implementation, '%s', was not found.",
		type);

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);

	if (sorter) clo_sort_destroy(sorter);
	sorter = NULL;

finish:

	/* Free stuff. */
	if (ocl_macros) g_string_free(ocl_macros, TRUE);

	/* Return new sorter instance. */
	return sorter;
}

/**
 * Destroy a sorter object.
 *
 * @public @memberof clo_sort
 *
 * @param[in] sorter Sorter object to destroy.
 * */
void clo_sort_destroy(CloSort* sorter) {

	/* Make sure sorter object is not NULL. */
	g_return_if_fail(sorter != NULL);

	/* Free specific algorithm data. */
	sorter->impl_def.finalize(sorter);

	/* Unreference context wrapper. */
	if (sorter->ctx) ccl_context_unref(sorter->ctx);

	/* Destroy program wrapper. */
	if (sorter->prg) ccl_program_destroy(sorter->prg);

	/* Free sorter object. */
	g_slice_free(CloSort, sorter);
}

/**
 * Perform sort using device data.
 *
 * @public @memberof clo_sort
 *
 * @param[in] sorter Sorter object.
 * @param[in] cq_exec A valid command queue wrapper for kernel
 * execution, cannot be `NULL`.
 * @param[in] cq_comm A command queue wrapper for data transfers.
 * If `NULL`, `cq_exec` will be used for data transfers.
 * @param[in] data_in Data to be sorted.
 * @param[out] data_out Location where to place sorted data. If
 * `NULL`, data will be sorted in-place or copied back from auxiliar
 * device buffer, depending on the sort implementation.
 * @param[in] numel Number of elements in `data_in`.
 * @param[in] lws_max Max. local worksize. If 0, the local worksize
 * will be automatically determined.
 * @param[out] err Return location for a GError, or `NULL` if error
 * reporting is to be ignored.
 * @return An event wait list which contains events which must
 * terminate before sorting is considered complete.
 * */
CCLEventWaitList clo_sort_with_device_data(CloSort* sorter,
	CCLQueue* cq_exec, CCLQueue* cq_comm, CCLBuffer* data_in,
	CCLBuffer* data_out, size_t numel, size_t lws_max, GError** err) {

	/* Make sure scanner object is not NULL. */
	g_return_val_if_fail(sorter != NULL, NULL);

	/* Make sure err is NULL or it is not set. */
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	/* Make sure cq_exec is not NULL. */
	g_return_val_if_fail(cq_exec != NULL, NULL);

	/* Use specific implementation. */
	return sorter->impl_def.sort_with_device_data(sorter, cq_exec,
		cq_comm, data_in, data_out, numel, lws_max, err);

}

/**
 * Perform sort using host data. Device buffers will be created and
 * destroyed by sort implementation.
 *
 * @public @memberof clo_sort
 *
 * @param[in] sorter Sorter object.
 * @param[in] cq_exec Command queue wrapper for kernel execution. If
 * `NULL` a queue will be created.
 * @param[in] cq_comm A command queue wrapper for data transfers.
 * If `NULL`, `cq_exec` will be used for data transfers.
 * @param[in] data_in Data to be sorted.
 * @param[out] data_out Location where to place sorted data.
 * @param[in] numel Number of elements in `data_in`.
 * @param[in] lws_max Max. local worksize. If 0, the local worksize
 * will be automatically determined.
 * @param[out] err Return location for a GError, or `NULL` if error
 * reporting is to be ignored.
 * @return `CL_TRUE` if sort was successfully performed, `CL_FALSE`
 * otherwise.
 * */
cl_bool clo_sort_with_host_data(CloSort* sorter, CCLQueue* cq_exec,
	CCLQueue* cq_comm, void* data_in, void* data_out, size_t numel,
	size_t lws_max, GError** err) {

	/* Make sure scanner object is not NULL. */
	g_return_val_if_fail(sorter != NULL, CL_FALSE);

	/* Make sure err is NULL or it is not set. */
	g_return_val_if_fail(err == NULL || *err == NULL, CL_FALSE);

	/* Function return status. */
	cl_bool status;

	/* OpenCL wrapper objects. */
	CCLContext* ctx = NULL;
	CCLBuffer* data_in_dev = NULL;
	CCLBuffer* data_out_dev = NULL;
	CCLBuffer* data_aux_dev = NULL;
	CCLBuffer* data_read_dev = NULL;
	CCLQueue* intern_queue = NULL;
	CCLDevice* dev = NULL;
	CCLEvent* evt;

	/* Event wait list. */
	CCLEventWaitList ewl = NULL;

	/* Internal error object. */
	GError* err_internal = NULL;

	/* Determine data size. */
	size_t data_size = numel * clo_type_sizeof(sorter->elem_type);

	/* Get context wrapper. */
	ctx = clo_sort_get_context(sorter);

	/* If execution queue is NULL, create own queue using first device
	 * in context. */
	if (cq_exec == NULL) {
		/* Get first device in context. */
		dev = ccl_context_get_device(ctx, 0, &err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
		/* Create queue. */
		intern_queue = ccl_queue_new(ctx, dev, 0, &err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
		cq_exec = intern_queue;
	}

	/* If data transfer queue is NULL, use exec queue for data
	 * transfers. */
	if (cq_comm == NULL) cq_comm = cq_exec;

	/* Create device data in buffer. */
	data_in_dev = ccl_buffer_new(
		ctx, CL_MEM_READ_ONLY, data_size, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Create device data out buffer if sort does not occur in place. */
	if (!sorter->impl_def.in_place) {
		/* Create buffer, put it in aux var (which will be released if
		 * not NULL). */
		data_aux_dev = ccl_buffer_new(
			ctx, CL_MEM_WRITE_ONLY, data_size, NULL, &err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
		/* Set data out to data aux. */
		data_out_dev = data_aux_dev;
		/* Sorted data will be read from data aux. */
		data_read_dev = data_aux_dev;
	} else {
		/* If sorting is in-place, sorted data will be read from data
		 * in. */
		data_read_dev = data_in_dev;
	}

	/* Transfer data to device. */
	evt = ccl_buffer_enqueue_write(data_in_dev, cq_comm, CL_FALSE, 0,
		data_size, data_in, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	ccl_event_set_name(evt, "write_gselect");

	/* Explicitly wait for transfer (some OpenCL implementations don't
	 * respect CL_TRUE in data transfers). */
	ccl_event_wait_list_add(&ewl, evt, NULL);
	ccl_event_wait(&ewl, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Perform sort with device data. */
	ewl = sorter->impl_def.sort_with_device_data(sorter, cq_exec,
		cq_comm, data_in_dev, data_out_dev, numel, lws_max,
		&err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Transfer data back to host. */
	evt = ccl_buffer_enqueue_read(data_read_dev, cq_comm, CL_FALSE, 0,
		data_size, data_out, &ewl, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	ccl_event_set_name(evt, "read_gselect");

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
	if (data_aux_dev) ccl_buffer_destroy(data_aux_dev);
	if (intern_queue) ccl_queue_destroy(intern_queue);

	/* Return function status. */
	return status;

}

/**
 * Get the context wrapper associated with the given sorter object.
 *
 * @public @memberof clo_sort
 *
 * @param[in] sorter Sorter object.
 * @return The context wrapper associated with the given sorter object.
 * */
CCLContext* clo_sort_get_context(CloSort* sorter) {

	/* Make sure sorter object is not NULL. */
	g_return_val_if_fail(sorter != NULL, NULL);

	/* Return context wrapper. */
	return sorter->ctx;
}


/**
 * Get the program wrapper associated with the given sorter object.
 *
 * @public @memberof clo_sort
 *
 * @param[in] sorter Sorter object.
 * @return The program wrapper associated with the given sorter object.
 * */
CCLProgram* clo_sort_get_program(CloSort* sorter) {

	/* Make sure sorter object is not NULL. */
	g_return_val_if_fail(sorter != NULL, NULL);

	/* Return program wrapper. */
	return sorter->prg;
}

/**
 * Get the element type associated with the given sorter object.
 *
 * @public @memberof clo_sort
 *
 * @param[in] sorter Sorter object.
 * @return The element type associated with the given sorter object.
 * */
CloType clo_sort_get_element_type(CloSort* sorter) {

	/* Make sure sorter object is not NULL. */
	g_return_val_if_fail(sorter != NULL, -1);

	/* Return element type. */
	return sorter->elem_type;
}

/**
 * Get the size in bytes of each element to be sorted.
 *
 * @public @memberof clo_sort
 *
 * @param[in] sorter Sorter object.
 * @return The size in bytes of each element to be sorted.
 * */
size_t clo_sort_get_element_size(CloSort* sorter) {

	/* Make sure sorter object is not NULL. */
	g_return_val_if_fail(sorter != NULL, 0);

	/* Return element size. */
	return clo_type_sizeof(sorter->elem_type);

}

/**
 * Get the key type associated with the given sorter object.
 *
 * @public @memberof clo_sort
 *
 * @param[in] sorter Sorter object.
 * @return The key type associated with the given sorter object.
 * */
CloType clo_sort_get_key_type(CloSort* sorter) {

	/* Make sure sorter object is not NULL. */
	g_return_val_if_fail(sorter != NULL, -1);

	/* Return key type. */
	return sorter->key_type;
}

/**
 * Get the size in bytes of each key to be sorted.
 *
 * @public @memberof clo_sort
 *
 * @param[in] sorter Sorter object.
 * @return The size in bytes of each key to be sorted.
 * */
size_t clo_sort_get_key_size(CloSort* sorter) {

	/* Make sure sorter object is not NULL. */
	g_return_val_if_fail(sorter != NULL, 0);

	/* Return key size. */
	return clo_type_sizeof(sorter->key_type);

}

/**
 * Get sort specific data.
 *
 * @public @memberof clo_sort
 *
 * @param[in] sorter Sorter object.
 * @return Sort specific data.
 * */
void* clo_sort_get_data(CloSort* sorter) {

	/* Make sure sorter object is not NULL. */
	g_return_val_if_fail(sorter != NULL, NULL);

	/* Return sort specific data. */
	return sorter->data;

}

/**
 * Set sort specific data.
 *
 * @public @memberof clo_sort
 *
 * @param[in] sorter Sorter object.
 * @param[in] data Sort specific data.
 * */
void clo_sort_set_data(CloSort* sorter, void* data) {

	/* Make sure sorter object is not NULL. */
	g_return_if_fail(sorter != NULL);

	/* Set sort specific data. */
	sorter->data = data;

}

/**
 * Get the maximum number of kernels used by the sort implementation.
 *
 * @public @memberof clo_sort
 *
 * @param[in] sorter Sorter object.
 * @return Maximum number of kernels used by the sort implementation.
 * */
cl_uint clo_sort_get_num_kernels(CloSort* sorter) {

	/* Make sure sorter object is not NULL. */
	g_return_if_fail(sorter != NULL);

	/* Return number of kernels. */
	return sorter->impl_def.get_num_kernels(sorter);
}

/** @} */

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
 * @brief Abstract definitions for a sort algorithm.
 * */

#include "clo_sort_abstract.h"
#include "clo_sort_sbitonic.h"
#include "clo_sort_gselect.h"

/**
 * @internal
 * Internal sorter data.
 * */
struct clo_sort_data {

	/** Finalize function. */
	void (*finalize)(CloSort* sorter);

	/** Context wrapper. */
	CCLContext* ctx;
	/** Program wrapper. */
	CCLProgram* prg;
	/** Element type. */
	CloType elem_type;
	/** Sort-specific internal data. */
	void* other_data;
};

/**
 * This struct associates a sort type name with its initializer
 * function.
 * */
struct ocl_sort_impl {

	/**
	 * Sort algorithm name.
	 * @private
	 * */
	const char* name;

	/**
	 * Sort algorithm initializer function.
	 * @private
	 * */
	const char* (*init)(CloSort* sorter, const char* options, GError** err);

	/**
	 * Sort algorithm finalizer function.
	 * @private
	 * */
	void (*finalize)(CloSort* sorter);
};

/**
 * @internal
 * List of sort implementations.
 * */
static const struct ocl_sort_impl const sort_impls[] = {
	{ "sbitonic", clo_sort_sbitonic_init, clo_sort_sbitonic_finalize },
	//~ { "abitonic", clo_sort_abitonic_new },
	{ "gselect", clo_sort_gselect_init, clo_sort_gselect_finalize },
	//~ { "satradix", clo_sort_satradix_new },
	{ NULL, NULL, NULL }
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
 * keys, a and b, yielding a boolean; e.g. `"((a) < (b))"` will sort
 * element in descendent order. If NULL, this defaults to
 * `"((a) > (b))"`, i.e. sort in ascendent order.
 * @param[in] get_key One-liner OpenCL C code string which obtains a
 * value of type `key_type` from the element (of type `elem_type`)
 * defined in the `x` variable; e.g. `"((x) & 0xF)"` will obtain a key
 * which consists of four LSBs of the corresponding element. If NULL,
 * this defaults to `"(type_of_key) (x)"`, which is simply the element
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

	/* Search in the list of known sort classes. */
	for (guint i = 0; sort_impls[i].name != NULL; ++i) {
		if (g_strcmp0(type, sort_impls[i].name) == 0) {

			/* Allocate memory for sorter object. */
			sorter = g_slice_new0(CloSort);

			/* Allocate memory for sorter object data. */
			sorter->_data = g_slice_new0(struct clo_sort_data);

			/* Set finalizer. */
			sorter->_data->finalize = sort_impls[i].finalize;

			/* Initialize specific sort implementation and get source
			 * code. */
			src = sort_impls[i].init(sorter, options, err);

			/* Build sort macros which define element and key types,
			 * comparison type and how to obtain a key from the
			 * corresponding element. */
			ocl_macros = g_string_new("");

			/* Element type. */
			g_string_append_printf(ocl_macros,
				"#define CLO_SORT_ELEM_TYPE %s\n",
				clo_type_get_name(*elem_type, NULL));

			/* Key type. */
			g_string_append_printf(ocl_macros,
				"#define CLO_SORT_KEY_TYPE %s\n",
				key_type != NULL
					? clo_type_get_name(*key_type, NULL)
					: clo_type_get_name(*elem_type, NULL));

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
			sorter->_data->prg = ccl_program_new_from_sources(
				ctx, 2, src_full, NULL, &err_internal);
			ccl_if_err_propagate_goto(err, err_internal, error_handler);

			ccl_program_build(
				sorter->_data->prg, compiler_opts, &err_internal);
			ccl_if_err_propagate_goto(err, err_internal, error_handler);

			/* Keep context, program and element type. */
			ccl_context_ref(ctx);
			sorter->_data->ctx  = ctx;
			sorter->_data->elem_type = *elem_type;

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
	g_return_val_if_fail(sorter != NULL, NULL);

	/* Free specific algorithm data. */
	sorter->_data->finalize(sorter);

	/* Unreference context wrapper. */
	if (sorter->_data->ctx) ccl_context_unref(sorter->_data->ctx);

	/* Destroy program wrapper. */
	if (sorter->_data->prg) ccl_program_destroy(sorter->_data->prg);

	/* Free sorter object data. */
	g_slice_free(struct clo_sort_data, sorter->_data);

	/* Free sorter object. */
	g_slice_free(CloSort, sorter);
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
	return sorter->_data->ctx;
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
	return sorter->_data->prg;
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
	return sorter->_data->elem_type;
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
	g_return_val_if_fail(sorter != NULL, NULL);

	/* Return element size. */
	return clo_type_sizeof(sorter->_data->elem_type, NULL);

}

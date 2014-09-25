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

/**
 * Abstract sort class definition.
 * */
struct ocl_sort_impl {

	/**
	 * Sort algorithm name.
	 * @private
	 * */
	const char* name;

	/**
	 * Sort algorithm constructor.
	 * @private
	 * */
	CloSort* (*new)(const char* options, CCLContext* ctx,
		CloType elem_type, const char* compiler_opts, GError** err);
};

/**
 * @internal
 * The list of known sort implementations.
 * */
static const struct ocl_sort_impl const sort_impls[] = {
	//~ { "sbitonic", clo_sort_sbitonic_new },
	//~ { "abitonic", clo_sort_abitonic_new },
	//~ { "gselect", clo_sort_gselect_new },
	//~ { "satradix", clo_sort_satradix_new },
	{ NULL, NULL }
};

/**
 * Generic sort object constructor. The exact type is given in the
 * first parameter.
 *
 * @param[in] type Name of sort algorithm class to create.
 * @param[in] options Algorithm options.
 * @param[in] ctx OpenCL context wrapper.
 * @param[in] elem_type Type of elements to sort.
 * @param[in] compiler_opts OpenCL Compiler options.
 * @param[out] err Return location for a GError, or `NULL` if error
 * reporting is to be ignored.
 * @return A new sort object of the specified type or `NULL` if an
 * error occurs.
 * */
CloSort* clo_sort_new(const char* type, const char* options,
	CCLContext* ctx, CloType elem_type, const char* compiler_opts,
	GError** err) {

	/* Search in the list of known sort classes. */
	for (guint i = 0; sort_impls[i].name != NULL; ++i) {
		if (g_strcmp0(type, sort_impls[i].name) == 0) {
			/* If found, return an new instance.*/
			return sort_impls[i].new(options, ctx, elem_type,
				compiler_opts, err);
		}
	}

	/* If not found, specify error and return NULL. */
	g_set_error(err, CLO_ERROR, CLO_ERROR_IMPL_NOT_FOUND,
		"The requested sort implementation, '%s', was not found.", type);
	return NULL;
}


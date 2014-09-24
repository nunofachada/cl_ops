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
 * @brief Parallel prefix sum (scan) abstract definitions.
 * */

#include "clo_scan_abstract.h"
#include "clo_scan_blelloch.h"

/**
 * Abstract scan class definition.
 * */
struct ocl_scan_impl {

	/**
	 * Scan algorithm name.
	 * @private
	 * */
	const char* name;

	/**
	 * Scan algorithm constructor.
	 * @private
	 * */
	CloScan* (*new)(const char* options, CCLContext* ctx,
		CloType elem_type, CloType sum_type, const char* compiler_opts,
		GError** err);
};

/**
 * @internal
 * The list of known scan implementations.
 * */
static const struct ocl_scan_impl const scan_impls[] = {
	{ "blelloch", clo_scan_blelloch_new },
	{ NULL, NULL }
};

/**
 * Generic scan object constructor. The exact type is given in the
 * first parameter.
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

	/* Search in the list of known scan classes. */
	for (guint i = 0; scan_impls[i].name != NULL; ++i) {
		if (g_strcmp0(type, scan_impls[i].name) == 0) {
			/* If found, return an new instance.*/
			return scan_impls[i].new(options, ctx, elem_type, sum_type,
				compiler_opts, err);
		}
	}

	/* If not found, specify error and return NULL. */
	g_set_error(err, CLO_ERROR, CLO_ERROR_IMPL_NOT_FOUND,
		"The requested scan implementation, '%s', was not found.", type);
	return NULL;
}

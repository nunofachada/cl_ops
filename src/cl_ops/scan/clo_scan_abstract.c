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

#include "clo_scan_abstract.h"

struct ocl_scan_impl {
	const char* name;
	CloScan* (*new)(const char* options, CCLContext* ctx,
		CloType elem_type, CloType sum_type, const char* compiler_opts);
};

static const struct ocl_scan_impl* const scan_impls = {
	{ "blelloch", clo_scan_blelloch_new },
	{ NULL, NULL }
};

CloScan* clo_scan_new(const char* type, const char* options,
	CCLContext* ctx, CloType elem_type, CloType sum_type,
	const char* compiler_opts, GError** err) {

	for (guint i = 0; scan_impls[i].name != NULL; ++i) {
		if (g_strcmp0(type, scan_impls[i].name) == 0) {
			return scan.impls[i].new(options, ctx, elem_type, sum_type,
				compiler_opts, err);
		}
	}

	g_set_error(err, CLO_ERROR, CLO_ERROR_IMPL_NOT_FOUND,
		"The requested scan implementation, '%s', was not found.", type);

	return NULL;
}

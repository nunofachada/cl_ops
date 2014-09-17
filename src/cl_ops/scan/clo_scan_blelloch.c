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

#include "clo_scan_blelloch.h"

/** The kernels source. */
#define CLO_SCAN_BLELLOCH_SRC "@CLO_SCAN_BLELLOCH_SRC@"

struct clo_scan_blelloch_data {
	CCLContext* ctx;
	CCLProgram* prg;
	CloType elem_type;
	CloType sum_type;
}

static cl_bool clo_scan_blelloch_scan(CCLQueue* queue, void* data_in, void* data_out,
		cl_uint numel, size_t lws_max) {

	return CL_FALSE;

}


static void clo_scan_blelloch_destroy(CloScan* scan) {
	ccl_context_unref(
		((struct clo_scan_blelloch_data) scan)->_data->ctx);
	if (scan->_data->prg) ccl_program_destroy(prg);
	g_free(scan->_data);
	g_free(scan);
}

CloScan* clo_scan_blelloch_new(const char* options, CCLContext* ctx,
	CloType elem_type, CloType sum_type, const char* compiler_opts, &err) {

	/* Internal error management object. */
	GError *err_internal = NULL;

	gchar* compiler_opts_final;

	/* Allocate memory for scan object. */
	CloScan* scan = g_new0(CloScan, 1);

	/* Allocate data for private scan data. */
	struct clo_scan_blelloch_data data =
		g_new0(struct clo_scan_blelloch_data, 1);

	/* Keep data in scan private data. */
	ccl_context_ref(ctx);
	data->ctx = ctx;
	data->elem_size = elem_size;
	data->sum_size = sum_size;

	/* Set object methods. */
	scan->destroy = clo_scan_blelloch_destroy;
	scan->scan = clo_scan_blelloch_scan;
	scan->_data = data;

	/* For now ignore specific blelloch scan options. */
	/* ... */

	/* Determine final compiler options. */
	compiler_opts_final = g_strconcat(compiler_opts,
		" -DCLO_SCAN_ELEM_TYPE=", clo_type_get_name(elem_type),
		" -DCLO_SCAN_SUM_TYPE=", clo_type_get_name(sum_type),
		NULL);

	/* Create and build program. */
	data->prg = ccl_program_new_from_source(
		ctx, CLO_SCAN_BLELLOCH_SRC, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	ccl_program_build(data->prg, compiler_opts_final, &err);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	clo_scan_blelloch_destroy(scan);
	scan = NULL;

finish:

	/* Return scan object. */
	return scan;

}


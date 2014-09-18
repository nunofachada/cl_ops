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

struct clo_scan_blelloch_data {
	CCLContext* ctx;
	CCLProgram* prg;
	CloType elem_type;
	CloType sum_type;
};

/// Does not accept NULL queue
static cl_bool clo_scan_blelloch_scan_with_device_data(CloScan* scanner,
	CCLQueue* queue, CCLBuffer* data_in, CCLBuffer* data_out,
	cl_uint numel, size_t lws_max, double* duration, GError** err) {

	return CL_FALSE;

}

/// Accepts NULL queue
static cl_bool clo_scan_blelloch_scan_with_host_data(CloScan* scanner,
	CCLQueue* queue, void* data_in, void* data_out, cl_uint numel,
	size_t lws_max, double* duration, GError** err) {

	cl_bool status;

	CCLBuffer* data_in_dev = NULL;
	CCLBuffer* data_out_dev = NULL;
	CCLQueue* intern_queue = NULL;
	CCLDevice* dev = NULL;
	GError* err_internal = NULL;

	struct clo_scan_blelloch_data* data =
		(struct clo_scan_blelloch_data*) scanner->_data;

	size_t data_in_size = numel * clo_type_sizeof(data->elem_type, NULL);
	size_t data_out_size = numel * clo_type_sizeof(data->sum_type, NULL);

	/* If queue is NULL, create own queue using first device in
	 * context. */
	if (queue == NULL) {
		dev = ccl_context_get_device(data->ctx, 0, &err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
		intern_queue = ccl_queue_new(data->ctx, dev, 0, &err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
		queue = intern_queue;
	}

	/* Create device buffers. */
	data_in_dev = ccl_buffer_new(
		data->ctx, CL_MEM_READ_ONLY, data_in_size, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);
	data_out_dev = ccl_buffer_new(
		data->ctx, CL_MEM_READ_WRITE, data_out_size, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Transfer data to device. */
	ccl_buffer_enqueue_write(data_in_dev, queue, CL_TRUE, 0,
		data_in_size, data_in, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Perform scan with device data. */
	clo_scan_blelloch_scan_with_device_data(scanner, queue, data_in_dev,
		data_out_dev, numel, lws_max, duration, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Transfer data back to host. */
	ccl_buffer_enqueue_read(data_out_dev, queue, CL_TRUE, 0,
		data_out_size, data_out, NULL, &err_internal);
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

static void clo_scan_blelloch_destroy(CloScan* scan) {
	struct clo_scan_blelloch_data* data =
		(struct clo_scan_blelloch_data*) scan->_data;
	ccl_context_unref(data->ctx);
	if (data->prg) ccl_program_destroy(data->prg);
	g_free(scan->_data);
	g_free(scan);
}

CloScan* clo_scan_blelloch_new(const char* options, CCLContext* ctx,
	CloType elem_type, CloType sum_type, const char* compiler_opts,
	GError** err) {

	/* Internal error management object. */
	GError *err_internal = NULL;

	gchar* compiler_opts_final;

	/* Allocate memory for scan object. */
	CloScan* scan = g_new0(CloScan, 1);

	/* Allocate data for private scan data. */
	struct clo_scan_blelloch_data* data =
		g_new0(struct clo_scan_blelloch_data, 1);

	/* Keep data in scan private data. */
	ccl_context_ref(ctx);
	data->ctx = ctx;
	data->elem_type = elem_type;
	data->sum_type = sum_type;

	/* Set object methods. */
	scan->destroy = clo_scan_blelloch_destroy;
	scan->scan_with_host_data = clo_scan_blelloch_scan_with_host_data;
	scan->scan_with_device_data = clo_scan_blelloch_scan_with_device_data;
	scan->_data = data;

	/* For now ignore specific blelloch scan options. */
	/* ... */

	/* Determine final compiler options. */
	compiler_opts_final = g_strconcat(compiler_opts,
		" -DCLO_SCAN_ELEM_TYPE=", clo_type_get_name(elem_type, NULL),
		" -DCLO_SCAN_SUM_TYPE=", clo_type_get_name(sum_type, NULL),
		NULL);

	/* Create and build program. */
	data->prg = ccl_program_new_from_source(
		ctx, CLO_SCAN_BLELLOCH_SRC, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	ccl_program_build(data->prg, compiler_opts_final, &err_internal);
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


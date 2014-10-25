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
 * Satish et al Radix Sort host implementation.
 */

#include "clo_sort_satradix.h"

/**
 * Perform sort using device data.
 *
 * @copydetails ::CloSort::sort_with_device_data()
 * */
static CCLEventWaitList clo_sort_satradix_sort_with_device_data(
	CloSort* sorter, CCLQueue* cq_exec, CCLQueue* cq_comm,
	CCLBuffer* data_in, CCLBuffer* data_out, size_t numel,
	size_t lws_max, GError** err) {

	/* Make sure err is NULL or it is not set. */
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	/* Make sure cq_exec is not NULL. */
	g_return_val_if_fail(cq_exec != NULL, NULL);

	/* Event wait list. */
	CCLEventWaitList ewl = NULL;

	/* Internal error reporting object. */
	GError* err_internal = NULL;

	//~ /* Add last event to wait list to return. */
	//~ ccl_event_wait_list_add(&ewl, evt, NULL);

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);

finish:

	/* Return. */
	return ewl;

}

/**
 * Initializes a SatRadix sorter object and returns the
 * respective source code.
 *
 * @copydetails ::CloSort::init()
 * */
static const char* clo_sort_satradix_init(
	CloSort* sorter, const char* options, GError** err) {

	/* Make sure err is NULL or it is not set. */
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	/* Ignore specific sbitonic sort options and error handling. */
	(void)options;
	(void)err;
	(void)sorter;

	/* Return source to be compiled. */
	return CLO_SORT_SATRADIX_SRC;

}


/**
 * Finalizes a SatRadix sorter object.
 *
 * @copydetails ::CloSort::finalize()
 * */
static void clo_sort_satradix_finalize(CloSort* sorter) {
	/* Nothing to finalize. */
	(void)sorter;
	return;
}

/* Definition of the satradix sort implementation. */
const CloSortImplDef clo_sort_satradix_def = {
	"satradix",
	CL_TRUE,
	clo_sort_satradix_init,
	clo_sort_satradix_finalize,
	clo_sort_satradix_sort_with_device_data
};

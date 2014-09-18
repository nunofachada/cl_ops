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

#ifndef _CLO_SCAN_ABSTRACT_H_
#define _CLO_SCAN_ABSTRACT_H_

#include "common/clo_common.h"

typedef struct clo_scan {

	cl_bool (*scan)(CCLQueue* queue, void* data_in, void* data_out,
		cl_uint numel, size_t lws_max);

	void (*destroy)(struct clo_scan* scan);

	void* _data;

} CloScan;

CloScan* clo_scan_new(const char* type, const char* options,
	CCLContext* ctx, CloType elem_type, CloType sum_type,
	const char* compiler_opts, GError** err);

#endif




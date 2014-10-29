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
 * Aggregate header for CL-Ops.
 *
 * @author Nuno Fachada
 */

#ifndef _CL_OPS_H_
#define _CL_OPS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Common header. */
#include <cl_ops/clo_common.h>

/* RNG header. */
#include <cl_ops/clo_rng.h>

/* Sort headers. */
#include <cl_ops/clo_sort_abstract.h>
#include <cl_ops/clo_sort_abitonic.h>
#include <cl_ops/clo_sort_sbitonic.h>
#include <cl_ops/clo_sort_gselect.h>
#include <cl_ops/clo_sort_satradix.h>

/* Scan headers. */
#include <cl_ops/clo_scan_abstract.h>
#include <cl_ops/clo_scan_blelloch.h>

#ifdef __cplusplus
}
#endif


#endif

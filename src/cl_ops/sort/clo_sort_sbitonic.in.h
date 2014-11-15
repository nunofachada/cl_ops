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
 * Simple bitonic sort header file.
 */

#ifndef _CLO_SORT_SBITONIC_H_
#define _CLO_SORT_SBITONIC_H_

#include "cl_ops/clo_sort_abstract.h"

/** The simple bitonic sort kernels source. */
#define CLO_SORT_SBITONIC_SRC "@SBITONIC_SRC@"

/** The name of the simple bitonic sort kernel. */
#define CLO_SORT_SBITONIC_KNAME "sbitonic"

/** Definition of the sbitonic sort implementation. */
extern const CloSortImplDef clo_sort_sbitonic_def;

#endif

/*
 * This file is part of CL_Ops.
 *
 * CL_Ops is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * CL_Ops is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with CL_Ops. If not, see
 * <http://www.gnu.org/licenses/>.
 * */

/**
 * @file
 * Global memory selection sort header file.
 */

#ifndef _CLO_SORT_GSELECT_H_
#define _CLO_SORT_GSELECT_H_

#include "cl_ops/clo_sort_abstract.h"

/** The global selection sort kernels source. */
#define CLO_SORT_GSELECT_SRC "@GSELECT_SRC@"

/** The name of the simple bitonic sort kernel. */
#define CLO_SORT_GSELECT_KNAME "gselect"

/** Definition of the gselect sort implementation. */
extern const CloSortImplDef clo_sort_gselect_def;

#endif


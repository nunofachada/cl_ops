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
 * Blelloch scan declarations.
 * */

#ifndef _CLO_SCAN_BLELLOCH_H_
#define _CLO_SCAN_BLELLOCH_H_

#include "clo_scan_abstract.h"

/** The blelloch's scan kernels source. */
#define CLO_SCAN_BLELLOCH_SRC "@BLELLOCH_SRC@"

/* Number of kernels. */
#define CLO_SCAN_BLELLOCH_NUM_KERNELS 3

/* Index of the blelloch scan kernels. */
#define CLO_SCAN_BLELLOCH_KIDX_WGSCAN 0
#define CLO_SCAN_BLELLOCH_KIDX_WGSUMSSCAN 1
#define CLO_SCAN_BLELLOCH_KIDX_ADDWGSUMS 2

/* Blelloch scan kernel names. */
#define CLO_SCAN_BLELLOCH_KNAME_WGSCAN "workgroupScan"
#define CLO_SCAN_BLELLOCH_KNAME_WGSUMSSCAN "workgroupSumsScan"
#define CLO_SCAN_BLELLOCH_KNAME_ADDWGSUMS "addWorkgroupSums"

/** Definition of the Blelloch scan implementation. */
extern const CloScanImplDef clo_scan_blelloch_def;

#endif

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

/**
 * @file
 * Header for scan benchmark.
 */

#ifndef _CLO_SCAN_BENCHMARK_H
#define _CLO_SCAN_BENCHMARK_H

#include "clo_bench.h"

#define CLO_SCAN_HOST_GET(host_data, i, bytes) \
	((unsigned long) \
	(bytes == 1) ? ((unsigned char*) host_data)[i] : \
	((bytes == 2) ? ((unsigned short*) host_data)[i] : \
	((bytes == 4) ? ((unsigned int*) host_data)[i] : \
	((unsigned long*) host_data)[i])))

#define CLO_SCAN_MAXU(bytes) \
	((unsigned long) \
	(bytes == 1) ? 0xFF : \
	((bytes == 2) ? G_MAXUSHORT : \
	((bytes == 4) ? G_MAXUINT : \
	(G_MAXULONG))))

#endif


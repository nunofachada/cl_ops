/*
 * This file is part of CL_Ops.
 *
 * CL_Ops is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CL_Ops is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CL_Ops.  If not, see <http://www.gnu.org/licenses/>.
 * */

/**
 * @file
 * Header for common benchmark functions.
 */

#ifndef _CLO_BENCHMARK_H_
#define _CLO_BENCHMARK_H_

#include <cl_ops.h>

cl_int clo_bench_compare(CloType type, cl_uchar* a, cl_uchar* b);

void clo_bench_rand(GRand* rng, CloType type, void* location);

#endif


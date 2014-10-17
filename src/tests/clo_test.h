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
 * @brief Header for common test functions test.
 */

#ifndef _CLO_TEST_H_
#define _CLO_TEST_H_

#include "common/clo_common.h"

cl_int clo_test_compare(CloType type, cl_uchar* a, cl_uchar* b);

void clo_test_rand(GRand* rng, CloType type, void* location);

#endif


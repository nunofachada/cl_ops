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
 * @brief Ocl-Ops RNG algorithms header file.
 */

#ifndef CLO_RNG_H
#define CLO_RNG_H

#include "clo_common.h"

#define CLO_RNGS "lcg, xorshift64, xorshift128, mwc64x"

#define CLO_DEFAULT_RNG "lcg"

/**
 * @brief Information about a RNG.
 * */	
typedef struct clo_rng_info {
	char* tag;            /**< Tag identifying the RNG. */
	char* compiler_const; /**< RNG OpenCL compiler constant. */
	size_t bytes;         /**< Bytes required per RNG seed. */
} CloRngInfo;

/** @brief Information about the random number generation algorithms. */
extern CloRngInfo rng_infos[];

#endif

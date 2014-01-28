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
 * @brief RNG algorithms host implementation.
 */

#include "clo_rng.h"

/** @brief Information about the random number generation algorithms. */
CloRngInfo rng_infos[] = {
	{"lcg", "CLO_RNG_LCG", 8}, 
	{"xorshift64", "CLO_RNG_XORSHIFT64", 8},
	{"xorshift128", "CLO_RNG_XORSHIFT128", 16},
	{"mwc64x", "CLO_RNG_MWC64X", 8},
	{NULL, NULL, 0}
};

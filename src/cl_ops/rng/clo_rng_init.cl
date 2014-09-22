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
 * @brief Device seed initialization kernel.
 */

#ifndef CLO_RNG_HASH
	/* Default is no hash. */
	#define CLO_RNG_HASH(x) (x)
#endif

__kernel void clo_rng_init(
		const ulong main_seed,
		__global ulong *seeds) {

	ulong seed = get_global_id(0) + main_seed;
	CLO_RNG_HASH(seed);
	seeds[get_global_id(0)] = seed;

}


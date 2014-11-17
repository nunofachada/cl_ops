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
 * Device seed initialization kernel.
 */

/* Pre-defined hashes. */

/* Knuth multiplicative coefficient. */
#define KNUTH(x)  x = ((x*2654435761) % 0x100000000)

/* Xor shift hash. */
#define XS1(x) \
	x = ((x >> 16) ^ x) * 0x45d9f3b; \
	x = ((x >> 16) ^ x) * 0x45d9f3b; \
	x = ((x >> 16) ^ x);

#ifndef CLO_RNG_HASH
	/* Default is no hash. */
	#define CLO_RNG_HASH(x) x=x
#endif

/**
 * Initialize RNG seeds using each workitem's global ID and possibly
 * an externally specified hash.
 *
 * @param[in] main_seed Base seed.
 * @param[in,out] seeds Array of seeds.
 * */
__kernel void clo_rng_init(
		const ulong main_seed,
		__global clo_statetype *seeds) {

	/* Get initial seed for this workitem. */
	ulong seed = get_global_id(0) + main_seed;

	/* Apply hash, if any was specified. */
	CLO_RNG_HASH(seed);

	/* Update seeds array. */
	seeds[get_global_id(0)] = clo_ulong2statetype(seed);

}


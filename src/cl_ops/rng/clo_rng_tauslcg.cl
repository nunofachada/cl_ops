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
 * GPU implementation of a hybrid random number generator which uses a
 * Combined Tausworthe Generator with an LCG, as specified in:
 *
 * Howes, L. & Thomas, D. Nguyen, H. (Ed.) Efficient random number
 * generation and application using CUDA, Ch. 37, Gpu Gems 3,
 * Addison-Wesley Professional, 2007, 805-830
 *
 * The only difference is that we perform sub-stream skipping.
 */

/* For the LCG RNG, the size of each seed is ulong. */
typedef uint4 clo_statetype;

/* Convert ulong into uint4 */
#define clo_ulong2statetype(seed) as_uint4((ulong2) (seed, seed))
/// @todo The initial state are that the three Tausworthe state
/// components should be greater than 128.

/**
 * A Single Step of the Combined Tausworthe Generator.
 *
 * @todo This will be faster if its a macro
 *
 * s1, s2, s3, and m are all constants, and z is part of the private
 * per-thread generator state.
 * */
uint taus_step(uint z, int s1, int s2, int s3, uint m) {
	uint b = (((z << s1) ^ z) >> s2);
	return (((z & m) << s3) ^ b);
}

/**
 * A Simple Linear Congruential Generator
 *
 * @todo This will be faster if its a macro
 *
 * a and c are constants.
 * */
uint lcg_step(uint z, uint a, uint c) {
	return a * z + c;
}

/**
 * Returns the next pseudorandom state.
 *
 * @param[in,out] states Array of RNG states.
 * @param[in] index Index of relevant state to use and update.
 * @return The next pseudorandom state.
 */
uint clo_rng_next(__global clo_statetype *states, uint index) {

	/* Get current state */
	clo_statetype state = states[index];

	/* Keep x value. */
	uint x = state.x;

	/* Update state using stream skipping (is this the correct term?). */
	state.x = taus_step(state.y, 13, 19, 12, 4294967294U);
	state.y = taus_step(state.z,  2, 25,  4, 4294967288U);
	state.z = taus_step(state.w,  3, 11, 17, 4294967294U);
	/* Bellow is "an event quicker generator" from numerical receipts
	 * in C. */
	state.w = lcg_step(x, 1664525, 1013904223U);

	/* Keep state */
	states[index] = state;

	/* Return value */
	return state.x;
}

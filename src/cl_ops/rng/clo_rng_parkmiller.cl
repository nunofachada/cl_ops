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
 * Implementation of Park-Miller random number generator.
 *
 * "Random number generators: good ones are hard to find",
 * S. K. Park and K. W. Miller. Communications of the ACM, Vol. 31,
 * Issue 10, Oct. 1988, pp 1192-1201.
 */

/* For the Park-Miller RNG, the size of each seed is int. */
typedef int clo_statetype;

#define clo_ulong2statetype(seed) as_int((uint) (0xFFFFFFFF & seed))
///@todo Initial seed can't be zero

/**
 * Returns the next pseudorandom value using the minimal standard
 * Park-Miller random number generator.
 *
 * @param[in,out] states Array of RNG states.
 * @param[in] index Index of relevant state to use and update.
 * @return The next pseudorandom value using the minimal standard
 * Park-Miller random number generator.
 */
uint clo_rng_next(__global clo_statetype *states, uint index) {

	/* Get current state */
	clo_statetype state = states[index];

	/* Update state */
	int const a = 16807;
	int const m = INT_MAX; /* 2147483647 */
	state = (((long) state) * a) % m; /// @todo Maybe we can use a mask as in LCG (31 bit mask in this case)

	/* Keep state */
	states[index] = state;

	/* Return value */
	return as_uint(state) << 1; /* Put something in the sign bit, but will only return pairs */

}



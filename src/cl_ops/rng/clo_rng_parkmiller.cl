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
 * Implementation of Park-Miller random number generator.
 *
 * "Random number generators: good ones are hard to find",
 * S. K. Park and K. W. Miller. Communications of the ACM, Vol. 31,
 * Issue 10, Oct. 1988, pp 1192-1201.
 */

/* For the Park-Miller RNG, the size of each seed is long. */
typedef long rng_state;

/**
 * Returns the next pseudorandom value using a Park-Miller random
 * number generator with 64 bit state.
 *
 * @param[in,out] states Array of RNG states.
 * @param[in] index Index of relevant state to use and update.
 * @return The next pseudorandom value using a Park-Miller random
 * number generator with 64 bit state.
 */
uint clo_rng_next(__global rng_state *states, uint index) {

	/* Get current state */
	rng_state state = states[index];

	/* Update state */
	int const a = 16807;
	int const m = 2147483647;
	state = (state * a) % m;

	/* Keep state */
	states[index] = state;

	/* Return value */
	return ((uint) ((int2) state).x) + ((uint) ((int2) state).y);

}



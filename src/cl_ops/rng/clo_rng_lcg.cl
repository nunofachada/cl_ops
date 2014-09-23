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
 * @brief GPU implementation of a linear congruential pseudorandom
 * number generator (LCG), as defined by D. H. Lehmer and described by
 * Donald E. Knuth in The Art of Computer Programming, Volume 3:
 * Seminumerical Algorithms, section 3.2.1. It is a similar
 * implementation to Java Random class.
 */

typedef ulong rng_state;

/**
 * @brief Returns the next pseudorandom value using a LCG random number
 * generator.
 *
 * @param states Array of RNG states.
 * @param index Index of relevant state to use and update.
 * @return The next pseudorandom value using a LCG random number
 * generator.
 */
uint clo_rng_next(__global rng_state *states, uint index) {

	// Assume 32 bits
	uint bits = 32;
	// Get current state
	rng_state state = states[index];
	// Update state
	state = (state * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1);
	// Keep state
	states[index] = state;
	// Return value
	return (uint) (state >> (48 - bits));
}

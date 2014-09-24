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
 * @brief Implementation of simple XorShift random number generator.
 *
 * Based on code available [here](http://www.javamex.com/tutorials/random_numbers/xorshift.shtml).
 */

/* For the Xor-Shift64 RNG, the size of each seed is ulong. */
typedef ulong rng_state;

/**
 * @brief Returns the next pseudorandom value using a xorshift random
 * number generator with 64 bit state.
 *
 * @param[in,out] states Array of RNG states.
 * @param[in] index Index of relevant state to use and update.
 * @return The next pseudorandom value using a xorshift random number
 * generator with 64 bit state.
 */
uint clo_rng_next(__global rng_state *states, uint index) {

	/* Get current state */
	rng_state state = states[index];

	/* Update state */
	state ^= (state << 21);
	state ^= (state >> 35);
	state ^= (state << 4);

	/* Keep state */
	states[index] = state;

	/* Return value */
	return convert_uint(state);

	/* The instruction above should work because of what the OpenCL
	 * spec says: "Out-of-Range Behavior: When converting between
	 * integer types, the resulting value for out-of-range inputs will
	 * be equal to the set of least significant bits in the source
	 * operand element that fit in the corresponding destination
	 * element." */

}



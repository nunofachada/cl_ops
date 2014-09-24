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
 * @brief GPU implementation of a random number generator based on a
 * Multiply-With-Carry (MWC) generator, developed by David B. Thomas
 * from Imperial College London. More information at
 * http://cas.ee.ic.ac.uk/people/dt10/research/rngs-gpu-mwc64x.html.
 */

/* For the LCG RNG, the size of each seed is two integers. */
typedef uint2 rng_state;

/**
 * @brief Returns the next pseudorandom value using a MWC random number
 * generator.
 *
 * @param[in,out] states Array of RNG states.
 * @param[in] index Index of relevant state to use and update.
 * @return The next pseudorandom value using a MWC random number
 * generator.
 */
uint clo_rng_next(__global rng_state *states, uint index) {

    enum { A=4294883355U };

	/* Unpack the state. */
	uint x = states[index].x, c = states[index].y;

	/* Calculate the result */
	uint res = x^c;

	/* Step the RNG */
	uint hi = mul_hi(x,A);
	x= x * A + c;
	c= hi + (x < c);

	/* Pack the state back up */
	states[index] = (rng_state) (x, c);

	/* Return the next result */
	return res;
}


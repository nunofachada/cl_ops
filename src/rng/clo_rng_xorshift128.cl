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
 * @brief Implementation of XorShift random number generator with
 * 128 bit state.
 * 
 * Based on code available [here](http://en.wikipedia.org/wiki/Xorshift).
 */
 
#ifndef LIBCL_RNG
#define LIBCL_RNG
 
typedef uint4 rng_state;

/**
 * @brief Returns the next pseudorandom value using a xorshift random
 * number generator with 128 bit state.
 * 
 * @param states Array of RNG states.
 * @param index Index of relevant state to use and update.
 * @return The next pseudorandom value using a xorshift random number 
 * generator with 128 bit state.
 */
uint randomNext( __global rng_state *states, uint index) {

	// Get current state
	rng_state state = states[index];
	
	// Update state
	uint t = state.x ^ (state.x << 11);
	state.x = state.y;
	state.y = state.z;
	state.z = state.w;
	state.w = state.w ^ (state.w >> 19) ^ (t ^ (t >> 8));
	
	// Keep state
	states[index] = state;

	// Return value
	return state.w;

}

#endif

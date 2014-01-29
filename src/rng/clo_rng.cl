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

 
#include "clo_workitem.cl"

#ifdef CLO_RNG_LCG
#include "clo_rng_lcg.cl"
#elif defined CLO_RNG_MWC64X
#include "clo_rng_mwc64x.cl"
#elif defined CLO_RNG_XORSHIFT64
#include "clo_rng_xorshift64.cl"
#elif defined CLO_RNG_XORSHIFT128
#include "clo_rng_xorshift128.cl"
#endif

/**
 * @brief Returns next integer from 0 (including) to n (not including).
 * 
 * @param states Array of RNG states.
 * @param n Returned integer is less than this value.
 * @return Returns next integer from 0 (including) to n (not including).
 */
uint randomNextInt( __global rng_state *states, 
			uint n)
{
	// Get state index
	uint index = GID1();
	
	// Return next random integer from 0 to n.
	return randomNext(states, index) % n;
}

uint2 randomNextInt2( __global rng_state *states, 
			uint n)
{

	// Get state index
	uint2 index = GID2();

	// Return vector of random integers from 0 to n.
	return (uint2) (randomNext(states, index.s0) % n,
					randomNext(states, index.s1) % n);
}

uint4 randomNextInt4( __global rng_state *states, 
			uint n)
{
	// Get state index
	uint4 index = GID4();

	// Return vector of random integers from 0 to n.
	return (uint4) (randomNext(states, index.s0) % n,
					randomNext(states, index.s1) % n,
					randomNext(states, index.s2) % n,
					randomNext(states, index.s3) % n);
}

uint8 randomNextInt8( __global rng_state *states, 
			uint n)
{
	// Get state index
	uint8 index = GID8();

	// Return vector of random integers from 0 to n.
	return (uint8) (randomNext(states, index.s0) % n,
					randomNext(states, index.s1) % n,
					randomNext(states, index.s2) % n,
					randomNext(states, index.s3) % n,
					randomNext(states, index.s4) % n,
					randomNext(states, index.s5) % n,
					randomNext(states, index.s6) % n,
					randomNext(states, index.s7) % n);
}


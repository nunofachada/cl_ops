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
 * @brief Common data structures and functions.
 * 
 * @author Nuno Fachada
 */

#include "clo_common.h"

/** 
 * @brief Returns the next larger power of 2 of the given value.
 * 
 * @author Henry Gordon Dietz
 * 
 * @param x Value of which to get the next larger power of 2.
 * @return The next larger power of 2 of x.
 */
unsigned int clo_nlpo2(register unsigned int x)
{
	/* If value already is power of 2, return it has is. */
	if ((x & (x - 1)) == 0) return x;
	/* If not, determine next larger power of 2. */
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return(x + 1);
}

/** 
 * @brief Returns the number of one bits in the given value.
 * 
 * @author Henry Gordon Dietz
 *  
 * @param x Value of which to get the number of one bits.
 * @return The number of one bits in x.
 */
unsigned int clo_ones32(register unsigned int x)
{
	/* 32-bit recursive reduction using SWAR...
	but first step is mapping 2-bit values
	into sum of 2 1-bit values in sneaky way */
	x -= ((x >> 1) & 0x55555555);
	x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
	x = (((x >> 4) + x) & 0x0f0f0f0f);
	x += (x >> 8);
	x += (x >> 16);
	return(x & 0x0000003f);
}

/** 
 * @brief Returns the trailing Zero Count (i.e. the log2 of a base 2 number).
 * 
 * @author Henry Gordon Dietz
 * 
 * @param x Value of which to get the trailing Zero Count.
 * @return The trailing Zero Count in x.
 */
unsigned int clo_tzc(register int x)
{
	return(clo_ones32((x & -x) - 1));
}

/** 
 * @brief Returns the series (sum of sequence of 0 to) x.
 * 
 * @author Henry Gordon Dietz
 *  
 * @param x Series limit.
 * @return The series (sum of sequence of 0 to) x.
 */
unsigned int clo_sum(unsigned int x)
{
	return(x == 0 ? x : clo_sum(x - 1) + x);
}

/** 
 * @brief Resolves to error category identifying string, in this case an
 * error related to ocl-ops.
 * 
 * @return A GQuark structure defined by category identifying string, 
 * which identifies the error as an ocl-ops generated error.
 */
GQuark clo_error_quark() {
	return g_quark_from_static_string("clo-error-quark");
}

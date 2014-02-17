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
 * @brief Sorting algorithms kernels.
 */

#ifndef CLO_SORT_ELEM_TYPE
	#define CLO_SORT_ELEM_TYPE uint
#endif

#ifndef CLO_SORT_COMPARE
	#define CLO_SORT_COMPARE(a, b) ((a) > (b))
#endif

#ifdef CLO_SORT_SBITONIC
	#include "clo_sort_sbitonic.cl"
#endif


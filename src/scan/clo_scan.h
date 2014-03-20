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
 * @brief Parallel prefix sum (scan) implementation headers.
 */

#include "clo_common.h"

#define CLO_SCAN_NUMKERNELS 3

#define CLO_SCAN_KIDX_WGSCAN 0
#define CLO_SCAN_KIDX_WGSUMSSCAN 1
#define CLO_SCAN_KIDX_ADDWGSUMS 2

#define CLO_SCAN_KNAME_WGSCAN "workgroupScan"
#define CLO_SCAN_KNAME_WGSUMSSCAN "workgroupSumsScan"
#define CLO_SCAN_KNAME_ADDWGSUMS "addWorkgroupSums"

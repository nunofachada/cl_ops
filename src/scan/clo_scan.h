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
 
#ifndef OCLOPS_SCAN_H
#define OCLOPS_SCAN_H

#include "clo_common.h"

#define CLO_SCAN_NUMKERNELS 3

#define CLO_SCAN_KIDX_WGSCAN 0
#define CLO_SCAN_KIDX_WGSUMSSCAN 1
#define CLO_SCAN_KIDX_ADDWGSUMS 2

#define CLO_SCAN_KNAME_WGSCAN "workgroupScan"
#define CLO_SCAN_KNAME_WGSUMSSCAN "workgroupSumsScan"
#define CLO_SCAN_KNAME_ADDWGSUMS "addWorkgroupSums"

/** @brief Perform scan. */
int clo_scan(cl_command_queue queue, cl_kernel *krnls, 
	size_t lws_max, size_t len, unsigned int numel, const char* options, 
	GArray *evts, gboolean profile, GError **err);

/** @brief Returns the name of the kernel identified by the given
 * index. */
const char* clo_scan_kernelname_get(unsigned int index);

/** @brief Create kernels for the scan implementation. */
int clo_scan_kernels_create(cl_kernel **krnls, cl_program program, GError **err);

/** @brief Get local memory usage for the scan kernels. */
size_t clo_scan_localmem_usage(const char* kernel_name, size_t lws_max, size_t len);

/** @brief Set kernels arguments for the scan implemenation. */
int clo_scan_kernelargs_set(cl_kernel **krnls, cl_mem data2scan, cl_mem scanned_data, cl_mem wgsums, size_t lws, size_t len, GError **err);

/** @brief Free the scan kernels. */
void clo_scan_kernels_free(cl_kernel **krnls);

#endif

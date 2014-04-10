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
 * @brief Satish radix sort host implementation.
 */

#include "clo_sort_satradix.h"

/* Array of kernel names. */
static const char* const kernel_names[] = CLO_SORT_SATRADIX_KERNELNAMES;

/**
 * @brief Sort agents using the Satish radix sort.
 * 
 * @see clo_sort_sort()
 */
int clo_sort_satradix_sort(cl_command_queue *queues, cl_kernel *krnls, 
	size_t lws_max, size_t len, unsigned int numel, const char* options, 
	GArray *evts, gboolean profile, GError **err) {
	
	return 0;	
}

/** 
 * @brief Returns the name of the kernel identified by the given
 * index.
 * 
 * @see clo_sort_kernelname_get()
 * */
const char* clo_sort_satradix_kernelname_get(unsigned int index) {
	g_assert_cmpuint(index, <, CLO_SORT_SATRADIX_NUMKERNELS);
	return kernel_names[index];
}

/** 
 * @brief Create kernels for the Satish radix sort. 
 * 
 * @see clo_sort_kernels_create()
 * */
int clo_sort_satradix_kernels_create(cl_kernel **krnls, cl_program program, GError **err) {
	
	return 0;

}

/** 
 * @brief Get local memory usage for the Satish radix sort kernels.
 * 
 * Satish radix sort only uses one kernel which doesn't require
 * local memory. 
 * 
 * @see clo_sort_localmem_usage()
 * */
size_t clo_sort_satradix_localmem_usage(const char* kernel_name, size_t lws_max, size_t len, unsigned int numel) {
	
	return 0;
}

/** 
 * @brief Set kernels arguments for the Satish radix sort. 
 * 
 * @see clo_sort_kernelargs_set()
 * */
int clo_sort_satradix_kernelargs_set(cl_kernel **krnls, cl_mem data, size_t lws, size_t len, GError **err) {
	
	/* Return. */
	return 0;
	
}

/** 
 * @brief Free the Satish radix sort kernels. 
 * 
 * @see clo_sort_kernels_free()
 * */
void clo_sort_satradix_kernels_free(cl_kernel **krnls) {
	if (*krnls) {
		if ((*krnls)[0]) clReleaseKernel((*krnls)[0]);
		free(*krnls);
	}
}

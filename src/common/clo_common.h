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
 * @brief Common data structures and function headers for OCL-OPS.
 * 
 * @author Nuno Fachada
 */

#ifndef CLO_COMMON_H
#define CLO_COMMON_H

#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define __NO_STD_STRING

#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include "clutils.h"
#include "clprofiler.h"

/** Helper macros to convert int to string at compile time. */
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

/** Default RNG seed. */
#define CLO_DEFAULT_SEED 0

/** Default OpenCL source path. */
#define CLO_DEFAULT_PATH "."

/** Resolves to error category identifying string. Required by glib error reporting system. */
#define CLO_ERROR clo_error_quark()

/**
 * @brief Obtains information about an algorithm by finding where this
 * information is located within a vector.
 * 
 * @param info The requested information about an algorithm.
 * @param info_v The information vector containing info about algorithms.
 * @param arg_tag Tag identifying the requested algorithm.
 * */
#define CLO_ALG_GET(info, info_v, arg_tag) \
	for (int i___priv = 0; info_v[i___priv].tag; i___priv++) { \
		if (g_strcmp0(info_v[i___priv].tag, arg_tag) == 0) { \
			info = info_v[i___priv]; \
			break; \
		} \
	} \

/**
 * @brief Program error codes.
 * */ 
enum clo_error_codes {
	CLO_SUCCESS = 0,                     /**< Successfull operation. */
	CLO_UNKNOWN_ARGS = -1,               /**< Unknown arguments. */
	CLO_INVALID_ARGS = -2,               /**< Arguments are known but invalid. */
	CLO_LIBRARY_ERROR = -3,              /**< Error in external library. */
	CLO_UNABLE_TO_OPEN_PARAMS_FILE = -4, /**< Parameters file not found. */
	CLO_INVALID_PARAMS_FILE = -5,        /**< Invalid parameters file. */
	CLO_UNABLE_SAVE_STATS = -6,          /**< Unable to save stats. */
	CLO_ALLOC_MEM_FAIL = -7,             /**< Unable to allocate memory. */
	CLO_OUT_OF_RESOURCES = -8            /**< Program state above limits. */
};

/** @brief Returns the next larger power of 2 of the given value. */
unsigned int clo_nlpo2(register unsigned int x);

/** @brief Returns the number of one bits in the given value. */
unsigned int clo_ones32(register unsigned int x);

/** @brief Returns the trailing Zero Count (i.e. the log2 of a base 2 number). */
unsigned int clo_tzc(register int x);

/** @brief Returns the series (sum of sequence of 0 to) x. */
unsigned int clo_sum(unsigned int x);

/** @brief Resolves to error category identifying string, in this case
 *  an error related to ocl-ops. */
GQuark clo_error_quark(void);

#endif

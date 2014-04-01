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
#include <cf4ocl.h>

/** Helper macros to convert int to string at compile time. */
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

/** Default RNG seed. */
#define CLO_DEFAULT_SEED 0

/** Default OpenCL source path. */
#define CLO_DEFAULT_PATH ".." G_DIR_SEPARATOR_S "share" G_DIR_SEPARATOR_S "cl_ops" /// @todo This path should be configured in CMake

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
		if (g_str_has_prefix(arg_tag, info_v[i___priv].tag)) { \
			info = info_v[i___priv]; \
			break; \
		} \
	}

/**
 * @brief Performs integer division returning the ceiling instead of
 * the floor if it is not an exact division.
 * 
 * @param a Integer numerator.
 * @param b Integer denominator.
 * */
#define CLO_DIV_CEIL(a, b) ((a + b - 1) / b)

/** 
 * @brief Calculates an adjusted global worksize equal or larger than 
 * the given global worksize and is a multiple of the given local 
 * worksize. 
 * 
 * @param gws Minimum global worksize.
 * @param lws Local worksize. 
 * */
#define CLO_GWS_MULT(gws, lws) (lws * CLO_DIV_CEIL(gws, lws))

/**
 * @brief Performs integer division returning the ceiling instead of
 * the floor if it is not an exact division.
 * 
 * @param a Integer numerator.
 * @param b Integer denominator.
 * */
#define CLO_DIV_CEIL(a, b) ((a + b - 1) / b)

/** 
 * @brief Calculates an adjusted global worksize equal or larger than 
 * the given global worksize and is a multiple of the given local 
 * worksize. 
 * 
 * @param gws Minimum global worksize.
 * @param lws Local worksize. 
 * */
#define CLO_GWS_MULT(gws, lws) (lws * CLO_DIV_CEIL(gws, lws))

/**
 * @brief Program error codes.
 * */ 
/**
 * @brief Error codes.
 * */ 
enum clo_error_codes {
	CLO_SUCCESS = 0,        /**< Successful operation. */
	CLO_ERROR_NOALLOC = 1,  /**< Error code thrown when no memory allocation is possible. */
	CLO_ERROR_OPENFILE = 2, /**< Error code thrown when it's not possible to open file. */
	CLO_ERROR_ARGS = 3,     /**< Error code thrown when passed arguments are invalid. */
	CLO_ERROR_DEVICE_NOT_FOUND = 4, /**< Error code thrown when no OpenCL device is found. */
	CLO_ERROR_STREAM_WRITE = 5,     /**< Error code thrown when an error occurs while writing to a stream. */
	CLO_ERROR_LIBRARY = 10          /**< An error ocurred in a third party library. */
};

/** @brief Returns the next larger power of 2 of the given value. */
unsigned int clo_nlpo2(register unsigned int x);

/** @brief Returns the number of one bits in the given value. */
unsigned int clo_ones32(register unsigned int x);

/** @brief Returns the trailing Zero Count (i.e. the log2 of a base 2 number). */
unsigned int clo_tzc(register int x);

/** @brief Returns the series (sum of sequence of 0 to) x. */
unsigned int clo_sum(unsigned int x);

/** @brief Implementation of GLib's GPrintFunc which does not print the
 * string given as a parameter. */
void clo_print_to_null(const gchar *string);

/** @brief Get full kernel path name. */
gchar* clo_kernelpath_get(gchar* kernel_filename, char* exec_name);

/** @brief Resolves to error category identifying string, in this case
 *  an error related to ocl-ops. */
GQuark clo_error_quark(void);


#endif

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
 * @brief Common data structures and function headers for CL-Ops.
 *
 * @author Nuno Fachada
 */

#ifndef _CLO_COMMON_H
#define _CLO_COMMON_H

#include <cf4ocl2.h>

/** Default RNG seed. */
#define CLO_DEFAULT_SEED 0

/** Resolves to error category identifying string. Required by glib
 * error reporting system. */
#define CLO_ERROR clo_error_quark()

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
	/** Successful operation. */
	CLO_SUCCESS = 0,
	/** Unable to open file. */
	CLO_ERROR_OPENFILE = 1,
	/** Arguments or parameters are invalid. */
	CLO_ERROR_ARGS = 2,
	/** Error writing to stream. */
	CLO_ERROR_STREAM_WRITE = 3,
	/** An algorithm implementation was not found. */
	CLO_ERROR_IMPL_NOT_FOUND = 5,
	/** Requested OpenCL type does not exist.  */
	CLO_ERROR_UNKNOWN_TYPE = 6,
	/** Error in external library. */
	CLO_ERROR_LIBRARY = 7
};

/**
 * Enumeration of OpenCL types.
 * */
typedef enum {
	CLO_CHAR = 0,
	CLO_UCHAR = 1,
	CLO_SHORT = 2,
	CLO_USHORT = 3,
	CLO_INT = 4,
	CLO_UINT = 5,
	CLO_LONG = 6,
	CLO_ULONG = 7,
	CLO_HALF = 8,
	CLO_FLOAT = 9,
	CLO_DOUBLE = 10
} CloType;

/**
 * Information about an OpenCL type.
 * */
typedef struct clo_type_info CloTypeInfo;

/* Return OpenCL type name. */
const char* clo_type_get_name(CloType type);

/* Return OpenCL type size in bytes. */
size_t clo_type_sizeof(CloType type);

/* Return an OpenCL type constant given a type name. */
CloType clo_type_by_name(const char* name, GError** err);

/* Returns the next larger power of 2 of the given value. */
unsigned int clo_nlpo2(register unsigned int x);

/* Returns the number of one bits in the given value. */
unsigned int clo_ones32(register unsigned int x);

/* Returns the trailing Zero Count (i.e. the log2 of a base 2 number). */
unsigned int clo_tzc(register int x);

/* Returns the series (sum of sequence of 0 to) x. */
unsigned int clo_sum(unsigned int x);

/* Implementation of GLib's GPrintFunc which does not print the string
 * given as a parameter. */
void clo_print_to_null(const gchar *string);

/* Get a local worksize based on what was requested by the user in
 * `lws_max`, the global worksize and the kernel and device
 * capabilities. */
size_t clo_get_lws(CCLKernel* krnl, CCLDevice* dev, size_t gws,
	size_t lws_max, GError** err);

/* Resolves to error category identifying string, in this case an error
 * related to ocl-ops. */
GQuark clo_error_quark(void);

#endif

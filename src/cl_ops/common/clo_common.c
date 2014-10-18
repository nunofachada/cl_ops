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
 * Information about an OpenCL type.
 * */
struct clo_type_info {

	/**
	 * Type name.
	 * @private
	 * */
	const char* name;

	/**
	 * Type size in bytes.
	 * @private
	 * */
	const int size;
};

/* Relation between OpenCL type names and sizes in bytes. */
static const CloTypeInfo clo_types[] = {

	{"char",   1}, /* CCL_CHAR   = 0 */
	{"uchar",  1}, /* CCL_UCHAR  = 1 */
	{"short",  2}, /* CCL_SHORT  = 2 */
	{"ushort", 2}, /* CCL_USHORT = 3 */
	{"int",    4}, /* CCL_INT    = 4 */
	{"uint",   4}, /* CCL_UINT   = 5 */
	{"long",   8}, /* CCL_LONG   = 6 */
	{"ulong",  8}, /* CCL_ULONG  = 7 */
	{"half",   2}, /* CCL_HALF   = 8 */
	{"float",  4}, /* CCL_FLOAT  = 9 */
	{"double", 8}, /* CCL_DOUBLE = 10 */
	{NULL,     0}
};

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
 * @brief Implementation of GLib's GPrintFunc which does not print the
 * string given as a parameter.
 *
 * @param string String to ignore.
 * */
void clo_print_to_null(const gchar *string) {
	string = string;
	return;
}

/**
 * Return OpenCL type name.
 *
 * @param[in] type Type constant.
 * @return A string containing the OpenCL type name.
 * */
const char* clo_type_get_name(CloType type, GError** err) {
	if ((type < CLO_CHAR) || (type > CLO_DOUBLE)) {
		g_set_error(err, CLO_ERROR, CLO_ERROR_UNKNOWN_TYPE,
			"Unknown type enum '%d'", type);
		return NULL;
	}
	return clo_types[type].name;
}

/**
 * Return OpenCL type size in bytes.
 *
 * @param[in] type Type constant.
 * @return The size of the OpenCL type in bytes.
 * */
int clo_type_sizeof(CloType type, GError** err) {
	if ((type < CLO_CHAR) || (type > CLO_DOUBLE)) {
		g_set_error(err, CLO_ERROR, CLO_ERROR_UNKNOWN_TYPE,
			"Unknown type enum '%d'", type);
		return 0;
	}
	return clo_types[type].size;
}

CloType clo_type_by_name(const char* name, GError** err) {
	for (guint i = 0; clo_types[i].name != NULL; ++i)
		if (g_strcmp0(name, clo_types[i].name) == 0)
			return i;
	g_set_error(err, CLO_ERROR, CLO_ERROR_UNKNOWN_TYPE,
		"Unknown type '%s'", name);
	return -1;
}


size_t clo_get_lws(CCLKernel* krnl, CCLDevice* dev, size_t gws,
	size_t lws_max, GError** err) {
		
	/* Make sure err is NULL or it is not set. */
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	size_t lws;
	GError* err_internal = NULL;

	if (lws_max != 0) {
		lws = MIN(lws_max, gws);
	} else {
		ccl_kernel_suggest_worksizes(
			krnl, dev, 1, &gws, NULL, &lws, &err_internal);
		ccl_if_err_propagate_goto(err, err_internal, error_handler);
	}

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	goto finish;

error_handler:
	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	lws = 0;

finish:

	return lws;

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

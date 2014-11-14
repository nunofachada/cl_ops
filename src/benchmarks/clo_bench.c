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
 * Common benchmark function implementations.
 */

#include "clo_bench.h"

#define CLO_TYPE_CMP(type, a, b) \
	((*((type*) a) > *((type*) b)) \
		? 1 \
		: ((*((type*) a) == *((type*) b)) ? 0 : -1))

cl_int clo_bench_compare(CloType type, cl_uchar* a, cl_uchar* b) {

	g_return_val_if_fail(a != NULL, CL_INT_MAX);
	g_return_val_if_fail(b != NULL, CL_INT_MAX);

	/* Compare depending on type. */
	switch (type) {
		case CLO_CHAR:
			return CLO_TYPE_CMP(cl_char, a, b);
		case CLO_UCHAR:
			return CLO_TYPE_CMP(cl_uchar, a, b);
		case CLO_SHORT:
			return CLO_TYPE_CMP(cl_short, a, b);
		case CLO_USHORT:
			return CLO_TYPE_CMP(cl_ushort, a, b);
		case CLO_INT:
			return CLO_TYPE_CMP(cl_int, a, b);
		case CLO_UINT:
			return CLO_TYPE_CMP(cl_uint, a, b);
		case CLO_LONG:
			return CLO_TYPE_CMP(cl_long, a, b);
		case CLO_ULONG:
			return CLO_TYPE_CMP(cl_ulong, a, b);
		case CLO_HALF:
			return CLO_TYPE_CMP(cl_half, a, b);
		case CLO_FLOAT:
			return CLO_TYPE_CMP(cl_float, a, b);
		case CLO_DOUBLE:
			return CLO_TYPE_CMP(cl_double, a, b);
		default:
			g_assert_not_reached();

	}

}

void clo_bench_rand(GRand* rng, CloType type, void* location) {

	g_return_if_fail(location != NULL);

	int bytes = clo_type_sizeof(type);

	if (type == CLO_CHAR) {
		/* char type. */
		cl_char value = (cl_char) g_rand_int_range(
			rng, CL_CHAR_MIN, CL_CHAR_MAX);
		/* But just use the specified bits. */
		memcpy(location, &value, bytes);
	} else if (type == CLO_UCHAR) {
		/* uchar type. */
		cl_uchar value = (cl_uchar) g_rand_int_range(
			rng, 0, CL_UCHAR_MAX);
		/* But just use the specified bits. */
		memcpy(location, &value, bytes);
	} else if (type == CLO_SHORT) {
		/* short type. */
		cl_short value = (cl_short) g_rand_int_range(
			rng, CL_SHRT_MIN, CL_SHRT_MAX);
		/* But just use the specified bits. */
		memcpy(location, &value, bytes);
	} else if (type == CLO_USHORT) {
		/* ushort type. */
		cl_ushort value = (cl_ushort) g_rand_int_range(
			rng, 0, CL_USHRT_MAX);
		/* But just use the specified bits. */
		memcpy(location, &value, bytes);
	} else if (type == CLO_INT) {
		/* int type. */
		cl_int value = (cl_int) g_rand_int_range(
			rng, CL_INT_MIN, CL_INT_MAX);
		/* But just use the specified bits. */
		memcpy(location, &value, bytes);
	} else if (type == CLO_UINT) {
		/* uint type. */
		cl_uint value = (cl_uint)  (g_rand_double(rng) * CL_UINT_MAX);
		/* But just use the specified bits. */
		memcpy(location, &value, bytes);
	} else if (type == CLO_LONG) {
		/* long type. */
		cl_long value = (cl_long) (g_rand_double(rng) *
			(g_rand_boolean(rng) ? CL_LONG_MIN : CL_LONG_MAX));
		/* But just use the specified bits. */
		memcpy(location, &value, bytes);
	} else if (type == CLO_ULONG) {
		/* ulong type. */
		cl_ulong value =
			(cl_ulong) (g_rand_double(rng) * CL_ULONG_MAX);
		/* But just use the specified bits. */
		memcpy(location, &value, bytes);
	} else if (type == CLO_FLOAT) {
		/* Float type. */
		cl_float value = (cl_float)
			g_rand_double_range(rng, CL_FLT_MIN, CL_FLT_MAX);
		/* But just use the specified bits. */
		memcpy(location, &value, bytes);
	} else if (type == CLO_DOUBLE) {
		/* Double type. */
		cl_double value = (cl_double)
			g_rand_double_range(rng, CL_DBL_MIN, CL_DBL_MAX);
		/* But just use the specified bits. */
		memcpy(location, &value, bytes);
	} else if (type == CLO_HALF) {
		/* Half type. */
		cl_half value =
			(cl_half) g_rand_double_range(rng, -1024.0, 1024.0);
		/* But just use the specified bits. */
		memcpy(location, &value, bytes);
	} else {
		g_assert_not_reached();
	}

}

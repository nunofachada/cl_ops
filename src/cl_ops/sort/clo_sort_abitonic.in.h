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
 * Advanced bitonic sort header file.
 */

#ifndef _CLO_SORT_ABITONIC_H_
#define _CLO_SORT_ABITONIC_H_

#include "clo_sort_abstract.h"

/** The advanced bitonic sort kernels source. */
#define CLO_SORT_ABITONIC_SRC "@ABITONIC_SRC@"

/* Number of kernels. */
#define CLO_SORT_ABITONIC_NUM_KERNELS 26

/* Index of the advance bitonic sort kernels. */
#define CLO_SORT_ABITONIC_KIDX_ANY 0
#define CLO_SORT_ABITONIC_KIDX_LOCAL_S2 1
#define CLO_SORT_ABITONIC_KIDX_LOCAL_S3 2
#define CLO_SORT_ABITONIC_KIDX_LOCAL_S4 3
#define CLO_SORT_ABITONIC_KIDX_LOCAL_S5 4
#define CLO_SORT_ABITONIC_KIDX_LOCAL_S6 5
#define CLO_SORT_ABITONIC_KIDX_LOCAL_S7 6
#define CLO_SORT_ABITONIC_KIDX_LOCAL_S8 7
#define CLO_SORT_ABITONIC_KIDX_LOCAL_S9 8
#define CLO_SORT_ABITONIC_KIDX_LOCAL_S10 9
#define CLO_SORT_ABITONIC_KIDX_LOCAL_S11 10
#define CLO_SORT_ABITONIC_KIDX_PRIV_2S4V 11
#define CLO_SORT_ABITONIC_KIDX_PRIV_3S8V 12
#define CLO_SORT_ABITONIC_KIDX_PRIV_4S16V 13
#define CLO_SORT_ABITONIC_KIDX_HYB_S4_2S4V 14
#define CLO_SORT_ABITONIC_KIDX_HYB_S6_2S4V 15
#define CLO_SORT_ABITONIC_KIDX_HYB_S8_2S4V 16
#define CLO_SORT_ABITONIC_KIDX_HYB_S10_2S4V 17
#define CLO_SORT_ABITONIC_KIDX_HYB_S12_2S4V 18
#define CLO_SORT_ABITONIC_KIDX_HYB_S3_3S8V 19
#define CLO_SORT_ABITONIC_KIDX_HYB_S6_3S8V 20
#define CLO_SORT_ABITONIC_KIDX_HYB_S9_3S8V 21
#define CLO_SORT_ABITONIC_KIDX_HYB_S12_3S8V 22
#define CLO_SORT_ABITONIC_KIDX_HYB_S4_4S16V 23
#define CLO_SORT_ABITONIC_KIDX_HYB_S8_4S16V 24
#define CLO_SORT_ABITONIC_KIDX_HYB_S12_4S16V 25

/* Advanced bitonic sort kernel names. */
#define CLO_SORT_ABITONIC_KNAME_ANY "abit_any"
#define CLO_SORT_ABITONIC_KNAME_LOCAL_S2 "abit_local_s2"
#define CLO_SORT_ABITONIC_KNAME_LOCAL_S3 "abit_local_s3"
#define CLO_SORT_ABITONIC_KNAME_LOCAL_S4 "abit_local_s4"
#define CLO_SORT_ABITONIC_KNAME_LOCAL_S5 "abit_local_s5"
#define CLO_SORT_ABITONIC_KNAME_LOCAL_S6 "abit_local_s6"
#define CLO_SORT_ABITONIC_KNAME_LOCAL_S7 "abit_local_s7"
#define CLO_SORT_ABITONIC_KNAME_LOCAL_S8 "abit_local_s8"
#define CLO_SORT_ABITONIC_KNAME_LOCAL_S9 "abit_local_s9"
#define CLO_SORT_ABITONIC_KNAME_LOCAL_S10 "abit_local_s10"
#define CLO_SORT_ABITONIC_KNAME_LOCAL_S11 "abit_local_s11"
#define CLO_SORT_ABITONIC_KNAME_PRIV_2S4V "abit_priv_2s4v"
#define CLO_SORT_ABITONIC_KNAME_PRIV_3S8V "abit_priv_3s8v"
#define CLO_SORT_ABITONIC_KNAME_PRIV_4S16V "abit_priv_4s16v"
#define CLO_SORT_ABITONIC_KNAME_HYB_S4_2S4V "abit_hyb_s4_2s4v"
#define CLO_SORT_ABITONIC_KNAME_HYB_S6_2S4V "abit_hyb_s6_2s4v"
#define CLO_SORT_ABITONIC_KNAME_HYB_S8_2S4V "abit_hyb_s8_2s4v"
#define CLO_SORT_ABITONIC_KNAME_HYB_S10_2S4V "abit_hyb_s10_2s4v"
#define CLO_SORT_ABITONIC_KNAME_HYB_S12_2S4V "abit_hyb_s12_2s4v"
#define CLO_SORT_ABITONIC_KNAME_HYB_S3_3S8V "abit_hyb_s3_3s8v"
#define CLO_SORT_ABITONIC_KNAME_HYB_S6_3S8V "abit_hyb_s6_3s8v"
#define CLO_SORT_ABITONIC_KNAME_HYB_S9_3S8V "abit_hyb_s9_3s8v"
#define CLO_SORT_ABITONIC_KNAME_HYB_S12_3S8V "abit_hyb_s12_3s8v"
#define CLO_SORT_ABITONIC_KNAME_HYB_S4_4S16V "abit_hyb_s4_4s16v"
#define CLO_SORT_ABITONIC_KNAME_HYB_S8_4S16V "abit_hyb_s8_4s16v"
#define CLO_SORT_ABITONIC_KNAME_HYB_S12_4S16V "abit_hyb_s12_4s16v"

/* Array of strings containing names of the kernels used by the advanced
 * bitonic sort strategy. */
#define CLO_SORT_ABITONIC_KERNELNAMES {CLO_SORT_ABITONIC_KNAME_ANY, \
	CLO_SORT_ABITONIC_KNAME_LOCAL_S2, CLO_SORT_ABITONIC_KNAME_LOCAL_S3, \
	CLO_SORT_ABITONIC_KNAME_LOCAL_S4, CLO_SORT_ABITONIC_KNAME_LOCAL_S5, \
	CLO_SORT_ABITONIC_KNAME_LOCAL_S6, CLO_SORT_ABITONIC_KNAME_LOCAL_S7, \
	CLO_SORT_ABITONIC_KNAME_LOCAL_S8, CLO_SORT_ABITONIC_KNAME_LOCAL_S9, \
	CLO_SORT_ABITONIC_KNAME_LOCAL_S10, CLO_SORT_ABITONIC_KNAME_LOCAL_S11, \
	CLO_SORT_ABITONIC_KNAME_PRIV_2S4V, CLO_SORT_ABITONIC_KNAME_PRIV_3S8V, \
	CLO_SORT_ABITONIC_KNAME_PRIV_4S16V, CLO_SORT_ABITONIC_KNAME_HYB_S4_2S4V, \
	CLO_SORT_ABITONIC_KNAME_HYB_S6_2S4V, CLO_SORT_ABITONIC_KNAME_HYB_S8_2S4V, \
	CLO_SORT_ABITONIC_KNAME_HYB_S10_2S4V, CLO_SORT_ABITONIC_KNAME_HYB_S12_2S4V, \
	CLO_SORT_ABITONIC_KNAME_HYB_S3_3S8V, CLO_SORT_ABITONIC_KNAME_HYB_S6_3S8V, \
	CLO_SORT_ABITONIC_KNAME_HYB_S9_3S8V, CLO_SORT_ABITONIC_KNAME_HYB_S12_3S8V, \
	CLO_SORT_ABITONIC_KNAME_HYB_S4_4S16V, CLO_SORT_ABITONIC_KNAME_HYB_S8_4S16V, \
	CLO_SORT_ABITONIC_KNAME_HYB_S12_4S16V}

#define CLO_SORT_ABITONIC_KNAME_LOCAL_MARK "local"
#define CLO_SORT_ABITONIC_KNAME_PRIV_MARK "priv"
#define CLO_SORT_ABITONIC_KNAME_HYB_MARK "hyb"

#define CLO_SORT_ABITONIC_KPARSE_V(kname) atoi(g_strrstr(kname, "s") + 1)
#define CLO_SORT_ABITONIC_KPARSE_S(kname) atoi(g_strrstr(kname, "_") + 1)

/** Definition of the abitonic sort implementation. */
extern const CloSortImplDef clo_sort_abitonic_def;

#endif


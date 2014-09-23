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

#ifndef _CLO_RNG_H_
#define _CLO_RNG_H_

#include "common/clo_common.h"

/* Kernels source code. */
#define CLO_RNG_SRC_WORKITEM "@RNG_SRC_WORKITEM@"
#define CLO_RNG_SRC_LCG "@RNG_SRC_LCG@"
#define CLO_RNG_SRC_XORSHIFT64 "@RNG_SRC_XORSHIFT64@"
#define CLO_RNG_SRC_XORSHIFT128 "@RNG_SRC_XORSHIFT128@"
#define CLO_RNG_SRC_MWC64X "@RNG_SRC_MWC64X@"
#define CLO_RNG_SRC "@RNG_SRC@"

/* Device seed initialization kernel. */
#define CLO_RNG_SRC_INIT "@RNG_SRC_INIT@"

/* Available RNGs */
#define CLO_RNG_IMPLS "lcg, xorshift64, xorshift128, mwc64x"

typedef struct clo_rng CloRng;

typedef enum clo_rng_seed_type {

	CLO_RNG_SEED_DEV_GID = 0,
	CLO_RNG_SEED_HOST_MT = 1,
	CLO_RNG_EXT_DEV      = 2,
	CLO_RNG_EXT_HOST     = 3

} CloRngSeedType;

CloRng* clo_rng_new(const char* type, CloRngSeedType seed_type,
	void* seeds, size_t seeds_count, cl_ulong main_seed,
	const char* hash, CCLContext* ctx, CCLQueue* cq, GError** err);

void clo_rng_destroy(CloRng* rng);

const char* clo_rng_get_source(CloRng* rng);

CCLBuffer* clo_rng_get_device_seeds(CloRng* rng);

#endif

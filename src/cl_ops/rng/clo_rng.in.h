/*
 * This file is part of CL_Ops.
 *
 * CL_Ops is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * CL_Ops is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with CL_Ops. If not, see
 * <http://www.gnu.org/licenses/>.
 * */

/**
 * @file
 * CL_Ops RNG class declarations.
 * */

#ifndef _CLO_RNG_H_
#define _CLO_RNG_H_

#include "cl_ops/clo_common.h"
#include <string.h>

/* Kernels source code. */
#define CLO_RNG_SRC_WORKITEM "@RNG_SRC_WORKITEM@"
#define CLO_RNG_SRC_LCG "@RNG_SRC_LCG@"
#define CLO_RNG_SRC_XORSHIFT64 "@RNG_SRC_XORSHIFT64@"
#define CLO_RNG_SRC_XORSHIFT128 "@RNG_SRC_XORSHIFT128@"
#define CLO_RNG_SRC_MWC64X "@RNG_SRC_MWC64X@"
#define CLO_RNG_SRC_PARKMILLER "@RNG_SRC_PARKMILLER@"
#define CLO_RNG_SRC "@RNG_SRC@"

/* Device seed initialization kernel. */
#define CLO_RNG_SRC_INIT "@RNG_SRC_INIT@"

/* Available RNGs */
#define CLO_RNG_IMPLS "lcg, xorshift64, xorshift128, mwc64x, parkmiller"

/**
 * A RNG algorithm information: name, kernel constant and seed size in
 * bytes.
 * */
struct clo_rng_info {

	/** RNG algorithm name. */
	const char* name;

	/** RNG algorithm source. */
	const char* src;

	/** Seed size in butes. */
	const size_t seed_size;
};

/**
 * Information about the random number generation algorithms.
 * */
extern const struct clo_rng_info clo_rng_infos[];

/**
 * @defgroup CLO_RNG Random number generators
 *
 * This module provides several random number generators.
 *
 * @{
 */

/**
 * Type of seed to use.
 * */
typedef enum clo_rng_seed_type {

	/** Device initialized seeds based on workitem global ID. */
	CLO_RNG_SEED_DEV_GID  = 0,

	/** Host initialized seeds (Mersenne Twister). */
	CLO_RNG_SEED_HOST_MT  = 1,

	/** Client initialized seeds, already in device. */
	CLO_RNG_SEED_EXT_DEV  = 2,

	/** Client initialized seeds, still in host. */
	CLO_RNG_SEED_EXT_HOST = 3

} CloRngSeedType;

/** @} */

/* Create a new RNG object. */
CloRng* clo_rng_new(const char* type, CloRngSeedType seed_type,
	void* seeds, size_t seeds_count, cl_ulong main_seed,
	const char* hash, CCLContext* ctx, CCLQueue* cq, GError** err);

/* Destroy a RNG object. */
void clo_rng_destroy(CloRng* rng);

/* Get the OpenCL source code for the RNG object. */
const char* clo_rng_get_source(CloRng* rng);

/* Get in-device seeds. */
CCLBuffer* clo_rng_get_device_seeds(CloRng* rng);

/* Get size in bytes of seeds buffer in device. */
size_t clo_rng_get_size(CloRng* rng) ;

#endif

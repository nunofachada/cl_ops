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
 * CL-Ops RNG class definitions.
 * */

#include "clo_rng.h"

/**
 * @addtogroup CLO_RNG
 * @{
 */

/**
 * RNG class.
 * */
struct clo_rng {

	/**
	 * Kernels source code.
	 * @private
	 * */
	char* src;

	/**
	 * Device seeds state.
	 * @private
	 * */
	CCLBuffer* seeds_device;

};

/**
 * @internal
 * A RNG algorithm information: name, kernel constant and seed size in
 * bytes.
 * */
struct ocl_rng_info {

	/** RNG algorithm name. */
	const char* name;

	/** RNG algorithm source. */
	const char* src;

	/** Seed size in butes. */
	size_t seed_size;
};

/**
 * @internal
 * Information about the random number generation algorithms.
 * */
static struct ocl_rng_info rng_infos[] = {
	{"lcg", CLO_RNG_SRC_LCG, 8},
	{"xorshift64", CLO_RNG_SRC_XORSHIFT64, 8},
	{"xorshift128", CLO_RNG_SRC_XORSHIFT128, 16},
	{"mwc64x", CLO_RNG_SRC_MWC64X, 8},
	{NULL, NULL, 0}
};

/**
 * @internal
 * Perform seed initialization in device.
 *
 * @param[in] ctx Context wrapper object.
 * @param[in] cq Command-queue wrapper object.
 * @param[in] hash Seed hash macro code. If `NULL` defaults to no hash.
 * @param[in] seeds_count Number of seeds.
 * @param[in] seed_size Size of each seed.
 * @param[in] main_seed Base seed.
 * @param[out] err Return location for a GError, or `NULL` if error
 * reporting is to be ignored.
 * @return Buffer of in-device seeds.
 * */
static CCLBuffer* clo_rng_device_seed_init(CCLContext* ctx,
	CCLQueue* cq, const char* hash, size_t seeds_count,
	size_t seed_size, cl_ulong main_seed, GError** err) {

	/* Program wrapper object. */
	CCLProgram* prg;
	/* Buffer of in-device seeds. */
	CCLBuffer* seeds = NULL;
	/* Internal error handling object. */
	GError* err_internal = NULL;
	/* Effective seed hash macro code. */
	const char* hash_eff;
	/* Source code to prepend to init kernel. */
	gchar* init_src = NULL;

	/* Determine size of seed vector. */
	size_t seeds_vec_size = seed_size * seeds_count / sizeof(cl_ulong);

	/* Determine effective seed hash macro code. */
	if ((hash) && (*hash))
		hash_eff = hash;
	else
		hash_eff = "x";

	/* Construct seeds init source code. */
	init_src = g_strconcat("#define CLO_RNG_HASH(x) ", hash_eff, "\n"
		CLO_RNG_SRC_INIT, NULL);

	/* Create in-device seeds buffer. */
	seeds = ccl_buffer_new(ctx, CL_MEM_READ_WRITE,
		seeds_vec_size * sizeof(cl_ulong), NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Create seed initialization program. */
	prg = ccl_program_new_from_source(ctx, init_src, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Build seed initialization program. */
	ccl_program_build(prg, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Enqueue seed initialization kernel. */
	ccl_program_enqueue_kernel(prg, "clo_rng_init", cq, 1, 0,
		&seeds_vec_size, NULL, NULL, &err_internal,
		ccl_arg_priv(main_seed, cl_ulong), seeds, NULL);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	goto finish;

error_handler:

	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);

	/* If an error occurred make sure the seeds buffer is destroyed and
	 * set to NULL. */
	if (seeds) ccl_buffer_destroy(seeds);
	seeds = NULL;

finish:

	/* Free stuff. */
	if (prg) ccl_program_destroy(prg);
	if (init_src) g_free(init_src);

	/* Return seeds buffer. */
	return seeds;

}

/**
 * @internal
 * Perform seed initialization in host, then transfer to device.
 *
 * @param[in] ctx Context wrapper object.
 * @param[in] cq Command-queue wrapper object.
 * @param[in] seeds_count Number of seeds.
 * @param[in] seed_size Size of each seed.
 * @param[in] main_seed Base seed.
 * @param[out] err Return location for a GError, or `NULL` if error
 * reporting is to be ignored.
 * @return Buffer of in-device seeds.
 * */
static CCLBuffer* clo_rng_host_seed_init(CCLContext* ctx, CCLQueue* cq,
	size_t seeds_count, size_t seed_size, cl_ulong main_seed,
	GError** err) {

	/* In-device seeds buffer. */
	CCLBuffer* seeds_dev;
	/* Random number generator. */
	GRand* rng_host = NULL;
	/* Size in bytes of seeds vector. */
	size_t seeds_vec_size;
	/* Host seeds vector. */
	void* seeds_host = NULL;
	/* Internal error handling object. */
	GError* err_internal = NULL;

	/* Determine size in bytes of seeds vector. */
	seeds_vec_size = seed_size * seeds_count;
	/* Allocate memory for host seeds vector. */
	seeds_host = g_slice_alloc(seeds_vec_size);

	/* Create in-device seeds buffer. */
	seeds_dev = ccl_buffer_new(ctx, CL_MEM_READ_WRITE, seeds_vec_size,
		NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* Initialize generator. */
	rng_host = g_rand_new_with_seed(main_seed);

	/* Generate seeds. */
	for (cl_uint i = 0; i < seeds_vec_size / sizeof(guint32); i++) {
		guint32 rand_value = g_rand_int(rng_host);
		void* dest = ((unsigned char*) seeds_host) + i * sizeof(guint32);
		g_memmove(dest, &rand_value, sizeof(guint32));
	}

	/* Copy seeds to device. */
	ccl_buffer_enqueue_write(seeds_dev, cq, CL_TRUE, 0, seeds_vec_size,
		seeds_host, NULL, &err_internal);
	ccl_if_err_propagate_goto(err, err_internal, error_handler);

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	goto finish;

error_handler:

	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);

	/* If an error occurred make sure the seeds buffer is destroyed and
	 * set to NULL. */
	if (seeds_dev) ccl_buffer_destroy(seeds_dev);
	seeds_dev = NULL;

finish:

	/* Free stuff. */
	if (seeds_host) g_slice_free1(seeds_vec_size, seeds_host);
	if (rng_host) g_rand_free(rng_host);

	/* Return seeds buffer. */
	return seeds_dev;

}

/**
 * Create a new RNG object.
 *
 * @public @memberof clo_rng
 *
 * @param[in] type Type of RNG: lcg, xorshift64, xorshift128, mwc64x.
 * @param[in] seed_type Type of seed.
 * @param[in] seeds Array of seeds (only for ::CLO_RNG_SEED_EXT_HOST
 * seed type, assumed to be an array of `cl_ulong` seeds with length
 * equal to `seeds_count`).
 * @param[in] seeds_count Number of seeds.
 * @param[in] main_seed Base seed (ignored for external seed types).
 * @param[in] hash Hash for ::CLO_RNG_SEED_DEV_GID seed type.
 * @param[in] ctx Context wrapper (not required for
 * ::CLO_RNG_SEED_EXT_DEV seed type).
 * @param[in] cq Command queue wrapper (not required for
 * ::CLO_RNG_SEED_EXT_DEV seed type).
 * @param[out] err Return location for a GError, or `NULL` if error
 * reporting is to be ignored.
 * @return A new RNG object.
 * */
CloRng* clo_rng_new(const char* type, CloRngSeedType seed_type,
	void* seeds, size_t seeds_count, cl_ulong main_seed,
	const char* hash, CCLContext* ctx, CCLQueue* cq, GError** err) {

	/* Make sure err is NULL or it is not set. */
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	/* The new RNG object. */
	CloRng* rng = NULL;

	/* Device seeds. */
	CCLBuffer* dev_seeds = NULL;

	/* Internal error object. */
	GError* err_internal = NULL;

	/* Search in the list of known scan classes. */
	for (guint i = 0; rng_infos[i].name != NULL; ++i) {
		if (g_strcmp0(type, rng_infos[i].name) == 0) {
			/* If found, return an new instance.*/

			/* Deal with seeds. */
			switch (seed_type) {
				case CLO_RNG_SEED_DEV_GID:
					/* Check that seeds parameter is NULL. */
					ccl_if_err_create_goto(*err, CLO_ERROR,
						seeds != NULL, CLO_ERROR_ARGS, error_handler,
						"The DEV_GID seed type expects a NULL seeds "\
						"parameter.");
					/* Initialize seeds in device using GID. */
					dev_seeds = clo_rng_device_seed_init(ctx, cq, hash,
						seeds_count, rng_infos[i].seed_size, main_seed,
						&err_internal);
					ccl_if_err_propagate_goto(err, err_internal,
						error_handler);
					/* Get out of switch. */
					break;
				case CLO_RNG_SEED_HOST_MT:
					/* Check that seeds parameter is NULL. */
					ccl_if_err_create_goto(*err, CLO_ERROR,
						seeds != NULL, CLO_ERROR_ARGS, error_handler,
						"The HOST_MT seed type expects a NULL seeds "\
						"parameter.");
					/* Initialize seeds in host and copy them to
					 * device. */
					dev_seeds = clo_rng_host_seed_init(ctx, cq,
						seeds_count, rng_infos[i].seed_size, main_seed,
						&err_internal);
					ccl_if_err_propagate_goto(err, err_internal,
						error_handler);
					/* Get out of switch. */
					break;
				case CLO_RNG_SEED_EXT_DEV:
					/* Check that seeds parameter is NULL. */
					ccl_if_err_create_goto(*err, CLO_ERROR,
						seeds != NULL, CLO_ERROR_ARGS, error_handler,
						"The EXT_DEV seed type expects a NULL seeds "\
						"parameter.");
					/* Get out of switch. */
					break;
				case CLO_RNG_SEED_EXT_HOST:
					/* Check that seeds is not NULL. */
					ccl_if_err_create_goto(*err, CLO_ERROR,
						seeds == NULL, CLO_ERROR_ARGS, error_handler,
						"The EXT_HOST seed type expects a non-NULL "\
						"seeds parameter.");
					/* Create device buffer and copy seeds to device. */
					dev_seeds = ccl_buffer_new(ctx, CL_MEM_READ_WRITE,
						seeds_count * sizeof(cl_ulong), NULL,
						&err_internal);
					ccl_if_err_propagate_goto(err, err_internal,
						error_handler);
					ccl_buffer_enqueue_write(dev_seeds , cq, CL_TRUE, 0,
						seeds_count * sizeof(cl_ulong), seeds, NULL,
						&err_internal);
					ccl_if_err_propagate_goto(err, err_internal,
						error_handler);
					/* Get out of switch. */
					break;
				default:
					/* An invalid seed type was specified, throw error. */
					ccl_if_err_create_goto(*err, CLO_ERROR, TRUE,
						CLO_ERROR_ARGS, error_handler,
						"Unknown seed type.");
			}

			/* Allocate memory for RNG object. */
			rng = g_slice_new0(CloRng);

			/* Construct source code. */
			rng->src = g_strconcat(CLO_RNG_SRC_WORKITEM,
				rng_infos[i].src, CLO_RNG_SRC, NULL);

			/* Set seeds buffer. */
			rng->seeds_device = dev_seeds;

		}
	}

	/* If no implementation found, throw error. */
	ccl_if_err_create_goto(*err, CLO_ERROR, rng == NULL,
		CLO_ERROR_IMPL_NOT_FOUND, error_handler,
		"The requested RNG implementation, '%s', was not found.", type);

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	goto finish;

error_handler:

	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);

	/* If a buffer was created, destroy it. */
	if (dev_seeds != NULL) ccl_buffer_destroy(dev_seeds);

finish:

	/* Return RNG object. */
	return rng;
}

/**
 * Destroy a RNG object.
 *
 * @public @memberof clo_rng
 *
 * @param[in] rng RNG object to destroy.
 * */
void clo_rng_destroy(CloRng* rng) {

	/* Make sure rng object is not NULL. */
	g_return_if_fail(rng != NULL);

	/* Destroy in-device seeds buffer. */
	if (rng->seeds_device != NULL)
		ccl_buffer_destroy(rng->seeds_device);

	/* Destroy source code string. */
	g_free(rng->src);

	/* Destroy rng object. */
	g_slice_free(CloRng, rng);

}

/**
 * Get the OpenCL source code for the RNG object.
 *
 * @public @memberof clo_rng
 *
 * @param[in] rng RNG object.
 * @return The OpenCL source code associated with the given RNG object.
 * */
const char* clo_rng_get_source(CloRng* rng) {

	/* Make sure rng object is not NULL. */
	g_return_val_if_fail(rng != NULL, NULL);

	/* Return source. */
	return (const char*) rng->src;

}

/**
 * Get in-device seeds. Only for non-external seed types.
 *
 * @public @memberof clo_rng
 *
 * @param[in] rng RNG object.
 * @return Buffer representing in-device seeds.
 * */
CCLBuffer* clo_rng_get_device_seeds(CloRng* rng) {

	/* Make sure rng object is not NULL. */
	g_return_val_if_fail(rng != NULL, NULL);

	/* Make sure in-device seeds are not NULL. */
	g_return_val_if_fail(rng->seeds_device != NULL, NULL);

	/* Return in-device seeds. */
	return rng->seeds_device;
}

/** @} */

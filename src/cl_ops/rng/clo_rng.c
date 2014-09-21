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

#include "clo_rng.h"

struct clo_rng {

	char* src;
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
 * @brief Information about the random number generation algorithms.
 * */
static struct ocl_rng_info rng_infos[] = {
	{"lcg", CLO_RNG_LCG, 8},
	{"xorshift64", CLO_RNG_XORSHIFT64, 8},
	{"xorshift128", CLO_RNG_XORSHIFT128, 16},
	{"mwc64x", CLO_RNG_MWC64X, 8},
	{NULL, NULL, 0}
};

static CCLBuffer* clo_rng_device_seed_init(CCLContext* ctx,
	CCLDevice* dev, CCLQueue* cq, const char* hash, size_t size,
	GError** err) {

}

static CCLBuffer* clo_rng_host_seed_init(CCLContext* ctx,
	CCLDevice* dev, CCLQueue* cq, size_t size, GError** err) {

}

CloRng* clo_rng_new(const char* type, CloRngSeedType seed_type,
	void* seeds, const char* hash, size_t size,
	CCLContext* ctx, CCLDevice* dev, CCLQueue* cq, GError** err) {

	/* The new RNG object. */
	CloRng* rng;

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
					dev_seeds = clo_rng_device_seed_init(ctx, dev, cq,
						hash, size, &err_internal);
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
					dev_seeds = clo_rng_host_seed_init(ctx, dev, cq,
						size, &err_internal);
					ccl_if_err_propagate_goto(err, err_internal, error_handler);
					/* Get out of switch. */
					break;
				case CLO_RNG_EXT_DEV:
					/* Check that seeds parameter is NULL. */
					ccl_if_err_create_goto(*err, CLO_ERROR,
						seeds != NULL, CLO_ERROR_ARGS, error_handler,
						"The EXT_DEV seed type expects a NULL seeds "\
						"parameter.");
					/* Get out of switch. */
					break;
				case CLO_RNG_EXT_HOST:
					/* Check that seeds is not NULL. */
					ccl_if_err_create_goto(*err, CLO_ERROR,
						seeds != NULL, CLO_ERROR_ARGS, error_handler,
						"The EXT_HOST seed type expects a non-NULL "\
						"seeds parameter.");
					/* Create device buffer and copy seeds to device. */
					dev_seeds = ccl_buffer_new(ctx, CL_MEM_READ_WRITE, size, NULL, &err_internal);
					ccl_if_err_propagate_goto(err, err_internal, error_handler);
					ccl_buffer_enqueue_write(dev_seeds , cq, CL_TRUE, 0, size, seeds, NULL, &err_internal);
					ccl_if_err_propagate_goto(err, err_internal, error_handler);
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

void clo_rng_destroy(CloRng* rng) {

	if (rng->seeds_device != NULL)
		ccl_buffer_destroy(rng->seeds_device);

	g_free(rng->src);

	g_slice_free(CloRng, rng);

}

const char* clo_rng_get_source(CloRng* rng) {

	return (const char*) rng->source;

}

/// Only for non-ext seed type
CCLBuffer* clo_rng_get_device_seeds(CloRng* rng) {

	g_return_val_if_fail(rng->seeds_device, NULL);

	return rng->seeds_device;
}


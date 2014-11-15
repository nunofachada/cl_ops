/*
 * This file is part of cf4ocl (C Framework for OpenCL).
 *
 * cf4ocl is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cf4ocl is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with cf4ocl. If not, see <http://www.gnu.org/licenses/>.
 * */

/**
 * @file
 * Test the RNG class on functionality of the several options. No tests
 * regarding the quality of the produced pseudo-random numbers are
 * performed.
 *
 * @author Nuno Fachada
 * @date 2014
 * @copyright [GNU General Public License version 3 (GPLv3)](http://www.gnu.org/licenses/gpl.html)
 * */

#include <cl_ops.h>

#define CLO_RNG_TEST_KERNEL "clo_rng_test"
#define CLO_RNG_TEST_SRC \
	"__kernel void " CLO_RNG_TEST_KERNEL "(" \
	"		__global rng_state* seeds, __global ulong* output) {" \
	"	uint gid = get_global_id(0);" \
	"	uint x = clo_rng_next_int(seeds, UINT_MAX);" \
	"	output[gid] = (ulong) x;" \
	"}"

#define CLO_RNG_TEST_NUM_SEEDS 10000
#define CLO_RNG_TEST_INIT_SEED 1234
#define CLO_RNG_TEST_HASH "(x * 3 + 1)"


/**
 * Test RNG with GID-based device generated seeds.
 * */
static void seed_dev_gid_test() {

	/* Test variables. */
	CCLContext* ctx = NULL;
	CCLDevice* dev = NULL;
	CCLQueue* cq = NULL;
	CCLProgram* prg = NULL;
	CCLKernel* krnl = NULL;
	CCLBuffer* seeds_dev = NULL;
	CCLBuffer* output_dev = NULL;
	GError* err = NULL;
	CloRng* rng = NULL;
	size_t lws = 0;
	size_t ws = CLO_RNG_TEST_NUM_SEEDS;
	gchar* src;

	/* Get context and device. */
	ctx = ccl_context_new_any(&err);
	g_assert_no_error(err);

	dev = ccl_context_get_device(ctx, 0, &err);
	g_assert_no_error(err);

	/* Create command queue. */
	cq = ccl_queue_new(ctx, dev, 0, &err);
	g_assert_no_error(err);

	/* Test all RNGs. */
	for (cl_uint i = 0; clo_rng_infos[i].name != NULL; ++i) {

		/* Create RNG object. */
		rng = clo_rng_new(clo_rng_infos[i].name, CLO_RNG_SEED_DEV_GID,
			NULL, CLO_RNG_TEST_NUM_SEEDS, CLO_RNG_TEST_INIT_SEED,
			CLO_RNG_TEST_HASH, ctx, cq, &err);
		g_assert_no_error(err);

		/* Get RNG seeds device buffer. */
		seeds_dev = clo_rng_get_device_seeds(rng);

		/* Get RNG kernels source. */
		src = g_strconcat(
			clo_rng_get_source(rng), CLO_RNG_TEST_SRC, NULL);

		/* Create and build program. */
		prg = ccl_program_new_from_source(ctx, src, &err);
		g_assert_no_error(err);

		ccl_program_build(prg, NULL, &err);
		g_assert_no_error(err);

		/* Create output buffer. */
		output_dev = ccl_buffer_new(ctx, CL_MEM_WRITE_ONLY,
			CLO_RNG_TEST_NUM_SEEDS * sizeof(cl_ulong), NULL, &err);
		g_assert_no_error(err);

		/* Get kernel from program. */
		krnl = ccl_program_get_kernel(prg, CLO_RNG_TEST_KERNEL, &err);
		g_assert_no_error(err);

		/* Get a "nice" local worksize. */
		ccl_kernel_suggest_worksizes(
			krnl, dev, 1, &ws, NULL, &lws, &err);
		g_assert_no_error(err);

		/* Execute kernel. */
		ccl_kernel_set_args_and_enqueue_ndrange(
			krnl, cq, 1, NULL, &ws, &lws, NULL, &err,
			seeds_dev, output_dev, NULL);
		g_assert_no_error(err);

		/* Release this iteration stuff. */
		g_free(src);
		ccl_buffer_destroy(output_dev);
		ccl_program_destroy(prg);
		clo_rng_destroy(rng);

	}

	/* Destroy queue and context. */
	ccl_queue_destroy(cq);
	ccl_context_destroy(ctx);

	/* Confirm that memory allocated by wrappers has been properly
	 * freed. */
	g_assert(ccl_wrapper_memcheck());

}

/**
 * Test RNG with Mersenne Twister host generated seeds.
 * */
static void seed_host_mt_test() {

	/* Test variables. */
	CCLContext* ctx = NULL;
	CCLDevice* dev = NULL;
	CCLQueue* cq = NULL;
	CCLProgram* prg = NULL;
	CCLKernel* krnl = NULL;
	CCLBuffer* seeds_dev = NULL;
	CCLBuffer* output_dev = NULL;
	GError* err = NULL;
	CloRng* rng = NULL;
	size_t lws = 0;
	size_t ws = CLO_RNG_TEST_NUM_SEEDS;
	gchar* src;

	/* Get context and device. */
	ctx = ccl_context_new_any(&err);
	g_assert_no_error(err);

	dev = ccl_context_get_device(ctx, 0, &err);
	g_assert_no_error(err);

	/* Create command queue. */
	cq = ccl_queue_new(ctx, dev, 0, &err);
	g_assert_no_error(err);

	/* Test all RNGs. */
	for (cl_uint i = 0; clo_rng_infos[i].name != NULL; ++i) {

		/* Create RNG object. */
		rng = clo_rng_new(clo_rng_infos[i].name, CLO_RNG_SEED_HOST_MT,
			NULL, CLO_RNG_TEST_NUM_SEEDS, CLO_RNG_TEST_INIT_SEED,
			NULL, ctx, cq, &err);
		g_assert_no_error(err);

		/* Get RNG seeds device buffer. */
		seeds_dev = clo_rng_get_device_seeds(rng);

		/* Get RNG kernels source. */
		src = g_strconcat(
			clo_rng_get_source(rng), CLO_RNG_TEST_SRC, NULL);

		/* Create and build program. */
		prg = ccl_program_new_from_source(ctx, src, &err);
		g_assert_no_error(err);

		ccl_program_build(prg, NULL, &err);
		g_assert_no_error(err);

		/* Create output buffer. */
		output_dev = ccl_buffer_new(ctx, CL_MEM_WRITE_ONLY,
			CLO_RNG_TEST_NUM_SEEDS * sizeof(cl_ulong), NULL, &err);
		g_assert_no_error(err);

		/* Get kernel from program. */
		krnl = ccl_program_get_kernel(prg, CLO_RNG_TEST_KERNEL, &err);
		g_assert_no_error(err);

		/* Get a "nice" local worksize. */
		ccl_kernel_suggest_worksizes(
			krnl, dev, 1, &ws, NULL, &lws, &err);
		g_assert_no_error(err);

		/* Execute kernel. */
		ccl_kernel_set_args_and_enqueue_ndrange(
			krnl, cq, 1, NULL, &ws, &lws, NULL, &err,
			seeds_dev, output_dev, NULL);
		g_assert_no_error(err);

		/* Release this iteration stuff. */
		g_free(src);
		ccl_buffer_destroy(output_dev);
		ccl_program_destroy(prg);
		clo_rng_destroy(rng);

	}

	/* Destroy queue and context. */
	ccl_queue_destroy(cq);
	ccl_context_destroy(ctx);

	/* Confirm that memory allocated by wrappers has been properly
	 * freed. */
	g_assert(ccl_wrapper_memcheck());

}

/**
 * Test RNG with client generated seeds in device.
 * */
static void seed_ext_dev_test() {

	/* Test variables. */
	CCLContext* ctx = NULL;
	CCLDevice* dev = NULL;
	CCLQueue* cq = NULL;
	CCLProgram* prg = NULL;
	CCLKernel* krnl = NULL;
	CCLBuffer* seeds_dev = NULL;
	CCLBuffer* output_dev = NULL;
	GError* err = NULL;
	CloRng* rng = NULL;
	size_t lws = 0;
	size_t ws = CLO_RNG_TEST_NUM_SEEDS;
	gchar* src;
	cl_uchar* host_seeds;

	/* Get context and device. */
	ctx = ccl_context_new_any(&err);
	g_assert_no_error(err);

	dev = ccl_context_get_device(ctx, 0, &err);
	g_assert_no_error(err);

	/* Create command queue. */
	cq = ccl_queue_new(ctx, dev, 0, &err);
	g_assert_no_error(err);

	/* Test all RNGs. */
	for (cl_uint i = 0; clo_rng_infos[i].name != NULL; ++i) {

		/* Host seeds must account for the seed size of current RNG. */
		size_t seed_size =
			clo_rng_infos[i].seed_size * CLO_RNG_TEST_NUM_SEEDS;
		host_seeds = g_slice_alloc(seed_size);

		/* Initialize host seeds with any value. */
		for (cl_uint i = 0; i < seed_size; ++i)
			host_seeds[i] = (cl_uchar) (((i + 1) * 3) & 0xFF);

		/* Allocate memory for device seeds and copy host seeds. */
		seeds_dev = ccl_buffer_new(
			ctx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, seed_size,
			host_seeds, &err);
		g_assert_no_error(err);

		/* Create RNG object. */
		rng = clo_rng_new(clo_rng_infos[i].name, CLO_RNG_SEED_EXT_DEV,
			seeds_dev, CLO_RNG_TEST_NUM_SEEDS, CLO_RNG_TEST_INIT_SEED,
			NULL, ctx, cq, &err);
		g_assert_no_error(err);

		/* Get RNG kernels source. */
		src = g_strconcat(
			clo_rng_get_source(rng), CLO_RNG_TEST_SRC, NULL);

		/* Create and build program. */
		prg = ccl_program_new_from_source(ctx, src, &err);
		g_assert_no_error(err);

		ccl_program_build(prg, NULL, &err);
		g_assert_no_error(err);

		/* Create output buffer. */
		output_dev = ccl_buffer_new(ctx, CL_MEM_WRITE_ONLY,
			CLO_RNG_TEST_NUM_SEEDS * sizeof(cl_ulong), NULL, &err);
		g_assert_no_error(err);

		/* Get kernel from program. */
		krnl = ccl_program_get_kernel(prg, CLO_RNG_TEST_KERNEL, &err);
		g_assert_no_error(err);

		/* Get a "nice" local worksize. */
		ccl_kernel_suggest_worksizes(
			krnl, dev, 1, &ws, NULL, &lws, &err);
		g_assert_no_error(err);

		/* Execute kernel. */
		ccl_kernel_set_args_and_enqueue_ndrange(
			krnl, cq, 1, NULL, &ws, &lws, NULL, &err,
			seeds_dev, output_dev, NULL);
		g_assert_no_error(err);

		/* Release this iteration stuff. */
		g_slice_free1(seed_size, host_seeds);
		g_free(src);
		ccl_buffer_destroy(seeds_dev);
		ccl_buffer_destroy(output_dev);
		ccl_program_destroy(prg);
		clo_rng_destroy(rng);

	}

	/* Destroy queue and context. */
	ccl_queue_destroy(cq);
	ccl_context_destroy(ctx);

	/* Confirm that memory allocated by wrappers has been properly
	 * freed. */
	g_assert(ccl_wrapper_memcheck());

}

/**
 * Test RNG with client generated seeds in host.
 * */
static void seed_ext_host_test() {

	/* Test variables. */
	CCLContext* ctx = NULL;
	CCLDevice* dev = NULL;
	CCLQueue* cq = NULL;
	CCLProgram* prg = NULL;
	CCLKernel* krnl = NULL;
	CCLBuffer* seeds_dev = NULL;
	CCLBuffer* output_dev = NULL;
	GError* err = NULL;
	CloRng* rng = NULL;
	size_t lws = 0;
	size_t ws = CLO_RNG_TEST_NUM_SEEDS;
	gchar* src;
	cl_uchar* host_seeds;

	/* Get context and device. */
	ctx = ccl_context_new_any(&err);
	g_assert_no_error(err);

	dev = ccl_context_get_device(ctx, 0, &err);
	g_assert_no_error(err);

	/* Create command queue. */
	cq = ccl_queue_new(ctx, dev, 0, &err);
	g_assert_no_error(err);

	/* Test all RNGs. */
	for (cl_uint i = 0; clo_rng_infos[i].name != NULL; ++i) {

		/* Host seeds must account for the seed size of current RNG. */
		size_t seed_size =
			clo_rng_infos[i].seed_size * CLO_RNG_TEST_NUM_SEEDS;
		host_seeds = g_slice_alloc(seed_size);

		/* Initialize host seeds with any value. */
		for (cl_uint i = 0; i < seed_size; ++i)
			host_seeds[i] = (cl_uchar) (((i + 1) * 3) & 0xFF);

		/* Create RNG object. */
		rng = clo_rng_new(clo_rng_infos[i].name, CLO_RNG_SEED_EXT_HOST,
			host_seeds, CLO_RNG_TEST_NUM_SEEDS, CLO_RNG_TEST_INIT_SEED,
			NULL, ctx, cq, &err);
		g_assert_no_error(err);

		/* Get RNG seeds device buffer. */
		seeds_dev = clo_rng_get_device_seeds(rng);

		/* Get RNG kernels source. */
		src = g_strconcat(
			clo_rng_get_source(rng), CLO_RNG_TEST_SRC, NULL);

		/* Create and build program. */
		prg = ccl_program_new_from_source(ctx, src, &err);
		g_assert_no_error(err);

		ccl_program_build(prg, NULL, &err);
		g_assert_no_error(err);

		/* Create output buffer. */
		output_dev = ccl_buffer_new(ctx, CL_MEM_WRITE_ONLY,
			CLO_RNG_TEST_NUM_SEEDS * sizeof(cl_ulong), NULL, &err);
		g_assert_no_error(err);

		/* Get kernel from program. */
		krnl = ccl_program_get_kernel(prg, CLO_RNG_TEST_KERNEL, &err);
		g_assert_no_error(err);

		/* Get a "nice" local worksize. */
		ccl_kernel_suggest_worksizes(
			krnl, dev, 1, &ws, NULL, &lws, &err);
		g_assert_no_error(err);

		/* Execute kernel. */
		ccl_kernel_set_args_and_enqueue_ndrange(
			krnl, cq, 1, NULL, &ws, &lws, NULL, &err,
			seeds_dev, output_dev, NULL);
		g_assert_no_error(err);

		/* Release this iteration stuff. */
		g_slice_free1(seed_size, host_seeds);
		g_free(src);
		ccl_buffer_destroy(output_dev);
		ccl_program_destroy(prg);
		clo_rng_destroy(rng);

	}

	/* Destroy queue and context. */
	ccl_queue_destroy(cq);
	ccl_context_destroy(ctx);

	/* Confirm that memory allocated by wrappers has been properly
	 * freed. */
	g_assert(ccl_wrapper_memcheck());

}


/**
 * Main function.
 * @param[in] argc Number of command line arguments.
 * @param[in] argv Command line arguments.
 * @return Result of test run.
 * */
int main(int argc, char** argv) {

	g_test_init(&argc, &argv, NULL);

	g_test_add_func(
		"/rng/seed-dev-gid",
		seed_dev_gid_test);

	g_test_add_func(
		"/rng/seed-host-mt",
		seed_host_mt_test);

	g_test_add_func(
		"/rng/seed-ext-dev",
		seed_ext_dev_test);

	g_test_add_func(
		"/rng/seed-ext-host",
		seed_ext_host_test);

	return g_test_run();
}




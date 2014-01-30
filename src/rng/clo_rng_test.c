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
 * @brief Test RNGs.
 */

#include "clo_rng_test.h"

#define CLO_RNG_DEFAULT "lcg"
#define CLO_RNG_OUTPUT_PREFIX "out"
#define CLO_RNG_OUTPUT "file-tsv"
#define CLO_RNG_GWS 262144
#define CLO_RNG_LWS 256
#define CLO_RNG_RUNS 10
#define CLO_RNG_BITS 32

/** A description of the program. */
#define CLO_RNG_DESCRIPTION "Test RNGs"

/** The kernels source file. */
#define CLO_RNG_KERNEL_SRC "clo_rng_test.cl"

/* Command line arguments and respective default values. */
static gchar *rng = NULL;
static gchar *output = NULL;
static size_t gws = CLO_RNG_GWS;
static size_t lws = CLO_RNG_LWS;
static unsigned int runs = CLO_RNG_RUNS;
static int dev_idx = -1;
static guint32 rng_seed = CLO_DEFAULT_SEED;
static gboolean gid_seed = FALSE;
static unsigned int bits = CLO_RNG_BITS;
static unsigned int maxint = 0;
static gchar *path = NULL;

/* Valid command line options. */
static GOptionEntry entries[] = {
	{"rng",          'r', 0, G_OPTION_ARG_STRING, &rng,      "Random number generator: " CLO_RNGS " (default is " CLO_RNG_DEFAULT ")",           "RNG"},
	{"output",       'o', 0, G_OPTION_ARG_STRING, &output,   "Output: file-tsv, file-dh, stdout-bin, stdout-uint (default: " CLO_RNG_OUTPUT ")", "OUTPUT"},
	{"globalsize",   'g', 0, G_OPTION_ARG_INT,    &gws,      "Global work size (default is " STR(CLO_RNG_GWS) ")",                               "SIZE"},
	{"localsize",    'l', 0, G_OPTION_ARG_INT,    &lws,      "Local work size (default is " STR(CLO_RNG_LWS) ")",                                "SIZE"},
	{"runs",         'n', 0, G_OPTION_ARG_INT,    &runs,     "Random numbers per workitem (0 is non-stop, default is " STR(CLO_RNG_RUNS) ")",    "SIZE"},
	{"device",       'd', 0, G_OPTION_ARG_INT,    &dev_idx,  "Device index",                                                                     "INDEX"},
	{"rng-seed",     's', 0, G_OPTION_ARG_INT,    &rng_seed, "Seed for random number generator (default is " STR(CLO_DEFAULT_SEED) ")",          "SEED"},
	{"use-gid-seed", 'u', 0, G_OPTION_ARG_NONE,   &gid_seed, "Use GID-based workitem seeds instead of MT derived seeds from host.",              NULL},
	{"bits",         'b', 0, G_OPTION_ARG_INT,    &bits,     "Number of bits in unsigned integers to produce (default " STR(CLO_RNG_BITS) ")",   NULL},
	{"max",          'm', 0, G_OPTION_ARG_INT,    &maxint,   "Maximum integer to produce, overrides --bits option",                              NULL},
	{"path",         'p', 0, G_OPTION_ARG_STRING, &path,     "Path of OpenCL source files (default is " CLO_DEFAULT_PATH,                        "PATH"},  
	{ NULL, 0, 0, 0, NULL, NULL, NULL }	
};

/** Information about the requested random number generation algorithm. */
static CloRngInfo rng_info = {NULL, NULL, 0};

/**
 * @brief Main program.
 * 
 * @param argc Number of command line arguments.
 * @param argv Vector of command line arguments.
 * @return @link clo_error_codes::CLO_SUCCESS @endlink if program 
 * terminates successfully, or another value of #clo_error_codes if an
 * error occurs.
 * */
int main(int argc, char **argv)
{

	/* Status var aux */
	int status, ocl_status;
	
	/* Context object for command line argument parsing. */
	GOptionContext *context = NULL;
	
	/* Kernel file. */
	gchar* kernelFile = NULL;
			
	/* Test data structures. */
	cl_kernel init_rng = NULL, test_rng = NULL;
	cl_uint **result_host = NULL;
	cl_ulong *seeds_host = NULL;
	cl_mem seeds_dev = NULL, result_dev = NULL;
	FILE *fp;
	gchar *output_filename = NULL;
	gchar* compilerOpts = NULL;
	size_t seeds_count;

	/* Host-based random number generator (mersenne twister) */
	GRand* rng_host = NULL;	

	/* OpenCL zone: platform, device, context, queues, etc. */
	CLUZone* zone = NULL;
	
	/* Error management object. */
	GError *err = NULL;

	/* How long will it take? */
	GTimer* timer = NULL;
	
	/* Parse command line options. */
	context = g_option_context_new (" - " CLO_RNG_DESCRIPTION);
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_parse(context, &argc, &argv, &err);	
	gef_if_error_goto(err, CLO_LIBRARY_ERROR, status, error_handler);
	if (output == NULL) output = g_strdup(CLO_RNG_OUTPUT);
	if (rng == NULL) rng = g_strdup(CLO_RNG_DEFAULT);
	if (path == NULL) path = g_strdup(CLO_DEFAULT_PATH);
	CLO_ALG_GET(rng_info, rng_infos, rng);
	gef_if_error_create_goto(err, CLO_ERROR, 
		!rng_info.tag, 
		status = CLO_INVALID_ARGS, error_handler, 
		"Unknown random number generator '%s'.", rng);
	gef_if_error_create_goto(err, CLO_ERROR, 
		g_strcmp0(output, "file-tsv") && g_strcmp0(output, "file-dh") && g_strcmp0(output, "stdout-bin") && g_strcmp0(output, "stdout-uint"), 
		status = CLO_INVALID_ARGS, error_handler, 
		"Unknown output '%s'.", output);
	gef_if_error_create_goto(err, CLO_ERROR, 
		(bits > 32) || (bits < 1), 
		status = CLO_INVALID_ARGS, error_handler, 
		"Number of bits must be between 1 and 32.");
	
	/* Get the required CL zone. */
	zone = clu_zone_new(CL_DEVICE_TYPE_ALL, 1, 0, clu_menu_device_selector, (dev_idx != -1 ? &dev_idx : NULL), &err);
	gef_if_error_goto(err, CLO_LIBRARY_ERROR, status, error_handler);
	
	/* Build compiler options. */
	compilerOpts = g_strconcat(
		"-I ", path,
		" -D ", rng_info.compiler_const, 
		maxint ? " -D CLO_RNG_MAXINT" : "", 
		NULL);

	/* Determine complete kernel file location. */
	kernelFile = g_build_filename(path, CLO_RNG_KERNEL_SRC, NULL);

	/* Build program. */
	clu_program_create(zone, &kernelFile, 1, compilerOpts, &err);
	gef_if_error_goto(err, CLO_LIBRARY_ERROR, status, error_handler);
	
	/* Create test kernel. */
	test_rng = clCreateKernel(zone->program, "testRng", &ocl_status);
	gef_if_error_create_goto(err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_LIBRARY_ERROR, error_handler, "Create kernel test_rng: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
	
	/* Create host buffer */
	result_host = (cl_uint**) malloc(sizeof(cl_uint*) * runs);
	for (unsigned int i = 0; i < runs; i++) {
		result_host[i] = (cl_uint*) malloc(sizeof(cl_uint) * gws);
	}
	
	/* Create device buffer */
	seeds_count = gws * rng_info.bytes / sizeof(cl_ulong);
	seeds_dev = clCreateBuffer(zone->context, CL_MEM_READ_WRITE, seeds_count * sizeof(cl_ulong), NULL, &ocl_status);
	gef_if_error_create_goto(err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_LIBRARY_ERROR, error_handler, "Create device buffer 1: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
	
	result_dev = clCreateBuffer(zone->context, CL_MEM_READ_WRITE, gws * sizeof(cl_int), NULL, &ocl_status);
	gef_if_error_create_goto(err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_LIBRARY_ERROR, error_handler, "Create device buffer 2: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

	/*  Set test kernel arguments. */
	ocl_status = clSetKernelArg(test_rng, 0, sizeof(cl_mem), (void*) &seeds_dev);
	gef_if_error_create_goto(err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_LIBRARY_ERROR, error_handler, "Set test kernel arg 0: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
	
	ocl_status = clSetKernelArg(test_rng, 1, sizeof(cl_mem), (void*) &result_dev);
	gef_if_error_create_goto(err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_LIBRARY_ERROR, error_handler, "Set test kernel arg 1: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
	
	ocl_status = clSetKernelArg(test_rng, 2, sizeof(cl_uint), (void*) (maxint ? &maxint : &bits));
	gef_if_error_create_goto(err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_LIBRARY_ERROR, error_handler, "Set test kernel arg 2: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
	

	/* Print options. */
	g_print("\n   =========================== Selected options ============================\n\n");
	g_print("     Random number generator (seed): %s (%u)\n", rng, rng_seed);
	g_print("     Seeds in workitems: %s\n", gid_seed ? "GID-based" : "Host-based (using Mersenne Twister)");
	g_print("     Global/local worksizes: %d/%d\n", (int) gws, (int) lws);
	g_print("     Number of runs: %d\n", runs);
	g_print("     Number of bits / Maximum integer: %d / %u\n", bits, (unsigned int) ((1ul << bits) - 1));
	g_print("     Compiler Options: %s\n", compilerOpts);
	
	/* Inform about execution. */
	g_print("\n   =========================== Execution status ============================\n\n");
	g_print("     Producing random numbers...\n");
	
	/* Start timming. */
	timer = g_timer_new();
	
	/* If gid_seed is set, then initialize seeds in device with a kernel
	 * otherwise, initialize seeds in host and send them to device. */
	if (gid_seed) {
		/* *** Device initalization of seeds, GID-based. *** */
		
		/* Create init kernel. */
		init_rng = clCreateKernel(zone->program, "initRng", &ocl_status);
		gef_if_error_create_goto(err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_LIBRARY_ERROR, error_handler, "Create kernel init_rng: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
		
		/* Set init kernel args. */
		ocl_status = clSetKernelArg(init_rng, 0, sizeof(cl_ulong), (void*) &rng_seed);
		gef_if_error_create_goto(err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_LIBRARY_ERROR, error_handler, "Set init kernel arg 0: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
		ocl_status = clSetKernelArg(init_rng, 1, sizeof(cl_mem), (void*) &seeds_dev);
		gef_if_error_create_goto(err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_LIBRARY_ERROR, error_handler, "Set init kernel arg 1: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
		
		/* Run init kernel. */
		ocl_status = clEnqueueNDRangeKernel(
			zone->queues[0], 
			init_rng, 
			1, 
			NULL, 
			&seeds_count, 
			&lws, 
			0, 
			NULL, 
			NULL
		);
		gef_if_error_create_goto(err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_LIBRARY_ERROR, error_handler, "Kernel init, OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
		

	} else {
		/* *** Host initialization of seeds with Mersenne Twister. *** */
		
		/* Initialize generator. */
		rng_host = g_rand_new_with_seed(rng_seed);
		
		/* Allocate host memory for seeds. */
		seeds_host = (cl_ulong*) malloc(seeds_count * sizeof(cl_ulong));
		
		/* Generate seeds. */
		for (unsigned int i = 0; i < seeds_count; i++) {
			seeds_host[i] = (cl_ulong) (g_rand_double(rng_host) * CL_ULONG_MAX);
		}
		
		/* Copy seeds to device. */
		ocl_status = clEnqueueWriteBuffer(
			zone->queues[0], 
			seeds_dev, 
			CL_FALSE, 
			0, 
			seeds_count * sizeof(cl_ulong), 
			seeds_host, 
			0, 
			NULL, 
			NULL
		);
		gef_if_error_create_goto(err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_LIBRARY_ERROR, error_handler, "Write seeds to device buffer: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
		
	}
	
	/* Test! */
	for (unsigned int i = 0; i < runs; i++) {
		/* Run kernel. */
		ocl_status = clEnqueueNDRangeKernel(
			zone->queues[0], 
			test_rng, 
			1, 
			NULL, 
			&gws, 
			&lws, 
			0, 
			NULL, 
			NULL
		);
		gef_if_error_create_goto(err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_LIBRARY_ERROR, error_handler, "Kernel exec iter %d: OpenCL error %d (%s).", i, ocl_status, clerror_get(ocl_status));
		/* Read data. */
		ocl_status = clEnqueueReadBuffer(
			zone->queues[0], 
			result_dev, 
			CL_TRUE, 
			0, 
			gws * sizeof(cl_uint), 
			result_host[i], 
			0, 
			NULL, 
			NULL
		);
		gef_if_error_create_goto(err, CLO_ERROR, CL_SUCCESS != ocl_status, status = CLO_LIBRARY_ERROR, error_handler, "Read back results, iteration %d: OpenCL error %d (%s).", i, ocl_status, clerror_get(ocl_status));
	}

	/* Stop timming. */
	g_timer_stop(timer);
	
	/* Print timming. */
	g_print("     Finished, ellapsed time: %lfs\n", g_timer_elapsed(timer, NULL));
	g_print("     Saving to files...\n");

	/* Output results to file. */
	if (!g_strcmp0(output, "file-tsv")) {
		output_filename = g_strconcat(output, "_", rng, "_", gid_seed ? "gid" : "host",".tsv", NULL);
		fp = fopen(output_filename, "w");
		for (unsigned int i = 0; i < runs; i++) {
			for (unsigned int j = 0; j < gws; j++) {
				fprintf(fp, "%u\t", result_host[i][j]);
			}
			fprintf(fp, "\n");
		}
	} else if (!g_strcmp0(output, "file-dh")) {
		output_filename = g_strconcat(output, "_", rng, "_", gid_seed ? "gid" : "host", ".dh.txt", NULL);
		fp = fopen(output_filename, "w");
		fprintf(fp, "type: d\n");
		fprintf(fp, "count: %d\n", (int) (gws * runs));
		fprintf(fp, "numbit: %d\n", bits);
		for (unsigned int i = 0; i < runs; i++) {
			for (unsigned int j = 0; j < gws; j++) {
				fprintf(fp, "%u\n", result_host[i][j]);
			}
		}
	}
	fclose(fp);

	g_print("     Done...\n");

	/* If we get here, everything went Ok. */
	status = CLO_SUCCESS;
	g_assert(err == NULL);
	goto cleanup;
	
error_handler:
	/* Handle error. */
	g_assert(err != NULL);
	g_assert(status != CLO_SUCCESS);
	fprintf(stderr, "Error: %s\n", err->message);
	g_error_free(err);	

cleanup:

	/* Free command line options. */
	if (context) g_option_context_free(context);
	if (rng) g_free(rng);
	if (output) g_free(output);
	if (compilerOpts) g_free(compilerOpts);
	if (path) g_free(path);
	if (kernelFile) g_free(kernelFile);
	
	/* Free timer. */
	if (timer) g_timer_destroy(timer);
	
	/* Free host-based random number generator. */
	if (rng_host) g_rand_free(rng_host);
	
	/* Free filename strings. */
	if (output_filename) g_free(output_filename);
	
	/* Release OpenCL kernels */
	if (init_rng) clReleaseKernel(init_rng);
	if (test_rng) clReleaseKernel(test_rng);
	
	/* Release OpenCL memory objects */
	if (seeds_dev) clReleaseMemObject(seeds_dev);
	if (result_dev) clReleaseMemObject(result_dev);

	/* Release OpenCL zone (program, command queue, context) */
	if (zone) clu_zone_free(zone);

	/* Free host resources */
	if (result_host) {
		for (unsigned int i = 0; i < runs; i++) {
			if (result_host[i]) free(result_host[i]);
		}
		free(result_host);
	}
	if (seeds_host) free(seeds_host);
	
	/* Bye bye. */
	return status;
	
}


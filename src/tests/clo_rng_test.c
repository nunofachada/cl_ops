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
 * Test RNG implementation.
 */

#include "clo_rng_test.h"
#include "clo_test.h"

#define CLO_RNG_TEST_FILE_PREFIX "out"
#define CLO_RNG_TEST_OUTPUT "file-tsv"
#define CLO_RNG_TEST_GWS 262144
#define CLO_RNG_TEST_LWS 256
#define CLO_RNG_TEST_RUNS 10
#define CLO_RNG_TEST_BITS 32
#define CLO_RNG_TEST_BUFF_SIZE 1073741824 /* One gigabyte.*/
#define CLO_RNG_TEST_DEFAULT "lcg"

/** A description of the program. */
#define CLO_RNG_TEST_DESCRIPTION "Test RNGs"

/* Command line arguments and respective default values. */
static gchar *rng = NULL;
static gchar *output = NULL;
static size_t gws = CLO_RNG_TEST_GWS;
static size_t lws = CLO_RNG_TEST_LWS;
static unsigned int runs = CLO_RNG_TEST_RUNS;
static int dev_idx = -1;
static guint32 rng_seed = CLO_DEFAULT_SEED;
static gchar *gid_hash = NULL;
static unsigned int bits = CLO_RNG_TEST_BITS;
static unsigned int maxint = 0;

/* Valid command line options. */
static GOptionEntry entries[] = {
	{"rng",          'r', 0,
		G_OPTION_ARG_STRING, &rng,
		"Random number generator: " CLO_RNG_IMPLS " (default is " CLO_RNG_TEST_DEFAULT ")",
		"RNG"},
	{"output",       'o', 0,
		G_OPTION_ARG_STRING, &output,
		"Output: file-tsv, file-dh, stdout-bin, stdout-uint (default: " CLO_RNG_TEST_OUTPUT ")",
		"OUTPUT"},
	{"globalsize",   'g', 0,
		G_OPTION_ARG_INT,    &gws,
		"Global work size (default is " G_STRINGIFY(CLO_RNG_TEST_GWS) ")",
		"SIZE"},
	{"localsize",    'l', 0,
		G_OPTION_ARG_INT,    &lws,
		"Local work size (default is " G_STRINGIFY(CLO_RNG_TEST_LWS) ")",
		"SIZE"},
	{"runs",         'n', 0,
		G_OPTION_ARG_INT,    &runs,
		"Random numbers per workitem (default is " G_STRINGIFY(CLO_RNG_TEST_RUNS) ", 0 means continuous generation)",
		"SIZE"},
	{"device",       'd', 0,
		G_OPTION_ARG_INT,    &dev_idx,
		"Device index",
		"INDEX"},
	{"rng-seed",     's', 0,
		G_OPTION_ARG_INT,    &rng_seed,
		"Seed for random number generator (default is " G_STRINGIFY(CLO_DEFAULT_SEED) ")",
		"SEED"},
	{"gid-hash",     'h', 0,
		G_OPTION_ARG_STRING, &gid_hash,
		"Use workitem GID-based seeds instead of MT derived seeds from host. The option value is the hash to apply to seeds (KNUTH, XS1 or source code modifying variable x, e.g. x = x << 2).",
		"HASH"},
	{"bits",         'b', 0,
		G_OPTION_ARG_INT,    &bits,
		"Number of bits in unsigned integers to produce (default " G_STRINGIFY(CLO_RNG_TEST_BITS) ")",
		NULL},
	{"max",          'm', 0,
		G_OPTION_ARG_INT,    &maxint,
		"Maximum integer to produce, overrides --bits option",
		NULL},
	{ NULL, 0, 0, 0, NULL, NULL, NULL }
};

/**
 * Main program.
 *
 * @param argc Number of command line arguments.
 * @param argv Vector of command line arguments.
 * @return ::CLO_SUCCESS if program terminates successfully, or another
 * value of ::clo_error_codes if an error occurs.
 * */
int main(int argc, char **argv) {

	/* Status var aux */
	int status;

	/* Context object for command line argument parsing. */
	GOptionContext *context = NULL;

	/* Test data structures. */
	cl_uint *result_host = NULL;
	FILE *output_pointer = NULL;
	char *output_buffer = NULL;
	gboolean output_raw;
	const char *output_sep_field, *output_sep_line;
	gchar *output_filename = NULL;
	gchar* compiler_opts = NULL;
	gchar* src = NULL;

	/* cf4ocl wrappers. */
	CCLContext* ctx = NULL;
	CCLDevice* dev = NULL;
	CCLProgram* prg = NULL;
	CCLQueue* queue = NULL;
	CCLKernel* test_rng = NULL;
	CCLBuffer* seeds_dev = NULL;
	CCLBuffer* result_dev = NULL;

	/* Error management object. */
	GError *err = NULL;

	/* How long will it take? */
	GTimer* timer = NULL;

	CloRng* rng_ocl;

	CloRngSeedType seed_type;

	/* Parse command line options. */
	context = g_option_context_new (" - " CLO_RNG_TEST_DESCRIPTION);
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_parse(context, &argc, &argv, &err);
	ccl_if_err_goto(err, error_handler);

	if (output == NULL) output = g_strdup(CLO_RNG_TEST_OUTPUT);
	if (rng == NULL) rng = g_strdup(CLO_RNG_TEST_DEFAULT);

	/* Determine seed type. */
	if (gid_hash == NULL) {
		seed_type = CLO_RNG_SEED_HOST_MT;
	} else {
		seed_type = CLO_RNG_SEED_DEV_GID;
	}

	ccl_if_err_create_goto(err, CLO_ERROR,
		g_strcmp0(output, "file-tsv") && g_strcmp0(output, "file-dh") && g_strcmp0(output, "stdout-bin") && g_strcmp0(output, "stdout-uint"),
		CLO_ERROR_ARGS, error_handler,
		"Unknown output '%s'.", output);
	ccl_if_err_create_goto(err, CLO_ERROR,
		(bits > 32) || (bits < 1),
		CLO_ERROR_ARGS, error_handler,
		"Number of bits must be between 1 and 32.");
	ccl_if_err_create_goto(err, CLO_ERROR,
		(runs == 0) && (!g_ascii_strncasecmp("file", output, 4)),
		CLO_ERROR_ARGS, error_handler,
		"Continuous generation can only be performed to stdout.");

	/* Setup OpenCL context and get device. */
	ctx = ccl_context_new_from_menu_full(&dev_idx, &err);
	ccl_if_err_goto(err, error_handler);

	dev = ccl_context_get_device(ctx, 0, &err);
	ccl_if_err_goto(err, error_handler);

	/* Build compiler options. */
	compiler_opts = g_strconcat(
		maxint ? " -D CLO_RNG_TEST_MAXINT" : "",
		NULL);

	/* Create command queue. */
	queue = ccl_queue_new(ctx, dev, 0, &err);
	ccl_if_err_goto(err, error_handler);

	/* Create RNG object. */
	rng_ocl = clo_rng_new(rng, seed_type, NULL, gws, rng_seed, gid_hash,
		ctx, queue, &err);
	ccl_if_err_goto(err, error_handler);

	/* Get RNG seeds device buffer. */
	seeds_dev = clo_rng_get_device_seeds(rng_ocl);

	/* Get RNG kernels source. */
	src = g_strconcat(
		clo_rng_get_source(rng_ocl), CLO_RNG_TEST_SRC, NULL);

	/* Create and build program. */
	prg = ccl_program_new_from_source(ctx, src, &err);
	ccl_if_err_goto(err, error_handler);

	ccl_program_build(prg, compiler_opts, &err);
	ccl_if_err_goto(err, error_handler);

	/* Create host results buffer */
	result_host = g_slice_alloc(sizeof(cl_uint) * gws);

	/* Create device results buffer. */
	result_dev = ccl_buffer_new(ctx, CL_MEM_READ_WRITE,
		gws * sizeof(cl_int), NULL, &err);
	ccl_if_err_goto(err, error_handler);

	/* Setup options depending whether the generated random numbers are
	 * to be output to stdout or to a file. */
	if (g_str_has_prefix(output, "stdout")) {
		/* Generated random numbers are to be output to stdout. */
		output_pointer = stdout;

		/* Information about program execution should not be printed to
		 * stdout. */
		g_set_print_handler(clo_print_to_null);

		/* Exact output type depends on cli option: */
		if (!g_strcmp0(output, "stdout-bin")) {
			output_raw = TRUE;
		} else if (!g_strcmp0(output, "stdout-uint")) {
			output_raw = FALSE;
			output_sep_field = "\n";
			output_sep_line = "";
		} else {
			g_assert_not_reached();
		}


	} else if (g_str_has_prefix(output, "file")) {
		/* Generated random numbers are to be output to file. */
		if (!g_strcmp0(output, "file-dh")) {
			output_filename = g_strconcat(
				CLO_RNG_TEST_FILE_PREFIX, "_", rng, "_",
				gid_hash ? "gid_" : "host_",
				gid_hash ? gid_hash : "mt",
				".dh.txt", NULL);
			output_sep_field = "\n";
			output_sep_line = "";
		} else if (!g_strcmp0(output, "file-tsv")) {
			output_filename = g_strconcat(
				CLO_RNG_TEST_FILE_PREFIX, "_", rng, "_",
				gid_hash ? "gid_" : "host_",
				gid_hash ? gid_hash : "mt",
				".tsv", NULL);
			output_sep_field = "\t";
			output_sep_line = "\n";
		} else {
			g_assert_not_reached();
		}

		/* Open file. */
		output_pointer = fopen(output_filename, "w");
		ccl_if_err_create_goto(err, CLO_ERROR, output_pointer == NULL,
			CLO_ERROR_OPENFILE, error_handler,
			"Unable to create output file '%s'.", output_filename);

		/* Create large file buffer to avoid trashing disk. */
		output_buffer = g_slice_alloc(CLO_RNG_TEST_BUFF_SIZE * sizeof(char));

		/* Set file buffer. */
		status = setvbuf(
			output_pointer, output_buffer, _IOFBF, CLO_RNG_TEST_BUFF_SIZE);
		ccl_if_err_create_goto(err, CLO_ERROR, status != 0,
			CLO_ERROR_STREAM_WRITE, error_handler,
			"Unable to set output file buffer.");

		/* Output type is uint, not raw. */
		output_raw = FALSE;

		/* Write initial data if file is to written in dieharder format. */
		if (!g_strcmp0(output, "file-dh")) {
			fprintf(output_pointer, "type: d\n");
			fprintf(output_pointer, "count: %d\n", (int) (gws * runs));
			fprintf(output_pointer, "numbit: %d\n", bits);
		}

	} else  {
		g_assert_not_reached();
	}

	/* Print options. */
	g_print("\n   =========================== Selected options ============================\n\n");
	g_print("     Random number generator (seed): %s (%u)\n", rng, rng_seed);
	g_print("     Seeds in workitems: %s %s\n", gid_hash ? "GID-based, hash:" : "Host-based,", gid_hash ? gid_hash : "Mersenne Twister");
	g_print("     Global/local worksizes: %d/%d\n", (int) gws, (int) lws);
	g_print("     Number of runs: %d\n", runs);
	g_print("     Number of bits / Maximum integer: %d / %u\n", bits, (unsigned int) ((1ul << bits) - 1));
	g_print("     Compiler Options: %s\n", compiler_opts);

	/* Inform about execution. */
	g_print("\n   =========================== Execution status ============================\n\n");
	g_print("     Producing random numbers...\n");

	/* Start timming. */
	timer = g_timer_new();

	/* Get test kernel. */
	test_rng = ccl_program_get_kernel(prg, "clo_rng_test", &err);
	ccl_if_err_goto(err, error_handler);

	/*  Set test kernel arguments. */
	cl_uint value = maxint ? maxint : bits;
	ccl_kernel_set_args(test_rng, seeds_dev, result_dev,
		ccl_arg_priv(value, cl_uint), NULL);

	/* Test! */
	for (guint i = 0; (i != runs) || (runs == 0); i++) {

		/* Run kernel. */
		ccl_kernel_enqueue_ndrange(
			test_rng, queue, 1, NULL, &gws, &lws, NULL, &err);
		ccl_if_err_goto(err, error_handler);

		/* Read data. */
		ccl_buffer_enqueue_read(result_dev, queue, CL_TRUE, 0,
			gws * sizeof(cl_uint), result_host, NULL, &err);
		ccl_if_err_goto(err, error_handler);

		/* Write data to output. */
		if (output_raw) {
			fwrite(result_host, sizeof(cl_uint), gws, output_pointer);
		} else {
			for (unsigned int i = 0; i < gws; i++) {
				fprintf(output_pointer, "%u%s", result_host[i],
					output_sep_field);
			}
			fprintf(output_pointer, "%s", output_sep_line);
		}
	}

	/* Stop timming. */
	g_timer_stop(timer);

	/* Print timming. */
	g_print("     Finished, ellapsed time: %lfs\n", g_timer_elapsed(timer, NULL));
	g_print("     Done...\n");

	/* If we get here, everything went Ok. */
	status = CLO_SUCCESS;
	g_assert(err == NULL);
	goto cleanup;

error_handler:

	/* Handle error. */
	g_assert(err != NULL);
	status = err->code;
	fprintf(stderr, "Error: %s\n", err->message);
	g_error_free(err);

cleanup:

	/* Free CLI options context. */
	if (context) g_option_context_free(context);

	/* Free host RNG object. */
	if (rng) g_free(rng);

	/* Free output data. */
	if (output) g_free(output);

	/* Free compiler options. */
	if (compiler_opts) g_free(compiler_opts);

	/* Free the gid hash. */
	if (gid_hash) g_free(gid_hash);

	/* Free the cl_ops device RNG object. */
	if (rng_ocl) clo_rng_destroy(rng_ocl);

	/* Free the source code string. */
	if (src) g_free(src);

	/* Free timer. */
	if (timer) g_timer_destroy(timer);

	/* Close file. */
	if (output_pointer) fclose(output_pointer);

	/* Free file output buffer. */
	if (output_buffer) g_slice_free1(CLO_RNG_TEST_BUFF_SIZE * sizeof(char), output_buffer);

	/* Free filename strings. */
	if (output_filename) g_free(output_filename);

	/* Destroy cf4ocl wrappers. */
	if (seeds_dev) ccl_buffer_destroy(seeds_dev);
	if (result_dev) ccl_buffer_destroy(result_dev);
	if (queue) ccl_queue_destroy(queue);
	if (prg) ccl_program_destroy(prg);
	if (ctx) ccl_context_destroy(ctx);

	/* Free host resources */
	if (result_host) g_slice_free1(sizeof(cl_uint) * gws, result_host);

	/* Bye bye. */
	return status;

}


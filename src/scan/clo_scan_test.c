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
 * @brief Test scan implementation.
 */

#include "clo_scan_test.h"

#define CLO_SCAN_LWS 256
#define CLO_SCAN_BITS 32
#define CLO_SCAN_BITS_SUM 32
#define CLO_SCAN_RUNS 1
#define CLO_SCAN_INITELEMS 4
#define CLO_SCAN_NUMDOUB 24

/** A description of the program. */
#define CLO_SCAN_DESCRIPTION "Test CL-Ops scan implementation"

/** The kernels source file. */
#define CLO_SCAN_KERNEL_SRC "clo_scan.cl"

/* Command line arguments and respective default values. */
static guint32 runs = CLO_SCAN_RUNS;
static size_t lws = CLO_SCAN_LWS;
static int dev_idx = -1;
static guint32 rng_seed = CLO_DEFAULT_SEED;
static unsigned int bits = CLO_SCAN_BITS;
static unsigned int bits_sum = CLO_SCAN_BITS_SUM;
static gchar* path = NULL;
static guint32 init_elems = CLO_SCAN_INITELEMS;
static guint32 num_doub = CLO_SCAN_NUMDOUB;
static gchar* out = NULL;
static gboolean no_check = FALSE;

/* Valid command line options. */
static GOptionEntry entries[] = {
	{"runs",         'r', 0, G_OPTION_ARG_INT,    &runs,
		"Number of runs (default is " STR(CLO_SCAN_RUNS) ")",                               "RUNS"},
	{"localsize",    'l', 0, G_OPTION_ARG_INT,    &lws,
		"Maximum local work size (default is " STR(CLO_SCAN_LWS) ")",                       "SIZE"},
	{"device",       'd', 0, G_OPTION_ARG_INT,    &dev_idx,
		"Device index",                                                                     "INDEX"},
	{"rng-seed",     's', 0, G_OPTION_ARG_INT,    &rng_seed,
		"Seed for random number generator (default is " STR(CLO_DEFAULT_SEED) ")",          "SEED"},
	{"bits",         'b', 0, G_OPTION_ARG_INT,    &bits,
		"Number of bits in unsigned integers to scan (default " STR(CLO_SCAN_BITS) ")",     "BITS"},
	{"bits-sum",     'e', 0, G_OPTION_ARG_INT,    &bits_sum,
		"Number of bits for elements of scan result (default " STR(CLO_SCAN_BITS_SUM) ")",  "BITS"},
	{"path",         'p', 0, G_OPTION_ARG_STRING, &path,
		"Path of OpenCL source files (default is " CLO_DEFAULT_PATH,                        "PATH"},
	{"init-elems",   'i', 0, G_OPTION_ARG_INT,    &init_elems,
		"The starting number of elements to scan (default is " STR(CLO_SCAN_INITELEMS) ")", "INIT"},
	{"num-doub",     'n', 0, G_OPTION_ARG_INT,    &num_doub,
		"Number of times min-elems is doubled (default is " STR(CLO_SCAN_NUMDOUB) ")",      "DOUB"},
	{"no-check",     'u', 0, G_OPTION_ARG_NONE,   &no_check,
		"Don't check scan with serial version",                                             NULL},
	{"out",          'o', 0, G_OPTION_ARG_STRING, &out,
		"File where to output scan benchmarks (default is no file output)",                 "FILENAME"},
	{ NULL, 0, 0, 0, NULL, NULL, NULL }
};


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
	int status;

	/* Context object for command line argument parsing. */
	GOptionContext *context = NULL;

	/* Kernel file. */
	gchar* kernelFile = NULL;

	/* Test data structures. */
	gchar* host_data = NULL;
	gchar* host_data_scanned = NULL;
	gchar* compilerOpts = NULL;
	size_t bytes, bytes_sum;
	size_t max_buffer_size, max_buffer_size_sum;
	gdouble total_time;
	FILE *outfile = NULL;

	/* cf4ocl wrappers. */
	CCLProgram* prg = NULL;
	CCLQueue* queue = NULL;
	CCLContext* ctx = NULL;
	CCLDevice* dev = NULL;
	CCLKernel** krnls = NULL;
	CCLBuffer* dev_data = NULL;
	CCLBuffer* dev_data_scanned = NULL;
	CCLBuffer* dev_wgsums = NULL;

	/* Algorithm options. */
	gchar *options = NULL;

	/* Host-based random number generator (mersenne twister) */
	GRand* rng_host = NULL;

	/* Error management object. */
	GError *err = NULL;

	/* How long will it take? */
	GTimer* timer = NULL;

	/* Scan benchmarks. */
	gdouble** benchmarks = NULL;

	/* Parse command line options. */
	context = g_option_context_new (" - " CLO_SCAN_DESCRIPTION);
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_parse(context, &argc, &argv, &err);
	ccl_if_err_goto(err, error_handler);
	if (path == NULL) {
		kernelFile = clo_kernelpath_get(CLO_DEFAULT_PATH G_DIR_SEPARATOR_S CLO_SCAN_KERNEL_SRC, argv[0]);
		path = g_path_get_dirname(kernelFile);
	} else {
		kernelFile = g_build_filename(path, CLO_SCAN_KERNEL_SRC, NULL);
	}
	ccl_if_err_create_goto(err, CLO_ERROR,
		(clo_ones32(bits) != 1) || (bits > 64) || (bits < 8),
		CLO_ERROR_ARGS, error_handler,
		"Number of bits must be 8, 16, 32 or 64.");

	/* Determine size in bytes of each element to sort. */
	bytes = bits / 8;
	bytes_sum = bits_sum / 8;

	/* Initialize random number generator. */
	rng_host = g_rand_new_with_seed(rng_seed);

	/* Get the context wrapper. */
	ctx = ccl_context_new_from_menu_full(&dev_idx, &err);
	ccl_if_err_goto(err, error_handler);

	/* Get compiler options. */
	compilerOpts = g_strconcat(
		"-I ", path,
		" -D ", "CLO_SCAN_ELEM_TYPE=", bits == 8 ? "uchar" : (bits == 16 ? "ushort" : (bits == 32 ? "uint" : "ulong")),
		" -D ", "CLO_SCAN_SUM_TYPE=", bits_sum == 8 ? "uchar" : (bits_sum == 16 ? "ushort" : (bits_sum == 32 ? "uint" : "ulong")),
		NULL);

	/* Create and build program. */
	prg = ccl_program_new_from_source_file(ctx, kernelFile, &err);
	ccl_if_err_goto(err, error_handler);

	ccl_program_build(prg, compilerOpts, &err);
	ccl_if_err_goto(err, error_handler);

	/* Create scan kernel(s). */
	krnls = clo_scan_kernels_create(prg, &err);
	ccl_if_err_goto(err, error_handler);

	/* Create command queue. */
	queue = ccl_queue_new(ctx, dev, 0, &err);
	ccl_if_err_goto(err, error_handler);

	/* Print options. */
	printf("\n   =========================== Selected options ============================\n\n");
	printf("     Random number generator seed: %u\n", rng_seed);
	printf("     Maximum local worksize: %d\n", (int) lws);
	printf("     Size in bits (bytes) of elements to scan: %d (%d)\n", bits, (int) bytes);
	printf("     Size in bits (bytes) of elements of scan result: %d (%d)\n", bits_sum, (int) bytes_sum);
	printf("     Starting number of elements: %d\n", init_elems);
	printf("     Number of times number of elements will be doubled: %d\n", num_doub);
	printf("     Number of runs: %d\n", runs);
	printf("     Compiler Options: %s\n", compilerOpts);

	/* Create timer. */
	timer = g_timer_new();

	/* Create benchmarks table. */
	benchmarks = g_new(gdouble*, num_doub);
	for (unsigned int i = 0; i < num_doub; i++)
		benchmarks[i] = g_new0(gdouble, runs);

	/* Determine maximum buffer size. */
	max_buffer_size = bytes * init_elems * (1 << num_doub);
	max_buffer_size_sum = bytes_sum * init_elems * (1 << num_doub);

	/* Create host buffers */
	host_data = g_new0(gchar, max_buffer_size);
	host_data_scanned = g_new0(gchar, max_buffer_size_sum);

	/* Create device buffers. */
	dev_data = ccl_buffer_new(
		ctx, CL_MEM_READ_ONLY, max_buffer_size, NULL, &err);
	ccl_if_err_goto(err, error_handler);

	dev_data_scanned = ccl_buffer_new(
		ctx, CL_MEM_READ_WRITE, max_buffer_size_sum, NULL, &err);
	ccl_if_err_goto(err, error_handler);

	/* Set kernel parameters. */
	clo_scan_kernelargs_set(krnls, dev_data, dev_data_scanned, lws,
		bytes, bytes_sum, &err);
	ccl_if_err_goto(err, error_handler);

	/* Start with the inital number of elements. */
	unsigned int num_elems = init_elems;

	/* Perform test. */
	for (unsigned int N = 1; N <= num_doub; N++) {

		const gchar* scan_ok;

		for (unsigned int r = 0; r < runs; r++) {

			g_debug("|===== Num. elems: %d (run %d): =====|", num_elems, r);

			/* Initialize host buffer. */
			for (unsigned int i = 0;  i < num_elems; i++) {
				/* Get a random 64-bit value by default, but keep it small... */
				gulong value = (gulong) (g_rand_double(rng_host) * 256);
				/* But just use the specified bits. */
				memcpy(host_data + bytes * i, &value, bytes);
				//gulong in_mem = CLO_SCAN_HOST_GET(host_data, i, bytes);
				//g_debug("Value: %lx\tIn memory: %lx [%lu]", value, in_mem, in_mem);
			}

			/* Copy data to device. */
			ccl_buffer_enqueue_write(dev_data, queue, CL_TRUE, 0,
				num_elems * bytes, host_data, NULL, &err);
			ccl_if_err_goto(err, error_handler);

			/* Start timming. */
			g_timer_start(timer);

			/* Perform scan. */
			clo_scan(queue, krnls, lws, bytes, bytes_sum, num_elems,
				options, &err);
			ccl_if_err_goto(err, error_handler);

			/* Wait for the kernel to terminate... */
			ccl_queue_finish(queue, &err);
			ccl_if_err_goto(err, error_handler);

			/* Stop timming and save time to benchmarks. */
			g_timer_stop(timer);
			benchmarks[N - 1][r] = g_timer_elapsed(timer, NULL);

			/* Copy scanned data to host. */
			ccl_buffer_enqueue_read(dev_data_scanned, queue, CL_TRUE, 0,
				num_elems * bytes_sum, host_data_scanned, NULL, &err);
			ccl_if_err_goto(err, error_handler);

			/* Check if scan was well performed. */
			if (no_check) {
				scan_ok = "[Unverified]";
			} else {
				g_debug("== CHECK ==");
				g_debug("%10s %10s %10s", "Host", "Serial", "Dev");
				scan_ok = "";
				gulong value_dev = 0, value_host = 0;
				for (unsigned int i = 0; i < num_elems; i++) {
					/* Perform a CPU scan. */
					value_host = (i == 0) ? 0 : value_host + CLO_SCAN_HOST_GET(host_data, i - 1, bytes);
					/* Check for overflow. */
					if (value_host > CLO_SCAN_MAXU(bytes_sum)) {
						scan_ok = "[Overflow]";
						break;
					}
					/* Get device value. */
					memcpy(&value_dev, host_data_scanned + bytes_sum * i, bytes_sum);
					/* Compare. */
					if (value_dev != value_host) {
						scan_ok = "[Scan did not work]";
						break;
					}
					g_debug("%10lu %10lu %10lu", CLO_SCAN_HOST_GET(host_data, i, bytes), value_host, value_dev);

				}
			}


		}

		/* Print info. */
		total_time = 0;
		for (unsigned int i = 0;  i < runs; i++)
			total_time += benchmarks[N - 1][i];
		printf("       - %10d : %f MValues/s %s\n", num_elems, 1e-6 * num_elems * runs / total_time, scan_ok);

		num_elems *= 2;

	}

	/* Save benchmarks to file, if filename was given as cli option. */
	if (out) {
		outfile = fopen(out, "w");
		for (unsigned int i = 0; i < num_doub; i++) {
			fprintf(outfile, "%d", i);
			for (unsigned int j = 0; j < runs; j++) {
				fprintf(outfile, "\t%lf", benchmarks[i][j]);
			}
			fprintf(outfile, "\n");
		}
		fclose(outfile);
	}

	/* If we get here, everything went Ok. */
	g_assert(err == NULL);
	status = CLO_SUCCESS;
	goto cleanup;

error_handler:
	/* Handle error. */
	g_assert(err != NULL);
	fprintf(stderr, "Error: %s\n", err->message);
	status = err->code;
	g_error_free(err);

cleanup:

	/* Release OpenCL wrappers. */
	if (dev_data) ccl_buffer_destroy(dev_data);
	if (dev_data_scanned) ccl_buffer_destroy(dev_data_scanned);
	if (dev_wgsums) ccl_buffer_destroy(dev_wgsums);
	if (queue) ccl_queue_destroy(queue);
	if (krnls) clo_scan_kernels_free(krnls);
	if (prg) ccl_program_destroy(prg);
	if (ctx) ccl_context_destroy(ctx);

	/* Free host resources */
	g_free(host_data);
	g_free(host_data_scanned);

	/* Free command line options. */
	if (context) g_option_context_free(context);
	if (path) g_free(path);
	if (kernelFile) g_free(kernelFile);
	if (compilerOpts) g_free(compilerOpts);
	if (out) g_free(out);

	/* Free benchmarks. */
	if (benchmarks) {
		for (unsigned int i = 0; i < num_doub; i++)
			if (benchmarks[i]) g_free(benchmarks[i]);
		g_free(benchmarks);
	}

	/* Free timer. */
	if (timer) g_timer_destroy(timer);

	/* Free host-based random number generator. */
	if (rng_host) g_rand_free(rng_host);

	/* Bye bye. */
	return status;

}


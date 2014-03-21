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
static gchar* path = NULL;
static guint32 init_elems = CLO_SCAN_INITELEMS;
static guint32 num_doub = CLO_SCAN_NUMDOUB;
static gchar* out = NULL;

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
		"Number of bits in unsigned integers to scan (default " STR(CLO_SCAN_BITS) ")",     NULL},
	{"path",         'p', 0, G_OPTION_ARG_STRING, &path,
		"Path of OpenCL source files (default is " CLO_DEFAULT_PATH,                        "PATH"}, 
	{"init-elems",   'i', 0, G_OPTION_ARG_INT,    &init_elems,
		"The starting number of elements to scan (default is " STR(CLO_SCAN_INITELEMS) ")", "INIT"},
	{"num-doub",     'n', 0, G_OPTION_ARG_INT,    &num_doub,
		"Number of times min-elems is doubled (default is " STR(CLO_SCAN_NUMDOUB) ")",      "DOUB"},
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
	int status, ocl_status;
	
	/* Context object for command line argument parsing. */
	GOptionContext *context = NULL;
	
	/* Kernel file. */
	gchar* kernelFile = NULL;
	
	/* Test data structures. */
	gchar* host_data = NULL;
	gchar* host_data_scanned = NULL;
	cl_mem dev_data = NULL;
	cl_mem dev_data_scanned = NULL;
	gchar* compilerOpts = NULL;
	cl_kernel *krnls = NULL;
	size_t bytes;
	gdouble total_time;
	FILE *outfile = NULL;
	
	/* Algorithm options. */
	gchar *options = NULL;

	/* Host-based random number generator (mersenne twister) */
	GRand* rng_host = NULL;	

	/* OpenCL zone: platform, device, context, queues, etc. */
	CLUZone* zone = NULL;
	
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
	gef_if_error_goto(err, CLO_ERROR_LIBRARY, status, error_handler);
	if (path == NULL) {
		kernelFile = clo_kernelpath_get(CLO_DEFAULT_PATH G_DIR_SEPARATOR_S CLO_SCAN_KERNEL_SRC, argv[0]);
		path = g_path_get_dirname(kernelFile);
	} else {
		kernelFile = g_build_filename(path, CLO_SCAN_KERNEL_SRC, NULL);
	}	
	gef_if_error_create_goto(err, CLO_ERROR, 
		(clo_ones32(bits) != 1) || (bits > 64) || (bits < 8), 
		status = CLO_ERROR_ARGS, error_handler, 
		"Number of bits must be 8, 16, 32 or 64.");

	/* Determine size in bytes of each element to sort. */
	bytes = bits / 8;
	
	/* Initialize random number generator. */
	rng_host = g_rand_new_with_seed(rng_seed);
	
	/* Get the required CL zone. */
	zone = clu_zone_new(CL_DEVICE_TYPE_ALL, 1, 0, clu_menu_device_selector, (dev_idx != -1 ? &dev_idx : NULL), &err);
	gef_if_error_goto(err, CLO_ERROR_LIBRARY, status, error_handler);
	
	/* Build compiler options. */
	compilerOpts = g_strconcat(
		"-I ", path,
		" -D ", "CLO_SCAN_ELEM_TYPE=", bits == 8 ? "uchar" : (bits == 16 ? "ushort" : (bits == 32 ? "uint" : "ulong")),
		NULL);
	
	/* Build program. */
	clu_program_create(zone, &kernelFile, 1, compilerOpts, &err);
	gef_if_error_goto(err, CLO_ERROR_LIBRARY, status, error_handler);
	
	/* Create sort kernel(s). */
	status = clo_scan_kernels_create(&krnls, zone->program, &err);
	gef_if_error_goto(err, GEF_USE_GERROR, status, error_handler);

	/* Print options. */
	printf("\n   =========================== Selected options ============================\n\n");
	printf("     Random number generator seed: %u\n", rng_seed);
	printf("     Maximum local worksize: %d\n", (int) lws);
	printf("     Size in bits (bytes) of elements to scan: %d (%d)\n", bits, (int) bytes);
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

	/* Create host buffers */
	host_data = g_new0(gchar, bytes * init_elems * num_doub);
	host_data_scanned = g_new0(gchar, bytes * init_elems * num_doub);

	/* Perform test. */
	for (unsigned int N = 1; N <= num_doub; N++) {
		
		unsigned int num_elems = init_elems;
		gboolean scan_ok;
		
		for (unsigned int r = 0; r < runs; r++) {
			
			/* Initialize host buffer. */
			for (unsigned int i = 0;  i < num_elems; i++) {
				/* Get a random 64-bit value by default... */
				gulong value = (gulong) (g_rand_double(rng_host) * G_MAXULONG);
				/* But just use the specified bits. */
				memcpy(host_data + bytes*i, &value, bytes);
			}
			
			/* Create device buffera. */
			dev_data = clCreateBuffer(zone->context, CL_MEM_READ_ONLY, num_elems * bytes, NULL, &ocl_status);
			gef_if_error_create_goto(err, CLO_ERROR, CL_SUCCESS != ocl_status, 
				status = CLO_ERROR_LIBRARY, error_handler, 
				"Error creating device buffer for original data: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
			
			dev_data_scanned = clCreateBuffer(zone->context, CL_MEM_READ_WRITE, num_elems * bytes, NULL, &ocl_status);
			gef_if_error_create_goto(err, CLO_ERROR, CL_SUCCESS != ocl_status, 
				status = CLO_ERROR_LIBRARY, error_handler, 
				"Error creating device buffer for scanned data: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));

			/* Copy data to device. */
			ocl_status = clEnqueueWriteBuffer(
				zone->queues[0], 
				dev_data, 
				CL_FALSE, 
				0, 
				num_elems * bytes, 
				host_data, 
				0, 
				NULL, 
				NULL
			);
			gef_if_error_create_goto(err, CLO_ERROR, CL_SUCCESS != ocl_status, 
				status = CLO_ERROR_LIBRARY, error_handler, 
				"Error writing data to device: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
			
			/* Set kernel parameters. */
			status = clo_scan_kernelargs_set(&krnls, dev_data, dev_data_scanned, lws, bytes, &err);
			gef_if_error_goto(err, GEF_USE_GERROR, status, error_handler);
			
			/* Start timming. */
			g_timer_start(timer);
			
			/* Perform scan. */
			clo_scan(
				zone->queues[0], 
				krnls, 
				lws,
				bytes,
				num_elems,
				options,
				NULL, 
				FALSE,
				&err
			);
			gef_if_error_goto(err, CLO_ERROR_LIBRARY, status, error_handler);

			/* Wait for the kernel to terminate... */
			ocl_status = clFinish(zone->queues[0]);
			gef_if_error_create_goto(err, CLO_ERROR, CL_SUCCESS != ocl_status, 
				status = CLO_ERROR_LIBRARY, error_handler, 
				"Waiting for kernel to terminate, OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
			
			/* Stop timming and save time to benchmarks. */
			g_timer_stop(timer);
			benchmarks[N - 1][r] = g_timer_elapsed(timer, NULL);
			
			/* Copy scanned data to host. */
			ocl_status = clEnqueueReadBuffer(
				zone->queues[0], 
				dev_data_scanned,
				CL_TRUE, 
				0, 
				num_elems * bytes, 
				host_data_scanned, 
				0, 
				NULL, 
				NULL
			);
			gef_if_error_create_goto(err, CLO_ERROR, CL_SUCCESS != ocl_status, 
				status = CLO_ERROR_LIBRARY, error_handler, 
				"Error reading data from device: OpenCL error %d (%s).", ocl_status, clerror_get(ocl_status));
			
			/* Release device buffer. */
			clReleaseMemObject(dev_data);
			clReleaseMemObject(dev_data_scanned);
			
			/* Check if scan was well performed. */
			scan_ok = TRUE;
			gulong value_dev = 0, value_host = 0;
			for (unsigned int i = 0; i < num_elems; i++) {
				/* Perform a CPU scan. */
				value_host = (i == 0) ? 0 : value_host + host_data[i - 1];
				/* Get device value. */
				memcpy(&value_dev, host_data_scanned + bytes*i, bytes);
				/* Compare. */
				if (value_dev != value_host) {
					scan_ok = FALSE;
					break;
				}
				
			}
			
			num_elems *= 2;
		
		}
			
		/* Print info. */
		total_time = 0;
		for (unsigned int i = 0;  i < runs; i++)
			total_time += benchmarks[N - 1][i];
		printf("       - 2^%d: %f MValues/s %s\n", N, 1e-6 * num_elems * runs / total_time, scan_ok ? "" : "(scan did not work)");
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
	
	/* Release OpenCL kernels */
	if (krnls) clo_scan_kernels_free(&krnls);
	
	/* Release OpenCL zone (program, command queue, context) */
	if (zone) clu_zone_free(zone);

	/* Free host resources */
	g_free(host_data);
	g_free(host_data_scanned);
	
	/* Bye bye. */
	return status;

}


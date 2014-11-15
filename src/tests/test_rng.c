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
 * Test the RNG class.
 *
 * @author Nuno Fachada
 * @date 2014
 * @copyright [GNU General Public License version 3 (GPLv3)](http://www.gnu.org/licenses/gpl.html)
 * */

#include <cl_ops.h>

/**
 * Tests
 * */
static void seed_dev_gid_test() {
	return;
}

/**
 * Tests
 * */
static void seed_host_mt_test() {
	return;
}

/**
 * Tests
 * */
static void seed_ext_dev_test() {
	return;
}

/**
 * Tests
 * */
static void seed_ext_host_test() {
	return;
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




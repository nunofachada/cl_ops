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

struct clo_rng {

	char* source;
	void* seeds_host;
	CCLBuffer* seeds_device;
	size_t size;

};

CloRng* clo_rng_new(const char* type, CloRngSeedType seed_type,
	void* seeds, const char* hash, size_t size, GError** err) {

	CloRng* rng;

	rng = g_slice_new0(CloRng);

}

void clo_rng_destroy(CloRng* rng) {

	g_slice_free(CloRng, rng);

}

const char* clo_rng_get_source(CloRng* rng) {

	return (const char*) rng->source;

}

/// Only for non-ext seed type
CCLBuffer* clo_rng_get_device_seeds(CloRng* rng) {

}


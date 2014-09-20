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

#include "common/clo_common.h"

typedef struct clo_rng CloRng;

CloRng* clo_rng_new(const char* type, clo_rng_seed_type seed_type,
	void* seeds, size_t size, GError** err);

void clo_rng_destroy(CloRng* rng);

const char* clo_rng_get_source(CloRng* rng);

CCLBuffer* clo_rng_get_seeds(CloRng* rng);

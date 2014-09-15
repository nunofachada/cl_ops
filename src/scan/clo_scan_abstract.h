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

typedef struct clo_scan CloScan;

typedef CloScan* (*clo_scan_new)(CCLContext* ctx, size_t size_elem,
	size_t size_sum, char* compiler_opts);

typedef cl_bool (*clo_scan_destroy)(CloScan* scan);

typedef cl_bool (*clo_scan_scan)();

typedef struct clo_scan {

	clo_scan_new new;
	clo_scan_destroy destroy;
	clo_scan_scan scan;

} CloScan;


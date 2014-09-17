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

CloScan* clo_scan_new_blelloch(const char* options, CCLContext* ctx,
	size_t elem_size, size_t sum_size, const char* compiler_opts) {

	CloScan* scan = g_new0(CloScan, 1);
	scan->destroy = clo_scan_blelloch_destroy;
	scan->scan = clo_scan_blelloch_scan;
}

/*
 * This file is part of CL_Ops.
 *
 * CL_Ops is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * CL_Ops is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with CL_Ops. If not, see
 * <http://www.gnu.org/licenses/>.
 * */

/**
 * @file
 * Utility macros for OpenCL workitems.
 */

#define GLOBAL_SIZE() get_global_size(0)

#define GID1() get_global_id(0)

#define GID2() ((uint2) (GID1(), GLOBAL_SIZE() + GID1()))

#define GID4() ((uint4) (GID1(), GLOBAL_SIZE() + GID1(), GLOBAL_SIZE() * 2 + GID1(), GLOBAL_SIZE() * 3 + GID1()))

#define GID8() ((uint8) (GID1(), GLOBAL_SIZE() + GID1(), GLOBAL_SIZE() * 2 + GID1(), GLOBAL_SIZE() * 3 + GID1(), GLOBAL_SIZE() * 4 + GID1(), GLOBAL_SIZE() * 5 + GID1(), GLOBAL_SIZE() * 6 + GID1(), GLOBAL_SIZE() * 7 + GID1()))

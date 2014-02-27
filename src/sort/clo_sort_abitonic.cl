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
 * @brief Advanced bitonic sort implementation.
 */
 
#define CLO_SORT_ABITONIC_CMPXCH(data, index1, index2) \
	data1 = data[index1]; \
	data2 = data[index2]; \
	if (CLO_SORT_COMPARE(data1, data2) ^ desc) { \
		data[index1] = data2; \
		data[index2] = data1; \
	} 
 
#define CLO_SORT_ABITONIC_STEP(data, stride) \
  	/* Determine what to compare and possibly swap. */ \
	index1 = (lid / stride) * stride * 2 + (lid % stride); \
	index2 = index1 + stride; \
	/* Compare and swap */ \
	CLO_SORT_ABITONIC_CMPXCH(data, index1, index2); \
	/* Local memory barrier */ \
	barrier(CLK_LOCAL_MEM_FENCE);
	
#define CLO_SORT_ABITONIC_INIT() \
	/* Global and local ids for this work-item. */ \
	uint gid = get_global_id(0); \
	uint lid = get_local_id(0); \
	uint local_size = get_local_size(0); \
	uint group_id = get_group_id(0); \
	/* Local and global indexes for moving data between local and \
	 * global memory. */ \
	uint local_index1 = lid; \
	uint local_index2 = local_size + lid; \
	uint global_index1 = group_id * local_size * 2 + lid; \
	uint global_index2 = local_size * (group_id * 2 + 1) + lid; \
	/* Load data locally */ \
	data_local[local_index1] = data_global[global_index1]; \
	data_local[local_index2] = data_global[global_index2]; \
	/* Local memory barrier */ \
	barrier(CLK_LOCAL_MEM_FENCE); \
	/* Determine if ascending or descending */ \
	bool desc = (bool) (0x1 & (gid >> (stage - 1))); \
	/* Index of values to possibly swap. */ \
	uint index1, index2; \
	/* Data elements to possibly swap. */ \
	CLO_SORT_ELEM_TYPE data1, data2;
	
#define CLO_SORT_ABITONIC_FINISH() \
	/* Store data globally */ \
	data_global[global_index1] = data_local[local_index1]; \
	data_global[global_index2] = data_local[local_index2];
	
#define CLO_SORT_ABITONIC_INIT_S(n) \
	__private CLO_SORT_ELEM_TYPE data_priv[n]; \
	CLO_SORT_ELEM_TYPE data1, data2; \
	/* Thread information. */ \
	uint gid = get_global_id(0); \
	uint lid = get_local_id(0); \
	/* Determine block size. */ \
	uint blockSize = 1 << step; \
	/* Determine if ascending or descending. Ascending if block id is \
	 * pair, descending otherwise. */ \
	bool desc = (bool) (0x1 & ((gid * n) / (1 << stage))); \
	/* Base global address to load/store values from/to. */ \
	uint gaddr = ((gid * n) / blockSize) * blockSize + (gid % (blockSize / n)); \
	/* Thread increment within block. */ \
	uint inc = blockSize / n;

/**
 * @brief This kernel can perform the two last steps of a stage in a
 * bitonic sort.
 * 
 * @param data_global Array of data to sort.
 * @param stage
 * @param data_local
 */
__kernel void abitonic_2(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local)
{
	
	/* *********** INIT ************** */
	CLO_SORT_ABITONIC_INIT();
	/* ********** STEP 2 ************** */
	CLO_SORT_ABITONIC_STEP(data_local, 2);
	/* ********** STEP 1 ************** */
	CLO_SORT_ABITONIC_STEP(data_local, 1);
	/* ********* FINISH *********** */
	CLO_SORT_ABITONIC_FINISH();
}

/**
 * @brief This kernel can perform the three last steps of a stage in a
 * bitonic sort.
 * 
 * @param data_global Array of data to sort.
 * @param stage
 * @param data_local
 */
__kernel void abitonic_3(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local)
{
	
	/* *********** INIT ************** */
	CLO_SORT_ABITONIC_INIT();
	/* *********** STEP 3 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 4);
	/* ********** STEP 2 ************** */
	CLO_SORT_ABITONIC_STEP(data_local, 2);
	/* ********** STEP 1 ************** */
	CLO_SORT_ABITONIC_STEP(data_local, 1);
	/* ********* FINISH *********** */
	CLO_SORT_ABITONIC_FINISH();
}

/**
 * @brief This kernel can perform the four last steps of a stage in a
 * bitonic sort.
 * 
 * @param data_global Array of data to sort.
 * @param stage
 * @param data_local
 */
__kernel void abitonic_4(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local)
{

	/* *********** INIT ************** */
	CLO_SORT_ABITONIC_INIT();
	/* *********** STEP 4 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 8);
	/* *********** STEP 3 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 4);
	/* ********** STEP 2 ************** */
	CLO_SORT_ABITONIC_STEP(data_local, 2);
	/* ********** STEP 1 ************** */
	CLO_SORT_ABITONIC_STEP(data_local, 1);
	/* ********* FINISH *********** */
	CLO_SORT_ABITONIC_FINISH();

}

/**
 * @brief This kernel can perform the five last steps of a stage in a
 * bitonic sort.
 * 
 * @param data_global Array of data to sort.
 * @param stage
 * @param data_local
 */
__kernel void abitonic_5(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local)
{

	/* *********** INIT ************** */
	CLO_SORT_ABITONIC_INIT();
	/* *********** STEP 5 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 16);
	/* *********** STEP 4 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 8);
	/* *********** STEP 3 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 4);
	/* ********** STEP 2 ************** */
	CLO_SORT_ABITONIC_STEP(data_local, 2);
	/* ********** STEP 1 ************** */
	CLO_SORT_ABITONIC_STEP(data_local, 1);
	/* ********* FINISH *********** */
	CLO_SORT_ABITONIC_FINISH();

}

/**
 * @brief This kernel can perform the six last steps of a stage in a
 * bitonic sort.
 * 
 * @param data_global Array of data to sort.
 * @param stage
 * @param data_local
 */
__kernel void abitonic_6(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local)
{

	/* *********** INIT ************** */
	CLO_SORT_ABITONIC_INIT();
	/* *********** STEP 6 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 32);
	/* *********** STEP 5 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 16);
	/* *********** STEP 4 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 8);
	/* *********** STEP 3 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 4);
	/* ********** STEP 2 ************** */
	CLO_SORT_ABITONIC_STEP(data_local, 2);
	/* ********** STEP 1 ************** */
	CLO_SORT_ABITONIC_STEP(data_local, 1);
	/* ********* FINISH *********** */
	CLO_SORT_ABITONIC_FINISH();

}

/**
 * @brief This kernel can perform the seven last steps of a stage in a
 * bitonic sort.
 * 
 * @param data_global Array of data to sort.
 * @param stage
 * @param data_local
 */
__kernel void abitonic_7(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local)
{

	/* *********** INIT ************** */
	CLO_SORT_ABITONIC_INIT();
	/* *********** STEP 7 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 64);
	/* *********** STEP 6 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 32);
	/* *********** STEP 5 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 16);
	/* *********** STEP 4 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 8);
	/* *********** STEP 3 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 4);
	/* ********** STEP 2 ************** */
	CLO_SORT_ABITONIC_STEP(data_local, 2);
	/* ********** STEP 1 ************** */
	CLO_SORT_ABITONIC_STEP(data_local, 1);
	/* ********* FINISH *********** */
	CLO_SORT_ABITONIC_FINISH();

}

/**
 * @brief This kernel can perform the eight last steps of a stage in a
 * bitonic sort.
 * 
 * @param data_global Array of data to sort.
 * @param stage
 * @param data_local
 */
__kernel void abitonic_8(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local)
{

	/* *********** INIT ************** */
	CLO_SORT_ABITONIC_INIT();
	/* *********** STEP 8 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 128);
	/* *********** STEP 7 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 64);
	/* *********** STEP 6 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 32);
	/* *********** STEP 5 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 16);
	/* *********** STEP 4 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 8);
	/* *********** STEP 3 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 4);
	/* ********** STEP 2 ************** */
	CLO_SORT_ABITONIC_STEP(data_local, 2);
	/* ********** STEP 1 ************** */
	CLO_SORT_ABITONIC_STEP(data_local, 1);
	/* ********* FINISH *********** */
	CLO_SORT_ABITONIC_FINISH();

}


/**
 * @brief This kernel can perform the nine last steps of a stage in a
 * bitonic sort.
 * 
 * @param data_global Array of data to sort.
 * @param stage
 * @param data_local
 */
__kernel void abitonic_9(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local)
{

	/* *********** INIT ************** */
	CLO_SORT_ABITONIC_INIT();
	/* *********** STEP 9 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 256);
	/* *********** STEP 8 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 128);
	/* *********** STEP 7 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 64);
	/* *********** STEP 6 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 32);
	/* *********** STEP 5 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 16);
	/* *********** STEP 4 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 8);
	/* *********** STEP 3 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 4);
	/* ********** STEP 2 ************** */
	CLO_SORT_ABITONIC_STEP(data_local, 2);
	/* ********** STEP 1 ************** */
	CLO_SORT_ABITONIC_STEP(data_local, 1);
	/* ********* FINISH *********** */
	CLO_SORT_ABITONIC_FINISH();

}

/**
 * @brief This kernel can perform the ten last steps of a stage in a
 * bitonic sort.
 * 
 * @param data_global Array of data to sort.
 * @param stage
 * @param data_local
 */
__kernel void abitonic_10(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local)
{

	/* *********** INIT ************** */
	CLO_SORT_ABITONIC_INIT();
	/* *********** STEP 10 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 512);
	/* *********** STEP 9 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 256);
	/* *********** STEP 8 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 128);
	/* *********** STEP 7 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 64);
	/* *********** STEP 6 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 32);
	/* *********** STEP 5 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 16);
	/* *********** STEP 4 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 8);
	/* *********** STEP 3 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 4);
	/* ********** STEP 2 ************** */
	CLO_SORT_ABITONIC_STEP(data_local, 2);
	/* ********** STEP 1 ************** */
	CLO_SORT_ABITONIC_STEP(data_local, 1);
	/* ********* FINISH *********** */
	CLO_SORT_ABITONIC_FINISH();

}

/**
 * @brief This kernel can perform the eleven last steps of a stage in a
 * bitonic sort.
 * 
 * @param data_global Array of data to sort.
 * @param stage
 * @param data_local
 */
__kernel void abitonic_11(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local)
{

	/* *********** INIT ************** */
	CLO_SORT_ABITONIC_INIT();
	/* *********** STEP 11 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 1024);
	/* *********** STEP 10 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 512);
	/* *********** STEP 9 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 256);
	/* *********** STEP 8 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 128);
	/* *********** STEP 7 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 64);
	/* *********** STEP 6 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 32);
	/* *********** STEP 5 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 16);
	/* *********** STEP 4 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 8);
	/* *********** STEP 3 ************ */
	CLO_SORT_ABITONIC_STEP(data_local, 4);
	/* ********** STEP 2 ************** */
	CLO_SORT_ABITONIC_STEP(data_local, 2);
	/* ********** STEP 1 ************** */
	CLO_SORT_ABITONIC_STEP(data_local, 1);
	/* ********* FINISH *********** */
	CLO_SORT_ABITONIC_FINISH();

}

/**
 * @brief This kernel can perform any step of any stage of a bitonic
 * sort.
 * 
 * @param data Array of data to sort.
 * @param stage
 * @param step
 */
__kernel void abitonic_any(
			__global CLO_SORT_ELEM_TYPE *data,
			uint stage,
			uint step)
{
	/* Global id for this work-item. */
	uint gid = get_global_id(0);
	
	/* Elements to possibly swap. */
	CLO_SORT_ELEM_TYPE data1, data2;

	/* Determine if ascending or descending */
	bool desc = (bool) (0x1 & (gid >> (stage - 1)));

	/* Determine stride. */
	uint pair_stride = (uint) (1 << (step - 1)); 
	
	/* Block of which this thread is part of. */
	uint block = gid / pair_stride;
	
	/* ID of thread in block. */
	uint bid = gid % pair_stride;

	/* Determine what to compare and possibly swap. */
	uint index1 = block * pair_stride * 2 + bid;
	uint index2 = index1 + pair_stride;

	/* Compare and possibly exchange elements. */
	CLO_SORT_ABITONIC_CMPXCH(data, index1, index2);
	
}

/* Each thread sorts 8 values (in three steps of a bitonic stage).
 * Assumes gws = numel2sort / 8 */
__kernel void abitonic_s8(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			uint step)
{

	CLO_SORT_ABITONIC_INIT_S(8);
	
	/* ***** Transfer 8 values to sort to private memory ***** */

	data_priv[0] = data_global[gaddr];
	data_priv[1] = data_global[gaddr + inc];
	data_priv[2] = data_global[gaddr + 2 * inc];
	data_priv[3] = data_global[gaddr + 3 * inc];
	data_priv[4] = data_global[gaddr + 4 * inc];
	data_priv[5] = data_global[gaddr + 5 * inc];
	data_priv[6] = data_global[gaddr + 6 * inc];
	data_priv[7] = data_global[gaddr + 7 * inc];

	/* ***** Sort the 8 values ***** */

	/* Step n */
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 0, 4);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 1, 5);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 2, 6);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 3, 7);
	/* Step n-1 */
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 0, 2);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 1, 3);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 4, 6);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 5, 7);
	/* Step n-2 */
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 0, 1);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 2, 3);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 4, 5);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 6, 7);

	/* ***** Transfer the n values to global memory ***** */

	data_global[gaddr] = data_priv[0];
	data_global[gaddr + inc] = data_priv[1];
	data_global[gaddr + 2 * inc] = data_priv[2];
	data_global[gaddr + 3 * inc] = data_priv[3];
	data_global[gaddr + 4 * inc] = data_priv[4];
	data_global[gaddr + 5 * inc] = data_priv[5];
	data_global[gaddr + 6 * inc] = data_priv[6];
	data_global[gaddr + 7 * inc] = data_priv[7];

}

/* Each thread sorts 16 values (in three steps of a bitonic stage).
 * Assumes gws = numel2sort / 16 */
__kernel void abitonic_s16(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			uint step)
{

	CLO_SORT_ABITONIC_INIT_S(16);
	
	/* ***** Transfer 16 values to sort to private memory ***** */

	data_priv[0] = data_global[gaddr];
	data_priv[1] = data_global[gaddr + inc];
	data_priv[2] = data_global[gaddr + 2 * inc];
	data_priv[3] = data_global[gaddr + 3 * inc];
	data_priv[4] = data_global[gaddr + 4 * inc];
	data_priv[5] = data_global[gaddr + 5 * inc];
	data_priv[6] = data_global[gaddr + 6 * inc];
	data_priv[7] = data_global[gaddr + 7 * inc];
	data_priv[8] = data_global[gaddr + 8 * inc];
	data_priv[9] = data_global[gaddr + 9 * inc];
	data_priv[10] = data_global[gaddr + 10 * inc];
	data_priv[11] = data_global[gaddr + 11 * inc];
	data_priv[12] = data_global[gaddr + 12 * inc];
	data_priv[13] = data_global[gaddr + 13 * inc];
	data_priv[14] = data_global[gaddr + 14 * inc];
	data_priv[15] = data_global[gaddr + 15 * inc];

	/* ***** Sort the 16 values ***** */

	/* Step n */
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 0, 8);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 1, 9);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 2, 10);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 3, 11);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 4, 12);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 5, 13);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 6, 14);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 7, 15);
	/* Step n-1 */
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 0, 4);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 1, 5);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 2, 6);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 3, 7);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 8, 12);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 9, 13);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 10, 14);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 11, 15);

	/* Step n-2 */
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 0, 2);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 1, 3);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 4, 6);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 5, 7);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 8, 10);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 9, 11);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 12, 14);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 13, 15);
	
	/* Step n-3 */
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 0, 1);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 2, 3);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 4, 5);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 6, 7);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 8, 9);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 10, 11);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 12, 13);
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 14, 15);

	/* ***** Transfer the n values to global memory ***** */

	data_global[gaddr] = data_priv[0];
	data_global[gaddr + inc] = data_priv[1];
	data_global[gaddr + 2 * inc] = data_priv[2];
	data_global[gaddr + 3 * inc] = data_priv[3];
	data_global[gaddr + 4 * inc] = data_priv[4];
	data_global[gaddr + 5 * inc] = data_priv[5];
	data_global[gaddr + 6 * inc] = data_priv[6];
	data_global[gaddr + 7 * inc] = data_priv[7];
	data_global[gaddr + 8 * inc] = data_priv[8];
	data_global[gaddr + 9 * inc] = data_priv[9];
	data_global[gaddr + 10 * inc] = data_priv[10];
	data_global[gaddr + 11 * inc] = data_priv[11];
	data_global[gaddr + 12 * inc] = data_priv[12];
	data_global[gaddr + 13 * inc] = data_priv[13];
	data_global[gaddr + 14 * inc] = data_priv[14];
	data_global[gaddr + 15 * inc] = data_priv[15];
}

#define CLO_SORT_ABITONIC_4_INIT() \
	/* Global and local ids for this work-item. */ \
	uint gid = get_global_id(0); \
	uint lid = get_local_id(0); \
	uint local_size = get_local_size(0); \
	uint group_id = get_group_id(0); \
	/* Base for local memory. */ \
	uint laddr; \
	uint inc; \
	uint blockSize; \
	/* Elements to possibly swap. */ \
	CLO_SORT_ELEM_TYPE data1, data2, data_priv[4]; \
	/* Local and global indexes for moving data between local and \
	 * global memory. */ \
	uint local_index1 = lid; \
	uint local_index2 = local_size + lid; \
	uint local_index3 = local_size * 2 + lid; \
	uint local_index4 = local_size * 3 + lid; \
	uint global_index1 = local_size * group_id * 4 + lid; \
	uint global_index2 = local_size * (group_id * 4 + 1) + lid; \
	uint global_index3 = local_size * (group_id * 4 + 2) + lid; \
	uint global_index4 = local_size * (group_id * 4 + 3) + lid; \
	/* Determine if ascending or descending */ \
	bool desc = (bool) (0x1 & (gid >> (stage - 2))); \
	/* Index of values to possibly swap. */ \
	uint index1, index2; \
	/* Load data locally */ \
	data_local[local_index1] = data_global[global_index1]; \
	data_local[local_index2] = data_global[global_index2]; \
	data_local[local_index3] = data_global[global_index3]; \
	data_local[local_index4] = data_global[global_index4]; \
	/* Local memory barrier */ \
	barrier(CLK_LOCAL_MEM_FENCE);
	
#define CLO_SORT_ABITONIC_4_FINISH() \
	/* Store data globally */ \
	data_global[global_index1] = data_local[local_index1]; \
	data_global[global_index2] = data_local[local_index2]; \
	data_global[global_index3] = data_local[local_index3]; \
	data_global[global_index4] = data_local[local_index4];
	
#define CLO_SORT_ABITONIC_4_S2(step) \
	/* ***** Transfer 4 values to sort from local to private memory ***** */ \
	blockSize = 1 << step; \
	laddr = ((lid * 4) / blockSize) * blockSize + (lid % (blockSize / 4)); \
	inc = blockSize / 4; \
	data_priv[0] = data_local[laddr]; \
	data_priv[1] = data_local[laddr + inc]; \
	data_priv[2] = data_local[laddr + 2 * inc]; \
	data_priv[3] = data_local[laddr + 3 * inc]; \
	/* ***** Sort the 4 values ***** */ \
	/* Step n */ \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 0, 2); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 1, 3); \
	/* Step n-1 */ \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 0, 1); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 2, 3); \
	/* ***** Transfer 4 sorted values from private to local memory ***** */ \
	data_local[laddr] = data_priv[0]; \
	data_local[laddr + inc] = data_priv[1]; \
	data_local[laddr + 2 * inc] = data_priv[2]; \
	data_local[laddr + 3 * inc] = data_priv[3]; \
	/* Local memory barrier */ \
	barrier(CLK_LOCAL_MEM_FENCE);


/* Works from step 4 to step 1, local barriers between each two steps,
 * each thread sorts 4 values. */
__kernel void abitonic_4_s4(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local) 
{
	CLO_SORT_ABITONIC_4_INIT();
	CLO_SORT_ABITONIC_4_S2(4);
	CLO_SORT_ABITONIC_4_S2(2);
	CLO_SORT_ABITONIC_4_FINISH();				
}


/* Works from step 6 to step 1, local barriers between each two steps,
 * each thread sorts 4 values. */
__kernel void abitonic_4_s6(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local) 
{
	CLO_SORT_ABITONIC_4_INIT();
	CLO_SORT_ABITONIC_4_S2(6);
	CLO_SORT_ABITONIC_4_S2(4);
	CLO_SORT_ABITONIC_4_S2(2);
	CLO_SORT_ABITONIC_4_FINISH();				
}


/* Works from step 8 to step 1, local barriers between each two steps,
 * each thread sorts 4 values. */
__kernel void abitonic_4_s8(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local) 
{
	CLO_SORT_ABITONIC_4_INIT();
	CLO_SORT_ABITONIC_4_S2(8);
	CLO_SORT_ABITONIC_4_S2(6);
	CLO_SORT_ABITONIC_4_S2(4);
	CLO_SORT_ABITONIC_4_S2(2);
	CLO_SORT_ABITONIC_4_FINISH();				
}

/* Works from step 10 to step 1, local barriers between each two steps,
 * each thread sorts 4 values. */
__kernel void abitonic_4_s10(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local) 
{
	CLO_SORT_ABITONIC_4_INIT();
	CLO_SORT_ABITONIC_4_S2(10);
	CLO_SORT_ABITONIC_4_S2(8);
	CLO_SORT_ABITONIC_4_S2(6);
	CLO_SORT_ABITONIC_4_S2(4);
	CLO_SORT_ABITONIC_4_S2(2);
	CLO_SORT_ABITONIC_4_FINISH();				
}

/* Works from step 12 to step 1, local barriers between each two steps,
 * each thread sorts 4 values. */
__kernel void abitonic_4_s12(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local) 
{
	CLO_SORT_ABITONIC_4_INIT();
	CLO_SORT_ABITONIC_4_S2(12);
	CLO_SORT_ABITONIC_4_S2(10);
	CLO_SORT_ABITONIC_4_S2(8);
	CLO_SORT_ABITONIC_4_S2(6);
	CLO_SORT_ABITONIC_4_S2(4);
	CLO_SORT_ABITONIC_4_S2(2);
	CLO_SORT_ABITONIC_4_FINISH();				
}


#define CLO_SORT_ABITONIC_8_INIT() \
	/* Global and local ids for this work-item. */ \
	uint gid = get_global_id(0); \
	uint lid = get_local_id(0); \
	uint local_size = get_local_size(0); \
	uint group_id = get_group_id(0); \
	/* Base for local memory. */ \
	uint laddr; \
	uint inc; \
	uint blockSize; \
	/* Elements to possibly swap. */ \
	CLO_SORT_ELEM_TYPE data1, data2, data_priv[8]; \
	/* Local and global indexes for moving data between local and \
	 * global memory. */ \
	uint local_index1 = lid; \
	uint local_index2 = local_size + lid; \
	uint local_index3 = local_size * 2 + lid; \
	uint local_index4 = local_size * 3 + lid; \
	uint local_index5 = local_size * 4 + lid; \
	uint local_index6 = local_size * 5 + lid; \
	uint local_index7 = local_size * 6 + lid; \
	uint local_index8 = local_size * 7 + lid; \
	uint global_index1 = local_size * group_id * 8 + lid; \
	uint global_index2 = local_size * (group_id * 8 + 1) + lid; \
	uint global_index3 = local_size * (group_id * 8 + 2) + lid; \
	uint global_index4 = local_size * (group_id * 8 + 3) + lid; \
	uint global_index5 = local_size * (group_id * 8 + 4) + lid; \
	uint global_index6 = local_size * (group_id * 8 + 5) + lid; \
	uint global_index7 = local_size * (group_id * 8 + 6) + lid; \
	uint global_index8 = local_size * (group_id * 8 + 7) + lid; \
	/* Determine if ascending or descending */ \
	bool desc = (bool) (0x1 & (gid >> (stage - 3))); \
	/* Index of values to possibly swap. */ \
	uint index1, index2; \
	/* Load data locally */ \
	data_local[local_index1] = data_global[global_index1]; \
	data_local[local_index2] = data_global[global_index2]; \
	data_local[local_index3] = data_global[global_index3]; \
	data_local[local_index4] = data_global[global_index4]; \
	data_local[local_index5] = data_global[global_index5]; \
	data_local[local_index6] = data_global[global_index6]; \
	data_local[local_index7] = data_global[global_index7]; \
	data_local[local_index8] = data_global[global_index8]; \
	/* Local memory barrier */ \
	barrier(CLK_LOCAL_MEM_FENCE);
	
#define CLO_SORT_ABITONIC_8_FINISH() \
	/* Store data globally */ \
	data_global[global_index1] = data_local[local_index1]; \
	data_global[global_index2] = data_local[local_index2]; \
	data_global[global_index3] = data_local[local_index3]; \
	data_global[global_index4] = data_local[local_index4]; \
	data_global[global_index5] = data_local[local_index5]; \
	data_global[global_index6] = data_local[local_index6]; \
	data_global[global_index7] = data_local[local_index7]; \
	data_global[global_index8] = data_local[local_index8];
	
#define CLO_SORT_ABITONIC_8_S3(step) \
	/* ***** Transfer 8 values to sort from local to private memory ***** */ \
	blockSize = 1 << step; \
	laddr = ((lid * 8) / blockSize) * blockSize + (lid % (blockSize / 8)); \
	inc = blockSize / 8; \
	data_priv[0] = data_local[laddr]; \
	data_priv[1] = data_local[laddr + inc]; \
	data_priv[2] = data_local[laddr + 2 * inc]; \
	data_priv[3] = data_local[laddr + 3 * inc]; \
	data_priv[4] = data_local[laddr + 4 * inc]; \
	data_priv[5] = data_local[laddr + 5 * inc]; \
	data_priv[6] = data_local[laddr + 6 * inc]; \
	data_priv[7] = data_local[laddr + 7 * inc]; \
	/* ***** Sort the 8 values ***** */ \
	/* Step n */ \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 0, 4); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 1, 5); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 2, 6); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 3, 7); \
	/* Step n-1 */ \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 0, 2); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 1, 3); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 4, 6); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 5, 7); \
	/* Step n-2 */ \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 0, 1); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 2, 3); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 4, 5); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 6, 7); \
	/* ***** Transfer 4 sorted values from private to local memory ***** */ \
	data_local[laddr] = data_priv[0]; \
	data_local[laddr + inc] = data_priv[1]; \
	data_local[laddr + 2 * inc] = data_priv[2]; \
	data_local[laddr + 3 * inc] = data_priv[3]; \
	data_local[laddr + 4 * inc] = data_priv[4]; \
	data_local[laddr + 5 * inc] = data_priv[5]; \
	data_local[laddr + 6 * inc] = data_priv[6]; \
	data_local[laddr + 7 * inc] = data_priv[7]; \
	/* Local memory barrier */ \
	barrier(CLK_LOCAL_MEM_FENCE);


/* Works from step 3 to step 1, local barriers between each three steps,
 * each thread sorts 8 values. */
__kernel void abitonic_8_s3(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local) 
{
	CLO_SORT_ABITONIC_8_INIT();
	CLO_SORT_ABITONIC_8_S3(3);
	CLO_SORT_ABITONIC_8_FINISH();				
}

/* Works from step 6 to step 1, local barriers between each three steps,
 * each thread sorts 8 values. */
__kernel void abitonic_8_s6(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local) 
{
	CLO_SORT_ABITONIC_8_INIT();
	CLO_SORT_ABITONIC_8_S3(6);
	CLO_SORT_ABITONIC_8_S3(3);
	CLO_SORT_ABITONIC_8_FINISH();				
}

/* Works from step 9 to step 1, local barriers between each three steps,
 * each thread sorts 8 values. */
__kernel void abitonic_8_s9(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local) 
{
	CLO_SORT_ABITONIC_8_INIT();
	CLO_SORT_ABITONIC_8_S3(9);
	CLO_SORT_ABITONIC_8_S3(6);
	CLO_SORT_ABITONIC_8_S3(3);
	CLO_SORT_ABITONIC_8_FINISH();				
}

/* Works from step 12 to step 1, local barriers between each three steps,
 * each thread sorts 8 values. */
__kernel void abitonic_8_s12(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local) 
{
	CLO_SORT_ABITONIC_8_INIT();
	CLO_SORT_ABITONIC_8_S3(12);
	CLO_SORT_ABITONIC_8_S3(9);
	CLO_SORT_ABITONIC_8_S3(6);
	CLO_SORT_ABITONIC_8_S3(3);
	CLO_SORT_ABITONIC_8_FINISH();				
}

#define CLO_SORT_ABITONIC_16_INIT() \
	/* Global and local ids for this work-item. */ \
	uint gid = get_global_id(0); \
	uint lid = get_local_id(0); \
	uint local_size = get_local_size(0); \
	uint group_id = get_group_id(0); \
	/* Base for local memory. */ \
	uint laddr; \
	uint inc; \
	uint blockSize; \
	/* Elements to possibly swap. */ \
	CLO_SORT_ELEM_TYPE data1, data2, data_priv[16]; \
	/* Local and global indexes for moving data between local and \
	 * global memory. */ \
	uint local_index1 = lid; \
	uint local_index2 = local_size + lid; \
	uint local_index3 = local_size * 2 + lid; \
	uint local_index4 = local_size * 3 + lid; \
	uint local_index5 = local_size * 4 + lid; \
	uint local_index6 = local_size * 5 + lid; \
	uint local_index7 = local_size * 6 + lid; \
	uint local_index8 = local_size * 7 + lid; \
	uint local_index9 = local_size * 8 + lid; \
	uint local_index10 = local_size * 9 + lid; \
	uint local_index11 = local_size * 10 + lid; \
	uint local_index12 = local_size * 11 + lid; \
	uint local_index13 = local_size * 12 + lid; \
	uint local_index14 = local_size * 13 + lid; \	
	uint local_index15 = local_size * 14 + lid; \	
	uint local_index16 = local_size * 15 + lid; \	
	uint global_index1 = local_size * group_id * 16 + lid; \
	uint global_index2 = local_size * (group_id * 16 + 1) + lid; \
	uint global_index3 = local_size * (group_id * 16 + 2) + lid; \
	uint global_index4 = local_size * (group_id * 16 + 3) + lid; \
	uint global_index5 = local_size * (group_id * 16 + 4) + lid; \
	uint global_index6 = local_size * (group_id * 16 + 5) + lid; \
	uint global_index7 = local_size * (group_id * 16 + 6) + lid; \
	uint global_index8 = local_size * (group_id * 16 + 7) + lid; \
	uint global_index9 = local_size * (group_id * 16 + 8) + lid; \
	uint global_index10 = local_size * (group_id * 16 + 9) + lid; \
	uint global_index11 = local_size * (group_id * 16 + 10) + lid; \
	uint global_index12 = local_size * (group_id * 16 + 11) + lid; \
	uint global_index13 = local_size * (group_id * 16 + 12) + lid; \
	uint global_index14 = local_size * (group_id * 16 + 13) + lid; \
	uint global_index15 = local_size * (group_id * 16 + 14) + lid; \
	uint global_index16 = local_size * (group_id * 16 + 15) + lid; \
	/* Determine if ascending or descending */ \
	bool desc = (bool) (0x1 & (gid >> (stage - 4))); \
	/* Index of values to possibly swap. */ \
	uint index1, index2; \
	/* Load data locally */ \
	data_local[local_index1] = data_global[global_index1]; \
	data_local[local_index2] = data_global[global_index2]; \
	data_local[local_index3] = data_global[global_index3]; \
	data_local[local_index4] = data_global[global_index4]; \
	data_local[local_index5] = data_global[global_index5]; \
	data_local[local_index6] = data_global[global_index6]; \
	data_local[local_index7] = data_global[global_index7]; \
	data_local[local_index8] = data_global[global_index8]; \
	data_local[local_index9] = data_global[global_index9]; \
	data_local[local_index10] = data_global[global_index10]; \
	data_local[local_index11] = data_global[global_index11]; \
	data_local[local_index12] = data_global[global_index12]; \
	data_local[local_index13] = data_global[global_index13]; \
	data_local[local_index14] = data_global[global_index14]; \
	data_local[local_index15] = data_global[global_index15]; \
	data_local[local_index16] = data_global[global_index16]; \
	/* Local memory barrier */ \
	barrier(CLK_LOCAL_MEM_FENCE);
	
#define CLO_SORT_ABITONIC_16_FINISH() \
	/* Store data globally */ \
	data_global[global_index1] = data_local[local_index1]; \
	data_global[global_index2] = data_local[local_index2]; \
	data_global[global_index3] = data_local[local_index3]; \
	data_global[global_index4] = data_local[local_index4]; \
	data_global[global_index5] = data_local[local_index5]; \
	data_global[global_index6] = data_local[local_index6]; \
	data_global[global_index7] = data_local[local_index7]; \
	data_global[global_index8] = data_local[local_index8]; \
	data_global[global_index9] = data_local[local_index9]; \
	data_global[global_index10] = data_local[local_index10]; \
	data_global[global_index11] = data_local[local_index11]; \
	data_global[global_index12] = data_local[local_index12]; \
	data_global[global_index13] = data_local[local_index13]; \
	data_global[global_index14] = data_local[local_index14]; \
	data_global[global_index15] = data_local[local_index15]; \
	data_global[global_index16] = data_local[local_index16];
	
#define CLO_SORT_ABITONIC_16_S4(step) \
	/* ***** Transfer 16 values to sort from local to private memory ***** */ \
	blockSize = 1 << step; \
	laddr = ((lid * 16) / blockSize) * blockSize + (lid % (blockSize / 16)); \
	inc = blockSize / 16; \
	data_priv[0] = data_local[laddr]; \
	data_priv[1] = data_local[laddr + inc]; \
	data_priv[2] = data_local[laddr + 2 * inc]; \
	data_priv[3] = data_local[laddr + 3 * inc]; \
	data_priv[4] = data_local[laddr + 4 * inc]; \
	data_priv[5] = data_local[laddr + 5 * inc]; \
	data_priv[6] = data_local[laddr + 6 * inc]; \
	data_priv[7] = data_local[laddr + 7 * inc]; \
	data_priv[8] = data_local[laddr + 8 * inc]; \
	data_priv[9] = data_local[laddr + 9 * inc]; \
	data_priv[10] = data_local[laddr + 10 * inc]; \
	data_priv[11] = data_local[laddr + 11 * inc]; \
	data_priv[12] = data_local[laddr + 12 * inc]; \
	data_priv[13] = data_local[laddr + 13 * inc]; \
	data_priv[14] = data_local[laddr + 14 * inc]; \
	data_priv[15] = data_local[laddr + 15 * inc]; \
	/* ***** Sort the 16 values ***** */ \
	/* Step n */ \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 0, 8); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 1, 9); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 2, 10); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 3, 11); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 4, 12); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 5, 13); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 6, 14); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 7, 15); \
	/* Step n-1 */ \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 0, 4); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 1, 5); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 2, 6); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 3, 7); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 8, 12); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 9, 13); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 10, 14); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 11, 15); \
	/* Step n-2 */ \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 0, 2); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 1, 3); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 4, 6); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 5, 7); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 8, 10); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 9, 11); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 12, 14); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 13, 15); \
	/* Step n-3 */ \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 0, 1); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 2, 3); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 4, 5); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 6, 7); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 8, 9); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 10, 11); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 12, 13); \
	CLO_SORT_ABITONIC_CMPXCH(data_priv, 14, 15); \
	/* ***** Transfer 16 sorted values from private to local memory ***** */ \
	data_local[laddr] = data_priv[0]; \
	data_local[laddr + inc] = data_priv[1]; \
	data_local[laddr + 2 * inc] = data_priv[2]; \
	data_local[laddr + 3 * inc] = data_priv[3]; \
	data_local[laddr + 4 * inc] = data_priv[4]; \
	data_local[laddr + 5 * inc] = data_priv[5]; \
	data_local[laddr + 6 * inc] = data_priv[6]; \
	data_local[laddr + 7 * inc] = data_priv[7]; \
	data_local[laddr + 8 * inc] = data_priv[8]; \
	data_local[laddr + 9 * inc] = data_priv[9]; \
	data_local[laddr + 10 * inc] = data_priv[10]; \
	data_local[laddr + 11 * inc] = data_priv[11]; \
	data_local[laddr + 12 * inc] = data_priv[12]; \
	data_local[laddr + 13 * inc] = data_priv[13]; \
	data_local[laddr + 14 * inc] = data_priv[14]; \
	data_local[laddr + 15 * inc] = data_priv[15]; \
	/* Local memory barrier */ \
	barrier(CLK_LOCAL_MEM_FENCE);


/* Works from step 4 to step 1, local barriers between each four steps,
 * each thread sorts 16 values. */
__kernel void abitonic_16_s4(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local) 
{
	CLO_SORT_ABITONIC_16_INIT();
	CLO_SORT_ABITONIC_16_S4(4);
	CLO_SORT_ABITONIC_16_FINISH();				
}

/* Works from step 8 to step 1, local barriers between each four steps,
 * each thread sorts 16 values. */
__kernel void abitonic_16_s8(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local) 
{
	CLO_SORT_ABITONIC_16_INIT();
	CLO_SORT_ABITONIC_16_S4(8);
	CLO_SORT_ABITONIC_16_S4(4);
	CLO_SORT_ABITONIC_16_FINISH();				
}

/* Works from step 12 to step 1, local barriers between each four steps,
 * each thread sorts 16 values. */
__kernel void abitonic_16_s12(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local) 
{
	CLO_SORT_ABITONIC_16_INIT();
	CLO_SORT_ABITONIC_16_S4(12);
	CLO_SORT_ABITONIC_16_S4(8);
	CLO_SORT_ABITONIC_16_S4(4);
	CLO_SORT_ABITONIC_16_FINISH();				
}

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
 
#define ABIT_CMPXCH(data, index1, index2) \
	data1 = data[index1]; \
	data2 = data[index2]; \
	if (CLO_SORT_COMPARE(data1, data2) ^ desc) { \
		data[index1] = data2; \
		data[index2] = data1; \
	} 
 
#define ABIT_LOCAL_SORT(data, stride) \
  	/* Determine what to compare and possibly swap. */ \
	index1 = (lid / stride) * stride * 2 + (lid % stride); \
	index2 = index1 + stride; \
	/* Compare and swap */ \
	ABIT_CMPXCH(data, index1, index2); \
	/* Local memory barrier */ \
	barrier(CLK_LOCAL_MEM_FENCE);

#define ABIT_SORT_LOAD4(data_priv, data, baddr, inc) \
	data_priv[0] = data[baddr]; \
	data_priv[1] = data[baddr + inc]; \
	data_priv[2] = data[baddr + 2 * inc]; \
	data_priv[3] = data[baddr + 3 * inc];

#define ABIT_SORT_STORE4(data_priv, data, baddr, inc) \
	data[baddr] = data_priv[0]; \
	data[baddr + inc] = data_priv[1]; \
	data[baddr + 2 * inc] = data_priv[2]; \
	data[baddr + 3 * inc] = data_priv[3];

#define ABIT_SORT_LOAD8(data_priv, data, baddr, inc) \
	data_priv[0] = data[baddr]; \
	data_priv[1] = data[baddr + inc]; \
	data_priv[2] = data[baddr + 2 * inc]; \
	data_priv[3] = data[baddr + 3 * inc]; \
	data_priv[4] = data[baddr + 4 * inc]; \
	data_priv[5] = data[baddr + 5 * inc]; \
	data_priv[6] = data[baddr + 6 * inc]; \
	data_priv[7] = data[baddr + 7 * inc];

#define ABIT_SORT_STORE8(data_priv, data, baddr, inc) \
	data[baddr] = data_priv[0]; \
	data[baddr + inc] = data_priv[1]; \
	data[baddr + 2 * inc] = data_priv[2]; \
	data[baddr + 3 * inc] = data_priv[3]; \
	data[baddr + 4 * inc] = data_priv[4]; \
	data[baddr + 5 * inc] = data_priv[5]; \
	data[baddr + 6 * inc] = data_priv[6]; \
	data[baddr + 7 * inc] = data_priv[7];
	
#define ABIT_SORT_LOAD16(data_priv, data, baddr, inc) \
	data_priv[0] = data[baddr]; \
	data_priv[1] = data[baddr + inc]; \
	data_priv[2] = data[baddr + 2 * inc]; \
	data_priv[3] = data[baddr + 3 * inc]; \
	data_priv[4] = data[baddr + 4 * inc]; \
	data_priv[5] = data[baddr + 5 * inc]; \
	data_priv[6] = data[baddr + 6 * inc]; \
	data_priv[7] = data[baddr + 7 * inc]; \
	data_priv[8] = data[baddr + 8 * inc]; \
	data_priv[9] = data[baddr + 9 * inc]; \
	data_priv[10] = data[baddr + 10 * inc]; \
	data_priv[11] = data[baddr + 11 * inc]; \
	data_priv[12] = data[baddr + 12 * inc]; \
	data_priv[13] = data[baddr + 13 * inc]; \
	data_priv[14] = data[baddr + 14 * inc]; \
	data_priv[15] = data[baddr + 15 * inc];


#define ABIT_SORT_STORE16(data_priv, data, baddr, inc) \
	data[baddr] = data_priv[0]; \
	data[baddr + inc] = data_priv[1]; \
	data[baddr + 2 * inc] = data_priv[2]; \
	data[baddr + 3 * inc] = data_priv[3]; \
	data[baddr + 4 * inc] = data_priv[4]; \
	data[baddr + 5 * inc] = data_priv[5]; \
	data[baddr + 6 * inc] = data_priv[6]; \
	data[baddr + 7 * inc] = data_priv[7]; \
	data[baddr + 8 * inc] = data_priv[8]; \
	data[baddr + 9 * inc] = data_priv[9]; \
	data[baddr + 10 * inc] = data_priv[10]; \
	data[baddr + 11 * inc] = data_priv[11]; \
	data[baddr + 12 * inc] = data_priv[12]; \
	data[baddr + 13 * inc] = data_priv[13]; \
	data[baddr + 14 * inc] = data_priv[14]; \
	data[baddr + 15 * inc] = data_priv[15];
			
#define ABIT_LOCAL_INIT() \
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
	
#define ABIT_LOCAL_FINISH() \
	/* Store data globally */ \
	data_global[global_index1] = data_local[local_index1]; \
	data_global[global_index2] = data_local[local_index2];
	
#define ABIT_PRIV_INIT(n) \
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

#define ABIT_SORT_4S16V(data2sort); \
	/* Step n */ \
	ABIT_CMPXCH(data2sort, 0, 8); \
	ABIT_CMPXCH(data2sort, 1, 9); \
	ABIT_CMPXCH(data2sort, 2, 10); \
	ABIT_CMPXCH(data2sort, 3, 11); \
	ABIT_CMPXCH(data2sort, 4, 12); \
	ABIT_CMPXCH(data2sort, 5, 13); \
	ABIT_CMPXCH(data2sort, 6, 14); \
	ABIT_CMPXCH(data2sort, 7, 15); \
	/* Step n-1 */ \
	ABIT_CMPXCH(data2sort, 0, 4); \
	ABIT_CMPXCH(data2sort, 1, 5); \
	ABIT_CMPXCH(data2sort, 2, 6); \
	ABIT_CMPXCH(data2sort, 3, 7); \
	ABIT_CMPXCH(data2sort, 8, 12); \
	ABIT_CMPXCH(data2sort, 9, 13); \
	ABIT_CMPXCH(data2sort, 10, 14); \
	ABIT_CMPXCH(data2sort, 11, 15); \
	/* Step n-2 */ \
	ABIT_CMPXCH(data2sort, 0, 2); \
	ABIT_CMPXCH(data2sort, 1, 3); \
	ABIT_CMPXCH(data2sort, 4, 6); \
	ABIT_CMPXCH(data2sort, 5, 7); \
	ABIT_CMPXCH(data2sort, 8, 10); \
	ABIT_CMPXCH(data2sort, 9, 11); \
	ABIT_CMPXCH(data2sort, 12, 14); \
	ABIT_CMPXCH(data2sort, 13, 15); \
	/* Step n-3 */ \
	ABIT_CMPXCH(data2sort, 0, 1); \
	ABIT_CMPXCH(data2sort, 2, 3); \
	ABIT_CMPXCH(data2sort, 4, 5); \
	ABIT_CMPXCH(data2sort, 6, 7); \
	ABIT_CMPXCH(data2sort, 8, 9); \
	ABIT_CMPXCH(data2sort, 10, 11); \
	ABIT_CMPXCH(data2sort, 12, 13); \
	ABIT_CMPXCH(data2sort, 14, 15);

#define ABIT_SORT_3S8V(data2sort) \
	/* Step n */ \
	ABIT_CMPXCH(data2sort, 0, 4); \
	ABIT_CMPXCH(data2sort, 1, 5); \
	ABIT_CMPXCH(data2sort, 2, 6); \
	ABIT_CMPXCH(data2sort, 3, 7); \
	/* Step n-1 */ \
	ABIT_CMPXCH(data2sort, 0, 2); \
	ABIT_CMPXCH(data2sort, 1, 3); \
	ABIT_CMPXCH(data2sort, 4, 6); \
	ABIT_CMPXCH(data2sort, 5, 7); \
	/* Step n-2 */ \
	ABIT_CMPXCH(data2sort, 0, 1); \
	ABIT_CMPXCH(data2sort, 2, 3); \
	ABIT_CMPXCH(data2sort, 4, 5); \
	ABIT_CMPXCH(data2sort, 6, 7);

#define ABIT_SORT_2S4V(data2sort) \
	/* Step n */ \
	ABIT_CMPXCH(data2sort, 0, 2); \
	ABIT_CMPXCH(data2sort, 1, 3); \
	/* Step n-1 */ \
	ABIT_CMPXCH(data2sort, 0, 1); \
	ABIT_CMPXCH(data2sort, 2, 3);

/**
 * @brief This kernel can perform the two last steps of a stage in a
 * bitonic sort.
 * 
 * @param data_global Array of data to sort.
 * @param stage
 * @param data_local
 */
__kernel void abit_local_s2(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local)
{
	
	/* *********** INIT ************** */
	ABIT_LOCAL_INIT();
	/* ********** STEP 2 ************** */
	ABIT_LOCAL_SORT(data_local, 2);
	/* ********** STEP 1 ************** */
	ABIT_LOCAL_SORT(data_local, 1);
	/* ********* FINISH *********** */
	ABIT_LOCAL_FINISH();
}

/**
 * @brief This kernel can perform the three last steps of a stage in a
 * bitonic sort.
 * 
 * @param data_global Array of data to sort.
 * @param stage
 * @param data_local
 */
__kernel void abit_local_s3(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local)
{
	
	/* *********** INIT ************** */
	ABIT_LOCAL_INIT();
	/* *********** STEP 3 ************ */
	ABIT_LOCAL_SORT(data_local, 4);
	/* ********** STEP 2 ************** */
	ABIT_LOCAL_SORT(data_local, 2);
	/* ********** STEP 1 ************** */
	ABIT_LOCAL_SORT(data_local, 1);
	/* ********* FINISH *********** */
	ABIT_LOCAL_FINISH();
}

/**
 * @brief This kernel can perform the four last steps of a stage in a
 * bitonic sort.
 * 
 * @param data_global Array of data to sort.
 * @param stage
 * @param data_local
 */
__kernel void abit_local_s4(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local)
{

	/* *********** INIT ************** */
	ABIT_LOCAL_INIT();
	/* *********** STEP 4 ************ */
	ABIT_LOCAL_SORT(data_local, 8);
	/* *********** STEP 3 ************ */
	ABIT_LOCAL_SORT(data_local, 4);
	/* ********** STEP 2 ************** */
	ABIT_LOCAL_SORT(data_local, 2);
	/* ********** STEP 1 ************** */
	ABIT_LOCAL_SORT(data_local, 1);
	/* ********* FINISH *********** */
	ABIT_LOCAL_FINISH();

}

/**
 * @brief This kernel can perform the five last steps of a stage in a
 * bitonic sort.
 * 
 * @param data_global Array of data to sort.
 * @param stage
 * @param data_local
 */
__kernel void abit_local_s5(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local)
{

	/* *********** INIT ************** */
	ABIT_LOCAL_INIT();
	/* *********** STEP 5 ************ */
	ABIT_LOCAL_SORT(data_local, 16);
	/* *********** STEP 4 ************ */
	ABIT_LOCAL_SORT(data_local, 8);
	/* *********** STEP 3 ************ */
	ABIT_LOCAL_SORT(data_local, 4);
	/* ********** STEP 2 ************** */
	ABIT_LOCAL_SORT(data_local, 2);
	/* ********** STEP 1 ************** */
	ABIT_LOCAL_SORT(data_local, 1);
	/* ********* FINISH *********** */
	ABIT_LOCAL_FINISH();

}

/**
 * @brief This kernel can perform the six last steps of a stage in a
 * bitonic sort.
 * 
 * @param data_global Array of data to sort.
 * @param stage
 * @param data_local
 */
__kernel void abit_local_s6(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local)
{

	/* *********** INIT ************** */
	ABIT_LOCAL_INIT();
	/* *********** STEP 6 ************ */
	ABIT_LOCAL_SORT(data_local, 32);
	/* *********** STEP 5 ************ */
	ABIT_LOCAL_SORT(data_local, 16);
	/* *********** STEP 4 ************ */
	ABIT_LOCAL_SORT(data_local, 8);
	/* *********** STEP 3 ************ */
	ABIT_LOCAL_SORT(data_local, 4);
	/* ********** STEP 2 ************** */
	ABIT_LOCAL_SORT(data_local, 2);
	/* ********** STEP 1 ************** */
	ABIT_LOCAL_SORT(data_local, 1);
	/* ********* FINISH *********** */
	ABIT_LOCAL_FINISH();

}

/**
 * @brief This kernel can perform the seven last steps of a stage in a
 * bitonic sort.
 * 
 * @param data_global Array of data to sort.
 * @param stage
 * @param data_local
 */
__kernel void abit_local_s7(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local)
{

	/* *********** INIT ************** */
	ABIT_LOCAL_INIT();
	/* *********** STEP 7 ************ */
	ABIT_LOCAL_SORT(data_local, 64);
	/* *********** STEP 6 ************ */
	ABIT_LOCAL_SORT(data_local, 32);
	/* *********** STEP 5 ************ */
	ABIT_LOCAL_SORT(data_local, 16);
	/* *********** STEP 4 ************ */
	ABIT_LOCAL_SORT(data_local, 8);
	/* *********** STEP 3 ************ */
	ABIT_LOCAL_SORT(data_local, 4);
	/* ********** STEP 2 ************** */
	ABIT_LOCAL_SORT(data_local, 2);
	/* ********** STEP 1 ************** */
	ABIT_LOCAL_SORT(data_local, 1);
	/* ********* FINISH *********** */
	ABIT_LOCAL_FINISH();

}

/**
 * @brief This kernel can perform the eight last steps of a stage in a
 * bitonic sort.
 * 
 * @param data_global Array of data to sort.
 * @param stage
 * @param data_local
 */
__kernel void abit_local_s8(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local)
{

	/* *********** INIT ************** */
	ABIT_LOCAL_INIT();
	/* *********** STEP 8 ************ */
	ABIT_LOCAL_SORT(data_local, 128);
	/* *********** STEP 7 ************ */
	ABIT_LOCAL_SORT(data_local, 64);
	/* *********** STEP 6 ************ */
	ABIT_LOCAL_SORT(data_local, 32);
	/* *********** STEP 5 ************ */
	ABIT_LOCAL_SORT(data_local, 16);
	/* *********** STEP 4 ************ */
	ABIT_LOCAL_SORT(data_local, 8);
	/* *********** STEP 3 ************ */
	ABIT_LOCAL_SORT(data_local, 4);
	/* ********** STEP 2 ************** */
	ABIT_LOCAL_SORT(data_local, 2);
	/* ********** STEP 1 ************** */
	ABIT_LOCAL_SORT(data_local, 1);
	/* ********* FINISH *********** */
	ABIT_LOCAL_FINISH();

}


/**
 * @brief This kernel can perform the nine last steps of a stage in a
 * bitonic sort.
 * 
 * @param data_global Array of data to sort.
 * @param stage
 * @param data_local
 */
__kernel void abit_local_s9(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local)
{

	/* *********** INIT ************** */
	ABIT_LOCAL_INIT();
	/* *********** STEP 9 ************ */
	ABIT_LOCAL_SORT(data_local, 256);
	/* *********** STEP 8 ************ */
	ABIT_LOCAL_SORT(data_local, 128);
	/* *********** STEP 7 ************ */
	ABIT_LOCAL_SORT(data_local, 64);
	/* *********** STEP 6 ************ */
	ABIT_LOCAL_SORT(data_local, 32);
	/* *********** STEP 5 ************ */
	ABIT_LOCAL_SORT(data_local, 16);
	/* *********** STEP 4 ************ */
	ABIT_LOCAL_SORT(data_local, 8);
	/* *********** STEP 3 ************ */
	ABIT_LOCAL_SORT(data_local, 4);
	/* ********** STEP 2 ************** */
	ABIT_LOCAL_SORT(data_local, 2);
	/* ********** STEP 1 ************** */
	ABIT_LOCAL_SORT(data_local, 1);
	/* ********* FINISH *********** */
	ABIT_LOCAL_FINISH();

}

/**
 * @brief This kernel can perform the ten last steps of a stage in a
 * bitonic sort.
 * 
 * @param data_global Array of data to sort.
 * @param stage
 * @param data_local
 */
__kernel void abit_local_s10(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local)
{

	/* *********** INIT ************** */
	ABIT_LOCAL_INIT();
	/* *********** STEP 10 ************ */
	ABIT_LOCAL_SORT(data_local, 512);
	/* *********** STEP 9 ************ */
	ABIT_LOCAL_SORT(data_local, 256);
	/* *********** STEP 8 ************ */
	ABIT_LOCAL_SORT(data_local, 128);
	/* *********** STEP 7 ************ */
	ABIT_LOCAL_SORT(data_local, 64);
	/* *********** STEP 6 ************ */
	ABIT_LOCAL_SORT(data_local, 32);
	/* *********** STEP 5 ************ */
	ABIT_LOCAL_SORT(data_local, 16);
	/* *********** STEP 4 ************ */
	ABIT_LOCAL_SORT(data_local, 8);
	/* *********** STEP 3 ************ */
	ABIT_LOCAL_SORT(data_local, 4);
	/* ********** STEP 2 ************** */
	ABIT_LOCAL_SORT(data_local, 2);
	/* ********** STEP 1 ************** */
	ABIT_LOCAL_SORT(data_local, 1);
	/* ********* FINISH *********** */
	ABIT_LOCAL_FINISH();

}

/**
 * @brief This kernel can perform the eleven last steps of a stage in a
 * bitonic sort.
 * 
 * @param data_global Array of data to sort.
 * @param stage
 * @param data_local
 */
__kernel void abit_local_s11(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local)
{

	/* *********** INIT ************** */
	ABIT_LOCAL_INIT();
	/* *********** STEP 11 ************ */
	ABIT_LOCAL_SORT(data_local, 1024);
	/* *********** STEP 10 ************ */
	ABIT_LOCAL_SORT(data_local, 512);
	/* *********** STEP 9 ************ */
	ABIT_LOCAL_SORT(data_local, 256);
	/* *********** STEP 8 ************ */
	ABIT_LOCAL_SORT(data_local, 128);
	/* *********** STEP 7 ************ */
	ABIT_LOCAL_SORT(data_local, 64);
	/* *********** STEP 6 ************ */
	ABIT_LOCAL_SORT(data_local, 32);
	/* *********** STEP 5 ************ */
	ABIT_LOCAL_SORT(data_local, 16);
	/* *********** STEP 4 ************ */
	ABIT_LOCAL_SORT(data_local, 8);
	/* *********** STEP 3 ************ */
	ABIT_LOCAL_SORT(data_local, 4);
	/* ********** STEP 2 ************** */
	ABIT_LOCAL_SORT(data_local, 2);
	/* ********** STEP 1 ************** */
	ABIT_LOCAL_SORT(data_local, 1);
	/* ********* FINISH *********** */
	ABIT_LOCAL_FINISH();

}

/**
 * @brief This kernel can perform any step of any stage of a bitonic
 * sort.
 * 
 * @param data Array of data to sort.
 * @param stage
 * @param step
 */
__kernel void abit_any(
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
	ABIT_CMPXCH(data, index1, index2);
	
}

/* Each thread sorts 4 values (in two steps of a bitonic stage).
 * Assumes gws = numel2sort / 4 */
__kernel void abit_priv_2s4v(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			uint step)
{

	ABIT_PRIV_INIT(4);
	
	/* ***** Transfer 4 values to sort to private memory ***** */
	ABIT_SORT_LOAD4(data_priv, data_global, gaddr, inc);

	/* ***** Sort the 4 values ***** */
	ABIT_SORT_2S4V(data_priv);

	/* ***** Transfer the 4 values to global memory ***** */
	ABIT_SORT_STORE4(data_priv, data_global, gaddr, inc);

}


/* Each thread sorts 8 values (in three steps of a bitonic stage).
 * Assumes gws = numel2sort / 8 */
__kernel void abit_priv_3s8v(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			uint step)
{

	ABIT_PRIV_INIT(8);
	
	/* ***** Transfer 8 values to sort to private memory ***** */
	ABIT_SORT_LOAD8(data_priv, data_global, gaddr, inc);

	/* ***** Sort the 8 values ***** */
	ABIT_SORT_3S8V(data_priv);

	/* ***** Transfer the n values to global memory ***** */
	ABIT_SORT_STORE8(data_priv, data_global, gaddr, inc);

}

/* Each thread sorts 16 values (in three steps of a bitonic stage).
 * Assumes gws = numel2sort / 16 */
__kernel void abit_priv_4s16v(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			uint step)
{

	ABIT_PRIV_INIT(16);
	
	/* ***** Transfer 16 values to sort to private memory ***** */
	ABIT_SORT_LOAD16(data_priv, data_global, gaddr, inc);

	/* ***** Sort the 16 values ***** */
	ABIT_SORT_4S16V(data_priv);

	/* ***** Transfer the n values to global memory ***** */
	ABIT_SORT_STORE16(data_priv, data_global, gaddr, inc);

}

#define ABIT_HYB_2S4V_INIT() \
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
	
#define ABIT_HYB_2S4V_FINISH() \
	/* Store data globally */ \
	data_global[global_index1] = data_local[local_index1]; \
	data_global[global_index2] = data_local[local_index2]; \
	data_global[global_index3] = data_local[local_index3]; \
	data_global[global_index4] = data_local[local_index4];
	
#define ABIT_HYB_2S4V_SORT(step) \
	/* ***** Transfer 4 values to sort from local to private memory ***** */ \
	blockSize = 1 << step; \
	laddr = ((lid * 4) / blockSize) * blockSize + (lid % (blockSize / 4)); \
	inc = blockSize / 4; \
	ABIT_SORT_LOAD4(data_priv, data_local, laddr, inc); \
	/* ***** Sort the 4 values ***** */ \
	ABIT_SORT_2S4V(data_priv); \
	/* ***** Transfer 4 sorted values from private to local memory ***** */ \
	ABIT_SORT_STORE4(data_priv, data_local, laddr, inc); \
	/* Local memory barrier */ \
	barrier(CLK_LOCAL_MEM_FENCE);


/* Works from step 4 to step 1, local barriers between each two steps,
 * each thread sorts 4 values. */
__kernel void abit_hyb_s4_2s4v(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local) 
{
	ABIT_HYB_2S4V_INIT();
	ABIT_HYB_2S4V_SORT(4);
	ABIT_HYB_2S4V_SORT(2);
	ABIT_HYB_2S4V_FINISH();				
}


/* Works from step 6 to step 1, local barriers between each two steps,
 * each thread sorts 4 values. */
__kernel void abit_hyb_s6_2s4v(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local) 
{
	ABIT_HYB_2S4V_INIT();
	ABIT_HYB_2S4V_SORT(6);
	ABIT_HYB_2S4V_SORT(4);
	ABIT_HYB_2S4V_SORT(2);
	ABIT_HYB_2S4V_FINISH();				
}


/* Works from step 8 to step 1, local barriers between each two steps,
 * each thread sorts 4 values. */
__kernel void abit_hyb_s8_2s4v(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local) 
{
	ABIT_HYB_2S4V_INIT();
	ABIT_HYB_2S4V_SORT(8);
	ABIT_HYB_2S4V_SORT(6);
	ABIT_HYB_2S4V_SORT(4);
	ABIT_HYB_2S4V_SORT(2);
	ABIT_HYB_2S4V_FINISH();				
}

/* Works from step 10 to step 1, local barriers between each two steps,
 * each thread sorts 4 values. */
__kernel void abit_hyb_s10_2s4v(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local) 
{
	ABIT_HYB_2S4V_INIT();
	ABIT_HYB_2S4V_SORT(10);
	ABIT_HYB_2S4V_SORT(8);
	ABIT_HYB_2S4V_SORT(6);
	ABIT_HYB_2S4V_SORT(4);
	ABIT_HYB_2S4V_SORT(2);
	ABIT_HYB_2S4V_FINISH();				
}

/* Works from step 12 to step 1, local barriers between each two steps,
 * each thread sorts 4 values. */
__kernel void abit_hyb_s12_2s4v(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local) 
{
	ABIT_HYB_2S4V_INIT();
	ABIT_HYB_2S4V_SORT(12);
	ABIT_HYB_2S4V_SORT(10);
	ABIT_HYB_2S4V_SORT(8);
	ABIT_HYB_2S4V_SORT(6);
	ABIT_HYB_2S4V_SORT(4);
	ABIT_HYB_2S4V_SORT(2);
	ABIT_HYB_2S4V_FINISH();				
}


#define ABIT_HYB_3S8V_INIT() \
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
	
#define ABIT_HYB_3S8V_FINISH() \
	/* Store data globally */ \
	data_global[global_index1] = data_local[local_index1]; \
	data_global[global_index2] = data_local[local_index2]; \
	data_global[global_index3] = data_local[local_index3]; \
	data_global[global_index4] = data_local[local_index4]; \
	data_global[global_index5] = data_local[local_index5]; \
	data_global[global_index6] = data_local[local_index6]; \
	data_global[global_index7] = data_local[local_index7]; \
	data_global[global_index8] = data_local[local_index8];
	
#define ABIT_HYB_3S8V_SORT(step) \
	/* ***** Transfer 8 values to sort from local to private memory ***** */ \
	blockSize = 1 << step; \
	laddr = ((lid * 8) / blockSize) * blockSize + (lid % (blockSize / 8)); \
	inc = blockSize / 8; \
	ABIT_SORT_LOAD8(data_priv, data_local, laddr, inc); \
	/* ***** Sort the 8 values ***** */ \
	ABIT_SORT_3S8V(data_priv); \
	/* ***** Transfer 4 sorted values from private to local memory ***** */ \
	ABIT_SORT_STORE8(data_priv, data_local, laddr, inc); \
	/* Local memory barrier */ \
	barrier(CLK_LOCAL_MEM_FENCE);


/* Works from step 3 to step 1, local barriers between each three steps,
 * each thread sorts 8 values. */
__kernel void abit_hyb_s3_3s8v(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local) 
{
	ABIT_HYB_3S8V_INIT();
	ABIT_HYB_3S8V_SORT(3);
	ABIT_HYB_3S8V_FINISH();				
}

/* Works from step 6 to step 1, local barriers between each three steps,
 * each thread sorts 8 values. */
__kernel void abit_hyb_s6_3s8v(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local) 
{
	ABIT_HYB_3S8V_INIT();
	ABIT_HYB_3S8V_SORT(6);
	ABIT_HYB_3S8V_SORT(3);
	ABIT_HYB_3S8V_FINISH();				
}

/* Works from step 9 to step 1, local barriers between each three steps,
 * each thread sorts 8 values. */
__kernel void abit_hyb_s9_3s8v(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local) 
{
	ABIT_HYB_3S8V_INIT();
	ABIT_HYB_3S8V_SORT(9);
	ABIT_HYB_3S8V_SORT(6);
	ABIT_HYB_3S8V_SORT(3);
	ABIT_HYB_3S8V_FINISH();				
}

/* Works from step 12 to step 1, local barriers between each three steps,
 * each thread sorts 8 values. */
__kernel void abit_hyb_s12_3s8v(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local) 
{
	ABIT_HYB_3S8V_INIT();
	ABIT_HYB_3S8V_SORT(12);
	ABIT_HYB_3S8V_SORT(9);
	ABIT_HYB_3S8V_SORT(6);
	ABIT_HYB_3S8V_SORT(3);
	ABIT_HYB_3S8V_FINISH();				
}

#define ABIT_HYB_4S16V_INIT() \
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
	
#define ABIT_HYB_4S16V_FINISH() \
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
	
#define ABIT_HYB_4S16V_SORT(step) \
	/* ***** Transfer 16 values to sort from local to private memory ***** */ \
	blockSize = 1 << step; \
	laddr = ((lid * 16) / blockSize) * blockSize + (lid % (blockSize / 16)); \
	inc = blockSize / 16; \
	ABIT_SORT_LOAD16(data_priv, data_local, laddr, inc); \
	/* ***** Sort the 16 values ***** */ \
	ABIT_SORT_4S16V(data_priv); \
	/* ***** Transfer 16 sorted values from private to local memory ***** */ \
	ABIT_SORT_STORE16(data_priv, data_local, laddr, inc); \
	/* Local memory barrier */ \
	barrier(CLK_LOCAL_MEM_FENCE);


/* Works from step 4 to step 1, local barriers between each four steps,
 * each thread sorts 16 values. */
__kernel void abit_hyb_s4_4s16v(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local) 
{
	ABIT_HYB_4S16V_INIT();
	ABIT_HYB_4S16V_SORT(4);
	ABIT_HYB_4S16V_FINISH();				
}

/* Works from step 8 to step 1, local barriers between each four steps,
 * each thread sorts 16 values. */
__kernel void abit_hyb_s8_4s16v(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local) 
{
	ABIT_HYB_4S16V_INIT();
	ABIT_HYB_4S16V_SORT(8);
	ABIT_HYB_4S16V_SORT(4);
	ABIT_HYB_4S16V_FINISH();				
}

/* Works from step 12 to step 1, local barriers between each four steps,
 * each thread sorts 16 values. */
__kernel void abit_hyb_s12_4s16v(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			__local CLO_SORT_ELEM_TYPE *data_local) 
{
	ABIT_HYB_4S16V_INIT();
	ABIT_HYB_4S16V_SORT(12);
	ABIT_HYB_4S16V_SORT(8);
	ABIT_HYB_4S16V_SORT(4);
	ABIT_HYB_4S16V_FINISH();				
}

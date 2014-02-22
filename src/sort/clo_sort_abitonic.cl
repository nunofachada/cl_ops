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
 
#define CLO_SORT_ABITONIC_STEP(stride) \
  	/* Determine what to compare and possibly swap. */ \
	index1 = (lid / stride) * stride * 2 + (lid % stride); \
	index2 = index1 + stride; \
	/* Get elements from global memory. */ \
	data1 = data_local[index1]; \
	data2 = data_local[index2]; \
	/* Determine if it's required to swap the elements. */ \
	swap = CLO_SORT_COMPARE(data1, data2) ^ desc;  \
	/* Perform swap if needed */ \
	if (swap) { \
		data_local[index1] = data2;  \
		data_local[index2] = data1;  \
	} \
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
	/* Swap or not to swap? */ \
	bool swap; \
	/* Index of values to possibly swap. */ \
	uint index1, index2; \
	/* Data elements to possibly swap. */ \
	CLO_SORT_ELEM_TYPE data1, data2;
	
#define CLO_SORT_ABITONIC_FINISH() \	
	/* Store data globally */ \
	data_global[global_index1] = data_local[local_index1]; \
	data_global[global_index2] = data_local[local_index2]; \
	
#define CLO_SORT_ABITONIC_CMPXCH(index1, index2) \
	data1 = data_local[index1]; \
	data2 = data_local[index2]; \
	swap = CLO_SORT_COMPARE(data1, data2) ^ desc; \
	if (swap) { \
		data_local[index1] = data2; \
		data_local[index2] = data1; \
	}

#define CLO_SORT_ABITONIC_GCMPXCH(index1, index2) \
	data1 = data_global[index1]; \
	data2 = data_global[index2]; \
	swap = CLO_SORT_COMPARE(data1, data2) ^ desc; \
	if (swap) { \
		data_global[index1] = data2; \
		data_global[index2] = data1; \
	}
	

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
	CLO_SORT_ABITONIC_STEP(2);
	/* ********** STEP 1 ************** */
	CLO_SORT_ABITONIC_STEP(1);
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
	CLO_SORT_ABITONIC_STEP(4);
	/* ********** STEP 2 ************** */
	CLO_SORT_ABITONIC_STEP(2);
	/* ********** STEP 1 ************** */
	CLO_SORT_ABITONIC_STEP(1);
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
	CLO_SORT_ABITONIC_STEP(8);
	/* *********** STEP 3 ************ */
	CLO_SORT_ABITONIC_STEP(4);
	/* ********** STEP 2 ************** */
	CLO_SORT_ABITONIC_STEP(2);
	/* ********** STEP 1 ************** */
	CLO_SORT_ABITONIC_STEP(1);
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
	CLO_SORT_ABITONIC_STEP(16);
	/* *********** STEP 4 ************ */
	CLO_SORT_ABITONIC_STEP(8);
	/* *********** STEP 3 ************ */
	CLO_SORT_ABITONIC_STEP(4);
	/* ********** STEP 2 ************** */
	CLO_SORT_ABITONIC_STEP(2);
	/* ********** STEP 1 ************** */
	CLO_SORT_ABITONIC_STEP(1);
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
	CLO_SORT_ABITONIC_STEP(32);
	/* *********** STEP 5 ************ */
	CLO_SORT_ABITONIC_STEP(16);
	/* *********** STEP 4 ************ */
	CLO_SORT_ABITONIC_STEP(8);
	/* *********** STEP 3 ************ */
	CLO_SORT_ABITONIC_STEP(4);
	/* ********** STEP 2 ************** */
	CLO_SORT_ABITONIC_STEP(2);
	/* ********** STEP 1 ************** */
	CLO_SORT_ABITONIC_STEP(1);
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
	CLO_SORT_ABITONIC_STEP(64);
	/* *********** STEP 6 ************ */
	CLO_SORT_ABITONIC_STEP(32);
	/* *********** STEP 5 ************ */
	CLO_SORT_ABITONIC_STEP(16);
	/* *********** STEP 4 ************ */
	CLO_SORT_ABITONIC_STEP(8);
	/* *********** STEP 3 ************ */
	CLO_SORT_ABITONIC_STEP(4);
	/* ********** STEP 2 ************** */
	CLO_SORT_ABITONIC_STEP(2);
	/* ********** STEP 1 ************** */
	CLO_SORT_ABITONIC_STEP(1);
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
	CLO_SORT_ABITONIC_STEP(128);
	/* *********** STEP 7 ************ */
	CLO_SORT_ABITONIC_STEP(64);
	/* *********** STEP 6 ************ */
	CLO_SORT_ABITONIC_STEP(32);
	/* *********** STEP 5 ************ */
	CLO_SORT_ABITONIC_STEP(16);
	/* *********** STEP 4 ************ */
	CLO_SORT_ABITONIC_STEP(8);
	/* *********** STEP 3 ************ */
	CLO_SORT_ABITONIC_STEP(4);
	/* ********** STEP 2 ************** */
	CLO_SORT_ABITONIC_STEP(2);
	/* ********** STEP 1 ************** */
	CLO_SORT_ABITONIC_STEP(1);
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
	CLO_SORT_ABITONIC_STEP(256);
	/* *********** STEP 8 ************ */
	CLO_SORT_ABITONIC_STEP(128);
	/* *********** STEP 7 ************ */
	CLO_SORT_ABITONIC_STEP(64);
	/* *********** STEP 6 ************ */
	CLO_SORT_ABITONIC_STEP(32);
	/* *********** STEP 5 ************ */
	CLO_SORT_ABITONIC_STEP(16);
	/* *********** STEP 4 ************ */
	CLO_SORT_ABITONIC_STEP(8);
	/* *********** STEP 3 ************ */
	CLO_SORT_ABITONIC_STEP(4);
	/* ********** STEP 2 ************** */
	CLO_SORT_ABITONIC_STEP(2);
	/* ********** STEP 1 ************** */
	CLO_SORT_ABITONIC_STEP(1);
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
	CLO_SORT_ABITONIC_STEP(512);
	/* *********** STEP 9 ************ */
	CLO_SORT_ABITONIC_STEP(256);
	/* *********** STEP 8 ************ */
	CLO_SORT_ABITONIC_STEP(128);
	/* *********** STEP 7 ************ */
	CLO_SORT_ABITONIC_STEP(64);
	/* *********** STEP 6 ************ */
	CLO_SORT_ABITONIC_STEP(32);
	/* *********** STEP 5 ************ */
	CLO_SORT_ABITONIC_STEP(16);
	/* *********** STEP 4 ************ */
	CLO_SORT_ABITONIC_STEP(8);
	/* *********** STEP 3 ************ */
	CLO_SORT_ABITONIC_STEP(4);
	/* ********** STEP 2 ************** */
	CLO_SORT_ABITONIC_STEP(2);
	/* ********** STEP 1 ************** */
	CLO_SORT_ABITONIC_STEP(1);
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
	CLO_SORT_ABITONIC_STEP(1024);
	/* *********** STEP 10 ************ */
	CLO_SORT_ABITONIC_STEP(512);
	/* *********** STEP 9 ************ */
	CLO_SORT_ABITONIC_STEP(256);
	/* *********** STEP 8 ************ */
	CLO_SORT_ABITONIC_STEP(128);
	/* *********** STEP 7 ************ */
	CLO_SORT_ABITONIC_STEP(64);
	/* *********** STEP 6 ************ */
	CLO_SORT_ABITONIC_STEP(32);
	/* *********** STEP 5 ************ */
	CLO_SORT_ABITONIC_STEP(16);
	/* *********** STEP 4 ************ */
	CLO_SORT_ABITONIC_STEP(8);
	/* *********** STEP 3 ************ */
	CLO_SORT_ABITONIC_STEP(4);
	/* ********** STEP 2 ************** */
	CLO_SORT_ABITONIC_STEP(2);
	/* ********** STEP 1 ************** */
	CLO_SORT_ABITONIC_STEP(1);
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
	
	/* Get hashes from global memory. */
	CLO_SORT_ELEM_TYPE data1 = data[index1];
	CLO_SORT_ELEM_TYPE data2 = data[index2];
		
	/* Determine it is required to swap the agents. */
	bool swap = CLO_SORT_COMPARE(data1, data2) ^ desc; 
		
	/* Perform swap if needed */ 
	if (swap) {
		data[index1] = data2; 
		data[index2] = data1; 
	}
		
}

/* Each thread sorts 8 values (in three steps of a bitonic stage).
 * Assumes gws = numel2sort / 8 */
__kernel void abitonic_s8(
			__global CLO_SORT_ELEM_TYPE *data_global,
			uint stage,
			uint step,
			__local CLO_SORT_ELEM_TYPE *data_local)
{

	bool swap;
	uint index1, index2;
	CLO_SORT_ELEM_TYPE data1;
	CLO_SORT_ELEM_TYPE data2;
	
	/* Thread information. */
	uint gid = get_global_id(0);
	uint lid = get_local_id(0);
	uint gws = get_global_size(0);
	
	/* Determine block size. */
	uint I = 1 << step;
	
	/* Determine number of blocks. */
	uint B = gws * 8 / I;
	
	/* Determine block id. */
	uint bid = (gid * 8) / I;
	
	/* Determine if ascending or descending. Ascending if block id is
	 * pair, descending otherwise. */
	bool desc = (bool) (0x1 & ((gid * 8) / (1 << stage)));
	
	/* Thread id in block. */
	uint tid = gid % (I/8);
		
	/* Base global address to load/store values from/to. */
	uint gaddr = bid * I + tid;
	
	//~ /* ***** Transfer 8 values to sort to local memory ***** */
	for (uint i = 0; i < 8; i++)
		data_local[lid * 8 + i] = data_global[gaddr + i * I/8];
	
	/* ***** Sort the 8 values ***** */
			
	/* Step n */
	CLO_SORT_ABITONIC_CMPXCH(lid * 8 + 0, lid * 8 + 4);
	CLO_SORT_ABITONIC_CMPXCH(lid * 8 + 1, lid * 8 + 5);
	CLO_SORT_ABITONIC_CMPXCH(lid * 8 + 2, lid * 8 + 6);
	CLO_SORT_ABITONIC_CMPXCH(lid * 8 + 3, lid * 8 + 7);
	/* Step n-1 */
	CLO_SORT_ABITONIC_CMPXCH(lid * 8 + 0, lid * 8 + 2);
	CLO_SORT_ABITONIC_CMPXCH(lid * 8 + 1, lid * 8 + 3);
	CLO_SORT_ABITONIC_CMPXCH(lid * 8 + 4, lid * 8 + 6);
	CLO_SORT_ABITONIC_CMPXCH(lid * 8 + 5, lid * 8 + 7);
	/* Step n-2 */
	CLO_SORT_ABITONIC_CMPXCH(lid * 8 + 0, lid * 8 + 1);
	CLO_SORT_ABITONIC_CMPXCH(lid * 8 + 2, lid * 8 + 3);
	CLO_SORT_ABITONIC_CMPXCH(lid * 8 + 4, lid * 8 + 5);
	CLO_SORT_ABITONIC_CMPXCH(lid * 8 + 6, lid * 8 + 7);

	//~ /* Step n */
	//~ CLO_SORT_ABITONIC_GCMPXCH(gaddr + 0 * I/8, gaddr + 4 * I/8);
	//~ CLO_SORT_ABITONIC_GCMPXCH(gaddr + 1 * I/8, gaddr + 5 * I/8);
	//~ CLO_SORT_ABITONIC_GCMPXCH(gaddr + 2 * I/8, gaddr + 6 * I/8);
	//~ CLO_SORT_ABITONIC_GCMPXCH(gaddr + 3 * I/8, gaddr + 7 * I/8);
	//~ /* Step n-1 */
	//~ CLO_SORT_ABITONIC_GCMPXCH(gaddr + 0 * I/8, gaddr + 2 * I/8);
	//~ CLO_SORT_ABITONIC_GCMPXCH(gaddr + 1 * I/8, gaddr + 3 * I/8);
	//~ CLO_SORT_ABITONIC_GCMPXCH(gaddr + 4 * I/8, gaddr + 6 * I/8);
	//~ CLO_SORT_ABITONIC_GCMPXCH(gaddr + 5 * I/8, gaddr + 7 * I/8);
	//~ /* Step n-2 */
	//~ CLO_SORT_ABITONIC_GCMPXCH(gaddr + 0 * I/8, gaddr + 1 * I/8);
	//~ CLO_SORT_ABITONIC_GCMPXCH(gaddr + 2 * I/8, gaddr + 3 * I/8);
	//~ CLO_SORT_ABITONIC_GCMPXCH(gaddr + 4 * I/8, gaddr + 5 * I/8);
	//~ CLO_SORT_ABITONIC_GCMPXCH(gaddr + 6 * I/8, gaddr + 7 * I/8);

	//~ /* ***** Transfer the n values to global memory ***** */
	for (uint i = 0; i < 8; i++)
		data_global[gaddr + i * I/8] = data_local[lid * 8 + i];
}

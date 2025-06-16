#include "RadixSort.h"
#define MAX_BLOCK_SZ 128
#define NUM_BANKS 32
#define LOG_NUM_BANKS 5

//#define ZERO_BANK_CONFLICTS
#ifdef ZERO_BANK_CONFLICTS
#   define CONFLICT_FREE_OFFSET(n) \
    ((n) >> NUM_BANKS + (n) >> (2 * LOG_NUM_BANKS))
#else
#   define CONFLICT_FREE_OFFSET(n) ((n) >> LOG_NUM_BANKS)
#endif

__global__
void gpu_add_block_sums(unsigned int* const d_out,
    const unsigned int* const d_in,
    unsigned int* const d_block_sums,
    const size_t numElems)
{
    //unsigned int glbl_t_idx = blockDim.x * blockIdx.x + threadIdx.x;
    unsigned int d_block_sum_val = d_block_sums[blockIdx.x];
    //unsigned int d_in_val_0 = 0;
    //unsigned int d_in_val_1 = 0;

    // Simple implementation's performance is not significantly (if at all)
    //  better than previous verbose implementation
    unsigned int cpy_idx = 2 * blockIdx.x * blockDim.x + threadIdx.x;
    if (cpy_idx < numElems)
    {
        d_out[cpy_idx] = d_in[cpy_idx] + d_block_sum_val;
        if (cpy_idx + blockDim.x < numElems)
            d_out[cpy_idx + blockDim.x] = d_in[cpy_idx + blockDim.x] + d_block_sum_val;
    }

    //if (2 * glbl_t_idx < numElems)
    //{
    //    d_out[2 * glbl_t_idx] = d_in[2 * glbl_t_idx] + d_block_sum_val;
    //    if (2 * glbl_t_idx + 1 < numElems)
    //        d_out[2 * glbl_t_idx + 1] = d_in[2 * glbl_t_idx + 1] + d_block_sum_val;
    //}

    //if (2 * glbl_t_idx < numElems)
    //{
    //    d_in_val_0 = d_in[2 * glbl_t_idx];
    //    if (2 * glbl_t_idx + 1 < numElems)
    //        d_in_val_1 = d_in[2 * glbl_t_idx + 1];
    //}
    //else
    //    return;
    //__syncthreads();

    //d_out[2 * glbl_t_idx] = d_in_val_0 + d_block_sum_val;
    //if (2 * glbl_t_idx + 1 < numElems)
    //    d_out[2 * glbl_t_idx + 1] = d_in_val_1 + d_block_sum_val;
}

// Modified version of Mark Harris' implementation of the Blelloch scan
//  according to https://www.mimuw.edu.pl/~ps209291/kgkp/slides/scan.pdf
__global__
void gpu_prescan(unsigned int* const d_out,
    const unsigned int* const d_in,
    unsigned int* const d_block_sums,
    const unsigned int len,
    const unsigned int shmem_sz,
    const unsigned int max_elems_per_block)
{
    // Allocated on invocation
    extern __shared__ unsigned int s_out[];
    int thid = threadIdx.x;
    int ai = thid;
    int bi = thid + blockDim.x;

    // Zero out the shared memory
    // Helpful especially when input size is not power of two
    s_out[thid] = 0;
    s_out[thid + blockDim.x] = 0;
    // If CONFLICT_FREE_OFFSET is used, shared memory size
    //  must be a 2 * blockDim.x + blockDim.x/num_banks
    s_out[thid + blockDim.x + (blockDim.x >> LOG_NUM_BANKS)] = 0;
    __syncthreads();

    // Copy d_in to shared memory
    // Note that d_in's elements are scattered into shared memory
    //  in light of avoiding bank conflicts
    unsigned int cpy_idx = max_elems_per_block * blockIdx.x + threadIdx.x;
    if (cpy_idx < len)
    {
        s_out[ai + CONFLICT_FREE_OFFSET(ai)] = d_in[cpy_idx];
        if (cpy_idx + blockDim.x < len)
            s_out[bi + CONFLICT_FREE_OFFSET(bi)] = d_in[cpy_idx + blockDim.x];
    }

    // For both upsweep and downsweep:
    // Sequential indices with conflict free padding
    //  Amount of padding = target index / num banks
    //  This "shifts" the target indices by one every multiple
    //   of the num banks
    // offset controls the stride and starting index of 
    //  target elems at every iteration
    // d just controls which threads are active
    // Sweeps are pivoted on the last element of shared memory

    // Upsweep/Reduce step
    int offset = 1;
    for (int d = max_elems_per_block >> 1; d > 0; d >>= 1)
    {
        __syncthreads();
        if (thid < d)
        {
            int ai = offset * ((thid << 1) + 1) - 1;
            int bi = offset * ((thid << 1) + 2) - 1;
            ai += CONFLICT_FREE_OFFSET(ai);
            bi += CONFLICT_FREE_OFFSET(bi);

            s_out[bi] += s_out[ai];
        }
        offset <<= 1;
    }

    // Save the total sum on the global block sums array
    // Then clear the last element on the shared memory
    if (thid == 0)
    {
        d_block_sums[blockIdx.x] = s_out[max_elems_per_block - 1
            + CONFLICT_FREE_OFFSET(max_elems_per_block - 1)];
        s_out[max_elems_per_block - 1
            + CONFLICT_FREE_OFFSET(max_elems_per_block - 1)] = 0;
    }

    // Downsweep step
    for (int d = 1; d < max_elems_per_block; d <<= 1)
    {
        offset >>= 1;
        __syncthreads();
        if (thid < d)
        {
            int ai = offset * ((thid << 1) + 1) - 1;
            int bi = offset * ((thid << 1) + 2) - 1;
            ai += CONFLICT_FREE_OFFSET(ai);
            bi += CONFLICT_FREE_OFFSET(bi);

            unsigned int temp = s_out[ai];
            s_out[ai] = s_out[bi];
            s_out[bi] += temp;
        }
    }
    __syncthreads();

    // Copy contents of shared memory to global memory
    if (cpy_idx < len)
    {
        d_out[cpy_idx] = s_out[ai + CONFLICT_FREE_OFFSET(ai)];
        if (cpy_idx + blockDim.x < len)
            d_out[cpy_idx + blockDim.x] = s_out[bi + CONFLICT_FREE_OFFSET(bi)];
    }
}

void sum_scan_blelloch(unsigned int* const d_out,
    const unsigned int* const d_in,
    const size_t numElems)
{
    // Zero out d_out
    checkCudaErrors(musaMemset(d_out, 0, numElems * sizeof(unsigned int)));

    // Set up number of threads and blocks
    unsigned int block_sz = MAX_BLOCK_SZ / 2;
    unsigned int max_elems_per_block = 2 * block_sz; // due to binary tree nature of algorithm

    // If input size is not power of two, the remainder will still need a whole block
    // Thus, number of blocks must be the ceiling of input size / max elems that a block can handle
    //unsigned int grid_sz = (unsigned int) std::ceil((double) numElems / (double) max_elems_per_block);
    // UPDATE: Instead of using ceiling and risking miscalculation due to precision, just automatically  
    //  add 1 to the grid size when the input size cannot be divided cleanly by the block's capacity
    unsigned int grid_sz = numElems / max_elems_per_block;
    // Take advantage of the fact that integer division drops the decimals
    if (numElems % max_elems_per_block != 0)
        grid_sz += 1;

    // Conflict free padding requires that shared memory be more than 2 * block_sz
    unsigned int shmem_sz = max_elems_per_block + ((max_elems_per_block) >> LOG_NUM_BANKS);

    // Allocate memory for array of total sums produced by each block
    // Array length must be the same as number of blocks
    unsigned int* d_block_sums;
    checkCudaErrors(musaMalloc(&d_block_sums, sizeof(unsigned int) * grid_sz));
    checkCudaErrors(musaMemset(d_block_sums, 0, sizeof(unsigned int) * grid_sz));

    // Sum scan data allocated to each block
    //gpu_sum_scan_blelloch<<<grid_sz, block_sz, sizeof(unsigned int) * max_elems_per_block >>>(d_out, d_in, d_block_sums, numElems);
    gpu_prescan <<<grid_sz, block_sz, sizeof(unsigned int)* shmem_sz >>> (d_out,
        d_in,
        d_block_sums,
        numElems,
        shmem_sz,
        max_elems_per_block);

    // Sum scan total sums produced by each block
    // Use basic implementation if number of total sums is <= 2 * block_sz
    //  (This requires only one block to do the scan)
    if (grid_sz <= max_elems_per_block)
    {
        unsigned int* d_dummy_blocks_sums;
        checkCudaErrors(musaMalloc(&d_dummy_blocks_sums, sizeof(unsigned int)));
        checkCudaErrors(musaMemset(d_dummy_blocks_sums, 0, sizeof(unsigned int)));
        //gpu_sum_scan_blelloch<<<1, block_sz, sizeof(unsigned int) * max_elems_per_block>>>(d_block_sums, d_block_sums, d_dummy_blocks_sums, grid_sz);
        gpu_prescan <<<1, block_sz, sizeof(unsigned int)* shmem_sz >>> (d_block_sums,
            d_block_sums,
            d_dummy_blocks_sums,
            grid_sz,
            shmem_sz,
            max_elems_per_block);
        checkCudaErrors(musaFree(d_dummy_blocks_sums));
    }
    // Else, recurse on this same function as you'll need the full-blown scan
    //  for the block sums
    else
    {
        unsigned int* d_in_block_sums;
        checkCudaErrors(musaMalloc(&d_in_block_sums, sizeof(unsigned int) * grid_sz));
        checkCudaErrors(musaMemcpy(d_in_block_sums, d_block_sums, sizeof(unsigned int) * grid_sz, musaMemcpyDeviceToDevice));
        sum_scan_blelloch(d_block_sums, d_in_block_sums, grid_sz);
        checkCudaErrors(musaFree(d_in_block_sums));
    }

    //// Uncomment to examine block sums
    //unsigned int* h_block_sums = new unsigned int[grid_sz];
    //checkCudaErrors(musaMemcpy(h_block_sums, d_block_sums, sizeof(unsigned int) * grid_sz, musaMemcpyDeviceToHost));
    //std::cout << "Block sums: ";
    //for (int i = 0; i < grid_sz; ++i)
    //{
    //    std::cout << h_block_sums[i] << ", ";
    //}
    //std::cout << std::endl;
    //std::cout << "Block sums length: " << grid_sz << std::endl;
    //delete[] h_block_sums;

    // Add each block's total sum to its scan output
    // in order to get the final, global scanned array
    gpu_add_block_sums <<<grid_sz, block_sz >>> (d_out, d_out, d_block_sums, numElems);
    checkCudaErrors(musaFree(d_block_sums));
}

__global__ void gpu_radix_sort_local(unsigned int* d_out_sorted,
    unsigned int* d_prefix_sums,
    unsigned int* d_block_sums,
    unsigned int input_shift_width,
    unsigned int* d_in,
    unsigned int d_in_len,
    unsigned int max_elems_per_block)
{
    // need shared memory array for:
    // - block's share of the input data (local sort will be put here too)
    // - mask outputs
    // - scanned mask outputs
    // - merged scaned mask outputs ("local prefix sum")
    // - local sums of scanned mask outputs
    // - scanned local sums of scanned mask outputs

    // for all radix combinations:
    //  build mask output for current radix combination
    //  scan mask ouput
    //  store needed value from current prefix sum array to merged prefix sum array
    //  store total sum of mask output (obtained from scan) to global block sum array
    // calculate local sorted address from local prefix sum and scanned mask output's total sums
    // shuffle input block according to calculated local sorted addresses
    // shuffle local prefix sums according to calculated local sorted addresses
    // copy locally sorted array back to global memory
    // copy local prefix sum array back to global memory

    extern __shared__ unsigned int shmem[];
    unsigned int* s_data = shmem;
    // s_mask_out[] will be scanned in place
    unsigned int s_mask_out_len = max_elems_per_block + 1;
    unsigned int* s_mask_out = &s_data[max_elems_per_block];
    unsigned int* s_merged_scan_mask_out = &s_mask_out[s_mask_out_len];
    unsigned int* s_mask_out_sums = &s_merged_scan_mask_out[max_elems_per_block];
    unsigned int* s_scan_mask_out_sums = &s_mask_out_sums[4];
    unsigned int thid = threadIdx.x;

    // Copy block's portion of global input data to shared memory
    unsigned int cpy_idx = max_elems_per_block * blockIdx.x + thid;
    if (cpy_idx < d_in_len)
        s_data[thid] = d_in[cpy_idx];
    else
        s_data[thid] = 0;
    __syncthreads();

    // To extract the correct 2 bits, we first shift the number
    //  to the right until the correct 2 bits are in the 2 LSBs,
    //  then mask on the number with 11 (3) to remove the bits
    //  on the left
    unsigned int t_data = s_data[thid];
    unsigned int t_2bit_extract = (t_data >> input_shift_width) & 3;
    for (unsigned int i = 0; i < 4; ++i)
    {
        // Zero out s_mask_out
        s_mask_out[thid] = 0;
        if (thid == 0)
            s_mask_out[s_mask_out_len - 1] = 0;
        __syncthreads();

        // build bit mask output
        bool val_equals_i = false;
        if (cpy_idx < d_in_len)
        {
            val_equals_i = t_2bit_extract == i;
            s_mask_out[thid] = val_equals_i;
        }
        __syncthreads();

        // Scan mask outputs (Hillis-Steele)
        int partner = 0;
        unsigned int sum = 0;
        unsigned int max_steps = (unsigned int)log2f(max_elems_per_block);
        for (unsigned int d = 0; d < max_steps; d++) {
            partner = thid - (1 << d);
            if (partner >= 0) {
                sum = s_mask_out[thid] + s_mask_out[partner];
            }
            else {
                sum = s_mask_out[thid];
            }
            __syncthreads();
            s_mask_out[thid] = sum;
            __syncthreads();
        }

        // Shift elements to produce the same effect as exclusive scan
        unsigned int cpy_val = 0;
        cpy_val = s_mask_out[thid];
        __syncthreads();
        s_mask_out[thid + 1] = cpy_val;
        __syncthreads();

        if (thid == 0)
        {
            // Zero out first element to produce the same effect as exclusive scan
            s_mask_out[0] = 0;
            unsigned int total_sum = s_mask_out[s_mask_out_len - 1];
            s_mask_out_sums[i] = total_sum;
            d_block_sums[i * gridDim.x + blockIdx.x] = total_sum;
        }
        __syncthreads();

        if (val_equals_i && (cpy_idx < d_in_len))
        {
            s_merged_scan_mask_out[thid] = s_mask_out[thid];
        }
        __syncthreads();
    }

    // Scan mask output sums
    // Just do a naive scan since the array is really small
    if (thid == 0)
    {
        unsigned int run_sum = 0;
        for (unsigned int i = 0; i < 4; ++i)
        {
            s_scan_mask_out_sums[i] = run_sum;
            run_sum += s_mask_out_sums[i];
        }
    }
    __syncthreads();

    if (cpy_idx < d_in_len)
    {
        // Calculate the new indices of the input elements for sorting
        unsigned int t_prefix_sum = s_merged_scan_mask_out[thid];
        unsigned int new_pos = t_prefix_sum + s_scan_mask_out_sums[t_2bit_extract];
        __syncthreads();

        // Shuffle the block's input elements to actually sort them
        // Do this step for greater global memory transfer coalescing
        //  in next step
        s_data[new_pos] = t_data;
        s_merged_scan_mask_out[new_pos] = t_prefix_sum;
        __syncthreads();

        // Copy block - wise prefix sum results to global memory
        // Copy block-wise sort results to global 
        d_prefix_sums[cpy_idx] = s_merged_scan_mask_out[thid];
        d_out_sorted[cpy_idx] = s_data[thid];
    }
}

__global__ void gpu_glbl_shuffle(unsigned int* d_out,
    unsigned int* d_in,
    unsigned int* d_scan_block_sums,
    unsigned int* d_prefix_sums,
    unsigned int input_shift_width,
    unsigned int d_in_len,
    unsigned int max_elems_per_block)
{
    // get d = digit
    // get n = blockIdx
    // get m = local prefix sum array value
    // calculate global position = P_d[n] + m
    // copy input element to final position in d_out

    unsigned int thid = threadIdx.x;
    unsigned int cpy_idx = max_elems_per_block * blockIdx.x + thid;

    if (cpy_idx < d_in_len)
    {
        unsigned int t_data = d_in[cpy_idx];
        unsigned int t_2bit_extract = (t_data >> input_shift_width) & 3;
        unsigned int t_prefix_sum = d_prefix_sums[cpy_idx];
        unsigned int data_glbl_pos = d_scan_block_sums[t_2bit_extract * gridDim.x + blockIdx.x]
            + t_prefix_sum;
        __syncthreads();
        d_out[data_glbl_pos] = t_data;
    }
}

// An attempt at the gpu radix sort variant described in this paper:
// https://vgc.poly.edu/~csilva/papers/cgf.pdf
void radix_sort(unsigned int* const d_out,
    unsigned int* const d_in,
    unsigned int d_in_len)
{
    unsigned int block_sz = MAX_BLOCK_SZ;
    unsigned int max_elems_per_block = block_sz;
    unsigned int grid_sz = d_in_len / max_elems_per_block;
    // Take advantage of the fact that integer division drops the decimals
    if (d_in_len % max_elems_per_block != 0)
        grid_sz += 1;

    unsigned int* d_prefix_sums;
    unsigned int d_prefix_sums_len = d_in_len;
    checkCudaErrors(musaMalloc(&d_prefix_sums, sizeof(unsigned int) * d_prefix_sums_len));
    checkCudaErrors(musaMemset(d_prefix_sums, 0, sizeof(unsigned int) * d_prefix_sums_len));

    unsigned int* d_block_sums;
    unsigned int d_block_sums_len = 4 * grid_sz; // 4-way split
    checkCudaErrors(musaMalloc(&d_block_sums, sizeof(unsigned int) * d_block_sums_len));
    checkCudaErrors(musaMemset(d_block_sums, 0, sizeof(unsigned int) * d_block_sums_len));

    unsigned int* d_scan_block_sums;
    checkCudaErrors(musaMalloc(&d_scan_block_sums, sizeof(unsigned int) * d_block_sums_len));
    checkCudaErrors(musaMemset(d_scan_block_sums, 0, sizeof(unsigned int) * d_block_sums_len));

    // shared memory consists of 3 arrays the size of the block-wise input
    //  and 2 arrays the size of n in the current n-way split (4)
    unsigned int s_data_len = max_elems_per_block;
    unsigned int s_mask_out_len = max_elems_per_block + 1;
    unsigned int s_merged_scan_mask_out_len = max_elems_per_block;
    unsigned int s_mask_out_sums_len = 4; // 4-way split
    unsigned int s_scan_mask_out_sums_len = 4;
    unsigned int shmem_sz = (s_data_len
        + s_mask_out_len
        + s_merged_scan_mask_out_len
        + s_mask_out_sums_len
        + s_scan_mask_out_sums_len)
        * sizeof(unsigned int);

    // for every 2 bits from LSB to MSB:
    //  block-wise radix sort (write blocks back to global memory)
    for (unsigned int shift_width = 0; shift_width <= 30; shift_width += 2)
    {
        gpu_radix_sort_local <<<grid_sz, block_sz, shmem_sz >>> (d_out,
            d_prefix_sums,
            d_block_sums,
            shift_width,
            d_in,
            d_in_len,
            max_elems_per_block);

        //unsigned int* h_test = new unsigned int[d_in_len];
        //checkCudaErrors(musaMemcpy(h_test, d_in, sizeof(unsigned int) * d_in_len, musaMemcpyDeviceToHost));
        //for (unsigned int i = 0; i < d_in_len; ++i)
        //    std::cout << h_test[i] << " ";
        //std::cout << std::endl;
        //delete[] h_test;

        // scan global block sum array
        sum_scan_blelloch(d_scan_block_sums, d_block_sums, d_block_sums_len);

        // scatter/shuffle block-wise sorted array to final positions
        gpu_glbl_shuffle <<<grid_sz, block_sz >>> (d_in,
            d_out,
            d_scan_block_sums,
            d_prefix_sums,
            shift_width,
            d_in_len,
            max_elems_per_block);
    }
    checkCudaErrors(musaMemcpy(d_out, d_in, sizeof(unsigned int) * d_in_len, musaMemcpyDeviceToDevice));
    checkCudaErrors(musaFree(d_scan_block_sums));
    checkCudaErrors(musaFree(d_block_sums));
    checkCudaErrors(musaFree(d_prefix_sums));
}

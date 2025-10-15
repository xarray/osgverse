// See https://github.com/mark-poscablo/gpu-radix-sort/
#pragma once

#include <cmath>
#include <iostream>
#include <stdint.h>
#include <cuda_runtime.h>
#define checkCudaErrors(val) check( (val), #val, __FILE__, __LINE__)

template<typename T>
void check(T err, const char* const func, const char* const file, const int line)
{
    if (err != cudaSuccess)
    {
        std::cerr << "CUDA error at: " << file << ":" << line << std::endl;
        std::cerr << cudaGetErrorString(err) << " " << func << std::endl; exit(1);
    }
}

extern void sum_scan_naive(unsigned int* const d_out,
                          const unsigned int* const d_in,
                          const size_t numElems);
extern void sum_scan_blelloch(unsigned int* const d_out,
                              const unsigned int* const d_in,
                              const size_t numElems);
extern void radix_sort(unsigned int* const d_out,
                       unsigned int* const d_in, unsigned int* const v_in,
                       unsigned int d_in_len);

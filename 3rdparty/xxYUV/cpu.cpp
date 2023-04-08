//==============================================================================
// xxYUV : cpu Source
//
// Copyright (c) 2020-2021 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================
#if defined(_M_IX86) || defined(_M_AMD64) || defined(__i386__) || defined(__amd64__)
#   if defined(_M_IX86) || defined(_M_AMD64)
#       include <intrin.h>
#       define bit_SSSE3    (1 << 9)
#       define bit_AVX2     (1 << 5)
#       define bit_AVX512BW (1 << 30)
        static inline int __get_cpuid(int leaf, unsigned int* eax, unsigned int* ebx, unsigned int* ecx, unsigned int* edx)
        {
            int regs[4];
            __cpuid(regs, leaf);
            *eax = regs[0];
            *ebx = regs[1];
            *ecx = regs[2];
            *edx = regs[3];
            return 1;
        }
        static inline int __get_cpuid_count(int leaf, int subleaf, unsigned int* eax, unsigned int* ebx, unsigned int* ecx, unsigned int* edx)
        {
            int regs[4];
            __cpuidex(regs, leaf, subleaf);
            *eax = regs[0];
            *ebx = regs[1];
            *ecx = regs[2];
            *edx = regs[3];
            return 1;
        }
#   elif defined(__i386__) || defined(__amd64__)
#       include <cpuid.h>
#   endif
#   include <immintrin.h>
#endif
#include "cpu.h"

//------------------------------------------------------------------------------
bool ssse3()
{
#if defined(_M_IX86) || defined(_M_AMD64) || defined(__i386__) || defined(__amd64__)
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx))
    {
        return (ecx & bit_SSSE3) != 0;
    }
#endif
    return false;
}
//------------------------------------------------------------------------------
bool avx2()
{
#if defined(_M_IX86) || defined(_M_AMD64) || defined(__i386__) || defined(__amd64__)
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx))
    {
        return (ebx & bit_AVX2) != 0;
    }
#endif
    return false;
}
//------------------------------------------------------------------------------
bool avx512bw()
{
#if defined(_M_IX86) || defined(_M_AMD64) || defined(__i386__) || defined(__amd64__)
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx))
    {
        return (ebx & bit_AVX512BW) != 0;
    }
#endif
    return false;
}
//------------------------------------------------------------------------------

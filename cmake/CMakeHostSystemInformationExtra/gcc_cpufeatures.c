/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt for details.  */

#include <errno.h>
#include <stdio.h>

#define PRINT_CPU_FEATURE(a) if(__builtin_cpu_supports(a)){printf("%s\n", a);}
int main() {
#if defined(GCC_VERSION)
    __builtin_cpu_init ();
#endif
    // these can be found by gcc8.2
    PRINT_CPU_FEATURE("cmov");
    PRINT_CPU_FEATURE("mmx");
    PRINT_CPU_FEATURE("popcnt");
    PRINT_CPU_FEATURE("sse");
    PRINT_CPU_FEATURE("sse2");
    PRINT_CPU_FEATURE("sse3");
    PRINT_CPU_FEATURE("ssse3");
    PRINT_CPU_FEATURE("sse4.1");
    PRINT_CPU_FEATURE("sse4.2");
    PRINT_CPU_FEATURE("avx");
    PRINT_CPU_FEATURE("avx2");
    PRINT_CPU_FEATURE("avx512f");

    printf("\n");
    return 0;
}


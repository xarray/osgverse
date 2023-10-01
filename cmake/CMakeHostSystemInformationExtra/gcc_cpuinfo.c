/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt for details.  */

#include <stdio.h>

#define PRINT_CPU_MICRO_ARCH(a) if(__builtin_cpu_is(a)){printf("%s\n", a);return 0;}

int main() {
#if defined(GCC_VERSION)
    __builtin_cpu_init ();
#endif
    // these can be found by gcc8.2
    PRINT_CPU_MICRO_ARCH("znver1");
    PRINT_CPU_MICRO_ARCH("amdfam17h");
    PRINT_CPU_MICRO_ARCH("btver2");
    PRINT_CPU_MICRO_ARCH("bdver4");
    PRINT_CPU_MICRO_ARCH("bdver3");
    PRINT_CPU_MICRO_ARCH("bdver2");
    PRINT_CPU_MICRO_ARCH("bdver1");
    PRINT_CPU_MICRO_ARCH("amdfam15h");
    PRINT_CPU_MICRO_ARCH("btver1");
    PRINT_CPU_MICRO_ARCH("istanbul");
    PRINT_CPU_MICRO_ARCH("shanghai");
    PRINT_CPU_MICRO_ARCH("barcelona");
    PRINT_CPU_MICRO_ARCH("amdfam10h");
    PRINT_CPU_MICRO_ARCH("amd");
    PRINT_CPU_MICRO_ARCH("sandybridge");
    PRINT_CPU_MICRO_ARCH("westmere");
    PRINT_CPU_MICRO_ARCH("nehalem");
    PRINT_CPU_MICRO_ARCH("corei7");
    PRINT_CPU_MICRO_ARCH("core2");
    PRINT_CPU_MICRO_ARCH("atom");
    PRINT_CPU_MICRO_ARCH("intel");

    return 0;
}


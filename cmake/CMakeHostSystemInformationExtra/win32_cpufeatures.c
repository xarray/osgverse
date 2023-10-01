/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt for details.  */

#include <windows.h>
#include <errno.h>
#include <stdio.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1400)
#  include <intrin.h>
#  define USE_CPUID_INTRINSICS 1
#else
#  define USE_CPUID_INTRINSICS 0
#endif

void cpuid(int result[4], int select)
{
#  if USE_CPUID_INTRINSICS
    __cpuid(result, select);
#  else
    int tmp[4];
    _asm {
      push eax
      push ebx
      push ecx
      push edx
      ; <<CPUID>>
      mov eax, select
      cpuid
      _asm _emit 0x0f
      _asm _emit 0xa2
      mov tmp[0 * TYPE int], eax
      mov tmp[1 * TYPE int], ebx
      mov tmp[2 * TYPE int], ecx
      mov tmp[3 * TYPE int], edx
      pop edx
      pop ecx
      pop ebx
      pop eax
    }
    memcpy(result, tmp, sizeof(tmp));
#endif
}

int main() {
    int info[4] = { 0, 0, 0, 0 };
#   define FLAG(a,b) (info[a] & ((int)1 << b))

    cpuid(info, 0);
    int nIds = info[0];
    cpuid(info,1);
    if (FLAG(3,23) != 0) printf("mmx ");
    if (FLAG(3,24) != 0) printf("fxsr ");
    if (FLAG(3,25) != 0) printf("sse ");
    if (FLAG(3,26) != 0) printf("sse2 ");
    if (FLAG(3,27) != 0) printf("ss ");
    if (FLAG(3,28) != 0) printf("ht ");
    if (FLAG(3,29) != 0) printf("tm ");
    if (FLAG(3,30) != 0) printf("ia64 ");
    if (FLAG(3,31) != 0) printf("pbe ");
    if (FLAG(2, 0) != 0) printf("pni "); // SSE3
    if (FLAG(2, 9) != 0) printf("ssse3 ");
    if (FLAG(2,19) != 0) printf("sse4_1 ");
    if (FLAG(2,20) != 0) printf("sse4_2 ");
    if (FLAG(2,25) != 0) printf("aes ");
    if (FLAG(2,28) != 0) printf("avx ");
    if (FLAG(2,12) != 0) printf("fma ");
    if (FLAG(2,30) != 0) printf("rdrand ");

    if (nIDs >= 0x00000007) {
        cpuid(info, 7);
        if (FLAG(1, 2) != 0) printf("tsc_adjust ");
        if (FLAG(1, 7) != 0) printf("smep ");
        if (FLAG(1, 9) != 0) printf("erms ");
        if (FLAG(1,10) != 0) printf("invpcid ");
        if (FLAG(1,11) != 0) printf("rtm ");
        if (FLAG(1,12) != 0) printf("cqm ");
        if (FLAG(1, 4) != 0) printf("hle ");
        if (FLAG(1, 5) != 0) printf("avx2 ");
        if (FLAG(1, 3) != 0) printf("bmi1 ");
        if (FLAG(1, 8) != 0) printf("bmi2 ");
        if (FLAG(1,19) != 0) printf("adx ");
        if (FLAG(1,14) != 0) printf("mpx ");
        if (FLAG(1,15) != 0) printf("rdt_a ");
        if (FLAG(1,18) != 0) printf("rdseed ");
        if (FLAG(1,20) != 0) printf("smap ");
        if (FLAG(1,23) != 0) printf("clflushopt ");
        if (FLAG(1,24) != 0) printf("clwb ");
        if (FLAG(1,25) != 0) printf("intel_pt ");
        if (FLAG(1,29) != 0) printf("sha_ni ");
        if (FLAG(1,16) != 0) printf("avx512f ");
        if (FLAG(1,28) != 0) printf("avx512cd ");
        if (FLAG(1,26) != 0) printf("avx512pf ");
        if (FLAG(1,27) != 0) printf("avx512er ");
        if (FLAG(1,30) != 0) printf("avx512bw ");
        if (FLAG(1,31) != 0) printf("avx512vl ");
        if (FLAG(1,17) != 0) printf("avx512dq ");
        if (FLAG(1,21) != 0) printf("avx512ifma ");
        if (FLAG(2, 0) != 0) printf("prefetchwt1 ");
        if (FLAG(2, 1) != 0) printf("avx512vbmi ");
        if (FLAG(2, 2) != 0) printf("umip ");
        if (FLAG(2, 3) != 0) printf("pku ");
        if (FLAG(2, 4) != 0) printf("ospke ");
        if (FLAG(2, 6) != 0) printf("avx512_vbmi2 ");
        if (FLAG(2, 8) != 0) printf("gfni ");
        if (FLAG(2, 9) != 0) printf("vaes ");
        if (FLAG(2,10) != 0) printf("vpclmulqdq ");
        if (FLAG(2,11) != 0) printf("avx512_vnni ");
        if (FLAG(2,12) != 0) printf("avx512_bitalg ");
        if (FLAG(2,13) != 0) printf("tme ");
        if (FLAG(2,14) != 0) printf("avx512_vpopcntdq ");
        if (FLAG(2,16) != 0) printf("la57 ");
        if (FLAG(2,22) != 0) printf("rdpid ");
    }
    cpuid(info, 0x80000000);
    uint32_t nExIds = info[0];
    if (nExIds >= 0x80000001) {
        cpuid(info, 0x80000001);
        if (FLAG(2, 0) != 0) printf("lahf_lm ");
        if (FLAG(2, 1) != 0) printf("cmp_legacy ");
        if (FLAG(2, 2) != 0) printf("svm ");
        if (FLAG(2, 3) != 0) printf("extapic ");
        if (FLAG(2, 4) != 0) printf("cr8_legacy ");
        if (FLAG(2, 5) != 0) printf("abm ");
        if (FLAG(2, 6) != 0) printf("sse4a ");
        if (FLAG(2, 7) != 0) printf("misalignsse ");
        if (FLAG(2, 8) != 0) printf("3dnowprefetch ");
        if (FLAG(2, 9) != 0) printf("osvw ");
        if (FLAG(2,10) != 0) printf("ibs ");
        if (FLAG(2,11) != 0) printf("xop ");
        if (FLAG(2,12) != 0) printf("skinit ");
        if (FLAG(2,13) != 0) printf("wdt ");
        if (FLAG(2,15) != 0) printf("lwp ");
        if (FLAG(2,16) != 0) printf("fma4 ");
        if (FLAG(2,17) != 0) printf("tce ");
        if (FLAG(2,19) != 0) printf("nodeid_msr ");
        if (FLAG(2,21) != 0) printf("tbm ");
        if (FLAG(2,22) != 0) printf("topoext ");
        if (FLAG(2,23) != 0) printf("perfctr_core ");
        if (FLAG(2,24) != 0) printf("perfctr_nb ");
        if (FLAG(2,26) != 0) printf("bpext ");
        if (FLAG(2,27) != 0) printf("ptsc ");
        if (FLAG(2,28) != 0) printf("perfctl_llc ");
        if (FLAG(2,29) != 0) printf("mwaitx ");
        if (FLAG(3,19) != 0) printf("mp ");
        if (FLAG(3,20) != 0) printf("nx ");
        if (FLAG(3,22) != 0) printf("mmxext ");
        if (FLAG(3,25) != 0) printf("fxsr_opt ");
        if (FLAG(3,26) != 0) printf("pdpe1gb ");
        if (FLAG(3,27) != 0) printf("rdtscp ");
        if (FLAG(3,29) != 0) printf("lm "); // x64:long-mode
        if (FLAG(3,30) != 0) printf("3dnowext ");
        if (FLAG(3,31) != 0) printf("3dnow ");
    }
    if (nExIds >= 0x80000007) {
        cpuid(info, 0x80000007);
        if (FLAG(2, 0) != 0) printf("overflow_recov ");
        if (FLAG(2, 1) != 0) printf("succor ");
        if (FLAG(2, 3) != 0) printf("smca ");
    }
    printf("\n");

    return 0;
}

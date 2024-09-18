#ifndef KJK_INTRIN_X86_64_H_
#define KJK_INTRIN_X86_64_H_

#if defined(_M_X64) || defined(__amd64) || defined(__amd64__) || defined(__x86_64) || defined(__x86_64__)
#   include "intrin_x64.h"
#else
#   include "intrin_x86.h"
#endif

#endif  // KJK_INTRIN_X86_64_H_

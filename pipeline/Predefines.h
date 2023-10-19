#ifndef MANA_PP_PREDEFINES
#define MANA_PP_PREDEFINES 1

// OS
#if defined(WIN64) || defined(_WIN64) || defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#   define VERSE_PLATFORM "Windows"

#elif defined(ANDROID) || defined(__ANDROID__)
#   define VERSE_PLATFORM "Android"

#elif defined(__MSYS__)
#   define VERSE_PLATFORM "MSYS"

#elif defined(__CYGWIN__)
#   define VERSE_PLATFORM "Cygwin"

#elif defined(__MINGW32__)
#   define VERSE_PLATFORM "MinGW"

#elif defined(__APPLE__)
#   include <TargetConditionals.h>
#   if defined(TARGET_OS_MAC) && TARGET_OS_MAC
#       define VERSE_PLATFORM "MacOSX"
#   elif defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#       define VERSE_PLATFORM "iOS"
#   else
#       define VERSE_PLATFORM "Darwin"
#   endif

#elif defined(__FreeBSD__) || defined(__FreeBSD) || defined(__FreeBSD_kernel__)
#   define VERSE_PLATFORM "FreeBSD"

#elif defined(__NetBSD__) || defined(__NetBSD)
#   define VERSE_PLATFORM "NetBSD"

#elif defined(__OpenBSD__) || defined(__OPENBSD)
#   define VERSE_PLATFORM "OpenBSD"

#elif defined(sun) || defined(__sun) || defined(__sun__)
#   define VERSE_PLATFORM "Solaris"

#elif defined(_AIX) || defined(__AIX) || defined(__AIX__) || defined(__aix) || defined(__aix__)
#   define VERSE_PLATFORM "AIX"

#elif defined(__hpux) || defined(__hpux__)
#   define VERSE_PLATFORM "HP-UX"

#elif defined(__HAIKU__)
#   define VERSE_PLATFORM "Haiku"

#elif defined(__BeOS) || defined(__BEOS__) || defined(_BEOS)
#   define VERSE_PLATFORM "BeOS"

#elif defined(__QNX__) || defined(__QNXNTO__)
#   define VERSE_PLATFORM "QNX"

#elif defined(__tru64) || defined(_tru64) || defined(__TRU64__)
#   define VERSE_PLATFORM "Tru64"

#elif defined(__riscos) || defined(__riscos__)
#   define VERSE_PLATFORM "RISCos"

#elif defined(__sinix) || defined(__sinix__) || defined(__SINIX__)
#   define VERSE_PLATFORM "SINIX"

#elif defined(__UNIX_SV__)
#   define VERSE_PLATFORM "UNIX_SV"

#elif defined(__bsdos__)
#   define VERSE_PLATFORM "BSDOS"

#elif defined(_MPRAS) || defined(MPRAS)
#   define VERSE_PLATFORM "MP-RAS"

#elif defined(__osf) || defined(__osf__)
#   define VERSE_PLATFORM "OSF1"

#elif defined(_SCO_SV) || defined(SCO_SV) || defined(sco_sv)
#   define VERSE_PLATFORM "SCO_SV"

#elif defined(__ultrix) || defined(__ultrix__) || defined(_ULTRIX)
#   define VERSE_PLATFORM "ULTRIX"

#elif defined(__XENIX__) || defined(_XENIX) || defined(XENIX)
#   define VERSE_PLATFORM "Xenix"

#elif defined(__linux) || defined(__linux__) || defined(linux)
#   define VERSE_PLATFORM "Linux"

#elif defined(__EMSCRIPTEN__)
#   define VERSE_PLATFORM "Emscripten"

#else /* unknown platform */
#   define VERSE_PLATFORM "?"

#endif

// ARCH
#if defined(__x86_64) || defined(__x86_64__)
#   define VERSE_ARCH_NAME "X86_64"
#   define VERSE_ARCH_BITS 64

#elif defined(_M_IA64)
#   define VERSE_ARCH_NAME "IA64"
#   define VERSE_ARCH_BITS 64

#elif defined(_M_ARM64EC)
#   define VERSE_ARCH_NAME "ARM64EC"
#   define VERSE_ARCH_BITS 64

#elif defined(_M_X64)
#   define VERSE_ARCH_NAME "X64"
#   define VERSE_ARCH_BITS 64

#elif defined(__i386) || defined(__i386__) || defined(_M_IX86)
#   define VERSE_ARCH_NAME "X86"
#   define VERSE_ARCH_BITS 32

#elif defined(__aarch64__) || defined(__ARM64__) || defined(_M_ARM64)
#   define VERSE_ARCH_NAME "ARM64"
#   define VERSE_ARCH_BITS 64

#elif defined(_M_ARM)
#   if _M_ARM == 4
#       define VERSE_ARCH_NAME "ARMV4I"
#   elif _M_ARM == 5
#       define VERSE_ARCH_NAME "ARMV5I"
#   else
#       define VERSE_ARCH_NAME "ARMV" STRINGIFY(_M_ARM)
#   endif
#   define VERSE_ARCH_BITS 32

#elif defined(__mips64__) || defined(_M_MIPS64)
#   define VERSE_ARCH_NAME "MIPS64"
#   define VERSE_ARCH_BITS 64

#elif defined(__mips__) || defined(_M_MIPS)
#   define VERSE_ARCH_NAME "MIPS"
#   define VERSE_ARCH_BITS 32

#elif defined(_M_SH)
#   define VERSE_ARCH_NAME "SHx"
#   define VERSE_ARCH_BITS 32

#elif defined(_M_I86)
#   define VERSE_ARCH_NAME "I86"
#   define VERSE_ARCH_BITS 32

#elif defined(__ICCARM__)
#   define VERSE_ARCH_NAME "ARM"
#   define VERSE_ARCH_BITS 32

#elif defined(__ICCRX__)
#   define VERSE_ARCH_NAME "RX"
#   define VERSE_ARCH_BITS 32

#elif defined(__ICCRH850__)
#   define VERSE_ARCH_NAME "RH850"
#   define VERSE_ARCH_BITS 32

#elif defined(__ICCRL78__)
#   define VERSE_ARCH_NAME "RL78"
#   define VERSE_ARCH_BITS 32

#elif defined(__ICCRISCV__)
#   define VERSE_ARCH_NAME "RISCV"
#   define VERSE_ARCH_BITS 32

#elif defined(__ICCAVR__)
#   define VERSE_ARCH_NAME "AVR"
#   define VERSE_ARCH_BITS 32

#elif defined(__ICC430__)
#   define VERSE_ARCH_NAME "MSP430"
#   define VERSE_ARCH_BITS 32

#elif defined(__ICCV850__)
#   define VERSE_ARCH_NAME "V850"
#   define VERSE_ARCH_BITS 32

#elif defined(__ICC8051__)
#   define VERSE_ARCH_NAME "8051"
#   define VERSE_ARCH_BITS 32

#elif defined(__ICCSTM8__)
#   define VERSE_ARCH_NAME "STM8"
#   define VERSE_ARCH_BITS 32

#elif defined(__PPC64__)
#   define VERSE_ARCH_NAME "PPC64"
#   define VERSE_ARCH_BITS 64

#elif defined(__ppc__)
#   define VERSE_ARCH_NAME "PPC"
#   define VERSE_ARCH_BITS 32

#elif defined(__arm__) || defined(__ARM__) || defined(__TI_ARM__)
#   define VERSE_ARCH_NAME "ARM"
#   define VERSE_ARCH_BITS 32

#elif defined(__MSP430__)
#   define VERSE_ARCH_NAME "MSP430"
#   define VERSE_ARCH_BITS 32

#elif defined(__TMS320C28XX__)
#   define VERSE_ARCH_NAME "TMS320C28x"
#   define VERSE_ARCH_BITS 32

#elif defined(__TMS320C6X__) || defined(_TMS320C6X)
#   define VERSE_ARCH_NAME "TMS320C6x"
#   define VERSE_ARCH_BITS 32

#elif defined(__ADSPSHARC__)
#   define VERSE_ARCH_NAME "SHARC"
#   define VERSE_ARCH_BITS 32

#elif defined(__ADSPBLACKFIN__)
#   define VERSE_ARCH_NAME "Blackfin"
#   define VERSE_ARCH_BITS 32

#elif defined(__CTC__) || defined(__CPTC__)
#   define VERSE_ARCH_NAME "TriCore"
#   define VERSE_ARCH_BITS 32

#elif defined(__CMCS__)
#   define VERSE_ARCH_NAME "MCS"
#   define VERSE_ARCH_BITS 32

#elif defined(__CARM__)
#   define VERSE_ARCH_NAME "ARM"
#   define VERSE_ARCH_BITS 32

#elif defined(__CARC__)
#   define VERSE_ARCH_NAME "ARC"
#   define VERSE_ARCH_BITS 32

#elif defined(__C51__)
#   define VERSE_ARCH_NAME "8051"
#   define VERSE_ARCH_BITS 32

#elif defined(__CPCP__)
#   define VERSE_ARCH_NAME "PCP"
#   define VERSE_ARCH_BITS 32

#elif defined(__EMSCRIPTEN__)
#   define VERSE_ARCH_NAME "Emscripten"
#   define VERSE_ARCH_BITS 32

#else
#   define VERSE_ARCH_NAME "?"
#   define VERSE_ARCH_BITS 32
#endif

#endif

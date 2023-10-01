# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
CmakeHostSystemInformationExtra
-------------------------------

Query host system extra information.

.. code-block:: cmake

  cmake_host_system_information_extra(RESULT <variable> [EXTEND] QUERY <key> ...)

Queries system information of the host system on which cmake runs.
One or more ``<key>`` can be provided to select the information to be
queried.  The list of queried values is stored in ``<variable>``.

When ``EXTEND`` keyword is added, this command extend
``cmake_host_system_information()`` command and also accept query ``<key>``
it supports.

``<key>`` can be one of the following values:

============================= ================================================
Key                           Description
============================= ================================================
``PROCESSOR_VENDOR``          Vendor name of a processor
``PROCESSOR_MICRO_ARCHITECTURE`` Micro architecture of a processor
``HAS_SSE2``                  One if processor supports SSE2 instructions
``HAS_SSE3``                  One if processor supports SSE3 Prescot New
                              Instruction sets instructions
``HAS_SSSE3``                 One if processor supports SSSE3 instructions
``HAS_SSE4_1``                One if processor supports SSE4.1 instructions
``HAS_SSE4_2``                One if processor supports SSE4.2 instructions
``HAS_AVX``                   One if processor supports AVX instructions
``HAS_AVX2``                  One if processor supports AVX2 instructions
``HAS_BMI1``                  One if processor supports BMI1 instructions
``HAS_BMI2``                  One if processor supports BMI2 instructions
``HAS_3DNOWPREFETCH``         One if processor supports 3DNow instructions
``HAS_CONSTANT_TSC``          One if processor has constant tsc
``HAS_VMX``                   One if processor supports VMX instructions
``HAS_AMD_3DNOW``             One if processor supports 3DNow! instructions
``HAS_AMD_3DNOWEXT``          One if processor supports 3DNow! Plus extensions
``HAS_3DNOWPREFETCH``         One if processor supports 3DNow Prefetch
                              instructions
``HAS_ARM_NEON``              One if processor supports NEON instructions
``HAS_ARM_VFPV3``             One if processor supports VFPv3 instructions
``HAS_ARM_VFPV3D16``          One if processor supports VFPv3 instructions
                              with only 16 double-precision registers (d0-d15)
``HAS_ARM_VFPV4``             One if processor supports fused multiply-add
                              instructions
``HAS_ARM_VFPD32``            One if processor supports VFP with 32 double-
                              precision registers (d0-d31)
``HAS_ARM_IWMMXT``            One if processor supports Intel/Marvell Wireless
                              MMX instructions. 64-bit integer SIMD
``HAS_AES``                   One if processor supports AES instructions
``HAS_SHA1``                  One if processor supports SHA1 instructions
``HAS_SHA2``                  One if processor supports SHA2 instructions
``HAS_CRC32``                 One if processor supports CRC32 instructions
============================= ================================================

#]=======================================================================]

include("${CMAKE_CURRENT_LIST_DIR}/CMakeHostSystemInformationExtra/DetectX64MicroArchitecture.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/CMakeHostSystemInformationExtra/DetectArmMicroArchitecture.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/CMakeHostSystemInformationExtra/CheckCPUFeature.cmake")

function(CMAKE_HOST_SYSTEM_INFORMATION_EXTRA)
  set(_options EXTEND)
  set(_oneValueArgs RESULT)
  set(_multiValueArgs QUERY)
  cmake_parse_arguments(_HSIE "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
  if(_HSIE_EXTEND)
    if(CMAKE_VERSION VERSION_LESS 3.10)
      set(HSI_KNOWN_QUERIES
          NUMBER_OF_LOGICAL_CORES NUMBER_OF_PHYSICAL_CORES HOSTNAME FQDN TOTAL_VIRTUAL_MEMORY AVAILABLE_VIRTUAL_MEMORY
          TOTAL_PHYSICAL_MEMORY AVAILABLE_PHYSICAL_MEMORY IS_64BIT HAS_FPU HAS_MMX HAS_MMX_PLUS HAS_SSE
          HAS_SERIAL_NUMBER PROCESSOR_SERIAL_NUMBER PROCESSOR_NAME PROCESSOR_DESCRIPTION OS_NAME OS_RELEASE OS_VERSION OS_PLATFORMa)
    else()
      set(HSI_KNOWN_QUERIES
          NUMBER_OF_LOGICAL_CORES NUMBER_OF_PHYSICAL_CORES HOSTNAME FQDN TOTAL_VIRTUAL_MEMORY AVAILABLE_VIRTUAL_MEMORY
          TOTAL_PHYSICAL_MEMORY AVAILABLE_PHYSICAL_MEMORY IS_64BIT HAS_FPU HAS_MMX HAS_MMX_PLUS HAS_SSE HAS_SSE2
          HAS_SSE_FP HAS_SSE_MMX HAS_AMD_3DNOW HAS_AMD_3DNOW_PLUS HAS_IA64 HAS_SERIAL_NUMBER PROCESSOR_SERIAL_NUMBER
          PROCESSOR_NAME PROCESSOR_DESCRIPTION OS_NAME OS_RELEASE OS_VERSION OS_PLATFORMa)
    endif()
  else()
    set(HSI_KNOWN_QUERIES)
  endif()

  set(_RESULT_LIST)
  foreach(_query IN LISTS _HSIE_QUERY)
    list(FIND HSI_KNOWN_QUERIES ${_query} _result)
    if(_result GREATER -1)
      set(_res)
      cmake_host_system_information(RESULT _res QUERY ${_query})
      list(APPEND _RESULT_LIST ${_res})
      continue()
    endif()
    if((_query STREQUAL PROCESSOR_MICRO_ARCHITECTURE) OR
       (_query STREQUAL PROCESSOR_VENDOR))
      set(vendor)
      set(architecture)
      if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "(x86|AMD64)")
        detect_x64_micro_architecture(vendor architecture)
      elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "(ARM|aarch64)")
        set(vendor)
        set(core)
        set(base)
        detect_arm_micro_architecture(vendor core base)
        message(STATUS "Found ${vendor}:${base}:${core}")
        set(architecture ${core})
      endif()
      if(_query STREQUAL PROCESSOR_MICRO_ARCHITECTURE)
        list(APPEND _RESULT_LIST ${architecture})
      else()
        list(APPEND _RESULT_LIST ${vendor})
      endif()
      continue()
    endif()
    if(_query MATCHES "^HAS_ARM_")
      set(_res)
      string(REPLACE "^HAS_ARM_" "" _feature "${_query}")
      check_cpu_feature(_res ${_feature})
      list(APPEND _RESULT_LIST ${_res})
      continue()
    elseif(_query MATCHES "^HAS_AMD_")
      set(_res)
      string(REPLACE "^HAS_AMD_" "" _feature "${_query}")
      check_cpu_feature(_res ${_feature})
      list(APPEND _RESULT_LIST ${_res})
      continue()
    elseif(_query MATCHES "^HAS_")
      set(_res)
      string(REPLACE "^HAS_" "" _feature "${_query}")
      string(TOLOWER "${_feature}" _feature)
      check_cpu_feature(_res ${_feature})
      list(APPEND _RESULT_LIST ${_res})
      continue()
    endif()
  endforeach()
  set(${_HSIE_RESULT} ${_RESULT_LIST} PARENT_SCOPE)
endfunction()

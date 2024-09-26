# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
DetectCPUMicroArchitecture
-------------------------

  Detect CPU micro architecture and return a code name

.. command:: detect_x64_micro_architecture

   detect_x64_micro_architecture(<output vendor> <output arch>)

  Determine the host Intel/AMD CPU micro architecture and retrun
  a code name.

#]=======================================================================]

# We must run the following at "include" time, not at function call time,
# to find the path to this module rather than the path to a calling list file
get_filename_component(_detextx64microarchitecturedir ${CMAKE_CURRENT_LIST_FILE} PATH)

function(DETECT_X64_MICRO_ARCHITECTURE out_vendor out_arch)
  set(_vendor_id)
  set(_cpu_family)
  set(_cpu_model)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(NOT _PROC_CPUINFO) # not override for unit-test
      set(_PROC_CPUINFO "/proc/cpuinfo")
    endif()
    file(READ ${_PROC_CPUINFO} _cpuinfo)
    string(REGEX REPLACE ".*vendor_id[ \t]*:[ \t]+([a-zA-Z0-9_-]+).*" "\\1" _vendor_id "${_cpuinfo}")
    string(REGEX REPLACE ".*cpu family[ \t]*:[ \t]+([a-zA-Z0-9_-]+).*" "\\1" _cpu_family "${_cpuinfo}")
    string(REGEX REPLACE ".*model[ \t]*:[ \t]+([a-zA-Z0-9_-]+).*" "\\1" _cpu_model "${_cpuinfo}")
    string(REGEX REPLACE ".*flags[ \t]*:[ \t]+([^\n]+).*" "\\1" _cpu_flags "${_cpuinfo}")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    exec_program("/usr/sbin/sysctl -n machdep.cpu.vendor machdep.cpu.model machdep.cpu.family machdep.cpu.features" OUTPUT_VARIABLE _sysctl_output_string)
    string(REPLACE "\n" ";" _sysctl_output ${_sysctl_output_string})
    list(GET _sysctl_output 0 _vendor_id)
    list(GET _sysctl_output 1 _cpu_model)
    list(GET _sysctl_output 2 _cpu_family)
    list(GET _sysctl_output 3 _cpu_flags)

    string(TOLOWER "${_cpu_flags}" _cpu_flags)
    string(REPLACE "." "_" _cpu_flags "${_cpu_flags}")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    get_filename_component(_vendor_id "[HKEY_LOCAL_MACHINE\\Hardware\\Description\\System\\CentralProcessor\\0;VendorIdentifier]" NAME CACHE)
    get_filename_component(_cpu_id "[HKEY_LOCAL_MACHINE\\Hardware\\Description\\System\\CentralProcessor\\0;Identifier]" NAME CACHE)
    string(REGEX REPLACE ".* Family ([0-9]+) .*" "\\1" _cpu_family "${_cpu_id}")
    string(REGEX REPLACE ".* Model ([0-9]+) .*" "\\1" _cpu_model "${_cpu_id}")
  else()
    if(CMAKE_COMPILER_IS_GNUCC)
      try_run(RUN_RESULT COMP_RESULT ${CMAKE_CURRENT_BINARY_DIR} ${_detectx64microarchitecturedir}/gcc_cpuinfo.c
              CMAKE_FLAGS -g
              RUN_OUTPUT_VARIABLE _cpu_model)
      message(STATUS "Detected CPU model: ${_cpu_model}")
    else()
      set(${outvar} "intel" PARENT_SCOPE)
      return()
    endif()
  endif()

  if(_vendor_id STREQUAL "GenuineIntel")
    if(_cpu_family EQUAL 6)
      # taken from the Intel ORM
      # http://www.intel.com/content/www/us/en/processors/architectures-software-developer-manuals.html
      # CPUID Signature Values of Of Recent Intel Microarchitectures
      # 4E 5E       | Skylake microarchitecture
      # 3D 47 56    | Broadwell microarchitecture
      # 3C 45 46 3F | Haswell microarchitecture
      # 3A 3E       | Ivy Bridge microarchitecture
      # 2A 2D       | Sandy Bridge microarchitecture
      # 25 2C 2F    | Intel microarchitecture Westmere
      # 1A 1E 1F 2E | Intel microarchitecture Nehalem
      # 17 1D       | Enhanced Intel Core microarchitecture
      # 0F          | Intel Core microarchitecture
      #
      # Intel SDM 2-2 Vol.4 / November 2018:
      # 66          | Future Core                [Cannon Lake]
      # 57          | Xeon Phi 3200, 5200, 7200  [Knights Landing]
      # 85          | Xeon Phi 7215, 7285, 7295  [Knights Mill]
      # 8E 9E       | 7th gen. Core              [Kaby Lake]
      # 55          | Xeon scalable              [Skylake w/ AVX512]
      # 4E 5E       | 6th gen. Core / E3 v5      [Skylake w/o AVX512]
      # 56          | Xeon D-1500                [Broadwell]
      # 4F          | Xeon E5 v4, E7 v4, i7-69xx [Broadwell]
      # 47          | 5th gen. Core / Xeon E3 v4 [Broadwell]
      # 3D          | M-5xxx / 5th gen.          [Broadwell]
      # 3F          | Xeon E5 v3, E7 v3, i7-59xx [Haswell-E]
      # 3C 45 46    | 4th gen. Core, Xeon E3 v3  [Haswell]
      # 3E          | Xeon E5 v2, E7 v2, i7-49xx [Ivy Bridge-E]
      # 3A          | 3rd gen. Core, Xeon E3 v2  [Ivy Bridge]
      # 2D          | Xeon E5, i7-39xx           [Sandy Bridge]
      # 2F          | Xeon E7
      # 2A          | Xeon E3, 2nd gen. Core     [Sandy Bridge]
      # 2E          | Xeon 7500, 6500 series
      # 25 2C       | Xeon 3600, 5600 series, Core i7, i5 and i3
      # 7A          | Atom [Goldmont Plus]
      # 5F,5C       | Atom [Goldmont]
      # 4C          | Atom [Airmont]
      # 5D          | Atom [Silvermont]
      # 5A,4A,37,4D,36| Atom
      # 1C, 26,27,35,36 |Atom
      # 06          | Xeon
      #
      # icelake-server icelake
      #
      # Values from the Intel SDE:
      # 5C | Goldmont
      # 5A | Silvermont
      # 57 | Knights Landing
      # 66 | Cannonlake
      # 55 | Skylake Server
      # 4E | Skylake Client
      # 3C | Broadwell (likely a bug in the SDE)
      # 3C | Haswell

      if(_cpu_model LESS 14)
        message(WARNING "${OFA_FUNCNAME}:Your CPU (family ${_cpu_family}, model ${_cpu_model}) is not known. Auto-detection of optimization flags failed and will use the generic CPU settings with SSE2.")
        set(MICRO_ARCHITECTURE "generic")
      else()
        # https://en.wikichip.org/wiki/intel/cpuid#Family_6
        set(architecture_lookup_hash
            183 "raptorlake"   186 "raptorlake"
            167 "rocketlake"   151 "alderlake"     154 "alderlake"
            87  "knl"          133 "knm"            92  "goldmont"
            90  "silvermont"    76 "silvermont"    102 "cannonlake"
            140 "tigerlake"    141 "tigerlake"
            142 "kabylake"     158 "coffelake"
            85  "skylake-avx512"
            78  "skylake"       94  "skylake"
            61  "broadwell"     71  "broadwell"     79  "broadwell"   86  "broadwell"
            60  "haswell"       69  "haswell"       70  "haswell"     63  "haswell"
            58  "ivy-bridge"    62  "ivy-bridge"
            42  "sandy-bridge"  45  "sandy-bridge"
            31  "westmere"      37  "westmere"      44  "westmere"    47  "westmere"
            26  "nehalem"       30  "nehalem"       31  "nehalem"     46  "nehalem"
            23  "penryn"        29  "penryn"
            14  "core"          15  "merom"         28  "atom"
            )
        # here lookup hash key and return value as MICRO_ARCHITECTURE
        list(FIND architecture_lookup_hash "${_cpu_model}" _found)
        if(_found GREATER -1)
          math(EXPR index "${_found}+1")
          list(GET architecture_lookup_hash ${index} MICRO_ARCHITECTURE)
        else()
          message(WARNING "${OFA_FUNCNAME}:Your CPU (family ${_cpu_family}, model ${_cpu_model}) is not known. Auto-detection of optimization flags failed and will use the 65nm Core 2 CPU settings.")
          set(MICRO_ARCHITECTURE "merom")
        endif()
      endif()
    elseif(_cpu_family EQUAL 7) # Itanium (not supported)
      message(WARNING "${OFA_FUNCNAME}:Your CPU (Itanium: family ${_cpu_family}, model ${_cpu_model}) is not supported.")
    elseif(_cpu_family EQUAL 15) # NetBurst
      list(APPEND _available_vector_units_list "sse" "sse2")
      if(_cpu_model GREATER 2) # Not sure whether this must be 3 or even 4 instead
        list(APPEND _available_vector_units_list "sse" "sse2" "sse3")
      endif(_cpu_model GREATER 2)
    endif(_cpu_family EQUAL 6)
  elseif(_vendor_id STREQUAL "AuthenticAMD")
    if(_cpu_family EQUAL 23)
      set(MICRO_ARCHITECTURE "amdfam17h" "znver1")
    elseif(_cpu_family EQUAL 22) # 16h
      if(_cpu_model LESS 16)
        set(MICRO_ARCHITECTURE "jaguar" "btver2")
      else()
        set(MICRO_ARCHITECTURE "puma" "btver2")
      endif()
    elseif(_cpu_family EQUAL 21) # 15h
      set(MICRO_ARCHITECTURE "amdfam15h")
      if(_cpu_model LESS 2)
        set(MICRO_ARCHITECTURE ${MICRO_ARCHITECTURE} "bulldozer" "bdver1")
      elseif(_cpu_model LESS 32)
        set(MICRO_ARCHITECTURE ${MICRO_ARCHITECTURE} "piledriver" "bdver2")
      elseif(_cpu_model LESS 64)
        set(MICRO_ARCHITECTURE ${MICRO_ARCHITECTURE} "steamroller" "bdver3")
      elseif(_cpu_model LESS 112)
        set(MICRO_ARCHITECTURE ${MICRO_ARCHITECTURE} "excavator" "bdver4")
      endif()
    elseif(_cpu_family EQUAL 20) # 14h
      set(MICRO_ARCHITECTURE "btver1")
    elseif(_cpu_family EQUAL 18) # 12h
      set(MICRO_ARCHITECTURE "barcelona")
    elseif(_cpu_family EQUAL 16) # 10h
      set(MICRO_ARCHITECTURE "amdfam10h")
    elseif(_cpu_family EQUAL 15)
      set(MICRO_ARCHITECTURE "k8")
      if(_cpu_model GREATER 64) # I don't know the right number to put here. This is just a guess from the hardware I have access to
        set(MICRO_ARCHITECTURE "k8-sse3")
      endif(_cpu_model GREATER 64)
    endif()
  endif(_vendor_id STREQUAL "GenuineIntel")
  set(${out_vendor} ${_vendor_id} PARENT_SCOPE)
  set(${out_arch} ${MICRO_ARCHITECTURE} PARENT_SCOPE)
endfunction()

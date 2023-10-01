# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
CheckCPUFeature
---------------

Check CPU feature

.. command:: check_cpu_feature

   check_cpu_feature(<out variable> <feature key>)

..note:
  It also alias names for features.
  /proc/cpuinfo reports `neon` as `asimd` in ARMv8-A architcture
  Linux reports SSE3 as `pni` as of prescot new instruction sets.

#]=======================================================================]

# We must run the following at "include" time, not at function call time,
# to find the path to this module rather than the path to a calling list file
get_filename_component(_checkcpufeaturedir ${CMAKE_CURRENT_LIST_FILE} PATH)

function(CHECK_CPU_FEATURE outvar feature)
  if(NOT _check_cpu_feature_values)
    set(_cpu_flags)
    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
      if(NOT _PROC_CPUINFO) # not override for unit-test
        set(_PROC_CPUINFO "/proc/cpuinfo")
      endif()
      file(READ ${_PROC_CPUINFO} _cpuinfo)
      string(REGEX REPLACE ".*(flags|Features)[ \t]*:[ \t]+([^\n]+).*" "\\2" _cpu_flags "${_cpuinfo}")
    elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
      execute_process(COMMAND "/usr/sbin/sysctl -n machdep.cpu.features"
                      OUTPUT_VARIABLE _cpu_flags
                      ERROR_QUIET
                      OUTPUT_STRIP_TRAILING_WHITESPACE)
      string(TOLOWER "${_cpu_flags}" _cpu_flags)
      string(REPLACE "." "_" _cpu_flags "${_cpu_flags}")
    elseif(MSVC)
      try_run(RUN_RESULT COMP_RESULT ${CMAKE_CURRENT_BINARY_DIR} ${_checkcpufeaturedir}/win32_cpufeatures.c
              CMAKE_FLAGS -g
              RUN_OUTPUT_VARIABLE flags)
      message(STATUS "Detected features: ${flags}")
    elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "OpenBSD" OR
           CMAKE_HOST_SYSTEM_NAME STREQUAL "FreeBSD" OR
           CMAKE_HOST_SYSTME_NAME STREQUAL "NetBSD")
      execute_process("grep Features /var/run/dmesg.boot"
                      OUTPUT_VARIABLE _cpu_features
                      ERROR_QUIET
                      OUTPUT_STRIP_TRAILING_WHITESPACE)
      string(REGEX REPLACE ".*=0x[0-9a-f]+<[ \t]+([^\n]+).*" "\\1" _cpu_flags "${_cpu_features}")
      string(REPLACE "\n" ";" _cpu_flags "${_cpu_features}")
      string(TOLOWER "${_cpu_flags}" _cpu_flags)
    elseif((CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG) AND
         ("${CMAKE_HOST_SYSTEM_PROCESSOR}" MATCHES "(x86|AMD64)"))
      try_run(RUN_RESULT COMP_RESULT ${CMAKE_CURRENT_BINARY_DIR} ${_checkcpufeaturedir}/gcc_cpufeatures.c
              CMAKE_FLAGS -g
              RUN_OUTPUT_VARIABLE flags)
      message(STATUS "Detected features: ${flags}")
    else()
      # TODO ARM and PPC with GCC/CLANG
      set(${outvar} 0 PARENT_SCOPE)
      return()
    endif()
    string(REPLACE " " ";" _check_cpu_feature_values "${_cpu_flags}")
  endif()
  # aliases
  # ARMv8-A returns asimd for neon on Linux
  if(feature STREQUAL neon)
    list(APPEND feature asimd)
  elseif(feature STREQUAL sse3)
    list(APPEND feature pni)
  endif()
  foreach(item IN ITEMS ${feature})
    list(FIND _check_cpu_feature_values ${item} _found)
    if(_found GREATER -1)
      set(_found 1)
      break()
    else()
      set(_found 0)
    endif()
  endforeach()
  set(${outvar} ${_found} PARENT_SCOPE)
endfunction()

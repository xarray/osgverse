# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
GetCPUSIMDFeatures
--------------------------

  Get feature list for target architecture

.. command:: get_cpu_simd_features

   get_cpu_simd_features(<output variable> <target architecture>)

#]=======================================================================]


include("${CMAKE_CURRENT_LIST_DIR}/GetCPUSIMDFeatures/GetX64SIMDFeatures.cmake")

function(GET_CPU_SIMD_FEATURES outvar target_architecture)
    if("${CMAKE_HOST_SYSTEM_PROCESSOR}" MATCHES "(x86|AMD64)")
        get_x64_simd_features(features ${target_architecture})
    elseif("${CMAKE_HOST_SYSTEM_PROCESSOR}" MATCHES "(ARM|aarch64)")
        # FIXME implement me
        message(WARNING "get_arm_simd_features():Not implemented yet. Implement me!")
    elseif("${CMAKE_HOST_SYSTEM_PROCESSOR}" MATCHES "(PPC|PPC64)")
        # FIXME implement me
        message(WARNING "get_ppc_simd_features():Not implemented yet. Implement me!")
    endif()
    set(${outvar} ${features} PARENT_SCOPE)
endfunction()
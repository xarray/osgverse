# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
CMakeCompilerMachineOption
--------------------------

  Get march flags for target Intel micro architecture

.. command:: cmake_compiler_machine_option

   cmake_compiler_machine_option(<output variable>
                                 [TARGET_ARCHITECTURE <target architecture>]
                                 [FEATURES <feature> ...])

#]=======================================================================]

include("${CMAKE_CURRENT_LIST_DIR}/CMakeCompilerMachineOption/X64CompilerOptions.cmake")

function(CMAKE_COMPILER_MACHINE_OPTION outvar tarch)
    set(compiler_options)
    if("${CMAKE_HOST_SYSTEM_PROCESSOR}" MATCHES "(x86|AMD64)")
        x64_compiler_options(compiler_options ${tarch})
    elseif("${CMAKE_HOST_SYSTEM_PROCESSOR}" MATCHES "(ARM|aarch64)")
        arm_compiler_options(compiler_options ${tarch})
    endif()
    set(${outvar} ${compiler_options} PARENT_SCOPE)
endfunction()
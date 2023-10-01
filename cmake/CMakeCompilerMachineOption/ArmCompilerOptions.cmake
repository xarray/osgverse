# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
GetArmMarchCompilerOptions
--------------------------

  Get march flags for target Intel micro architecture

.. command:: arm_compiler_options

   arm_compiler_options(<output variable> <target SoC name>)

#]=======================================================================]

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckIncludeFileCXX)
include(CheckIncludeFile)

function(ARM_COMPILER_OPTIONS outvar tarch)
  set(ARCHITECTURE_FLAGS)

  if(tarch STREQUAL "none")
    message(WARNING "Unsupported target architecture. No flag is added.")
  else()
    # -mfpu=       VFP      NEON
    #--------------------------------
    # vfpv3        VFPv3    -
    # neon         VFPv3    Y
    # vfpv3-d16    VFPv3    -
    # vfpv3-fp16   VFPv3    -
    # neon-fp16    VFPv3    Y
    # vfpv3xd      VFPv3    -
    # vfpv3xd-fp16 VFPv3    -
    # vfpv4        VFPv4    -
    # vfpv4-d16    VFPv4    -
    # neon-vfpv4   VFPv4    Y
    # fpv4-sp-d16  VFPv4    -
    # fp-armv8      AA64     -
    # neon-fp-armv8 AA64    Y
    # crypto-neon-fp-armv8 AA64 Y
    #
    # on gcc armv7A neon, -funsafe-math-optimizations is necessary.
    #
    # Tegra2 ARMv7 A9
    # Tegra3 ARMv7 A9
    # Tegra4 ARMv7 A15
    # TegraK1-T124 ARMv7 A15
    # TegraK1-T132 ARMv8 Denver
    # TegraX1-T210 ARMv8 A53+A57
    # TegraX1-T186 ARMv8 Denver2+A57
    # Xavier-T194 ARMv8 custom-carnel
    #
    # XScale
    # https://en.wikipedia.org/wiki/XScale
    # PXA25x Cotulla ARMv5TE
    # PXA26x Dalhart ARMv5TE
    # PXA27x Bulverde Wireless-MMX ARMv5TE
    # PXA3xx Monahans ARMv5TE
    # PXA90x
    # PXA16x
    #
    # PXA930 Sheeva
    # PXA935 Sheeva
    # PXA940 Cortex-A8
    # PXA986/988 Cortex-A9
    # PXA1088 Cortex-A7
    #
    set(ARMv7-core-list Cortex-A8 Cortex-A9 Cortex-A12 Cortex-A15 Cortex-A7 )
    set(ARMv8A-core-list Denver Dever2 ThunderX Kyro Kyro2 M1 M2 M3)
    set(ARMv8.0A-core-list Cortex-A32 Cortex-A35 Cortex-A53 Cortex-A55 Cortex-A57 Cortex-A72 Cortex-A73 Hurricane )
    set(ARMv8.1A-core-list ThunderX2 Falkor)
    set(ARMv8.2A-core-list Cortex-A75 Cortex-A76 Monsoon Kyro3)
    set(ARMv8.3A-core-list Vortex )

    if(MSVC)
      # TODO implement me
    elseif(CMAKE_CXX_COMPILER MATCHES "/(icpc|icc)$") # ICC (on Linux)
      # TODO implement me
    else() # not MSVC and not ICC => GCC, Clang, Open64
      set(soc-core-map
          Tegra2 Cortex-A9 Tegra3 Cortex-A9 Tegra4 Cortex-A15
          PXA25x ARMv5TE PXA26x ARMv5TE PXA27x ARMv5TE
          PXA3xx ARMv5TE PXA940 Cortex-A8 PXA986 Cortex-A9
          PXA988 Cortex-A9 PXA1088 Cortex-A7
          )
      set(core-mfpu-map
          ARMv5TE ""
          Cortex-A8 neon-vfpv3
          Cortex-A9 "neon,vfpv3-d16"
          Cortex-A5 "neon-vfpv4,vfpv4-d16"
          Cortex-A15 neon-vfpv4
          Cortex-A7  neon-vfpv4
          Cortex-A53 neon-fp-armv8
          Cortex-A57 neon-fp-armv8
          )
      list(FIND soc-core-map ${tarch} _res)
      if(_res GREATER -1)
        math(EXPR _index "${_res} + 1")
        list(GET soc-core-map ${_index} _ARMCORE)
        list(FIND core-mfpu-map ${_ARMCORE} _res)
        if(_res GREATER -1)
          math(EXPR _index "${_res} + 1")
          list(GET soc-mfpu-map ${_index} _flags)
          if(_flags)
            string(REPLACE "," ";" _flag_list "${_flags}")
            foreach(flag IN ITEMS ${_flag_list})
              __check_compiler_flag("-m${_flag}" test_${_flag})
              if(test_${_flag})
                list(APPEND ARCHITECTURE_FLAGS "-m${_flag}")
                break()
              endif()
            endforeach()
          endif()
        endif()
      endif()
    endif()
  endif()
  set(${outvar} "${ARCHITECTURE_FLAGS}" PARENT_SCOPE)
endfunction()

# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
DetectArmMicroArchitecture
-------------------------

  Detect CPU micro architecture and return a code name

.. command:: detect_arm_micro_architecture

   detect_arm_micro_architecture(<implementer> <core> <arch>)

  Determine the host ARM CPU and return implementer, core, and architecture.

#]=======================================================================]

function(DETECT_ARM_MICRO_ARCHITECTURE outvar1 outvar2 outvar3)
  # part : corename  for each implementer
  set(arm_list
      0x810 "ARM810" 0x920 "ARM920" 0x922 "ARM922" 0x926 "ARM926" 0x940 "ARM940"
      0x946 "ARM946" 0x966 "ARM966" 0xa20 "ARM1020" 0xa22 "ARM1022" 0xa26 "ARM1026"
      0xb02 "ARM11 MPCore" 0xb36 "ARM1136" 0xb56 "ARM1156" 0xb76 "ARM1176" 0xc05 "Cortex-A5"
      0xc07 "Cortex-A7" 0xc08 "Cortex-A8" 0xc09 "Cortex-A9" 0xc0d "Cortex-A12" 0xc0f "Cortex-A15"
      0xc0e "Cortex-A17" 0xc14 "Cortex-R4" 0xc15 "Cortex-R5" 0xc17 "Cortex-R7" 0xc18 "Cortex-R8"
      0xc20 "Cortex-M0" 0xc21 "Cortex-M1" 0xc23 "Cortex-M3" 0xc24 "Cortex-M4" 0xc20 "Cortex-M7"
      0xc60 "Cortex-M0+" 0xd01 "Cortex-A32" 0xd03 "Cortex-A53" 0xd04 "Cortex-A35" 0xd05 "Cortex-A55" 0xd07 "Cortex-A57"
      0xd08 "Cortex-A72" 0xd09 "Cortex-A73" 0xd0a "Cortex-A75" 0xd13 "Cortex-R52" 0xd20 "Cortex-M23" 0xd21 "Cortex-M33")
  set(brcm_list 0x516 "ThunderX2")
  set(dec_list 0xa10 "SA110" 0xa11 "SA1100")
  set(cavium_list 0x0a0 "ThunderX" 0x0a1 "ThunderX 88XX" 0x0a2 "ThunderX 81XX"
      0x0a3 "ThunderX 83XX" 0x0af "ThunderX2 99xx")
  set(apm_list 0x000 "X-Gene")
  set(qcom_list 0x00f "Scorpion" 0x02d "Scorpion" 0x04d "Krait" 0x06f "Krait" 0x201 "Kryo"
      0x205 "Kryo" 0x211 "Kryo" 0x800 "Falkor V1/Kryo" 0x801 "Kryo V2" 0xc00 "Falkor" 0xc01 "Saphira")
  set(samsung_list 0x001 "exynos-m1" )
  set(nvidia_list 0x000 "Denver" 0x003 "Denver 2")
  set(marvell_list 0x131 "Feroceon 88FR131" 0x581 "PJ4/PJ4b" 0x584 "PJ4B-MP")
  set(intel_list 0x200 "i80200" 0x210 "PXA250A" 0x212 "PXA210A" 0x242 "i80321-400" 0x243 "i80321-600"
      0x290 "PXA250B/PXA26x" 0x292 "PXA210B" 0x2c2 "i80321-400-B0" 0x2c3 "i80321-600-B0" 0x2d0 "PXA250C/PXA255/PXA26x"
      0x2d2 "PXA210C" 0x411 "PXA27x" 0x41c "IPX425-533" 0x41d "IPX425-400" 0x41f "IPX425-266" 0x682 "PXA32x"
      0x683 "PXA930/PXA935" 0x688 "PXA30x" 0x689 "PXA31x" 0xb11 "SA1110" 0xc12 "IPX1200")
  set(hw_implementer
      # id index   name
      0x41 arm     "ARM"
      0x42 brcm    "Broadcom"
      0x43 cavium  "Cavium"
      0x44 dec     "DEC"
      0x4e nvidia  "Nvidia"
      0x50 apm     "APM"
      0x51 qcom    "Qualcomm"
      0x53 samsung "Samsung"
      0x56 marvell "Marvell"
      0x69 intel   "Intel")

  set(_cpu_architecture)
  set(_cpu_implementer)
  set(_cpu_part)
  if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux") # Linux and Android
    if(NOT _PROC_CPUINFO) # not override for unit-test
      set(_PROC_CPUINFO "/proc/cpuinfo")
    endif()
    file(READ ${_PROC_CPUINFO} _cpuinfo)
    string(REGEX REPLACE ".*CPU[ \t]*architecture[ \t]*:[ \t]+([6-9]).*" "\\1" _cpu_architecture "${_cpuinfo}")
    string(REGEX REPLACE ".*CPU[ \t]*variant[ \t]*:[ \t]+(0x[a-f0-9]+).*" "\\1" _cpu_variant "${_cpuinfo}")
    string(REGEX REPLACE ".*CPU[ \t]*implementer[ \t]*:[ \t]+(0x[a-f0-9]+).*" "\\1" _cpu_implementer "${_cpuinfo}")
    string(REGEX REPLACE ".*CPU[ \t]*part[ \t]*:[ \t]+(0x[a-f0-9]+).*" "\\1" _cpu_part "${_cpuinfo}")
    # detect implementer and part ID
    list(FIND hw_implementer ${_cpu_implementer} _found)
    if(_found GREATER -1)
      math(EXPR index "${_found}+2")
      list(GET hw_implementer ${index} _implementer)
      math(EXPR index "${_found}+1")
      list(GET hw_implementer ${index} PART)
      # look cpu part
      list(FIND ${PART}_list ${_cpu_part} _found)
      if(_found GREATER -1)
        math(EXPR index "${_found}+1")
        list(GET ${PART}_list ${index} _core)
      else()
        set(_core Unknown)
      endif()
    else()
      set(_implementer Unknown)
      set(_core Unknown)
    endif()
    set(_archname "ARMv${_cpu_architecture}")
  else()
    set(_implementer Unknown)
    set(_core Unknown)
    set(_archname Unknown)
  endif()
  set(${outvar1} "${_implementer}" PARENT_SCOPE)
  set(${outvar2} "${_core}" PARENT_SCOPE)
  set(${outvar3} "${_archname}" PARENT_SCOPE)
endfunction()

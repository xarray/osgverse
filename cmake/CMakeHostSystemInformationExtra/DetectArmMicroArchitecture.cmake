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
  set(hw_implementer
      0x41 arm     "ARM"
      0x42 brcm    "Broadcom"
      0x43 cavium  "Cavium"
      0x44 dec     "DEC"
      0x46 fujitsu "Fujitsu"
      0x48 hisi    "HiSilicon"
      0x49 infineon "Infineon"
      0x4d motorola "Motorola/Freescale"
      0x4e nvidia  "NVIDIA"
      0x50 apm     "APM"
      0x51 qcom    "Qualcomm"
      0x53 samsung "Samsung"
      0x56 marvell "Marvell"
      0x61 apple   "Apple"
      0x66 faraday "Faraday"
      0x69 intel   "Intel"
      0x6d msft    "Microsoft"
      0x70 phytium "Phytium"
      0xc0 ampere  "Ampere")
  
  # part : corename  for each implementer
  set(arm_list
      0x810 "ARM810" 0x920 "ARM920" 0x922 "ARM922" 0x926 "ARM926" 0x940 "ARM940"
      0x946 "ARM946" 0x966 "ARM966" 0xa20 "ARM1020" 0xa22 "ARM1022" 0xa26 "ARM1026"
      0xb02 "ARM11-MPCore" 0xb36 "ARM1136" 0xb56 "ARM1156" 0xb76 "ARM1176"
      0xc05 "Cortex-A5" 0xc07 "Cortex-A7" 0xc08 "Cortex-A8" 0xc09 "Cortex-A9"
      0xc0d "Cortex-A12/A17" 0xc0e "Cortex-A17" 0xc0f "Cortex-A15"
      0xc14 "Cortex-R4" 0xc15 "Cortex-R5" 0xc17 "Cortex-R7" 0xc18 "Cortex-R8"
      0xc20 "Cortex-M0" 0xc21 "Cortex-M1" 0xc23 "Cortex-M3" 0xc24 "Cortex-M4"
      0xc27 "Cortex-M7" 0xc60 "Cortex-M0+"
      0xd01 "Cortex-A32" 0xd02 "Cortex-A34" 0xd03 "Cortex-A53" 0xd04 "Cortex-A35"
      0xd05 "Cortex-A55" 0xd06 "Cortex-A65" 0xd07 "Cortex-A57" 0xd08 "Cortex-A72"
      0xd09 "Cortex-A73" 0xd0a "Cortex-A75" 0xd0b "Cortex-A76" 0xd0d "Cortex-A77"
      0xd0e "Cortex-A76AE"
      0xd13 "Cortex-R52" 0xd14 "Cortex-R82AE" 0xd15 "Cortex-R82" 0xd16 "Cortex-R52+"
      0xd20 "Cortex-M23" 0xd21 "Cortex-M33" 0xd22 "Cortex-M55" 0xd23 "Cortex-M85"
      0xd24 "Cortex-M52"
      0xd41 "Cortex-A78" 0xd42 "Cortex-A78AE" 0xd43 "Cortex-A65AE"
      0xd46 "Cortex-A510" 0xd47 "Cortex-A710"
      0xd4b "Cortex-A78C" 0xd4d "Cortex-A715"
      0xd80 "Cortex-A520" 0xd81 "Cortex-A720" 0xd87 "Cortex-A725"
      0xd88 "Cortex-A520AE" 0xd89 "Cortex-A720AE" 0xd8f "Cortex-A320"
      0xd44 "Cortex-X1" 0xd48 "Cortex-X2" 0xd4c "Cortex-X1C"
      0xd4e "Cortex-X3" 0xd82 "Cortex-X4" 0xd85 "Cortex-X925"
      0xd0c "Neoverse-N1" 0xd40 "Neoverse-V1" 0xd49 "Neoverse-N2" 0xd4a "Neoverse-E1"
      0xd4f "Neoverse-V2" 0xd83 "Neoverse-V3AE" 0xd84 "Neoverse-V3" 0xd8e "Neoverse-N3"
      0xd8a "C1-Nano" 0xd8b "C1-Pro" 0xd8c "C1-Ultra" 0xd90 "C1-Premium")
  set(brcm_list 0x00f "Brahma-B15" 0x100 "Brahma-B53" 0x516 "ThunderX2")
  set(dec_list 0xa10 "SA110" 0xa11 "SA1100")
  set(cavium_list 0x0a0 "ThunderX" 0x0a1 "ThunderX-88XX" 0x0a2 "ThunderX-81XX"
      0x0a3 "ThunderX-83XX" 0x0af "ThunderX2-99xx")
  set(fujitsu_list 0x001 "A64FX" 0x003 "MONAKA")
  set(hisi_list
      0xd01 "TaiShan-v110"      # Kunpeng 920
      0xd02 "TaiShan-v120"      # Qilin 990A / 9000S
      0xd40 "Cortex-A76"        # Qilin 980
      0xd41 "Cortex-A77")       # Qilin 9000
  set(nvidia_list
      0x000 "Denver"            # Jetson TX2
      0x003 "Denver-2"          # Jetson TX2
      0x004 "Carmel"            # Jetson AGX Xavier
      0x010 "Olympus")          # Grace
  set(apm_list 0x000 "X-Gene")
  set(qcom_list
      0x001 "Oryon"
      0x00f "Scorpion" 0x02d "Scorpion" 0x04d "Krait" 0x06f "Krait"
      0x201 "Kryo" 0x205 "Kryo" 0x211 "Kryo"
      0x800 "Falkor-V1/Kryo" 0x801 "Kryo-V2"
      0x802 "Kryo-3XX-Gold" 0x803 "Kryo-3XX-Silver"
      0x804 "Kryo-4XX-Gold" 0x805 "Kryo-4XX-Silver"
      0xc00 "Falkor" 0xc01 "Saphira")
  set(samsung_list 0x001 "Exynos-M1" 0x002 "Exynos-M3" 0x003 "Exynos-M4" 0x004 "Exynos-M5")
  set(marvell_list 0x131 "Feroceon-88FR131" 0x581 "PJ4/PJ4b" 0x584 "PJ4B-MP")
  set(faraday_list 0x526 "FA526" 0x626 "FA626")
  set(intel_list 0x200 "i80200" 0x210 "PXA250A" 0x212 "PXA210A" 0x242 "i80321-400"
      0x243 "i80321-600" 0x290 "PXA250B/PXA26x" 0x292 "PXA210B" 0x2c2 "i80321-400-B0"
      0x2c3 "i80321-600-B0" 0x2d0 "PXA250C/PXA255/PXA26x" 0x2d2 "PXA210C" 0x411 "PXA27x"
      0x41c "IPX425-533" 0x41d "IPX425-400" 0x41f "IPX425-266" 0x682 "PXA32x"
      0x683 "PXA930/PXA935" 0x688 "PXA30x" 0x689 "PXA31x" 0xb11 "SA1110" 0xc12 "IPX1200")
  set(msft_list 0xd49 "Azure-Cobalt-100")
  set(phytium_list
      0x303 "FTC310"            # Feiteng E2000
      0x660 "FTC660" 0x661 "FTC661" 0x662 "FTC662"
      0x663 "FTC663"            # Feiteng D2000
      0x664 "FTC664"
      0x862 "FTC862")           # Feiteng D3000
  set(ampere_list 0xac3 "Ampere-1" 0xac4 "Ampere-1a")

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
        set(_core "Unknown-${_cpu_part}")
      endif()
    else()
      set(_implementer "Unknown-${_cpu_implementer}")
      set(_core Unknown)
    endif()
    set(_archname "v${_cpu_architecture}")
  else()
    set(_implementer Unknown)
    set(_core Unknown)
    set(_archname Unknown)
  endif()
  set(${outvar1} "${_implementer}" PARENT_SCOPE)
  set(${outvar2} "${_core}" PARENT_SCOPE)
  set(${outvar3} "${_archname}" PARENT_SCOPE)
endfunction()

PROJECT(Main_Android)
CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

GET_FILENAME_COMPONENT(SDL2_SOURCE_DIR ../../../SDL2 ABSOLUTE)
GET_FILENAME_COMPONENT(OSG_3RDPARTY_SOURCE_DIR ../../helpers/toolchain_builder ABSOLUTE)
GET_FILENAME_COMPONENT(OSG_SOURCE_DIR ../../../OpenSceneGraph ABSOLUTE)
GET_FILENAME_COMPONENT(OSGVERSE_SOURCE_DIR ../.. ABSOLUTE)

GET_FILENAME_COMPONENT(SDL2_BUILD_DIR ../../build/sdl2_android ABSOLUTE)
GET_FILENAME_COMPONENT(OSG_3RDPARTY_BUILD_DIR ../../build/3rdparty_android ABSOLUTE)
GET_FILENAME_COMPONENT(OSG_BUILD_DIR ../../build/osg_android/${CMAKE_BUILD_TYPE}/${CMAKE_ANDROID_ARCH_ABI} ABSOLUTE)
GET_FILENAME_COMPONENT(OSGVERSE_BUILD_DIR ../../build/verse_android/${CMAKE_BUILD_TYPE}/${CMAKE_ANDROID_ARCH_ABI} ABSOLUTE)

FIND_PACKAGE(OpenMP)
IF(OPENMP_FOUND)
    # Compile options only: keeping OpenMP in the linker flags would make the library
    # depend on the shared libomp.so, which the NDK provides for building only (see
    # the link step below, where the static runtime is used instead)
    ADD_COMPILE_OPTIONS(${OpenMP_C_FLAGS} ${OpenMP_CXX_FLAGS})
ENDIF()

# Build main bridge library
ADD_DEFINITIONS(-DSDL_DISABLE_ARM_NEON_H)
INCLUDE_DIRECTORIES("${SDL2_SOURCE_DIR}/include"
                    "${OSG_BUILD_DIR}/include"
                    "${OSG_SOURCE_DIR}/include"
                    ${OSGVERSE_SOURCE_DIR})
LINK_DIRECTORIES("${SDL2_BUILD_DIR}/lib/${CMAKE_BUILD_TYPE}/${CMAKE_ANDROID_ARCH_ABI}"
                 "${OSG_3RDPARTY_BUILD_DIR}/lib/${CMAKE_BUILD_TYPE}/${CMAKE_ANDROID_ARCH_ABI}"
                 "${OSG_BUILD_DIR}/lib" "${OSGVERSE_BUILD_DIR}/lib")
ADD_LIBRARY(osgVerseAndroid SHARED src/main.cpp)
TARGET_LINK_LIBRARIES(osgVerseAndroid log android EGL GLESv1_CM GLESv2 GLESv3 OpenSLES SDL2
                      debug osgVerseDependencyd optimized osgVerseDependency
                      debug osgVerseModelingd optimized osgVerseModeling
                      debug osgVerseAnimationd optimized osgVerseAnimation
                      debug osgVersePipelined optimized osgVersePipeline
                      debug osgVerseScriptd optimized osgVerseScript
                      debug osgVerseUId optimized osgVerseUI
                      debug osgVerseReaderWriterd optimized osgVerseReaderWriter
                      debug osgVerseWrappersd optimized osgVerseWrappers
                      debug osgdb_glsld optimized osgdb_glsl
                      debug osgdb_verse_gltfd optimized osgdb_verse_gltf
                      debug osgdb_verse_imaged optimized osgdb_verse_image
                      debug OpenThreadsd optimized OpenThreads
                      debug osgd optimized osg
                      debug osgDBd optimized osgDB
                      debug osgUtild optimized osgUtil
                      debug osgTextd optimized osgText
                      debug osgGAd optimized osgGA
                      debug osgTerraind optimized osgTerrain
                      debug osgViewerd optimized osgViewer)
IF(OPENMP_FOUND)
    # An Android application has to ship the OpenMP runtime itself, so the static one
    # coming with clang is linked in directly instead of the "omp" shared library
    EXECUTE_PROCESS(COMMAND ${CMAKE_CXX_COMPILER} -print-resource-dir
                    OUTPUT_VARIABLE OpenMPResourceDir OUTPUT_STRIP_TRAILING_WHITESPACE)
    
    # The runtime library is stored under the architecture name of the NDK, which differs
    # from the ABI name of the Android platform
    IF(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
        SET(OpenMPArchName "aarch64")
    ELSEIF(CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
        SET(OpenMPArchName "armv7a")
    ELSEIF(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86")
        SET(OpenMPArchName "i686")
    ELSE()
        SET(OpenMPArchName "${CMAKE_ANDROID_ARCH_ABI}")
    ENDIF()
    SET(OpenMPStaticLibrary "${OpenMPResourceDir}/lib/linux/${OpenMPArchName}/libomp.a")
    IF(EXISTS "${OpenMPStaticLibrary}")
        TARGET_LINK_LIBRARIES(osgVerseAndroid "${OpenMPStaticLibrary}")
    ELSE()
        TARGET_LINK_LIBRARIES(osgVerseAndroid omp)
    ENDIF()
ENDIF()

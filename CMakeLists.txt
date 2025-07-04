CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
PROJECT(osgVerse)

SET(VERSE_VERSION "0.8.5")
SET(VERSE_GRAPHICS_API "GL")
SET(VERSE_PLATFORM "")
SET(CMAKE_DEBUG_POSTFIX "d")
SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")
SET(FIND_LIB_POSTFIX lib)
SET_PROPERTY(GLOBAL PROPERTY VERSE_DEPENDENCIES "")
SET_PROPERTY(GLOBAL PROPERTY VERSE_PLUGIN_LIBRARIES "")
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)
INCLUDE(VerseMacros)

IF(POLICY CMP0146)
    CMAKE_POLICY(SET CMP0146 OLD)
ENDIF()

######################################### Install Configurations
SET(VERSE_3RDPARTY_PATH "${CMAKE_SOURCE_DIR}/../Dependencies" CACHE STRING "Dependency root path")
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    SET(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/sdk" CACHE PATH "..." FORCE)
ENDIF()

IF(NOT WIN32)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -fPIC")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -frtti -fpermissive")
    SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/bin;${CMAKE_INSTALL_PREFIX}/lib")
ENDIF(NOT WIN32)

IF(MSVC)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4251 /wd4275 /wd4996 /nologo /bigobj")
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
ELSEIF(MINGW OR CYGWIN)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og -Wa,-mbig-obj")
ENDIF()

SET(INSTALL_INCDIR include)
SET(INSTALL_BINDIR bin)
SET(INSTALL_PLUGINDIR bin)
IF(WIN32)
    SET(INSTALL_LIBDIR bin)
    SET(INSTALL_ARCHIVEDIR lib)
ELSE()
    INCLUDE(GNUInstallDirs)
    SET(INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})
    SET(INSTALL_ARCHIVEDIR ${CMAKE_INSTALL_LIBDIR})
ENDIF()

IF(APPLE)
    SET(CMAKE_SHARED_LIBRARY_SUFFIX ".so")
ENDIF(APPLE)

######################################### SIMD Finders
INCLUDE(CMakeHostSystemInformationExtra)
INCLUDE(GetCPUSIMDFeatures)
MACRO(APPLY_SIMD_FLAGS CHECK_NAME DEF_FLAG COMPILER_FLAG)

    IF(${CHECK_NAME} IN_LIST VERSE_SIMD_FEATURES)
        IF(MSVC)
            SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${DEF_FLAG}")
            SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DEF_FLAG}")
        ELSE()
            SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMPILER_FLAG}")
            SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILER_FLAG}")
            ADD_DEFINITIONS(${DEF_FLAG})
        ENDIF()
    ENDIF()

ENDMACRO(APPLY_SIMD_FLAGS)

SET(VERSE_SYSTEM_ARCHITECT "X86")
IF("${CMAKE_HOST_SYSTEM_PROCESSOR}" MATCHES "(ARM|aarch64)")
    SET(VERSE_SYSTEM_ARCHITECT "ARM")
    SET(FIND_SUBLIB_POSTFIX aarch64-linux-gnu)
ELSEIF("${CMAKE_HOST_SYSTEM_PROCESSOR}" MATCHES "(PPC|PPC64)")
    SET(VERSE_SYSTEM_ARCHITECT "PowerPC")
ELSE()
    SET(FIND_SUBLIB_POSTFIX x86_64-linux-gnu)
ENDIF()

SET(VERSE_NO_SIMD_FEATURES OFF CACHE BOOL "Enable to ignore all SIMD features (when struggling with compile errors)")
SET(VERSE_SIMD_FEATURES "")
IF(VERSE_NO_SIMD_FEATURES)
    # Do nothing...
ELSEIF(USE_WASM_OPTIONS)
    # https://emscripten.org/docs/porting/simd.html
    SET(VERSE_SIMD_FEATURES sse sse2 sse3 avx)
    MESSAGE(STATUS "[osgVerse] WebAssembly Porting. Features: ${VERSE_SIMD_FEATURES}")
ELSE()
    CMAKE_HOST_SYSTEM_INFORMATION_EXTRA(RESULT VERSE_CPU_ARCHITECT QUERY PROCESSOR_MICRO_ARCHITECTURE)
    IF(VERSE_CPU_ARCHITECT)
        GET_CPU_SIMD_FEATURES(VERSE_SIMD_FEATURES "${VERSE_CPU_ARCHITECT}")
        MESSAGE(STATUS "[osgVerse] CPU Architecture: ${VERSE_CPU_ARCHITECT}. Features: ${VERSE_SIMD_FEATURES}")
    ENDIF(VERSE_CPU_ARCHITECT)
ENDIF()

IF(VERSE_SIMD_FEATURES)
    # Find SIMD instruction supporting using the following statement:
    #   IF("sse3" IN_LIST VERSE_SIMD_FEATURES) ...
    IF(USE_WASM_OPTIONS)
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msimd128")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msimd128")
    ELSE()
        APPLY_SIMD_FLAGS("sse" "-D__SSE__" "-msse" "")
        APPLY_SIMD_FLAGS("sse2" "-D__SSE2__" "-msse2")
        APPLY_SIMD_FLAGS("sse3" "-D__SSE3__" "-msse3")
        APPLY_SIMD_FLAGS("ssse3" "-D__SSSE3__" "-mssse3")
        APPLY_SIMD_FLAGS("sse4.1" "-D__SSE4_1__" "-msse4.1")
        APPLY_SIMD_FLAGS("sse4.2" "-D__SSE4_2__" "-msse4.2")
        APPLY_SIMD_FLAGS("avx" "-D__AVX__" "-mavx")
        APPLY_SIMD_FLAGS("avx2" "-D__AVX2__" "-mavx2")
        APPLY_SIMD_FLAGS("avx512" "-D__AVX512__" "-mavx512")
        APPLY_SIMD_FLAGS("fma" "-D__FMA__" "-mfma")

        IF(MSVC)
            IF("avx512" IN_LIST VERSE_SIMD_FEATURES)
                ADD_COMPILE_OPTIONS(/arch:AVX512)
            ELSEIF("avx2" IN_LIST VERSE_SIMD_FEATURES)
                ADD_COMPILE_OPTIONS(/arch:AVX2)
            ELSEIF("avx" IN_LIST VERSE_SIMD_FEATURES)
                ADD_COMPILE_OPTIONS(/arch:AVX)
            ELSEIF("sse4.2" IN_LIST VERSE_SIMD_FEATURES)
                ADD_COMPILE_OPTIONS(/arch:SSE4.2)
            ENDIF()
        ENDIF()
    ENDIF()
ENDIF()

######################################### Core Dependency Finders
SET(THIRDPARTY_ROOT ${VERSE_3RDPARTY_PATH}/x86)
IF(CMAKE_CL_64)
    SET(FIND_LIB_POSTFIX lib64)
    SET(THIRDPARTY_ROOT ${VERSE_3RDPARTY_PATH}/x64)
ENDIF(CMAKE_CL_64)

SET(USE_UWP_OPTIONS OFF)
IF(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
    SET(USE_UWP_OPTIONS ON)
    MESSAGE("[osgVerse] Building Windows Store (UWP) libraries...")
ENDIF()

SET(BUILD_WASM_64 OFF)
IF(USE_WASM_OPTIONS)
    IF(USE_WASM_OPTIONS EQUAL 2)
        SET(THIRDPARTY_ROOT ${VERSE_3RDPARTY_PATH}/wasm64)
        SET(BUILD_WASM_64 ON)
    ELSE()
        SET(THIRDPARTY_ROOT ${VERSE_3RDPARTY_PATH}/wasm)
    ENDIF()
ELSEIF(USE_UWP_OPTIONS)
    SET(THIRDPARTY_ROOT ${VERSE_3RDPARTY_PATH}/uwp)
ELSEIF(VERSE_SYSTEM_ARCHITECT STREQUAL "ARM")
    SET(THIRDPARTY_ROOT ${VERSE_3RDPARTY_PATH}/aarch64)
ELSEIF(ANDROID)
    SET(THIRDPARTY_ROOT ${VERSE_3RDPARTY_PATH}/android)
ELSEIF(IOS)
    SET(THIRDPARTY_ROOT ${VERSE_3RDPARTY_PATH}/ios)
ELSEIF(APPLE)
    SET(THIRDPARTY_ROOT ${VERSE_3RDPARTY_PATH}/apple)
ENDIF()

IF(MSVC)
    SET(THIRDPARTY_ROOT_POSTFIX "msvc")
    IF(MSVC_VERSION LESS 1910)
        SET(THIRDPARTY_ROOT_POSTFIX "msvc14")
    ELSEIF(MSVC_VERSION LESS 1920)
        SET(THIRDPARTY_ROOT_POSTFIX "msvc15")
    ELSEIF(MSVC_VERSION LESS 1930)
        SET(THIRDPARTY_ROOT_POSTFIX "msvc16")
    ELSEIF(MSVC_VERSION LESS 1950)
        SET(THIRDPARTY_ROOT_POSTFIX "msvc17")
    ENDIF()

    IF(EXISTS "${THIRDPARTY_ROOT}/${THIRDPARTY_ROOT_POSTFIX}")
        SET(THIRDPARTY_ROOT ${THIRDPARTY_ROOT}/${THIRDPARTY_ROOT_POSTFIX})
    ENDIF()
ENDIF(MSVC)
GET_FILENAME_COMPONENT(THIRDPARTY_ROOT ${THIRDPARTY_ROOT} ABSOLUTE)

IF(OSG_ROOT)
    SET(OSG_INCLUDE_DIR "${OSG_ROOT}/include")
    SET(OSG_BUILD_INCLUDE_DIR "${OSG_ROOT}/include")
    SET(OSG_LIB_DIR "${OSG_ROOT}/lib")
ELSE(OSG_ROOT)
    FIND_PATH(OSG_INCLUDE_DIR osg/Referenced
        PATHS ${THIRDPARTY_ROOT}/include
        $ENV{OSG_ROOT}/include
        $ENV{OSG_DIR}/include
        /usr/include
        /usr/local/include
    )

    FIND_PATH(OSG_BUILD_INCLUDE_DIR osg/Config
        PATHS ${THIRDPARTY_ROOT}/include
        $ENV{OSG_ROOT}/include
        $ENV{OSG_DIR}/include
        /usr/include
        /usr/local/include
    )

    FIND_PATH(OSG_LIB_DIR
        NAMES libosg.a libosg.dll.a libosg.dylib libosg.so osg.lib
        PATHS ${THIRDPARTY_ROOT}/lib
        $ENV{OSG_ROOT}/lib $ENV{OSG_ROOT}/${FIND_LIB_POSTFIX}
        $ENV{OSG_DIR}/lib $ENV{OSG_DIR}/${FIND_LIB_POSTFIX}
        ${OSG_BUILD_INCLUDE_DIR}/../lib
        /usr/lib /usr/${FIND_LIB_POSTFIX} /usr/lib/${FIND_SUBLIB_POSTFIX}
        /usr/local/lib /usr/local/${FIND_LIB_POSTFIX}
    )
ENDIF(OSG_ROOT)
SET(VERSE_DEBUG_POSTFIX "_Debug" CACHE STRING "Postfix of thirdparty built-libraries")
SET(OSG_DEBUG_POSTFIX "d" CACHE STRING "Postfix of OSG built-libraries")
SET(OSG_VERSION_THREADS "")
SET(OSG_VERSION_CORE "")

MACRO(LINK_OSG_LIBRARY PROJECT_NAME)
    FOREACH(OSG_LIB_NAME ${ARGN})
        STRING(FIND ${OSG_LIB_NAME} "OpenThreads" FOUND_OT_INDEX)
        IF(FOUND_OT_INDEX GREATER "-1")
            TARGET_LINK_LIBRARIES(${PROJECT_NAME}
                debug ${OSG_VERSION_THREADS}${OSG_LIB_NAME}${OSG_DEBUG_POSTFIX}
                optimized ${OSG_VERSION_THREADS}${OSG_LIB_NAME})
        ELSE()
            TARGET_LINK_LIBRARIES(${PROJECT_NAME}
                debug ${OSG_VERSION_CORE}${OSG_LIB_NAME}${OSG_DEBUG_POSTFIX}
                optimized ${OSG_VERSION_CORE}${OSG_LIB_NAME})
        ENDIF()
    ENDFOREACH()
ENDMACRO(LINK_OSG_LIBRARY)

MACRO(RELINK_OSGPLUGINS OUTPUT_LIST)
    FOREACH(OSGPLUGIN_NAME ${ARGN})
        SET(${OUTPUT_LIST} ${${OUTPUT_LIST}}
            debug ${OSGPLUGIN_NAME}${OSG_DEBUG_POSTFIX} optimized ${OSGPLUGIN_NAME})
    ENDFOREACH()
ENDMACRO(RELINK_OSGPLUGINS)

SET(VERSE_FIND_LEGACY_OPENGL OFF CACHE BOOL "Enable to find OpenGL in legacy mode")
IF(VERSE_FIND_LEGACY_OPENGL)
    SET(OpenGL_GL_PREFERENCE "LEGACY")
ELSE(VERSE_FIND_LEGACY_OPENGL)
    SET(OpenGL_GL_PREFERENCE "GLVND")
ENDIF(VERSE_FIND_LEGACY_OPENGL)

FIND_PACKAGE(OpenGL)
FIND_PACKAGE(OpenMP)
FIND_PACKAGE(CUDA)
IF(OPENMP_FOUND)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
ENDIF()

IF(USE_WASM_OPTIONS OR ANDROID OR APPLE)
    # do nothing
ELSEIF(NOT OPENGL_gl_LIBRARY)
    IF(OPENGL_opengl_LIBRARY)
        SET(OPENGL_gl_LIBRARY ${OPENGL_opengl_LIBRARY})
    ELSE(OPENGL_opengl_LIBRARY)
        MESSAGE(FATAL_ERROR "[osgVerse] Dependency OpenGL not found. All following steps may not work...")
    ENDIF(OPENGL_opengl_LIBRARY)
ENDIF()

######################################### Global Options
SET(VERSE_INSTALL_PDB_FILES ON CACHE BOOL "Enable to install PDB files along with executables and dynamic libraries")
SET(VERSE_BUILD_EXAMPLES ON CACHE BOOL "Enable build of examples and tests")
SET(VERSE_BUILD_EXPORTERS OFF CACHE BOOL "Enable build of exporters of other software (e.g., 3dsmax)")
SET(VERSE_BUILD_3RDPARTIES ON CACHE BOOL "Enable build of common libraries like FreeType, Jpeg and PNG")
SET(VERSE_BUILD_WITH_QT OFF CACHE BOOL "Enable build of Qt based applications and tests")
SET(VERSE_BUILD_WITH_CUDA ON CACHE BOOL "Enable build of NVIDIA CUDA based libraries and tests")
SET(VERSE_BUILD_DEPRECATED_TESTS OFF CACHE BOOL "Enable build of deprecated tests")
SET(VERSE_SUPPORT_CPP17 OFF CACHE BOOL "Enable build of libraries using C++ 17 standard")
SET(VERSE_STATIC_BUILD OFF CACHE BOOL "Enable static build of osgVerse libraries")
SET(VERSE_USE_OSG_STATIC OFF CACHE BOOL "Use static build of OpenSceneGraph (will force osgVerse to be static)")
SET(VERSE_USE_MTT_DRIVER OFF CACHE BOOL "Use MooreThreads MTT drivers")

IF(VERSE_SUPPORT_CPP17)
    SET(CMAKE_CXX_STANDARD 17)
ENDIF(VERSE_SUPPORT_CPP17)

IF(VERSE_USE_MTT_DRIVER)
    SET(VERSE_BUILD_WITH_MUSA ON CACHE BOOL "Enable build of MooreThreads MUSA based libraries and tests")
    LIST(APPEND CMAKE_MODULE_PATH /usr/local/musa/cmake)
    FIND_PACKAGE(MUSA)
ENDIF(VERSE_USE_MTT_DRIVER)

IF(MSVC)
    SET(VERSE_USE_FORCED_MULTIPLE OFF CACHE BOOL "Use /FORCE:MULTIPLE flag under MSVC")
    IF(VERSE_USE_FORCED_MULTIPLE)
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /FORCE:MULTIPLE")
        SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /FORCE:MULTIPLE")
    ENDIF(VERSE_USE_FORCED_MULTIPLE)
ELSEIF(UNIX)
    SET(VERSE_USE_DWARF OFF CACHE BOOL "Enable detailed debug-info and symbols. 'libdw-dev' must be installed")
    SET(VERSE_USE_GLIBCXX11_ABI ON CACHE BOOL
        "Enable _GLIBCXX_USE_CXX11_ABI for newer GCC compilers. Set to OFF if you are using old ones")
ENDIF()

IF(VERSE_USE_OSG_STATIC)
    SET(VERSE_STATIC_BUILD ON CACHE BOOL "Enable static build of osgVerse" FORCE)
ENDIF(VERSE_USE_OSG_STATIC)

IF(USE_WASM_OPTIONS)
    SET(VERSE_WASM_USE_PTHREAD ON CACHE BOOL "Enable Pthread for WASM, which requires SharedArrayBuffer (COOP / COEP) on server-side")
    SET(VERSE_WASM_OPTIMIZE_SIZE OFF CACHE BOOL "Enable -O3 for WASM, which is very slow but with small generated size")
    SET(VERSE_USE_OSG_STATIC ON CACHE BOOL "Enable static build of osgVerse libraries" FORCE)
    SET(VERSE_STATIC_BUILD ON CACHE BOOL "Enable static build of osgVerse" FORCE)
    IF(BUILD_WASM_64)
        ADD_DEFINITIONS("-s MEMORY64=1 -s USE_SDL=2 -pthread")
    ELSE(BUILD_WASM_64)
        ADD_DEFINITIONS("-s USE_SDL=2 -pthread")
    ENDIF(BUILD_WASM_64)
ELSEIF(ANDROID)
    SET(VERSE_USE_OSG_STATIC ON CACHE BOOL "Enable static build of osgVerse libraries" FORCE)
    SET(VERSE_STATIC_BUILD ON CACHE BOOL "Enable static build of osgVerse" FORCE)
    ADD_DEFINITIONS(-DBACKWARD_SYSTEM_UNKNOWN=1)
ENDIF()

######################################### CUDA / MUSA configurations
IF(CUDA_FOUND)
    # Check CUDA / Compiler compatibility
    IF(MSVC)
        # https://quasar.ugent.be/files/doc/cuda-msvc-compatibility.html
        IF(CUDA_VERSION VERSION_GREATER_EQUAL 12.0)
            MATH(EXPR CUDA_CHECKVER "${MSVC_VERSION} - 1910")  # >= VS2017
        ELSEIF(CUDA_VERSION VERSION_GREATER_EQUAL 11.6)
            MATH(EXPR CUDA_CHECKVER "${MSVC_VERSION} - 1800")  # >= VS2013
        ELSEIF(CUDA_VERSION VERSION_GREATER_EQUAL 10.1)
            MATH(EXPR CUDA_CHECKVER "1919 - ${MSVC_VERSION}")  # <= VS2017
        ELSE()
            MATH(EXPR CUDA_CHECKVER "1919 - ${MSVC_VERSION}")  # <= VS2017
        ENDIF()
    ELSEIF(CMAKE_COMPILER_IS_GNUCC)
        # https://gist.github.com/ax3l/9489132
        IF(CUDA_VERSION VERSION_GREATER 11.8 AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 12)
            SET(CUDA_CHECKVER -1)
        ELSEIF(CUDA_VERSION VERSION_GREATER 11.3 AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 11)
            SET(CUDA_CHECKVER -1)
        ELSEIF(CUDA_VERSION VERSION_LESS_EQUAL 11.3 AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 10)
            SET(CUDA_CHECKVER -1)
        ENDIF()
    ENDIF()

    IF(CUDA_CHECKVER LESS 0)
        MESSAGE("[osgVerse] NVIDIA CUDA found: ${CUDA_VERSION}, But incompatible with MSVC version ${MSVC_VERSION}")
        SET(CUDA_FOUND FALSE)
    ELSE()
        SET(VERSE_NVCC_FLAGS "-gencode arch=compute_52,code=\"sm_52,compute_52\""
            CACHE STRING "NVCC Flags. See https://docs.nvidia.com/cuda/cuda-compiler-driver-nvcc/index.html#gpu-architecture-arch")
    ENDIF()

    SET(CUDA_HOST_COMPILER ${CMAKE_CXX_COMPILER})
    SET(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};${VERSE_NVCC_FLAGS})
ENDIF(CUDA_FOUND)

IF(MUSA_FOUND)
    SET(VERSE_MCC_FLAGS "--offload-arch=mp_21"
        CACHE STRING "Musa MCC Flags. MTT S50: mp_10; MTT S80/MTT S3000: mp_21; MTT S4000: mp_22")

    SET(MUSA_VERBOSE_BUILD ON)
    SET(MUSA_MCC_FLAGS ${MUSA_MCC_FLAGS} ${VERSE_MCC_FLAGS})
ENDIF(MUSA_FOUND)

######################################### Checking OSG version and definition
SET(VERSE_USE_EXTERNAL_GLCORE OFF)
SET(VERSE_USE_EXTERNAL_GLES2 OFF)
SET(VERSE_USE_EXTERNAL_GLES3 OFF)
IF(OSG_BUILD_INCLUDE_DIR)
    FILE(READ "${OSG_BUILD_INCLUDE_DIR}/osg/GL" OSG_GL_FILE_CONTENTS)
    STRING(FIND "${OSG_GL_FILE_CONTENTS}" "#define OSG_GL3_AVAILABLE" OSG_GLCORE_INDEX)
    STRING(FIND "${OSG_GL_FILE_CONTENTS}" "#define OSG_GLES2_AVAILABLE" OSG_GLES2_INDEX)
    STRING(FIND "${OSG_GL_FILE_CONTENTS}" "#define OSG_GLES3_AVAILABLE" OSG_GLES3_INDEX)

    IF(OSG_GLCORE_INDEX GREATER -1)
        MESSAGE(STATUS "[osgVerse] OSG is built with GL3/4 support.")
        SET(VERSE_USE_EXTERNAL_GLCORE ON)
        SET(VERSE_GRAPHICS_API "GLCORE")
    ENDIF()

    IF(OSG_GLES3_INDEX GREATER -1)
        MESSAGE(STATUS "[osgVerse] OSG is built with GLES3 support. Make sure to configure osgVerse to use GLES3, too.")
        FIND_PATH(OSG_GLES_INCLUDE_DIR GLES3/gl3.h PATHS ${THIRDPARTY_ROOT}/include /usr/include /usr/local/include)
        FIND_LIBRARY(OSG_GLES_LIBRARY NAMES libGLESv2.so libGLESv2.dylib libGLESv2.lib
                     HINTS ${THIRDPARTY_ROOT}/lib /usr/lib /usr/${FIND_LIB_POSTFIX} /usr/local/lib /usr/local/${FIND_LIB_POSTFIX})
        FIND_LIBRARY(OSG_EGL_LIBRARY NAMES libEGL.so libEGL.dylib libEGL.lib
                     HINTS ${THIRDPARTY_ROOT}/lib /usr/lib /usr/${FIND_LIB_POSTFIX} /usr/local/lib /usr/local/${FIND_LIB_POSTFIX})
        SET(VERSE_USE_EXTERNAL_GLES3 ON)
        SET(VERSE_GRAPHICS_API "GLES3")
    ELSEIF(OSG_GLES2_INDEX GREATER -1)
        MESSAGE(STATUS "[osgVerse] OSG is built with GLES2 support. Make sure to configure osgVerse to use GLES2, too.")
        FIND_PATH(OSG_GLES_INCLUDE_DIR GLES2/gl2.h PATHS ${THIRDPARTY_ROOT}/include /usr/include /usr/local/include)
        FIND_LIBRARY(OSG_GLES_LIBRARY NAMES libGLESv2.so libGLESv2.dylib libGLESv2.lib
                     HINTS ${THIRDPARTY_ROOT}/lib /usr/lib /usr/${FIND_LIB_POSTFIX} /usr/local/lib /usr/local/${FIND_LIB_POSTFIX})
        FIND_LIBRARY(OSG_EGL_LIBRARY NAMES libEGL.so libEGL.dylib libEGL.lib
                     HINTS ${THIRDPARTY_ROOT}/lib /usr/lib /usr/${FIND_LIB_POSTFIX} /usr/local/lib /usr/local/${FIND_LIB_POSTFIX})
        SET(VERSE_USE_EXTERNAL_GLES2 ON)
        SET(VERSE_GRAPHICS_API "GLES2")
    ENDIF()

    IF(VERSE_USE_EXTERNAL_GLES2 OR VERSE_USE_EXTERNAL_GLES3)
        IF(OSG_GLES_INCLUDE_DIR AND OSG_GLES_LIBRARY)
            SET(OPENGL_INCLUDE_DIR "${OSG_GLES_INCLUDE_DIR}" CACHE STRING "Set OpenGL include folder to use the GLES alternative" FORCE)
            SET(OPENGL_gl_LIBRARY "${OSG_GLES_LIBRARY}" CACHE STRING "Set OpenGL library to use the GLES alternative" FORCE)
            SET(EXTERNAL_LIBRARIES ${EXTERNAL_LIBRARIES} ${OSG_EGL_LIBRARY})
        ELSEIF(NOT USE_WASM_OPTIONS)
            MESSAGE("[osgVerse] Dependency GLES not found. The solution may be uncompilable...")
        ENDIF()
    ENDIF(VERSE_USE_EXTERNAL_GLES2 OR VERSE_USE_EXTERNAL_GLES3)
ENDIF(OSG_BUILD_INCLUDE_DIR)

SET(THIRDPARTY_INCLUDE_DIRS "")
SET(THIRDPARTY_LIBRARIES "")
SET(EXTERNAL_LIBRARIES ${EXTERNAL_LIBRARIES} ${OPENGL_gl_LIBRARY})
IF(OPENMP_FOUND)
    SET(EXTERNAL_LIBRARIES ${EXTERNAL_LIBRARIES} OpenMP::OpenMP_CXX)
ENDIF(OPENMP_FOUND)

SET(VERSE_WITH_COMMON_LIBRARIES OFF)
SET(VERSE_WITH_TIFF_LIBRARY OFF)
SET(VERSE_USE_EXTERNAL_GLES OFF)
IF(VERSE_USE_EXTERNAL_GLES2 OR VERSE_USE_EXTERNAL_GLES3)
    SET(VERSE_USE_EXTERNAL_GLES ON)
    ADD_DEFINITIONS(-DVERSE_NO_NATIVE_WINDOW)
ENDIF()

IF(ANDROID OR IOS OR USE_UWP_OPTIONS OR USE_WASM_OPTIONS)
    IF(NOT VERSE_USE_EXTERNAL_GLES)
        MESSAGE(FATAL_ERROR "[osgVerse] Dependency GLES not found, which is the only solution under current platform...")
    ENDIF()
ENDIF()

SET(OSG_MAJOR_VERSION 3)
SET(OSG_MINOR_VERSION 1)
SET(OSG_PATCH_VERSION 1)
IF(OSG_BUILD_INCLUDE_DIR)
    FILE(READ "${OSG_BUILD_INCLUDE_DIR}/OpenThreads/Version" OT_FILE_CONTENTS)
    FILE(READ "${OSG_BUILD_INCLUDE_DIR}/osg/Version" OSG_FILE_CONTENTS)
    STRING(FIND "${OT_FILE_CONTENTS}" "OPENTHREADS_SOVERSION" OT_VERSION_INDEX)
    STRING(FIND "${OSG_FILE_CONTENTS}" "OPENSCENEGRAPH_SOVERSION" OSG_VERSION_INDEX)
    STRING(FIND "${OSG_FILE_CONTENTS}" "OPENSCENEGRAPH_MAJOR_VERSION" OSGPLUGIN_INDEX0)
    STRING(FIND "${OSG_FILE_CONTENTS}" "OPENSCENEGRAPH_MINOR_VERSION" OSGPLUGIN_INDEX1)
    STRING(FIND "${OSG_FILE_CONTENTS}" "OPENSCENEGRAPH_PATCH_VERSION" OSGPLUGIN_INDEX2)

    MATH(EXPR OT_VERSION_INDEX "${OT_VERSION_INDEX} + 22")
    MATH(EXPR OSG_VERSION_INDEX "${OSG_VERSION_INDEX} + 25")
    MATH(EXPR OSGPLUGIN_INDEX0 "${OSGPLUGIN_INDEX0} + 29")
    MATH(EXPR OSGPLUGIN_INDEX1 "${OSGPLUGIN_INDEX1} + 29")
    MATH(EXPR OSGPLUGIN_INDEX2 "${OSGPLUGIN_INDEX2} + 29")
    STRING(SUBSTRING "${OT_FILE_CONTENTS}" "${OT_VERSION_INDEX}" "4" OT_VERSION)
    STRING(SUBSTRING "${OSG_FILE_CONTENTS}" "${OSG_VERSION_INDEX}" "12" OSG_VERSION)
    STRING(SUBSTRING "${OSG_FILE_CONTENTS}" "${OSGPLUGIN_INDEX0}" "5" OSG_MAJOR_VERSION)
    STRING(SUBSTRING "${OSG_FILE_CONTENTS}" "${OSGPLUGIN_INDEX1}" "5" OSG_MINOR_VERSION)
    STRING(SUBSTRING "${OSG_FILE_CONTENTS}" "${OSGPLUGIN_INDEX2}" "5" OSG_PATCH_VERSION)

    STRING(STRIP ${OT_VERSION} OT_VERSION)
    STRING(STRIP ${OSG_VERSION} OSG_VERSION)
    STRING(STRIP ${OSG_MAJOR_VERSION} OSG_MAJOR_VERSION)
    STRING(STRIP ${OSG_MINOR_VERSION} OSG_MINOR_VERSION)
    STRING(STRIP ${OSG_PATCH_VERSION} OSG_PATCH_VERSION)
    IF(VERSE_USE_OSG_STATIC)
        IF(MSVC)
            SET(OSG_VERSION_THREADS "ot${OT_VERSION}-")
            SET(OSG_VERSION_CORE "osg${OSG_VERSION}-")
        ENDIF(MSVC)

        SET(OSG_LIB_DIR ${OSG_LIB_DIR}
            "${OSG_LIB_DIR}/osgPlugins-${OSG_MAJOR_VERSION}.${OSG_MINOR_VERSION}.${OSG_PATCH_VERSION}")
        ADD_DEFINITIONS(-DOSG_LIBRARY_STATIC)
    ENDIF(VERSE_USE_OSG_STATIC)

    SET(INSTALL_PLUGINDIR "bin/osgPlugins-${OSG_MAJOR_VERSION}.${OSG_MINOR_VERSION}.${OSG_PATCH_VERSION}")
    ADD_DEFINITIONS(-DOSGPLUGIN_PREFIX="${INSTALL_PLUGINDIR}")
ENDIF(OSG_BUILD_INCLUDE_DIR)

######################################### Optional Dependencies
MESSAGE(STATUS "[osgVerse] Automatically Search for Dependencies in ${THIRDPARTY_ROOT}...")

FIND_DEPENDENCE(PYTHON "Python.h" "libpython.so;libpython3.so;python.lib;python3.lib" "")
FIND_DEPENDENCE(BULLET "btBulletDynamicsCommon.h" "libBulletDynamics.so;libBulletDynamics.a;BulletDynamics.lib" "bullet")
FIND_DEPENDENCE(DRACO "draco/draco_features.h" "libdraco.so;libdraco.a;draco.lib" "")
FIND_DEPENDENCE(FFMPEG "libavcodec/avcodec.h" "libavcodec.so;avcodec.lib" "")
FIND_DEPENDENCE(LIBCEF "cef_browser.h" "libcef.so;libcef.a;libcef.lib" "libcef")
FIND_DEPENDENCE(LIBIGL "igl/igl_inline.h" "" "")
FIND_DEPENDENCE(MIMALLOC "mimalloc.h" "libmimalloc.so;mimalloc.lib" "mimalloc")
IF(NOT USE_WASM_OPTIONS)
    FIND_DEPENDENCE(SDL2 "SDL.h" "libSDL2.so;libSDL2.a;SDL2.lib" "SDL")
    FIND_DEPENDENCE(VULKAN "vulkan/vulkan.h" "" "")
ENDIF(NOT USE_WASM_OPTIONS)

IF(MIMALLOC_FOUND)
    SET(VERSE_USE_MIMALLOC OFF CACHE BOOL "Enable use of mi-malloc to improve memory allocating")
ENDIF(MIMALLOC_FOUND)

######################################### Platform Definintions
ADD_DEFINITIONS(-DPREPENDED_HEADER="pipeline/Allocator.h")
IF(VERSE_USE_MIMALLOC)
    # TODO: static building support?
    SET(EXTERNAL_LIBRARIES mimalloc ${EXTERNAL_LIBRARIES})
    INCLUDE_DIRECTORIES(${MIMALLOC_INCLUDE_DIR}/mimalloc)
    ADD_DEFINITIONS(-DVERSE_USE_MIMALLOC)
ENDIF(VERSE_USE_MIMALLOC)

IF(ANDROID)
    ADD_DEFINITIONS(-DVERSE_ANDROID)
    SET(VERSE_PLATFORM "Android")
ELSEIF(IOS)
    ADD_DEFINITIONS(-DVERSE_APPLE -DVERSE_IOS)
    SET(VERSE_PLATFORM "IOS")
ELSEIF(WIN32 OR MINGW OR CYGWIN)
    IF(USE_UWP_OPTIONS)
        ADD_DEFINITIONS(-DVERSE_UWP -DNOMINMAX -D_CRT_SECURE_NO_WARNINGS)
    ELSEIF(MSVC)
        ADD_DEFINITIONS(-DVERSE_MSVC -DNOMINMAX)
		IF(MSVC_VERSION LESS 1910)
		    ADD_DEFINITIONS(-DVERSE_MSVC14)
		ENDIF()
    ELSEIF(MINGW OR CYGWIN)
        ADD_DEFINITIONS(-DVERSE_MINGW -DWIN32_HAS_PTHREADS)
    ENDIF()

    SET(EXTERNAL_LIBRARIES ${EXTERNAL_LIBRARIES} Ws2_32 Winmm imm32 Ole32 OleAut32 Dbghelp)
    ADD_DEFINITIONS(-DVERSE_WINDOWS)
    SET(VERSE_PLATFORM "Windows")
ELSEIF(APPLE)
    ADD_DEFINITIONS(-DVERSE_APPLE)
    SET(VERSE_PLATFORM "MacOSX")
ELSE()
    IF(NOT USE_WASM_OPTIONS)
        FIND_PACKAGE(X11)
        IF(VERSE_USE_DWARF)
            ADD_DEFINITIONS(-DBACKWARD_HAS_DW=1)
            FIND_PATH(DW_INCLUDE_DIR dwarf.h PATHS /usr/include /usr/local/include)
            FIND_LIBRARY(DW_LIBRARY NAMES dw HINTS /usr/${FIND_LIB_POSTFIX} /usr/local/${FIND_LIB_POSTFIX})
            SET(EXTERNAL_INCLUDES ${EXTERNAL_INCLUDES} ${DW_INCLUDE_DIR})
            SET(EXTERNAL_LIBRARIES ${EXTERNAL_LIBRARIES} ${DW_LIBRARY})
            SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
            SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
        ENDIF(VERSE_USE_DWARF)

        IF(VERSE_USE_GLIBCXX11_ABI)
            ADD_DEFINITIONS(-D_GLIBCXX_USE_CXX11_ABI=1)
        ELSE(VERSE_USE_GLIBCXX11_ABI)
            ADD_DEFINITIONS(-D_GLIBCXX_USE_CXX11_ABI=0)
        ENDIF(VERSE_USE_GLIBCXX11_ABI)

        IF(X11_FOUND)
            SET(EXTERNAL_LIBRARIES ${EXTERNAL_LIBRARIES} ${X11_LIBRARIES})
            ADD_DEFINITIONS(-DVERSE_X11)
        ENDIF(X11_FOUND)
        SET(VERSE_PLATFORM "Linux")
    ELSE()
        SET(VERSE_PLATFORM "Emscripten")
    ENDIF()
    ADD_DEFINITIONS(-DVERSE_UNIX)
ENDIF()

STRING(FIND "${CMAKE_C_COMPILER}" "icx" INTEL_COMPILER_INDEX)
IF(INTEL_COMPILER_INDEX GREATER -1)
    ADD_DEFINITIONS(-DINTEL_COMPILER -DRAPIDJSON_HAS_CXX11_RVALUE_REFS=1 -DRAPIDJSON_HAS_CXX11_NOEXCEPT=1)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Qstd=c++14")
ELSEIF(MSVC)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
ELSEIF(USE_WASM_OPTIONS)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-template-arg-list-after-template-kw")
ENDIF()

IF(VERSE_STATIC_BUILD)
    ADD_DEFINITIONS(-DVERSE_STATIC_BUILD)
ENDIF(VERSE_STATIC_BUILD)

IF(VERSE_SYSTEM_ARCHITECT STREQUAL "ARM")
    IF(VERSE_USE_EXTERNAL_GLES)    
        ADD_DEFINITIONS(-DVERSE_EMBEDDED)
    ENDIF()
ELSEIF(USE_WASM_OPTIONS)
    ADD_DEFINITIONS(-DVERSE_EMBEDDED -DVERSE_WASM)
ENDIF()

IF(VERSE_USE_EXTERNAL_GLES3)
    ADD_DEFINITIONS(-DVERSE_EMBEDDED_GLES3)
ELSEIF(VERSE_USE_EXTERNAL_GLES2)
    ADD_DEFINITIONS(-DVERSE_EMBEDDED_GLES2)
ENDIF()

######################################### Subdirectories
IF(OSG_INCLUDE_DIR AND OSG_LIB_DIR)

    IF(OSG_BUILD_INCLUDE_DIR)
        INCLUDE_DIRECTORIES(${OSG_BUILD_INCLUDE_DIR})
    ENDIF(OSG_BUILD_INCLUDE_DIR)
    INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR} ${OSG_INCLUDE_DIR} . 3rdparty
                        "${CMAKE_BINARY_DIR}/3rdparty")
    LINK_DIRECTORIES(${OSG_LIB_DIR} .)
    ADD_DEFINITIONS(-DINSTALL_PATH_PREFIX="${CMAKE_INSTALL_PREFIX}")

    IF(VERSE_BUILD_3RDPARTIES)
        MESSAGE(STATUS "[osgVerse] Build third-party libraries under helpers/toolchain_builder.")
        ADD_SUBDIRECTORY(helpers/toolchain_builder)
        SET(THIRDPARTY_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/helpers/toolchain_builder/freetype/include
                                    ${CMAKE_SOURCE_DIR}/helpers/toolchain_builder/jpeg ${CMAKE_SOURCE_DIR}/helpers/toolchain_builder/png
                                    ${CMAKE_SOURCE_DIR}/helpers/toolchain_builder/tiff ${CMAKE_BINARY_DIR}/helpers/toolchain_builder/tiff
                                    ${CMAKE_SOURCE_DIR}/helpers/toolchain_builder/zlib)
        SET(THIRDPARTY_LIBRARIES freetype jpeg png tiff zlib)
        SET_PROPERTY(GLOBAL APPEND PROPERTY VERSE_PLUGIN_LIBRARIES "${THIRDPARTY_LIBRARIES}")
        SET(VERSE_WITH_COMMON_LIBRARIES ON)
        SET(VERSE_WITH_TIFF_LIBRARY ON)
    ELSE(VERSE_BUILD_3RDPARTIES)
        IF(JPEG_INCLUDE_DIR AND PNG_PNG_INCLUDE_DIR AND ZLIB_INCLUDE_DIR)
            SET(THIRDPARTY_INCLUDE_DIRS ${FREETYPE_INCLUDE_DIR_freetype2} ${FREETYPE_INCLUDE_DIR_ft2build}
                                        ${JPEG_INCLUDE_DIR} ${PNG_PNG_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR})
            SET(THIRDPARTY_LIBRARIES ${FREETYPE_LIBRARY_RELEASE} ${JPEG_LIBRARY_RELEASE} ${PNG_LIBRARY_RELEASE} ${ZLIB_LIBRARY_RELEASE})
            SET_PROPERTY(GLOBAL APPEND PROPERTY VERSE_PLUGIN_LIBRARIES "${THIRDPARTY_LIBRARIES}")
            SET(VERSE_WITH_COMMON_LIBRARIES ON)
        ELSE()
            MESSAGE("[osgVerse] Common third-party libraries not found. Some modules and functionalities will be ignored.")
        ENDIF()
    ENDIF(VERSE_BUILD_3RDPARTIES)

    ADD_SUBDIRECTORY(3rdparty)
    ADD_SUBDIRECTORY(readerwriter)
    ADD_SUBDIRECTORY(pipeline)
    ADD_SUBDIRECTORY(animation)
    ADD_SUBDIRECTORY(modeling)
    ADD_SUBDIRECTORY(ai)
    ADD_SUBDIRECTORY(script)
    ADD_SUBDIRECTORY(ui)
    ADD_SUBDIRECTORY(wrappers)
    ADD_SUBDIRECTORY(plugins)

    IF(VERSE_BUILD_EXPORTERS)
        ADD_SUBDIRECTORY(helpers/3dsmax_exporter)
    ENDIF(VERSE_BUILD_EXPORTERS)

    IF(USE_WASM_OPTIONS)
        MESSAGE(STATUS "[osgVerse] Emscripten / WebAssembly toolchain is used. (WebGL2 = ${VERSE_USE_EXTERNAL_GLES3})")
        IF(BUILD_WASM_64)
            MESSAGE(STATUS "[osgVerse] Experimental WASM 64bit is enabled. "
                           "Remember to open chrome://flags and enable 'WebAssembly Memory64'")
        ENDIF()
        ADD_SUBDIRECTORY(wasm)
    ELSEIF(ANDROID)
        MESSAGE(STATUS "[osgVerse] Android toolchain is used. (API = ${VERSE_GRAPHICS_API})")
        # Nothing else to do... Start 'Android Studio' to continue compiling examples
    ELSEIF(IOS)
        MESSAGE(STATUS "[osgVerse] IOS toolchain is used. (API = ${VERSE_GRAPHICS_API})")
        # Add demo project here?
    ELSE()
        MESSAGE(STATUS "[osgVerse] Desktop toolchain is used. (API = ${VERSE_GRAPHICS_API})")
        IF(VERSE_BUILD_EXAMPLES)
            ADD_SUBDIRECTORY(tests)
        ENDIF(VERSE_BUILD_EXAMPLES)
        ADD_SUBDIRECTORY(applications)
        ADD_SUBDIRECTORY(assets)
    ENDIF()

    GET_PROPERTY(VERSE_DEPENDENCIES_TO_SHOW GLOBAL PROPERTY VERSE_DEPENDENCIES)
    MESSAGE(STATUS "[osgVerse] OpenSceneGraph version: ${OSG_MAJOR_VERSION}.${OSG_MINOR_VERSION}.${OSG_PATCH_VERSION}"
                   "; osgVerse version: ${VERSE_VERSION} (${VERSE_PLATFORM})")
    MESSAGE(STATUS "[osgVerse] Dependency libraries: ${VERSE_DEPENDENCIES_TO_SHOW}")
    IF(CUDA_FOUND AND VERSE_BUILD_WITH_CUDA)
        MESSAGE(STATUS "[osgVerse] NVIDIA CUDA found: ${CUDA_VERSION}")
    ELSEIF(MUSA_FOUND AND VERSE_BUILD_WITH_MUSA)
        MESSAGE(STATUS "[osgVerse] MooreThreads MUSA found: ${MUSA_VERSION}")
    ENDIF()
    CONFIGURE_FILE(cmake/osgVerseConfig.cmake.in "${CMAKE_INSTALL_PREFIX}/lib/osgVerse/osgVerseConfig.cmake" @ONLY)
    INSTALL(FILES VerseCommon.h DESTINATION ${INSTALL_INCDIR}/osgVerse COMPONENT libosgverse-dev)

ELSE(OSG_INCLUDE_DIR AND OSG_LIB_DIR)

    MESSAGE(FATAL_ERROR "[osgVerse] Dependency OpenSceneGraph not found. NOTHING will be created...")

ENDIF(OSG_INCLUDE_DIR AND OSG_LIB_DIR)

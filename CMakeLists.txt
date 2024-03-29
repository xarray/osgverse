CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
PROJECT(osgVerse)

SET(VERSE_VERSION "0.7.0")
SET(VERSE_GRAPHICS_API "GL")
SET(VERSE_PLATFORM "")
SET(CMAKE_DEBUG_POSTFIX "d")
SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

######################################### Macros
MACRO(NEW_EXECUTABLE EXECUTABLE_NAME)

    LINK_DIRECTORIES(${PATH_LIST})
    ADD_EXECUTABLE(${EXECUTABLE_NAME} ${EXECUTABLE_FILES})

    SET_TARGET_PROPERTIES(${EXECUTABLE_NAME} PROPERTIES DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}")
    SET_TARGET_PROPERTIES(${EXECUTABLE_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/lib"
                                                        ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BINARY_DIR}/lib"
                                                        LIBRARY_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/lib"
                                                        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BINARY_DIR}/lib"
                                                        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/bin"
                                                        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BINARY_DIR}/bin")

    INSTALL(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
                                       LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
                                       ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

    TARGET_INCLUDE_DIRECTORIES(${EXECUTABLE_NAME} PUBLIC ${EXTERNAL_INCLUDES})
    TARGET_LINK_LIBRARIES(${EXECUTABLE_NAME} ${EXTERNAL_LIBRARIES})

ENDMACRO(NEW_EXECUTABLE)

MACRO(NEW_LIBRARY LIBRARY_NAME LIBRARY_TYPE)

    LINK_DIRECTORIES(${PATH_LIST})
    ADD_LIBRARY(${LIBRARY_NAME} ${LIBRARY_TYPE} ${LIBRARY_FILES})

    SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}")
    SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/lib"
                                                     ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BINARY_DIR}/lib"
                                                     LIBRARY_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/lib"
                                                     LIBRARY_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BINARY_DIR}/lib"
                                                     RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/bin"
                                                     RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BINARY_DIR}/bin")

    INSTALL(TARGETS ${LIBRARY_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
                                    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
                                    ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

    TARGET_INCLUDE_DIRECTORIES(${LIBRARY_NAME} PUBLIC ${EXTERNAL_INCLUDES})
    TARGET_LINK_LIBRARIES(${LIBRARY_NAME} ${EXTERNAL_LIBRARIES})

ENDMACRO(NEW_LIBRARY)

MACRO(USE_MSVC_DEBUGGER EXECUTABLE_NAME PATH_ENV)

    IF(MSVC)
        SET_TARGET_PROPERTIES(${EXECUTABLE_NAME}
            PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin")
        IF("${CMAKE_VERSION}" VERSION_GREATER 3.12)
            SET_TARGET_PROPERTIES(${EXECUTABLE_NAME}
                PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=%PATH%;${PATH_ENV}")
        ENDIF()
    ENDIF(MSVC)

ENDMACRO(USE_MSVC_DEBUGGER)

MACRO(USE_STATIC_RUNTIME)

    IF(MSVC)
        FOREACH(FLAG_VAR
                CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
                CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
            IF(${FLAG_VAR} MATCHES "/MD")
                STRING(REGEX REPLACE "/MD" "/MT" ${FLAG_VAR} "${${FLAG_VAR}}")
            ENDIF(${FLAG_VAR} MATCHES "/MD")
        ENDFOREACH(FLAG_VAR)
    ENDIF(MSVC)

ENDMACRO(USE_STATIC_RUNTIME)

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
IF(WIN32)
    SET(INSTALL_LIBDIR bin)
    SET(INSTALL_ARCHIVEDIR lib)
ELSE()
    INCLUDE(GNUInstallDirs)
    SET(INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})
    SET(INSTALL_ARCHIVEDIR ${CMAKE_INSTALL_LIBDIR})
ENDIF()

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

SET(VERSE_SIMD_FEATURES "")
IF(USE_WASM_OPTIONS)
    #SET(VERSE_SIMD_FEATURES "sse2")
    #SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse -msimd128")
    #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msimd128")
ELSE()
    CMAKE_HOST_SYSTEM_INFORMATION_EXTRA(RESULT VERSE_CPU_ARCHITECT QUERY PROCESSOR_MICRO_ARCHITECTURE)
    IF(VERSE_CPU_ARCHITECT)
        GET_CPU_SIMD_FEATURES(VERSE_SIMD_FEATURES "${VERSE_CPU_ARCHITECT}")
        MESSAGE("[osgVerse] CPU Architecture: ${VERSE_CPU_ARCHITECT}. Features: ${VERSE_SIMD_FEATURES}")
    ENDIF(VERSE_CPU_ARCHITECT)
ENDIF()

IF(VERSE_SIMD_FEATURES)
    # Find SIMD instruction supporting using the following statement:
    #   IF("sse3" IN_LIST VERSE_SIMD_FEATURES) ...
    APPLY_SIMD_FLAGS("sse3" "-D__SSE3__" "-msse3")
    APPLY_SIMD_FLAGS("ssse3" "-D__SSSE3__" "-mssse3")
    APPLY_SIMD_FLAGS("sse4.1" "-D__SSE4_1__" "-msse4.1")
    APPLY_SIMD_FLAGS("sse4.2" "-D__SSE4_2__" "-msse4.2")
ENDIF()

######################################### Dependency Finders
SET(FIND_LIB_POSTFIX lib)
IF(CMAKE_CL_64)
    SET(FIND_LIB_POSTFIX lib64)
ENDIF(CMAKE_CL_64)

SET(USE_UWP_OPTIONS OFF)
IF(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
    SET(USE_UWP_OPTIONS ON)
    MESSAGE("[osgVerse] Building Windows Store (UWP) libraries...")
ENDIF()

SET(THIRDPARTY_ROOT ${VERSE_3RDPARTY_PATH}/x64)
IF(USE_WASM_OPTIONS)
    # WASM third-parties
    # - How to build Draco (https://github.com/google/draco/blob/master/BUILDING.md)
    #    export EMSCRIPTEN=/mnt/.../em_sdk/upstream/emscripten
    #    cmake ../ -DCMAKE_TOOLCHAIN_FILE=/mnt/.../em_sdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake -DDRACO_WASM=ON
    #              -DDRACO_JS_GLUE=OFF -DCMAKE_INSTALL_PREFIX=/mnt/.../Dependencies/wasm
    SET(THIRDPARTY_ROOT ${VERSE_3RDPARTY_PATH}/wasm)
ELSEIF(USE_UWP_OPTIONS)
    SET(THIRDPARTY_ROOT ${VERSE_3RDPARTY_PATH}/uwp)
ELSEIF(ANDROID)
    SET(THIRDPARTY_ROOT ${VERSE_3RDPARTY_PATH}/android)
ELSEIF(APPLE)
    SET(THIRDPARTY_ROOT ${VERSE_3RDPARTY_PATH}/apple)
ENDIF()

FIND_PATH(BULLET_INCLUDE_DIR btBulletDynamicsCommon.h
    PATHS ${THIRDPARTY_ROOT}/include/bullet
    $ENV{BULLET_ROOT}/include/bullet
    /usr/include/bullet
    /usr/local/include/bullet
    NO_CMAKE_FIND_ROOT_PATH
)

FIND_PATH(BULLET_LIB_DIR libBulletDynamics.so libBulletDynamics.a BulletDynamics.lib
    PATHS ${THIRDPARTY_ROOT}/lib
    $ENV{BULLET_ROOT}/lib $ENV{BULLET_ROOT}/${FIND_LIB_POSTFIX}
    /usr/lib /usr/${FIND_LIB_POSTFIX} /usr/lib/x86_64-linux-gnu
    /usr/local/lib /usr/local/${FIND_LIB_POSTFIX}
    NO_CMAKE_FIND_ROOT_PATH
)

FIND_PATH(DRACO_INCLUDE_DIR draco/draco_features.h
    PATHS ${THIRDPARTY_ROOT}/include
    /usr/include
    /usr/local/include
    NO_CMAKE_FIND_ROOT_PATH
)

FIND_PATH(DRACO_LIB_DIR libdraco.so libdraco.a draco.lib
    PATHS ${THIRDPARTY_ROOT}/lib
    /usr/lib /usr/${FIND_LIB_POSTFIX}
    /usr/local/lib /usr/local/${FIND_LIB_POSTFIX}
    NO_CMAKE_FIND_ROOT_PATH
)

FIND_PATH(SDL2_INCLUDE_DIR SDL.h
    PATHS ${THIRDPARTY_ROOT}/include/SDL
    ${THIRDPARTY_ROOT}/include/SDL2
    /usr/include /usr/local/include
    NO_CMAKE_FIND_ROOT_PATH
)

FIND_PATH(SDL2_LIB_DIR libSDL2.so libSDL2.a SDL2.lib
    PATHS ${THIRDPARTY_ROOT}/lib
    /usr/lib /usr/${FIND_LIB_POSTFIX} /usr/lib/x86_64-linux-gnu
    /usr/local/lib /usr/local/${FIND_LIB_POSTFIX}
    NO_CMAKE_FIND_ROOT_PATH
)

IF(OSG_ROOT)
    SET(OSG_INCLUDE_DIR "${OSG_ROOT}/include")
    SET(OSG_BUILD_INCLUDE_DIR "${OSG_ROOT}/include")
    SET(OSG_LIB_DIR "${OSG_ROOT}/lib")
ELSE(OSG_ROOT)
    FIND_PATH(OSG_INCLUDE_DIR osg/Referenced
        PATHS
        $ENV{OSG_ROOT}/include
        $ENV{OSG_DIR}/include
        /usr/include
        /usr/local/include
    )

    FIND_PATH(OSG_BUILD_INCLUDE_DIR osg/Config
        PATHS
        $ENV{OSG_ROOT}/include
        $ENV{OSG_DIR}/include
        /usr/include
        /usr/local/include
    )

    FIND_PATH(OSG_LIB_DIR libosg.a libosg.so osg.lib
        PATHS
        $ENV{OSG_ROOT}/lib $ENV{OSG_ROOT}/${FIND_LIB_POSTFIX}
        $ENV{OSG_DIR}/lib $ENV{OSG_DIR}/${FIND_LIB_POSTFIX}
        /usr/lib /usr/${FIND_LIB_POSTFIX} /usr/lib/x86_64-linux-gnu
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

######################################### Dependency Configuations
SET(VERSE_FIND_LEGACY_OPENGL OFF CACHE BOOL "Enable to find OpenGL in legacy mode")
IF(VERSE_FIND_LEGACY_OPENGL)
    SET(OpenGL_GL_PREFERENCE "LEGACY")
ELSE(VERSE_FIND_LEGACY_OPENGL)
    SET(OpenGL_GL_PREFERENCE "GLVND")
ENDIF(VERSE_FIND_LEGACY_OPENGL)

FIND_PACKAGE(OpenGL)
FIND_PACKAGE(OpenMP)
IF(OPENMP_FOUND)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
ENDIF()

SET(BULLET_FOUND FALSE)
IF(BULLET_INCLUDE_DIR AND BULLET_LIB_DIR)
    INCLUDE_DIRECTORIES(${BULLET_INCLUDE_DIR})
    LINK_DIRECTORIES(${BULLET_LIB_DIR})
    SET(BULLET_FOUND TRUE)
ELSE(BULLET_INCLUDE_DIR AND BULLET_LIB_DIR)
    MESSAGE("[osgVerse] Dependency Bullet not found. Some modules and functionalities will be ignored.")
ENDIF(BULLET_INCLUDE_DIR AND BULLET_LIB_DIR)

SET(DRACO_FOUND FALSE)
IF(DRACO_INCLUDE_DIR AND DRACO_LIB_DIR)
    INCLUDE_DIRECTORIES(${DRACO_INCLUDE_DIR})
    LINK_DIRECTORIES(${DRACO_LIB_DIR})
    SET(DRACO_FOUND TRUE)
ELSE(DRACO_INCLUDE_DIR AND DRACO_LIB_DIR)
    MESSAGE("[osgVerse] Dependency Draco not found. Some modules and functionalities will be ignored.")
ENDIF(DRACO_INCLUDE_DIR AND DRACO_LIB_DIR)

SET(SDL2_FOUND FALSE)
IF(SDL2_INCLUDE_DIR AND SDL2_LIB_DIR)
    SET(SDL2_FOUND TRUE)
ELSE(SDL2_INCLUDE_DIR AND SDL2_LIB_DIR)
    MESSAGE("[osgVerse] Dependency SDL2 not found. Some modules and functionalities will be ignored.")
ENDIF(SDL2_INCLUDE_DIR AND SDL2_LIB_DIR)

######################################### Global Options
SET(VERSE_BUILD_GPL OFF CACHE BOOL "Enable build of GPL dependencies (e.g., mcut), which will makes osgVerse a GPL3 library")
SET(VERSE_BUILD_3RDPARTIES ON CACHE BOOL "Enable build of common libraries like FreeType, Jpeg, PNG and Tiff")
SET(VERSE_BUILD_WITH_QT OFF CACHE BOOL "Enable build of Qt based applications and tests")
SET(VERSE_BUILD_DEPRECATED_TESTS OFF CACHE BOOL "Enable build of deprecated tests")
SET(VERSE_SUPPORT_CPP17 OFF CACHE BOOL "Enable build of libraries using C++ 17 standard")
SET(VERSE_STATIC_BUILD OFF CACHE BOOL "Enable static build of osgVerse libraries")
SET(VERSE_USE_OSG_STATIC OFF CACHE BOOL "Use static build of OpenSceneGraph (will force osgVerse to be static)")
SET(VERSE_USE_MTT_DRIVER OFF CACHE BOOL "Use MooreThreads MTT drivers")
SET_PROPERTY(GLOBAL PROPERTY VERSE_PLUGIN_LIBRARIES "")

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
    SET(VERSE_USE_OSG_STATIC ON CACHE BOOL "Enable static build of osgVerse libraries" FORCE)
    SET(VERSE_STATIC_BUILD ON CACHE BOOL "Enable static build of osgVerse" FORCE)
    ADD_DEFINITIONS("-O3 -s USE_SDL=2 -pthread")
ELSEIF(ANDROID)
    SET(VERSE_USE_OSG_STATIC ON CACHE BOOL "Enable static build of osgVerse libraries" FORCE)
    SET(VERSE_STATIC_BUILD ON CACHE BOOL "Enable static build of osgVerse" FORCE)
    ADD_DEFINITIONS(-DBACKWARD_SYSTEM_UNKNOWN=1)
ENDIF()

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
        MESSAGE(STATUS "[osgVerse] OSG is built with GLES3 support. Please configure osgVerse to use GLES3, too.")
        FIND_PATH(OSG_GLES_INCLUDE_DIR GLES3/gl3.h PATHS /usr/include /usr/local/include)
        FIND_LIBRARY(OSG_GLES_LIBRARY NAMES libGLESv2.so libGLESv2.lib
                     HINTS /usr/lib /usr/${FIND_LIB_POSTFIX} /usr/local/lib /usr/local/${FIND_LIB_POSTFIX})
        FIND_LIBRARY(OSG_EGL_LIBRARY NAMES libEGL.so libEGL.lib
                     HINTS /usr/lib /usr/${FIND_LIB_POSTFIX} /usr/local/lib /usr/local/${FIND_LIB_POSTFIX})
        SET(VERSE_USE_EXTERNAL_GLES3 ON)
        SET(VERSE_GRAPHICS_API "GLES3")
    ELSEIF(OSG_GLES2_INDEX GREATER -1)
        MESSAGE(STATUS "[osgVerse] OSG is built with GLES2 support. Please configure osgVerse to use GLES2, too.")
        FIND_PATH(OSG_GLES_INCLUDE_DIR GLES2/gl2.h PATHS /usr/include /usr/local/include)
        FIND_LIBRARY(OSG_GLES_LIBRARY NAMES libGLESv2.so libGLESv2.lib
                     HINTS /usr/lib /usr/${FIND_LIB_POSTFIX} /usr/local/lib /usr/local/${FIND_LIB_POSTFIX})
        FIND_LIBRARY(OSG_EGL_LIBRARY NAMES libEGL.so libEGL.lib
                     HINTS /usr/lib /usr/${FIND_LIB_POSTFIX} /usr/local/lib /usr/local/${FIND_LIB_POSTFIX})
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

IF(ANDROID OR IOS OR USE_UWP_OPTIONS OR USE_WASM_OPTIONS)
    IF(NOT VERSE_USE_EXTERNAL_GLES2 AND NOT VERSE_USE_EXTERNAL_GLES3)
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
ENDIF(OSG_BUILD_INCLUDE_DIR)

######################################### Global Definintions
IF(NOT OPENGL_gl_LIBRARY)
    MESSAGE(FATAL_ERROR "[osgVerse] Dependency OpenGL not found. All following steps may not work...")
ENDIF(NOT OPENGL_gl_LIBRARY)

SET(THIRDPARTY_INCLUDE_DIRS "")
SET(THIRDPARTY_LIBRARIES "")
SET(EXTERNAL_LIBRARIES ${EXTERNAL_LIBRARIES} ${OPENGL_gl_LIBRARY})
IF(OPENMP_FOUND)
    SET(EXTERNAL_LIBRARIES ${EXTERNAL_LIBRARIES} OpenMP::OpenMP_CXX)
ENDIF(OPENMP_FOUND)

IF(ANDROID)
    ADD_DEFINITIONS(-DVERSE_ANDROID)
    SET(VERSE_PLATFORM "Android")
ELSEIF(IOS)
    ADD_DEFINITIONS(-DVERSE_IOS)
    SET(VERSE_PLATFORM "IOS")
ELSEIF(WIN32 OR MINGW OR CYGWIN)
    IF(USE_UWP_OPTIONS)
        ADD_DEFINITIONS(-DVERSE_UWP -DNOMINMAX -D_CRT_SECURE_NO_WARNINGS)
    ELSEIF(MSVC)
        ADD_DEFINITIONS(-DVERSE_MSVC -DNOMINMAX)
    ELSEIF(MINGW OR CYGWIN)
        ADD_DEFINITIONS(-DVERSE_MINGW)
    ENDIF()

    SET(EXTERNAL_LIBRARIES ${EXTERNAL_LIBRARIES} imm32)
    ADD_DEFINITIONS(-DVERSE_WINDOWS)
    SET(VERSE_PLATFORM "Windows")
ELSEIF(APPLE)
    ADD_DEFINITIONS(-DVERSE_APPLE)
    SET(VERSE_PLATFORM "MacOSX")
    # TODO
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
ENDIF()

IF(VERSE_STATIC_BUILD)
    ADD_DEFINITIONS(-DVERSE_STATIC_BUILD)
ENDIF(VERSE_STATIC_BUILD)

IF(USE_WASM_OPTIONS)
    ADD_DEFINITIONS(-DVERSE_WASM)
    IF(VERSE_USE_EXTERNAL_GLES3)
        ADD_DEFINITIONS(-DVERSE_WEBGL2)
    ELSE()
        ADD_DEFINITIONS(-DVERSE_WEBGL1)
    ENDIF()
ENDIF(USE_WASM_OPTIONS)

######################################### Subdirectories
IF(OSG_INCLUDE_DIR AND OSG_LIB_DIR)

    IF(OSG_BUILD_INCLUDE_DIR)
        INCLUDE_DIRECTORIES(${OSG_BUILD_INCLUDE_DIR})
    ENDIF(OSG_BUILD_INCLUDE_DIR)
    INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR} ${OSG_INCLUDE_DIR} . 3rdparty
                        "${CMAKE_BINARY_DIR}/3rdparty")
    LINK_DIRECTORIES(${OSG_LIB_DIR} .)
    ADD_DEFINITIONS(-DINSTALL_PATH_PREFIX="${CMAKE_INSTALL_PREFIX}")
    MESSAGE(STATUS "[osgVerse] OpenSceneGraph version: "
                   "${OSG_MAJOR_VERSION}.${OSG_MINOR_VERSION}.${OSG_PATCH_VERSION}")

    IF(VERSE_BUILD_3RDPARTIES)
        MESSAGE(STATUS "[osgVerse] Build third-party libraries under helpers/toolchain_builder.")
        ADD_SUBDIRECTORY(helpers/toolchain_builder)
        SET(THIRDPARTY_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/helpers/toolchain_builder/freetype/include
                                    ${CMAKE_SOURCE_DIR}/helpers/toolchain_builder/jpeg ${CMAKE_SOURCE_DIR}/helpers/toolchain_builder/png
                                    ${CMAKE_SOURCE_DIR}/helpers/toolchain_builder/zlib ${CMAKE_SOURCE_DIR}/helpers/toolchain_builder/tiff
                                    ${CMAKE_BINARY_DIR}/helpers/toolchain_builder/tiff)
        SET(THIRDPARTY_LIBRARIES freetype jpeg png tiff zlib)
        SET_PROPERTY(GLOBAL APPEND PROPERTY VERSE_PLUGIN_LIBRARIES "${THIRDPARTY_LIBRARIES}")
    ELSE(VERSE_BUILD_3RDPARTIES)
        IF(JPEG_INCLUDE_DIR AND TIFF_INCLUDE_DIR AND ZLIB_INCLUDE_DIR)
            SET(THIRDPARTY_INCLUDE_DIRS ${FREETYPE_INCLUDE_DIR_freetype2} ${FREETYPE_INCLUDE_DIR_ft2build}
                                        ${JPEG_INCLUDE_DIR} ${PNG_PNG_INCLUDE_DIR} ${TIFF_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR})
            SET(THIRDPARTY_LIBRARIES ${FREETYPE_LIBRARY_RELEASE} ${JPEG_LIBRARY_RELEASE} ${PNG_LIBRARY_RELEASE}
                                    ${TIFF_LIBRARY_RELEASE} ${ZLIB_LIBRARY_RELEASE})
            SET_PROPERTY(GLOBAL APPEND PROPERTY VERSE_PLUGIN_LIBRARIES "${THIRDPARTY_LIBRARIES}")
        ELSE()
            MESSAGE("[osgVerse] Common third-party libraries not found. Some modules and functionalities will be ignored.")
        ENDIF()
    ENDIF(VERSE_BUILD_3RDPARTIES)

    ADD_SUBDIRECTORY(3rdparty)
    ADD_SUBDIRECTORY(readerwriter)
    ADD_SUBDIRECTORY(pipeline)
    ADD_SUBDIRECTORY(animation)
    ADD_SUBDIRECTORY(modeling)
    ADD_SUBDIRECTORY(script)
    ADD_SUBDIRECTORY(ui)
    ADD_SUBDIRECTORY(plugins)
    IF(USE_WASM_OPTIONS)
        MESSAGE(STATUS "[osgVerse] Emscripten / WebAssembly toolchain is used. (WebGL2 = ${VERSE_USE_EXTERNAL_GLES3})")
        ADD_SUBDIRECTORY(wasm)
    ELSEIF(ANDROID)
        MESSAGE(STATUS "[osgVerse] Android toolchain is used. (GLES3 = ${VERSE_USE_EXTERNAL_GLES3})")
        # Nothing else to do... Start 'Android Studio' to continue compiling examples
    ELSEIF(IOS)
        MESSAGE(STATUS "[osgVerse] IOS toolchain is used. (GLES3 = ${VERSE_USE_EXTERNAL_GLES3})")
        # Add demo project here?
    ELSE()
        MESSAGE(STATUS "[osgVerse] Desktop toolchain is used. (GLCore = ${VERSE_USE_EXTERNAL_GLCORE})")
        ADD_SUBDIRECTORY(applications)
        ADD_SUBDIRECTORY(tests)
        ADD_SUBDIRECTORY(assets)
    ENDIF()
    CONFIGURE_FILE(cmake/osgVerseConfig.cmake.in "${CMAKE_INSTALL_PREFIX}/lib/FindosgVerse.cmake" @ONLY)

ELSE(OSG_INCLUDE_DIR AND OSG_LIB_DIR)

    MESSAGE(FATAL_ERROR "[osgVerse] Dependency OpenSceneGraph not found. NOTHING will be created...")

ENDIF(OSG_INCLUDE_DIR AND OSG_LIB_DIR)

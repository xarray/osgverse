CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

GET_FILENAME_COMPONENT(SDL2_SOURCE_DIR ../../../SDL2 ABSOLUTE)
GET_FILENAME_COMPONENT(OSG_3RDPARTY_SOURCE_DIR ../../helpers/toolchain_builder ABSOLUTE)
GET_FILENAME_COMPONENT(OSG_SOURCE_DIR ../../../OpenSceneGraph ABSOLUTE)
GET_FILENAME_COMPONENT(OSGVERSE_SOURCE_DIR ../.. ABSOLUTE)
SET(SDL2_BUILD_DIR ../../build/sdl2_android)
SET(OSG_3RDPARTY_BUILD_DIR ../../build/3rdparty_android)
SET(OSG_BUILD_DIR ../../build/osg_android)
SET(OSGVERSE_BUILD_DIR ../../build/verse_android)
FILE(MAKE_DIRECTORY ${SDL2_BUILD_DIR})
FILE(MAKE_DIRECTORY ${OSG_3RDPARTY_BUILD_DIR})
FILE(MAKE_DIRECTORY ${OSG_BUILD_DIR})
FILE(MAKE_DIRECTORY ${OSGVERSE_BUILD_DIR})

# Specify critical SDL & OpenSceneGraph build variables.
SET(SDL2_DISABLE_INSTALL ON CACHE BOOL "Disable installation of SDL2")
SET(SDL2_DISABLE_UNINSTALL ON CACHE BOOL "Disable uninstallation of SDL2")

SET(BUILD_OSG_APPLICATIONS OFF CACHE BOOL "Do not build applications")
SET(EGL_LIBRARY "GL" CACHE STRING "Suppress linkage error")
SET(OPENGL_PROFILE "GLES2" CACHE STRING "OpenGL Profile to use")
SET(OSG_GL1_AVAILABLE OFF CACHE BOOL "Unavailable under Android")
SET(OSG_GL2_AVAILABLE OFF CACHE BOOL "Unavailable under Android")
SET(OSG_GLES2_AVAILABLE ON CACHE BOOL "GLES2 is what Android uses")
SET(OSG_WINDOWING_SYSTEM "None" CACHE STRING "Unavailable under Android")
SET(OSG_FIND_3RD_PARTY_DEPS OFF CACHE BOOL "Disable for searching dependencies in ./3rdparty")
SET(DYNAMIC_OPENTHREADS OFF CACHE BOOL "Link OpenThreads statically")
SET(DYNAMIC_OPENSCENEGRAPH OFF CACHE BOOL "Link OpenSceneGraph statically")
SET(_OPENTHREADS_ATOMIC_USE_GCC_BUILTINS_EXITCODE "0" CACHE STRING "Prevent cfg error")

# Build 3rd-parties and OSG core libraries
ADD_DEFINITIONS(-DFREETYPE_INCLUDE_DIR_freetype2=../../helpers/toolchain_builder/freetype/include
                -DFREETYPE_INCLUDE_DIR_ft2build=../../helpers/toolchain_builder/freetype/include
                -DFREETYPE_LIBRARY_RELEASE=${OSG_3RDPARTY_BUILD_DIR}/freetype/libfreetype.a
                -DJPEG_INCLUDE_DIR=../../helpers/toolchain_builder/jpeg
                -DJPEG_LIBRARY_RELEASE=${OSG_3RDPARTY_BUILD_DIR}/jpeg/libjpeg.a
                -DPNG_PNG_INCLUDE_DIR=../../helpers/toolchain_builder/png
                -DPNG_LIBRARY_RELEASE=${OSG_3RDPARTY_BUILD_DIR}/png/libpng.a
                -DZLIB_INCLUDE_DIR=../../helpers/toolchain_builder/zlib
                -DZLIB_LIBRARY_RELEASE=${OSG_3RDPARTY_BUILD_DIR}/zlib/libzlib.a)

ADD_SUBDIRECTORY(${SDL2_SOURCE_DIR} ${SDL2_BUILD_DIR})
ADD_SUBDIRECTORY(${OSG_3RDPARTY_SOURCE_DIR} ${OSG_3RDPARTY_BUILD_DIR})
ADD_SUBDIRECTORY(${OSG_SOURCE_DIR} ${OSG_BUILD_DIR})
ADD_SUBDIRECTORY(${OSGVERSE_SOURCE_DIR} ${OSGVERSE_BUILD_DIR})

# Build main bridge library
ADD_LIBRARY(osgVerseAndroid SHARED
            src/SDL_android_main.c src/main.cpp)
FIND_LIBRARY(ANDROID_LOG_LIB log)
TARGET_LINK_LIBRARIES(osgVerseAndroid SDL2 OpenThreads osg osgDB osgUtil ${ANDROID_LOG_LIB})

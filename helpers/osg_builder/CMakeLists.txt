CMAKE_MINIMUM_REQUIRED(VERSION 3.16)
PROJECT(osgVerse_AllBuilder)

########## User Configuations ###########

# Environment variables
#   OSG_SOURCE_DIR: Where to find OSG source root
#   GL3_INCLUDE_DIR: Where to find glcorearb.h from Khronos website
#   ANGLE_ROOT: Where to find Google Angle SDK root
SET(ALLBUILD_TYPE "Default" CACHE STRING "Select among GL3/GLES2/GLES3")

IF(ALLBUILD_TYPE EQUAL "GL3")

    SET(OPENGL_PROFILE "GL3Core" CACHE STRING "Enable core profile")
    SET(GL3_INCLUDE_DIR "$ENV{GL3_INCLUDE_DIR}" CACHE STRING "Include folder containing gl3.h")
    INCLUDE_DIRECTORIES(${GL3_INCLUDE_DIR})

ELSEIF(ALLBUILD_TYPE EQUAL "GLES2")

    FIND_PATH(ANGLE_INCLUDE_DIR GLES2/gl2.h PATHS $ENV{ANGLE_ROOT}/include)
    FIND_LIBRARY(ANGLE_GL_LIBRARY NAMES libGLESv2.lib libGLESv2.so PATHS $ENV{ANGLE_ROOT}/lib)
    FIND_LIBRARY(ANGLE_EGL_LIBRARY NAMES libEGL.lib libEGL.so PATHS $ENV{ANGLE_ROOT}/lib)
    SET(OPENGL_PROFILE "GLES2" CACHE STRING "Enable GLES2 profile")
    SET(EGL_INCLUDE_DIR "${ANGLE_INCLUDE_DIR}" CACHE STRING "Include folder containing EGL")
    SET(EGL_LIBRARY "${ANGLE_EGL_LIBRARY}" CACHE STRING "Library folder containing EGL")
    SET(OPENGL_INCLUDE_DIR "${ANGLE_INCLUDE_DIR}" CACHE STRING "Include folder containing GLES")
    SET(OPENGL_gl_LIBRARY "${ANGLE_EGL_LIBRARY}" CACHE STRING "Library folder containing GLES")
    SET(DYNAMIC_OPENTHREADS OFF CACHE BOOL "Link OpenThreads statically")
    SET(DYNAMIC_OPENSCENEGRAPH OFF CACHE BOOL "Link OpenSceneGraph statically")
    SET(BUILD_OSG_APPLICATIONS OFF CACHE BOOL "Do not build applications")
    SET(OSG_WINDOWING_SYSTEM "None" CACHE STRING "Unavailable under current config")

ELSEIF(ALLBUILD_TYPE EQUAL "GLES3")

    FIND_PATH(ANGLE_INCLUDE_DIR GLES3/gl3.h PATHS $ENV{ANGLE_ROOT}/include)
    FIND_LIBRARY(ANGLE_GL_LIBRARY NAMES libGLESv2.lib libGLESv2.so PATHS $ENV{ANGLE_ROOT}/lib)
    FIND_LIBRARY(ANGLE_EGL_LIBRARY NAMES libEGL.lib libEGL.so PATHS $ENV{ANGLE_ROOT}/lib)
    SET(OPENGL_PROFILE "GLES3" CACHE STRING "Enable GLES3 profile")
    SET(EGL_INCLUDE_DIR "${ANGLE_INCLUDE_DIR}" CACHE STRING "Include folder containing EGL")
    SET(EGL_LIBRARY "${ANGLE_EGL_LIBRARY}" CACHE STRING "Library folder containing EGL")
    SET(OPENGL_INCLUDE_DIR "${ANGLE_INCLUDE_DIR}" CACHE STRING "Include folder containing GLES")
    SET(OPENGL_gl_LIBRARY "${ANGLE_EGL_LIBRARY}" CACHE STRING "Library folder containing GLES")
    SET(DYNAMIC_OPENTHREADS OFF CACHE BOOL "Link OpenThreads statically")
    SET(DYNAMIC_OPENSCENEGRAPH OFF CACHE BOOL "Link OpenSceneGraph statically")
    SET(BUILD_OSG_APPLICATIONS OFF CACHE BOOL "Do not build applications")
    SET(OSG_WINDOWING_SYSTEM "None" CACHE STRING "Unavailable under current config")

ELSE()

    # Default configuation

ENDIF()

########## Build OSG libraries #########
GET_FILENAME_COMPONENT(OSG_SOURCE_DIR "${CMAKE_SOURCE_DIR}/../../../OpenSceneGraph/" ABSOLUTE)
IF(DEFINED ENV{OSG_SOURCE_DIR})
    SET(OSG_SOURCE_DIR "$ENV{OSG_SOURCE_DIR}")
ENDIF()

IF(EXISTS "${OSG_SOURCE_DIR}/CMakeLists.txt")

    MESSAGE(STATUS "OpenSceneGraph source directory: ${OSG_SOURCE_DIR}")
    SET(OSG_BUILD_DIR "${CMAKE_BINARY_DIR}/osg")
    FILE(MAKE_DIRECTORY ${OSG_BUILD_DIR})
    ADD_SUBDIRECTORY(${OSG_SOURCE_DIR} ${OSG_BUILD_DIR})

ELSE()

    MESSAGE(FATAL_ERROR "Failed to find OpenSceneGraph source directory!")

ENDIF()

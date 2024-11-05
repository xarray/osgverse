CMAKE_MINIMUM_REQUIRED(VERSION 3.1.0 FATAL_ERROR)

# Specify critical osgEarth build variables.
SET(BUILD_APPLICATIONS OFF CACHE BOOL "Do not build applications")
SET(BUILD_OSGEARTH_EXAMPLES OFF CACHE BOOL "Do not build examples")
SET(BUILD_TESTS OFF CACHE BOOL "Do not build tests")
SET(DYNAMIC_OSGEARTH OFF CACHE BOOL "Use static build")

# Set special build flags:
# * -O3: make compact binary, critical for web
# * -s USE_SDL=2: provide SDL2
IF(USE_WASM_OPTIONS EQUAL 2)
    SET(BUILD_FLAGS "-O3 -s MEMORY64=1 -s USE_SDL=2 -pthread")
ELSE()
    SET(BUILD_FLAGS "-O3 -s USE_SDL=2 -pthread")
ENDIF()
ADD_DEFINITIONS(${BUILD_FLAGS})

# Set dependencies
SET(USE_EXTERNAL_WASM_DEPENDS ON CACHE BOOL "Use external roots instead of finding them")
SET(THIRD_PARTY_DIR ${THIRDPARTY_ROOT})
SET(GDAL_DIR "${THIRDPARTY_ROOT}" CACHE PATH "Root directory of GDAL")
SET(GEOS_DIR "${THIRDPARTY_ROOT}" CACHE PATH "Root directory of GEOS")

FILE(MAKE_DIRECTORY ${OSGEARTH_BUILD_DIR})
ADD_SUBDIRECTORY(${OSGEARTH_SOURCE_DIR} ${OSGEARTH_BUILD_DIR})

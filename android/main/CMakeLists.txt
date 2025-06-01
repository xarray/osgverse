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
GET_FILENAME_COMPONENT(OSG_BUILD_DIR ../../build/osg_android ABSOLUTE)
GET_FILENAME_COMPONENT(OSGVERSE_BUILD_DIR ../../build/verse_android ABSOLUTE)

FIND_PACKAGE(OpenMP)
IF(OPENMP_FOUND)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
ENDIF()

# Build main bridge library
INCLUDE_DIRECTORIES("${SDL2_SOURCE_DIR}/include"
                    "${OSG_BUILD_DIR}/include"
                    "${OSG_SOURCE_DIR}/include"
                    ${OSGVERSE_SOURCE_DIR})
LINK_DIRECTORIES("${SDL2_BUILD_DIR}/lib/${CMAKE_BUILD_TYPE}/${CMAKE_ANDROID_ARCH_ABI}"
                 "${OSG_3RDPARTY_BUILD_DIR}/lib/${CMAKE_BUILD_TYPE}/${CMAKE_ANDROID_ARCH_ABI}"
                 "${OSG_BUILD_DIR}/lib/${CMAKE_BUILD_TYPE}/${CMAKE_ANDROID_ARCH_ABI}" "${OSG_BUILD_DIR}/lib"
                 "${OSGVERSE_BUILD_DIR}/lib/${CMAKE_BUILD_TYPE}/${CMAKE_ANDROID_ARCH_ABI}" "${OSGVERSE_BUILD_DIR}/lib")
ADD_LIBRARY(osgVerseAndroid SHARED src/main.cpp)
TARGET_LINK_LIBRARIES(osgVerseAndroid log android EGL GLESv1_CM GLESv2 OpenSLES SDL2
                      debug osgVerseDependencyd optimized osgVerseDependency
                      debug osgVerseModelingd optimized osgVerseModeling
                      debug osgVerseAnimationd optimized osgVerseAnimation
                      debug osgVersePipelined optimized osgVersePipeline
                      debug osgVerseScriptd optimized osgVerseScript
                      debug osgVerseUId optimized osgVerseUI
                      debug osgVerseReaderWriterd optimized osgVerseReaderWriter
                      debug osgVerseWrappersd optimized osgVerseWrappers
                      debug OpenThreadsd optimized OpenThreads
                      debug osgd optimized osg
                      debug osgDBd optimized osgDB
                      debug osgUtild optimized osgUtil
                      debug osgTextd optimized osgText
                      debug osgGAd optimized osgGA
                      debug osgTerraind optimized osgTerrain
                      debug osgViewerd optimized osgViewer)
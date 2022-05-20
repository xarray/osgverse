SET(EXE_NAME osgVerse_EarthViewer)
SET(EXECUTABLE_FILES
    viewer_main.cpp
)

#########################################
FIND_PATH(OSGEARTH_INCLUDE_DIR osgEarth/EarthManipulator
    PATHS
    $ENV{OSG_ROOT}/include
    $ENV{OSG_DIR}/include
    /usr/include
    /usr/local/include
)

FIND_PATH(OSGEARTH_BUILD_INCLUDE_DIR osgEarth/BuildConfig
    PATHS
    $ENV{OSG_ROOT}/include
    $ENV{OSG_DIR}/include
    /usr/include
    /usr/local/include
)

FIND_PATH(OSGEARTH_LIB_DIR libosgEarth.so osgEarth.lib
    PATHS
    $ENV{OSG_ROOT}/lib
    $ENV{OSG_DIR}/lib
    /usr/lib
    /usr/local/lib
)
#########################################

IF(OSGEARTH_INCLUDE_DIR AND OSGEARTH_LIB_DIR)

    IF(OSGEARTH_BUILD_INCLUDE_DIR)
        INCLUDE_DIRECTORIES(${OSGEARTH_BUILD_INCLUDE_DIR})
    ENDIF(OSGEARTH_BUILD_INCLUDE_DIR)
    INCLUDE_DIRECTORIES(${OSGEARTH_INCLUDE_DIR})
    LINK_DIRECTORIES(${OSGEARTH_LIB_DIR})

    NEW_EXECUTABLE(${EXE_NAME} SHARED)
    TARGET_LINK_LIBRARIES(${EXE_NAME} ${OPENGL_LIBRARIES} osgEarth
                          osgVerseDependency osgVerseReaderWriter osgVersePipeline)
    LINK_OSG_LIBRARY(${EXE_NAME} OpenThreads osg osgDB osgUtil osgGA osgText osgSim osgTerrain osgViewer)
    TARGET_COMPILE_OPTIONS(${EXE_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)
    
ELSE(OSGEARTH_INCLUDE_DIR AND OSGEARTH_LIB_DIR)
    
    MESSAGE("[osgVerse] Dependency osgEarth not found. ${EXE_NAME} will be ignored.")
    
ENDIF(OSGEARTH_INCLUDE_DIR AND OSGEARTH_LIB_DIR)
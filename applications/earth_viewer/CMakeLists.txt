SET(EXE_NAME osgVerse_EarthViewer)
SET(EXECUTABLE_FILES
    viewer_main.cpp
)

#########################################
FIND_PATH(OSGEARTH_INCLUDE_DIR osgEarth/MapNode
    PATHS ${THIRDPARTY_ROOT}/include
    $ENV{OSG_ROOT}/include
    $ENV{OSG_DIR}/include
    /usr/include
    /usr/local/include
)

FIND_PATH(OSGEARTH_BUILD_INCLUDE_DIR osgEarth/BuildConfig osgEarth/Config
    PATHS ${THIRDPARTY_ROOT}/include
    $ENV{OSG_ROOT}/include
    $ENV{OSG_DIR}/include
    /usr/include
    /usr/local/include
)

FIND_PATH(OSGEARTH_LIB_DIR libosgEarth.so osgEarth.lib
    PATHS ${THIRDPARTY_ROOT}/lib
    $ENV{OSG_ROOT}/${FIND_LIB_POSTFIX}
    $ENV{OSG_DIR}/${FIND_LIB_POSTFIX}
    /usr/lib /usr/${FIND_LIB_POSTFIX} /usr/lib/x86_64-linux-gnu
    /usr/local/lib /usr/local/${FIND_LIB_POSTFIX}
)
#########################################

IF(OSGEARTH_INCLUDE_DIR AND OSGEARTH_LIB_DIR)

    IF(OSGEARTH_BUILD_INCLUDE_DIR)
        INCLUDE_DIRECTORIES(${OSGEARTH_BUILD_INCLUDE_DIR})
    ENDIF(OSGEARTH_BUILD_INCLUDE_DIR)
    INCLUDE_DIRECTORIES(${OSGEARTH_INCLUDE_DIR})
    LINK_DIRECTORIES(${OSGEARTH_LIB_DIR})

    NEW_EXECUTABLE(${EXE_NAME} SHARED)
    USE_MSVC_DEBUGGER(${EXE_NAME} "${OSGEARTH_LIB_DIR}/../bin")
    SET_PROPERTY(TARGET ${EXE_NAME} PROPERTY FOLDER "APPLICATIONS")

    TARGET_LINK_LIBRARIES(${EXE_NAME} osgVerseDependency osgVerseReaderWriter osgVersePipeline)
    LINK_OSG_LIBRARY(${EXE_NAME} OpenThreads osg osgDB osgUtil osgGA osgText
                                 osgSim osgTerrain osgViewer osgEarth)
    TARGET_COMPILE_OPTIONS(${EXE_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)

	FIND_LIBRARY(OSGEARTH_UTIL NAMES osgEarthUtil PATHS ${OSGEARTH_LIB_DIR})
	FIND_LIBRARY(OSGEARTH_UTIL_DEBUG NAMES osgEarthUtil${OSG_DEBUG_POSTFIX} PATHS ${OSGEARTH_LIB_DIR})
	IF(OSGEARTH_UTIL AND OSGEARTH_UTIL_DEBUG)
		TARGET_LINK_LIBRARIES(${EXE_NAME} debug ${OSGEARTH_UTIL_DEBUG} optimized ${OSGEARTH_UTIL})
	ELSEIF(OSGEARTH_UTIL)
		TARGET_LINK_LIBRARIES(${EXE_NAME} ${OSGEARTH_UTIL})
    ENDIF()

ELSE(OSGEARTH_INCLUDE_DIR AND OSGEARTH_LIB_DIR)

    MESSAGE("[osgVerse] Dependency osgEarth not found. ${EXE_NAME} will be ignored.")

ENDIF(OSGEARTH_INCLUDE_DIR AND OSGEARTH_LIB_DIR)

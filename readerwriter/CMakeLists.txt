SET(LIB_NAME osgVerseReaderWriter)
SET(LIBRARY_FILES
    LoadSceneFBX.cpp LoadSceneFBX.h
    LoadSceneGLTF.cpp LoadSceneGLTF.h
    LoadTextureKTX.cpp LoadTextureKTX.h
    Utilities.h Utilities.cpp
)

#########################################
FIND_PATH(DRACO_INCLUDE_DIR draco/draco_features.h
    PATHS
    /usr/include
    /usr/local/include
)

FIND_PATH(DRACO_LIB_DIR libdraco.so draco.lib
    PATHS
    /usr/lib /usr/${FIND_LIB_POSTFIX}
    /usr/local/lib /usr/local/${FIND_LIB_POSTFIX}
)
#########################################

INCLUDE_DIRECTORIES(../3rdparty/libhv ../3rdparty/libhv/all
                    ../3rdparty/stb ../3rdparty/rapidjson)
ADD_DEFINITIONS(-DVERSE_RW_LIBRARY -DKHRONOS_STATIC -DHV_STATICLIB)

IF(DRACO_INCLUDE_DIR AND DRACO_LIB_DIR)

    INCLUDE_DIRECTORIES(${DRACO_INCLUDE_DIR})
    LINK_DIRECTORIES(${DRACO_LIB_DIR})
    SET(LIBRARY_FILES ${LIBRARY_FILES}
        DracoProcessor.cpp DracoProcessor.h)
    SET_PROPERTY(GLOBAL APPEND PROPERTY EXTERNAL_LIBRARY_PATHS ${DRACO_LIB_DIR})

ELSE(DRACO_INCLUDE_DIR AND DRACO_LIB_DIR)
    MESSAGE("[osgVerse] Dependency Draco not found. Some features in ${LIB_NAME} will be ignored.")
ENDIF(DRACO_INCLUDE_DIR AND DRACO_LIB_DIR)

IF(VERSE_STATIC_BUILD)
    NEW_LIBRARY(${LIB_NAME} STATIC)
ELSE()
    NEW_LIBRARY(${LIB_NAME} SHARED)
ENDIF()

TARGET_COMPILE_OPTIONS(${LIB_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)
IF(DRACO_INCLUDE_DIR AND DRACO_LIB_DIR)
    TARGET_LINK_LIBRARIES(${LIB_NAME} draco)
ENDIF(DRACO_INCLUDE_DIR AND DRACO_LIB_DIR)

IF(NOT UNIX)
    TARGET_LINK_LIBRARIES(${LIB_NAME} osgVerseDependency)
ENDIF()
LINK_OSG_LIBRARY(${LIB_NAME} OpenThreads osg osgDB osgUtil)

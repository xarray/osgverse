SET(LIB_NAME osgVerseReaderWriter)
SET(LIBRARY_FILES
    LoadSceneFBX.cpp LoadSceneFBX.h
    LoadSceneGLTF.cpp LoadSceneGLTF.h
    LoadTextureKTX.cpp LoadTextureKTX.h
    DracoProcessor.cpp DracoProcessor.h
    Utilities.h Utilities.cpp Export.h
)

INCLUDE_DIRECTORIES(../3rdparty/libhv ../3rdparty/libhv/all
                    ../3rdparty/stb ../3rdparty/rapidjson)
ADD_DEFINITIONS(-DVERSE_RW_LIBRARY -DKHRONOS_STATIC -DHV_STATICLIB)
IF(DRACO_FOUND)
    ADD_DEFINITIONS(-DVERSE_USE_DRACO)
ENDIF(DRACO_FOUND)

IF(VERSE_STATIC_BUILD)
    NEW_LIBRARY(${LIB_NAME} STATIC)
ELSE()
    NEW_LIBRARY(${LIB_NAME} SHARED)
ENDIF()

TARGET_COMPILE_OPTIONS(${LIB_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)
IF(DRACO_FOUND)
    TARGET_LINK_LIBRARIES(${LIB_NAME} draco)
ENDIF(DRACO_FOUND)

IF(NOT UNIX)
    TARGET_LINK_LIBRARIES(${LIB_NAME} osgVerseDependency osgVerseAnimation)
ENDIF()
LINK_OSG_LIBRARY(${LIB_NAME} OpenThreads osg osgDB osgUtil)

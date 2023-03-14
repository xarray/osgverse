SET(EXE_NAME osgVerse_ViewerGLES)
SET(EXECUTABLE_FILES
    sdl_viewer_main.cpp
)

#########################################
FIND_PATH(SDL2_INCLUDE_DIR SDL.h
    PATHS
    /usr/include
    /usr/local/include
)

FIND_PATH(SDL2_LIB_DIR libSDL2.so SDL2.lib
    PATHS
    /usr/lib /usr/${FIND_LIB_POSTFIX}
    /usr/local/lib /usr/local/${FIND_LIB_POSTFIX}
)
#########################################

IF(SDL2_INCLUDE_DIR AND SDL2_LIB_DIR)

    INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIR})
    LINK_DIRECTORIES(${SDL2_LIB_DIR})
    NEW_EXECUTABLE(${EXE_NAME} SHARED)

    SET_PROPERTY(TARGET ${EXE_NAME} PROPERTY FOLDER "APPLICATIONS")
    TARGET_LINK_LIBRARIES(${EXE_NAME} SDL2 SDL2main osgVerseDependency osgVerseReaderWriter osgVersePipeline)
    LINK_OSG_LIBRARY(${EXE_NAME} OpenThreads osg osgDB osgUtil osgGA osgText osgSim osgTerrain osgViewer)
    TARGET_COMPILE_OPTIONS(${EXE_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)

ELSE(SDL2_INCLUDE_DIR AND SDL2_LIB_DIR)
    
    MESSAGE("[osgVerse] Dependency SDL2 not found. ${EXE_NAME} will be ignored.")
    
ENDIF(SDL2_INCLUDE_DIR AND SDL2_LIB_DIR)
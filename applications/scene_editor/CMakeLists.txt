SET(EXE_NAME osgVerse_SceneEditor)
SET(EXECUTABLE_FILES
    editor_main.cpp
	hierarchy.cpp hierarchy.h
	properties.cpp properties.h
	scenelogic.cpp scenelogic.h
)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/3rdparty/imgui)
NEW_EXECUTABLE(${EXE_NAME} SHARED)

TARGET_LINK_LIBRARIES(${EXE_NAME} ${OPENGL_LIBRARIES} osgVerseDependency osgVersePipeline osgVerseUI)
LINK_OSG_LIBRARY(${EXE_NAME} OpenThreads osg osgDB osgUtil osgGA osgText osgSim osgTerrain osgViewer)
TARGET_COMPILE_OPTIONS(${EXE_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)

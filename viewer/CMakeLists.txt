SET(EXE_NAME osgVerse_ExeViewer)
SET(EXECUTABLE_FILES
    viewer_main.cpp
)

NEW_EXECUTABLE(${EXE_NAME} SHARED)
TARGET_LINK_LIBRARIES(${EXE_NAME} ${OPENGL_LIBRARIES}
                      osgVerseDependency osgVerseReaderWriter osgVersePipeline)
LINK_OSG_LIBRARY(${EXE_NAME} OpenThreads osg osgDB osgUtil osgGA osgText osgSim osgTerrain osgViewer)

TARGET_COMPILE_OPTIONS(${EXE_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)
IF(APPLE)
    TARGET_LINK_LIBRARIES(${EXE_NAME} objc)
ENDIF(APPLE)

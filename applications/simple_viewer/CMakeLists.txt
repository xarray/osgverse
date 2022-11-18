SET(EXE_NAME osgVerse_SimpleViewer)
SET(EXECUTABLE_FILES
    simple_viewer_main.cpp
)

NEW_EXECUTABLE(${EXE_NAME} SHARED)
TARGET_LINK_LIBRARIES(${EXE_NAME} osgVerseDependency osgVerseReaderWriter osgVersePipeline
                                  ${ADDITIONAL_LIBRARIES})
LINK_OSG_LIBRARY(${EXE_NAME} OpenThreads osg osgDB osgUtil osgGA osgText osgSim osgTerrain osgViewer)
TARGET_COMPILE_OPTIONS(${EXE_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)

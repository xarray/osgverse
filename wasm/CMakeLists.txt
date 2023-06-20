SET(CMAKE_EXECUTABLE_SUFFIX ".html")
SET(EXE_NAME osgVerse_ViewerWASM)
SET(EXECUTABLE_FILES
    wasm_viewer.h wasm_viewer.cpp
)

NEW_EXECUTABLE(${EXE_NAME} SHARED)
SET_TARGET_PROPERTIES(${EXE_NAME} PROPERTIES LINK_FLAGS
                      "--preload-file ${CMAKE_SOURCE_DIR}/assets --shell-file ${CMAKE_SOURCE_DIR}/wasm/shell.html -O3 -s USE_SDL=2")

SET_PROPERTY(TARGET ${EXE_NAME} PROPERTY FOLDER "APPLICATIONS")
TARGET_LINK_LIBRARIES(${EXE_NAME} osgVerseDependency osgVerseReaderWriter osgVersePipeline)
LINK_OSG_LIBRARY(${EXE_NAME} OpenThreads osg osgDB osgUtil osgGA osgText osgSim osgTerrain osgViewer)
TARGET_COMPILE_OPTIONS(${EXE_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)

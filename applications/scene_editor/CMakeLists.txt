SET(EXE_NAME osgVerse_SceneEditor)
SET(EXECUTABLE_FILES
    editor_main.cpp defines.h app.rc
	editor_menu1.cpp editor_menu2.cpp editor_menu3.cpp
)

INCLUDE_DIRECTORIES(../3rdparty/imgui)
NEW_EXECUTABLE(${EXE_NAME} SHARED)
SET_PROPERTY(TARGET ${EXE_NAME} PROPERTY FOLDER "APPLICATIONS")

TARGET_LINK_LIBRARIES(${EXE_NAME} osgVerseUI osgVerseDependency osgVerseModeling osgVerseAnimation
                      osgVersePipeline osgVerseReaderWriter osgVerseWrappers)
LINK_OSG_LIBRARY(${EXE_NAME} OpenThreads osg osgDB osgUtil osgGA osgText osgSim osgTerrain osgViewer)
TARGET_COMPILE_OPTIONS(${EXE_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)

USE_MSVC_DEBUGGER(${EXE_NAME} "${THIRDPARTY_ROOT}/bin;${OSG_BUILD_INCLUDE_DIR}/../bin")
IF(MSVC AND VERSE_INSTALL_PDB_FILES)
    INSTALL(FILES $<TARGET_PDB_FILE:${EXE_NAME}> DESTINATION ${INSTALL_BINDIR} OPTIONAL)
ENDIF()

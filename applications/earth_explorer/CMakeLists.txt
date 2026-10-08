SET(EXE_NAME osgVerse_EarthExplorer)
SET(EXECUTABLE_FILES
    earth_main.cpp earth_ui.cpp EarthAppConfig.cpp DataSource.cpp feed_sources.cpp
    ai_chat.h ai_chat.cpp ai_setup.h ai_setup.cpp
    marker_style.h marker_style.cpp render_effects.cpp
    city_data.cpp ReaderWriterCSV.cpp
)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/3rdparty/imgui)
NEW_EXECUTABLE(${EXE_NAME} SHARED)
SET_PROPERTY(TARGET ${EXE_NAME} PROPERTY FOLDER "APPLICATIONS")
TARGET_LINK_LIBRARIES(${EXE_NAME} osgVerseReaderWriter osgVerseAnimation osgVersePipeline
                      osgVerseWrappers osgVerseModeling osgVerseScript osgVerseUI osgVerseAI
                      osgVerseDependency ${EXTERNAL_LIBRARIES})
LINK_OSG_LIBRARY(${EXE_NAME} OpenThreads osg osgDB osgUtil osgGA osgText osgSim osgTerrain osgViewer)
TARGET_COMPILE_OPTIONS(${EXE_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)
IF(MSVC)
    # Source files are UTF-8 (no BOM); tell MSVC explicitly
    TARGET_COMPILE_OPTIONS(${EXE_NAME} PRIVATE /utf-8)
ENDIF()

USE_MSVC_DEBUGGER(${EXE_NAME} "${THIRDPARTY_ROOT}/bin;${OSG_BUILD_INCLUDE_DIR}/../bin")
IF(MSVC AND VERSE_INSTALL_PDB_FILES)
    INSTALL(FILES $<TARGET_PDB_FILE:${EXE_NAME}> DESTINATION ${INSTALL_BINDIR} OPTIONAL)
ENDIF()

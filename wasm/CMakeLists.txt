SET(CMAKE_EXECUTABLE_SUFFIX ".html")
SET(EXE_NAME osgVerse_ViewerWASM)
SET(EXECUTABLE_FILES
    wasm_viewer.h wasm_viewer.cpp
)

IF(VERSE_STATIC_BUILD)
    GET_PROPERTY(PLUGIN_LIBRARIES_TO_LINK GLOBAL PROPERTY VERSE_PLUGIN_LIBRARIES)
    SET(EXTERNAL_LIBRARIES ${EXTERNAL_LIBRARIES} ${PLUGIN_LIBRARIES_TO_LINK})
    
    IF(VERSE_USE_OSG_STATIC)
        SET(OSGPLUGIN_LIBRARIES osgdb_glsl osgdb_trans osgdb_rot osgdb_scale osgdb_osg osgdb_rgb osgdb_bmp)
        SET(OSGPLUGIN_LIBRARIES ${OSGPLUGIN_LIBRARIES}
            osgdb_deprecated_osg osgdb_deprecated_osgsim osgdb_deprecated_osgtext
            osgdb_deprecated_osgviewer osgdb_deprecated_osgterrain)
        SET(OSGPLUGIN_LIBRARIES ${OSGPLUGIN_LIBRARIES}
            osgdb_serializers_osg osgdb_serializers_osgtext osgdb_serializers_osgterrain osgdb_serializers_osgsim)
        SET(EXTERNAL_LIBRARIES ${EXTERNAL_LIBRARIES} ${OSGPLUGIN_LIBRARIES})
    ENDIF(VERSE_USE_OSG_STATIC)
ENDIF(VERSE_STATIC_BUILD)

NEW_EXECUTABLE(${EXE_NAME} SHARED)
SET_TARGET_PROPERTIES(${EXE_NAME} PROPERTIES LINK_FLAGS
                      "--preload-file ${CMAKE_SOURCE_DIR}/assets --shell-file ${CMAKE_SOURCE_DIR}/wasm/shell.html -O3 -s USE_SDL=2")

SET_PROPERTY(TARGET ${EXE_NAME} PROPERTY FOLDER "APPLICATIONS")
TARGET_LINK_LIBRARIES(${EXE_NAME} osgVerseDependency osgVerseReaderWriter osgVersePipeline)
LINK_OSG_LIBRARY(${EXE_NAME} OpenThreads osg osgDB osgUtil osgGA osgText osgSim osgTerrain osgViewer)
TARGET_COMPILE_OPTIONS(${EXE_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)

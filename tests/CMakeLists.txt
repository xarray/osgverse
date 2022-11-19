MACRO(NEW_TEST_EXECUTABLE EXECUTABLE_NAME SOURCE_FILE)

    SET(EXE_NAME ${EXECUTABLE_NAME})
    SET(EXECUTABLE_FILES ${SOURCE_FILE})
    NEW_EXECUTABLE(${EXE_NAME} SHARED)
    
    IF(VERSE_STATIC_BUILD)
        GET_PROPERTY(PLUGIN_LIBRARIES_TO_LINK GLOBAL PROPERTY VERSE_PLUGIN_LIBRARIES)
        SET(ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES} ${PLUGIN_LIBRARIES_TO_LINK})
        
        IF(VERSE_USE_OSG_STATIC)
            SET(OSGPLUGIN_LIBRARIES osgdb_glsl osgdb_trans osgdb_rot osgdb_scale osgdb_osg osgdb_rgb osgdb_bmp)
            SET(OSGPLUGIN_LIBRARIES ${OSGPLUGIN_LIBRARIES}
                osgdb_deprecated_osg osgdb_deprecated_osgsim osgdb_deprecated_osgtext
                osgdb_deprecated_osgviewer osgdb_deprecated_osgterrain)
            SET(OSGPLUGIN_LIBRARIES ${OSGPLUGIN_LIBRARIES}
                osgdb_serializers_osg osgdb_serializers_osgtext osgdb_serializers_osgterrain osgdb_serializers_osgsim)
            SET(ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES} ${OSGPLUGIN_LIBRARIES})
        ENDIF(VERSE_USE_OSG_STATIC)
    ENDIF(VERSE_STATIC_BUILD)
    
    TARGET_COMPILE_OPTIONS(${EXE_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)
    TARGET_LINK_LIBRARIES(${EXE_NAME} osgVerseUI osgVerseDependency osgVerseReaderWriter
                          osgVersePipeline osgVerseAnimation osgVerseModeling ${ADDITIONAL_LIBRARIES})
    LINK_OSG_LIBRARY(${EXE_NAME} OpenThreads osg osgDB osgUtil osgGA osgText osgSim osgTerrain osgViewer)

ENDMACRO(NEW_TEST_EXECUTABLE)

INCLUDE_DIRECTORIES(.)
NEW_TEST_EXECUTABLE(osgVerse_Test_ImGui imgui_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_FastRtt fast_rtt_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Shadow shadow_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Pbr_Prerequisite pbr_prerequisite.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Obb_KDop obb_kdop_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Mesh_Process mesh_process_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Paging_Lod paging_lod_test.cpp)
IF(BULLET_FOUND)
	NEW_TEST_EXECUTABLE(osgVerse_Test_Physics_Basic physics_basic_test.cpp)
	# TODO: physics_drawbridge_test, physics_softbody_test, player_walk_test
ENDIF(BULLET_FOUND)

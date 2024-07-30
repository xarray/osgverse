MACRO(NEW_TEST_EXECUTABLE EXECUTABLE_NAME SOURCE_FILE)

    SET(EXE_NAME ${EXECUTABLE_NAME})
    SET(EXECUTABLE_FILES ${SOURCE_FILE})
    NEW_EXECUTABLE(${EXE_NAME} SHARED)
    SET_PROPERTY(TARGET ${EXE_NAME} PROPERTY FOLDER "TESTS")

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

    TARGET_COMPILE_OPTIONS(${EXE_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)
    TARGET_INCLUDE_DIRECTORIES(${EXE_NAME} PUBLIC ${EXTERNAL_INCLUDES})
    TARGET_LINK_LIBRARIES(${EXE_NAME} osgVerseUI osgVerseDependency osgVerseReaderWriter osgVerseScript
                          osgVersePipeline osgVerseAnimation osgVerseModeling osgVerseAI ${EXTERNAL_LIBRARIES})
    LINK_OSG_LIBRARY(${EXE_NAME} OpenThreads osg osgDB osgUtil osgGA osgText osgSim osgTerrain osgViewer)
    IF(NOT VERSE_STATIC_BUILD)
        IF(MSVC AND VERSE_INSTALL_PDB_FILES)
            INSTALL(FILES $<TARGET_PDB_FILE:${EXE_NAME}> DESTINATION ${INSTALL_BINDIR} OPTIONAL)
        ENDIF()
    ENDIF()

ENDMACRO(NEW_TEST_EXECUTABLE)

INCLUDE_DIRECTORIES(. ${CMAKE_SOURCE_DIR}/3rdparty/libhv ${CMAKE_SOURCE_DIR}/3rdparty/libhv/all)
ADD_DEFINITIONS(-DHV_STATICLIB)

NEW_TEST_EXECUTABLE(osgVerse_Test_Report_Graph report_graph_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_ImGui imgui_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Pipeline pipeline_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Shadow shadow_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Media_Stream media_stream_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Pbr_Prerequisite pbr_prerequisite.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Mesh_Process mesh_process_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Mesh_Boolean mesh_boolean_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Paging_Lod paging_lod_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Point_Cloud point_cloud_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Restful_Server restful_server_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Player_Animation player_animation_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Compressing compressing_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Thread hybrid_thread_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Volume_Rendering volume_rendering_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Symbols symbols_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Tween_Animation tween_animation_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Navigation navigation_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Texture_Mapping texture_mapping_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Auto_LOD auto_lod_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Polygon_Process polygon_process_test.cpp)

IF(BULLET_FOUND)
	NEW_TEST_EXECUTABLE(osgVerse_Test_Physics_Basic physics_basic_test.cpp)
	# TODO: physics_drawbridge_test, physics_softbody_test, player_walk_test
ENDIF(BULLET_FOUND)

IF(OSG_MAJOR_VERSION GREATER 2 AND OSG_MINOR_VERSION GREATER 5)
    NEW_TEST_EXECUTABLE(osgVerse_Test_Scripting scripting_test.cpp)
ENDIF()

IF(VERSE_SUPPORT_CPP17)
    SET(CMAKE_CXX_STANDARD 17)
    NEW_TEST_EXECUTABLE(osgVerse_Test_ECS entity_components_test.cpp)
ENDIF(VERSE_SUPPORT_CPP17)

IF(VERSE_BUILD_DEPRECATED_TESTS)
    NEW_TEST_EXECUTABLE(osgVerse_Test_FastRtt deprecated/fast_rtt_test.cpp)
    NEW_TEST_EXECUTABLE(osgVerse_Test_CubeRtt deprecated/render_to_cube_test.cpp)
    NEW_TEST_EXECUTABLE(osgVerse_Test_Obb_KDop deprecated/obb_kdop_test.cpp)

    IF(OSG_MAJOR_VERSION GREATER 2 AND OSG_MINOR_VERSION GREATER 5)
        NEW_TEST_EXECUTABLE(osgVerse_Test_Indirect_Draw deprecated/indirect_drawing_test.cpp)
        NEW_TEST_EXECUTABLE(osgVerse_Test_Tessellation deprecated/tessellation_test.cpp)
        NEW_TEST_EXECUTABLE(osgVerse_Test_MultiView_Shader deprecated/multiview_shader_test.cpp)
    ENDIF()
ENDIF(VERSE_BUILD_DEPRECATED_TESTS)

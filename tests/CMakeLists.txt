MACRO(NEW_SIMPLE_EXECUTABLE EXECUTABLE_NAME SOURCE_FILE)

    SET(EXE_NAME ${EXECUTABLE_NAME})
    SET(EXECUTABLE_FILES ${SOURCE_FILE})
    NEW_EXECUTABLE(${EXE_NAME} SHARED)

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
                          osgVersePipeline osgVerseAnimation osgVerseModeling osgVerseAI
                          osgVerseWrappers ${EXTERNAL_LIBRARIES})
    LINK_OSG_LIBRARY(${EXE_NAME} OpenThreads osg osgDB osgUtil osgGA osgText osgSim osgTerrain osgViewer)
    IF(NOT VERSE_STATIC_BUILD)
        IF(MSVC AND VERSE_INSTALL_PDB_FILES)
            INSTALL(FILES $<TARGET_PDB_FILE:${EXE_NAME}> DESTINATION ${INSTALL_BINDIR} OPTIONAL)
        ENDIF()
    ENDIF()

ENDMACRO(NEW_SIMPLE_EXECUTABLE)

MACRO(NEW_TEST EXECUTABLE_NAME SOURCE_FILE)
    NEW_SIMPLE_EXECUTABLE(${EXECUTABLE_NAME} ${SOURCE_FILE})
    SET_PROPERTY(TARGET ${EXECUTABLE_NAME} PROPERTY FOLDER "TESTS")
ENDMACRO(NEW_TEST)

MACRO(NEW_EXAMPLE EXECUTABLE_NAME SOURCE_FILE)
    NEW_SIMPLE_EXECUTABLE(${EXECUTABLE_NAME} ${SOURCE_FILE})
    SET_PROPERTY(TARGET ${EXECUTABLE_NAME} PROPERTY FOLDER "EXAMPLES")
ENDMACRO(NEW_EXAMPLE)

INCLUDE_DIRECTORIES(. ../3rdparty/libhv ../3rdparty/libhv/all)
ADD_DEFINITIONS(-DHV_STATICLIB)

IF(NOT VERSE_USE_EXTERNAL_GLES)
    NEW_TEST(osgVerse_Test_Compressing compressing_test.cpp)
    NEW_TEST(osgVerse_Test_Thread hybrid_thread_test.cpp)
    NEW_TEST(osgVerse_Test_Volume_Rendering volume_rendering_test.cpp)
    NEW_TEST(osgVerse_Test_Instance_Param instance_param_test.cpp)
    NEW_TEST(osgVerse_Test_Auto_LOD auto_lod_test.cpp)
    NEW_TEST(osgVerse_Test_Sky_Box sky_box_test.cpp)
ENDIF(NOT VERSE_USE_EXTERNAL_GLES)

NEW_EXAMPLE(osgVerse_Test_Plugins plugins_test.cpp)
NEW_EXAMPLE(osgVerse_Test_Pipeline pipeline_test.cpp)
NEW_EXAMPLE(osgVerse_Test_Report_Graph report_graph_test.cpp)
NEW_EXAMPLE(osgVerse_Test_Shader_Library shader_library_test.cpp)
NEW_EXAMPLE(osgVerse_Test_Shadow shadow_test.cpp)
NEW_EXAMPLE(osgVerse_Test_Forward_Pbr forward_pbr_test.cpp)
IF(NOT VERSE_USE_EXTERNAL_GLES)
    NEW_EXAMPLE(osgVerse_Test_ImGui imgui_test.cpp)
    NEW_EXAMPLE(osgVerse_Test_Media_Stream media_stream_test.cpp)
    NEW_EXAMPLE(osgVerse_Test_Pbr_Prerequisite pbr_prerequisite.cpp)
    NEW_EXAMPLE(osgVerse_Test_Paging_Lod paging_lod_test.cpp)
    NEW_EXAMPLE(osgVerse_Test_Point_Cloud point_cloud_test.cpp)
    NEW_EXAMPLE(osgVerse_Test_Player_Animation player_animation_test.cpp)
    NEW_EXAMPLE(osgVerse_Test_Symbols symbols_test.cpp)
    NEW_EXAMPLE(osgVerse_Test_Tween_Animation tween_animation_test.cpp)
    NEW_EXAMPLE(osgVerse_Test_Polygon2D polygon2d_test.cpp)
    NEW_EXAMPLE(osgVerse_Test_Mesh_Process mesh_process_test.cpp)
    NEW_EXAMPLE(osgVerse_Test_Navigation navigation_test.cpp)

	IF(MSVC_VERSION GREATER 1900)
        NEW_TEST(osgVerse_Test_Restful_Server restful_server_test.cpp)
	ENDIF(MSVC_VERSION GREATER 1900)

    IF(BULLET_FOUND)
        NEW_EXAMPLE(osgVerse_Test_Physics_Basic physics_basic_test.cpp)
	    # TODO: physics_drawbridge_test, physics_softbody_test, player_walk_test
    ENDIF(BULLET_FOUND)

    NEW_EXAMPLE(osgVerse_Test_Particle_U3D particle_u3d_test.cpp)
    IF(EFFEKSEER_FOUND)
        NEW_EXAMPLE(osgVerse_Test_Particle_Effekseer particle_effekseer_test.cpp)
    ENDIF(EFFEKSEER_FOUND)

    IF(OSG_MAJOR_VERSION GREATER 2 AND OSG_MINOR_VERSION GREATER 5)
        NEW_EXAMPLE(osgVerse_Test_Scripting scripting_test.cpp)
    ENDIF()

    IF(VERSE_SUPPORT_CPP17)
        SET(CMAKE_CXX_STANDARD 17)
        ##
    ENDIF(VERSE_SUPPORT_CPP17)
ENDIF(NOT VERSE_USE_EXTERNAL_GLES)

IF(VERSE_BUILD_DEPRECATED_TESTS)
    IF(NOT VERSE_USE_EXTERNAL_GLES)
        NEW_TEST(osgVerse_Test_FastRtt deprecated/fast_rtt_test.cpp)
        NEW_TEST(osgVerse_Test_CubeRtt deprecated/render_to_cube_test.cpp)
        NEW_TEST(osgVerse_Test_Obb_KDop deprecated/obb_kdop_test.cpp)
        NEW_TEST(osgVerse_Test_Mesh_Boolean deprecated/mesh_boolean_test.cpp)
        NEW_TEST(osgVerse_Test_Auto_Imposter deprecated/auto_imposter_test.cpp)
        NEW_TEST(osgVerse_Test_Texture_Mapping deprecated/texture_mapping_test.cpp)

        IF(OSG_MAJOR_VERSION GREATER 2 AND OSG_MINOR_VERSION GREATER 5)
            NEW_TEST(osgVerse_Test_Indirect_Draw deprecated/indirect_drawing_test.cpp)
            NEW_TEST(osgVerse_Test_Tessellation deprecated/tessellation_test.cpp)
            NEW_TEST(osgVerse_Test_MultiView_Shader deprecated/multiview_shader_test.cpp)
        ENDIF()
    ENDIF(NOT VERSE_USE_EXTERNAL_GLES)
ENDIF(VERSE_BUILD_DEPRECATED_TESTS)

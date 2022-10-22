MACRO(NEW_TEST_EXECUTABLE EXECUTABLE_NAME SOURCE_FILE)

    SET(EXE_NAME ${EXECUTABLE_NAME})
    SET(EXECUTABLE_FILES ${SOURCE_FILE})
    NEW_EXECUTABLE(${EXE_NAME} SHARED)
    TARGET_COMPILE_OPTIONS(${EXE_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)
    TARGET_LINK_LIBRARIES(${EXE_NAME} osgVerseUI osgVerseDependency osgVerseReaderWriter
                          osgVersePipeline osgVerseAnimation osgVerseModeling ${OPENGL_LIBRARIES})
    LINK_OSG_LIBRARY(${EXE_NAME} OpenThreads osg osgDB osgUtil osgGA osgText osgSim osgTerrain osgViewer)

ENDMACRO(NEW_TEST_EXECUTABLE)

INCLUDE_DIRECTORIES(.)
NEW_TEST_EXECUTABLE(osgVerse_Test_ImGui imgui_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_FastRtt fast_rtt_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Pbr_Prerequisite pbr_prerequisite.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Obb_KDop obb_kdop_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Mesh_Process mesh_process_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Paging_Lod paging_lod_test.cpp)
IF(BULLET_FOUND)
	NEW_TEST_EXECUTABLE(osgVerse_Test_Physics_Basic physics_basic_test.cpp)
	# TODO: physics_drawbridge_test, physics_softbody_test, player_walk_test
ENDIF(BULLET_FOUND)

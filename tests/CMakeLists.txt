MACRO(NEW_TEST_EXECUTABLE EXECUTABLE_NAME SOURCE_FILE)

    SET(EXE_NAME ${EXECUTABLE_NAME})
    SET(EXECUTABLE_FILES ${SOURCE_FILE})
    NEW_EXECUTABLE(${EXE_NAME} SHARED)
    TARGET_COMPILE_OPTIONS(${EXE_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)
    TARGET_LINK_LIBRARIES(${EXE_NAME} osgVerseDependency osgVerseReaderWriter osgVersePipeline
	                      osgVerseAnimation osgVerseModeling osgVerseUI)
    LINK_OSG_LIBRARY(${EXE_NAME} OpenThreads osg osgDB osgUtil osgGA osgText osgSim osgTerrain osgViewer)

ENDMACRO(NEW_TEST_EXECUTABLE)

INCLUDE_DIRECTORIES(.)
NEW_TEST_EXECUTABLE(osgVerse_Test_ImGui imgui_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_FastRtt fast_rtt_test.cpp)
NEW_TEST_EXECUTABLE(osgVerse_Test_Obb_KDop obb_kdop_test.cpp)
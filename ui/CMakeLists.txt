SET(LIB_NAME osgVerseUI)
SET(LIBRARY_INCLUDE_FILES ImGui.h ImGuiComponents.h
    SerializerInterface.h SceneHierarchy.h SceneNavigation.h)
SET(LIBRARY_FILES ${LIBRARY_INCLUDE_FILES}
	ImGui.cpp ImGui3D.cpp ImGuiComponents.cpp ImGui.Styles.h
    SerializerInterface.cpp SceneHierarchy.cpp SceneNavigation.cpp
    serializers/object_serializer.cpp serializers/image_serializer.cpp
    serializers/bool_serializer.cpp serializers/string_serializer.cpp
    serializers/value_serializer.cpp serializers/vector_serializer.cpp
    serializers/enum_serializer.cpp serializers/matrix_serializer.cpp
    serializers/quat_serializer.cpp serializers/bound_serializer.cpp
    serializers/list_serializer.cpp serializers/empty_serializer.cpp
    serializers/stateset_serializer.cpp
	compontents/SpiderEditor.cpp compontents/Timeline.cpp
	compontents/VirtualKeyboard.cpp
)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/3rdparty/imgui)
ADD_DEFINITIONS(-DGLEW_STATIC -DGLEW_NO_GLU -DIMGUI_IMPL_OPENGL_LOADER_GLEW)
IF(VERSE_USE_EXTERNAL_GLES2)
    ADD_DEFINITIONS(-DGLEW_EGL -DIMGUI_IMPL_OPENGL_ES2)
ELSEIF(VERSE_USE_EXTERNAL_GLES3)
    ADD_DEFINITIONS(-DGLEW_EGL -DIMGUI_IMPL_OPENGL_ES3)
ENDIF()
NEW_LIBRARY(${LIB_NAME} STATIC)

TARGET_LINK_LIBRARIES(${LIB_NAME} osgVerseDependency osgVerseModeling osgVerseScript)
TARGET_COMPILE_OPTIONS(${LIB_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)

INSTALL(TARGETS ${LIB_NAME} EXPORT ${LIB_NAME}
        RUNTIME DESTINATION ${INSTALL_BINDIR} COMPONENT libosgverse
        LIBRARY DESTINATION ${INSTALL_LIBDIR} COMPONENT libosgverse
        ARCHIVE DESTINATION ${INSTALL_ARCHIVEDIR} COMPONENT libosgverse-dev)
INSTALL(FILES ${LIBRARY_INCLUDE_FILES} DESTINATION ${INSTALL_INCDIR}/osgVerse/ui COMPONENT libosgverse-dev)

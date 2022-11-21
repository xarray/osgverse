SET(LIB_NAME osgVerseUI)
SET(LIBRARY_INCLUDE_FILES
    ImGui.h ImGuiComponents.h UserComponent.h
	CommandHandler.h PropertyInterface.h)
SET(LIBRARY_FILES ${LIBRARY_INCLUDE_FILES}
	ImGui.cpp ImGui3D.cpp ImGuiComponents.cpp ImGui.Styles.h
	UserComponent.cpp CommandHandler.cpp PropertyInterface.cpp
	ui_compontents/SpiderEditor.cpp
	ui_compontents/Timeline.cpp
	ui_compontents/VirtualKeyboard.cpp
	std_properties/BasicPropertyItem.cpp
	std_properties/TransformPropertyItem.cpp
	std_properties/GeometryPropertyItem.cpp
	std_properties/TexturePropertyItem.cpp
	executors/DefaultExecutors.cpp
)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/3rdparty/imgui)
ADD_DEFINITIONS(-DGLEW_STATIC -DGLEW_NO_GLU -DIMGUI_IMPL_OPENGL_LOADER_GLEW)
NEW_LIBRARY(${LIB_NAME} STATIC)

TARGET_LINK_LIBRARIES(${LIB_NAME} osgVerseDependency)
TARGET_COMPILE_OPTIONS(${LIB_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)

INSTALL(TARGETS ${LIB_NAME} EXPORT ${LIB_NAME}
        RUNTIME DESTINATION ${INSTALL_BINDIR} COMPONENT libosgverse
        LIBRARY DESTINATION ${INSTALL_LIBDIR} COMPONENT libosgverse
        ARCHIVE DESTINATION ${INSTALL_ARCHIVEDIR} COMPONENT libosgverse-dev)
INSTALL(FILES ${LIBRARY_INCLUDE_FILES} DESTINATION ${INSTALL_INCDIR}/osgVerse/ui COMPONENT libosgverse-dev)

SET(LIB_NAME osgVerseUI)
SET(LIBRARY_FILES
	ImGui.cpp ImGui.h ImGui.Styles.h
	ImGuiComponents.cpp ImGuiComponents.h
	CommandHandler.cpp CommandHandler.h
	PropertyInterface.h PropertyInterface.cpp
	ui_compontents/SpiderEditor.cpp
	ui_compontents/Timeline.cpp
	std_properties/BasicPropertyItem.cpp
	std_properties/TransformPropertyItem.cpp
	std_properties/GeometryPropertyItem.cpp
	std_properties/TexturePropertyItem.cpp
)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/3rdparty/imgui)
NEW_LIBRARY(${LIB_NAME} STATIC)
TARGET_LINK_LIBRARIES(${LIB_NAME} osgVerseDependency)
TARGET_COMPILE_OPTIONS(${LIB_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)

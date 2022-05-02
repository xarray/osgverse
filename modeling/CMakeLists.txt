SET(LIB_NAME osgVerseModeling)
SET(LIBRARY_FILES
    MeshDeformer.cpp MeshDeformer.hpp
    Math.h Math.cpp Utilities.cpp Utilities.h
)

NEW_LIBRARY(${LIB_NAME} STATIC)
TARGET_LINK_LIBRARIES(${LIB_NAME} osgVerseDependency)
TARGET_COMPILE_OPTIONS(${LIB_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)

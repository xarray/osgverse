SET(LIB_NAME osgVersePipeline)
SET(LIBRARY_FILES
    Pipeline.cpp Pipeline.h PipelineStandard.cpp
	DeferredCallback.cpp DeferredCallback.h
	ShadowModule.cpp ShadowModule.h
	Utilities.cpp Utilities.h
)

NEW_LIBRARY(${LIB_NAME} STATIC)
TARGET_LINK_LIBRARIES(${LIB_NAME} ${OPENGL_LIBRARIES} osgVerseDependency)
TARGET_COMPILE_OPTIONS(${LIB_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)

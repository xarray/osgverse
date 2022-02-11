SET(LIB_NAME osgVersePipeline)
SET(LIBRARY_FILES
    Pipeline.cpp Pipeline.h
	DeferredCallback.cpp DeferredCallback.h
	ShadowModule.cpp ShadowModule.h
	Utilities.cpp Utilities.h
)

NEW_LIBRARY(${LIB_NAME} STATIC)
TARGET_COMPILE_OPTIONS(${LIB_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)

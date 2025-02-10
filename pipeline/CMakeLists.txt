SET(LIB_NAME osgVersePipeline)
SET(LIBRARY_INCLUDE_FILES
    Pipeline.h DeferredCallback.h UserInputModule.h ShadowModule.h
	LightModule.h LightDrawable.h SkyBox.h NodeSelector.h
    SymbolManager.h Drawer2D.h IntersectionManager.h
    ShaderLibrary.h Utilities.h Global.h Allocator.h
)
SET(LIBRARY_FILES ${LIBRARY_INCLUDE_FILES}
    Pipeline.cpp PipelineStandard.cpp PipelineLoader.cpp DeferredCallback.cpp
    UserInputModule.cpp ShadowModule.cpp LightModule.cpp LightDrawable.cpp
    SkyBox.cpp NodeSelector.cpp SymbolManager.cpp Drawer2D.cpp
    IntersectionManager.cpp ShaderLibrary.cpp Utilities.cpp
)

IF(MSVC AND NOT VERSE_USE_EXTERNAL_GLES)
    SET(LIBRARY_FILES ${LIBRARY_FILES} TsfFramework.cpp TsfFramework.h)
ENDIF()

IF(VERSE_USE_MTT_DRIVER)
    ADD_DEFINITIONS(-DVERSE_ENABLE_MTT)
ENDIF(VERSE_USE_MTT_DRIVER)

ADD_DEFINITIONS(-DGLEW_STATIC -DGLEW_NO_GLU)
IF(VERSE_USE_EXTERNAL_GLES2)
    ADD_DEFINITIONS(-DGLEW_EGL)
ELSEIF(VERSE_USE_EXTERNAL_GLES3)
    ADD_DEFINITIONS(-DGLEW_EGL)
ENDIF()

IF(CUDA_FOUND)
    SET(LIBRARY_FILES ${LIBRARY_FILES} CudaTexture2D.cpp CudaTexture2D.h
        CudaUtils/BitDepth.cu CudaUtils/ColorSpace.cu CudaUtils/ColorSpace.h)
    NEW_CUDA_LIBRARY(${LIB_NAME} STATIC)
ELSE(CUDA_FOUND)
    NEW_LIBRARY(${LIB_NAME} STATIC)
ENDIF(CUDA_FOUND)

TARGET_LINK_LIBRARIES(${LIB_NAME} ${OPENGL_LIBRARIES} osgVerseDependency osgVerseModeling)
LINK_OSG_LIBRARY(${LIB_NAME} OpenThreads osg osgDB osgUtil osgGA osgViewer)
TARGET_COMPILE_OPTIONS(${LIB_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)

INSTALL(TARGETS ${LIB_NAME} EXPORT ${LIB_NAME}
        RUNTIME DESTINATION ${INSTALL_BINDIR} COMPONENT libosgverse
        LIBRARY DESTINATION ${INSTALL_LIBDIR} COMPONENT libosgverse
        ARCHIVE DESTINATION ${INSTALL_ARCHIVEDIR} COMPONENT libosgverse-dev)
INSTALL(FILES ${LIBRARY_INCLUDE_FILES} DESTINATION ${INSTALL_INCDIR}/osgVerse/pipeline COMPONENT libosgverse-dev)

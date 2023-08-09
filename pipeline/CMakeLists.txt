SET(LIB_NAME osgVersePipeline)
SET(LIBRARY_INCLUDE_FILES
    Pipeline.h DeferredCallback.h ShadowModule.h
	LightModule.h LightDrawable.h SkyBox.h NodeSelector.h
	IntersectionManager.h Utilities.h Global.h
)
SET(LIBRARY_FILES ${LIBRARY_INCLUDE_FILES}
    Pipeline.cpp PipelineStandard.cpp DeferredCallback.cpp
	ShadowModule.cpp LightModule.cpp LightDrawable.cpp SkyBox.cpp
	NodeSelector.cpp IntersectionManager.cpp Utilities.cpp 
)

IF(VERSE_USE_MTT_DRIVER)
    ADD_DEFINITIONS(-DVERSE_ENABLE_MTT)
ENDIF(VERSE_USE_MTT_DRIVER)

NEW_LIBRARY(${LIB_NAME} STATIC)
TARGET_LINK_LIBRARIES(${LIB_NAME} ${OPENGL_LIBRARIES} osgVerseDependency osgVerseModeling)
TARGET_COMPILE_OPTIONS(${LIB_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)

INSTALL(TARGETS ${LIB_NAME} EXPORT ${LIB_NAME}
        RUNTIME DESTINATION ${INSTALL_BINDIR} COMPONENT libosgverse
        LIBRARY DESTINATION ${INSTALL_LIBDIR} COMPONENT libosgverse
        ARCHIVE DESTINATION ${INSTALL_ARCHIVEDIR} COMPONENT libosgverse-dev)
INSTALL(FILES ${LIBRARY_INCLUDE_FILES} DESTINATION ${INSTALL_INCDIR}/osgVerse/pipeline COMPONENT libosgverse-dev)

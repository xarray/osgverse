SET(LIB_NAME osgVerseAnimation)
SET(LIBRARY_INCLUDE_FILES
    PlayerAnimation.h TweenAnimation.h BlendShapeAnimation.h
    PhysicsEngine.h ParticleEngine.h Utilities.h)
SET(LIBRARY_FILES ${LIBRARY_INCLUDE_FILES}
    PlayerAnimation.cpp PlayerAnimationInternal.h PlayerAnimationInternal.cpp
    TweenAnimation.cpp BlendShapeAnimation.cpp ParticleEngineU3D.cpp Utilities.cpp)

IF(BULLET_FOUND)
    INCLUDE("${BULLET_LIB_DIR}/cmake/bullet/BulletConfig.cmake" OPTIONAL)
    ADD_DEFINITIONS(-DVERSE_WITH_BULLET ${BULLET_DEFINITIONS})
	SET(LIBRARY_FILES ${LIBRARY_FILES} PhysicsEngine.cpp)
ENDIF(BULLET_FOUND)

IF(NOT VERSE_SUPPORT_CPP17)
    MESSAGE("[osgVerse] C++ 17 standard is disabled at present. Effekseer will be ignored.")
ELSEIF(EFFEKSEER_FOUND)
    SET(LIBRARY_FILES ${LIBRARY_FILES} ParticleEngine.cpp)
ENDIF()

NEW_LIBRARY(${LIB_NAME} STATIC)
TARGET_LINK_LIBRARIES(${LIB_NAME} osgVerseDependency osgVersePipeline osgVerseModeling)
LINK_OSG_LIBRARY(${LIB_NAME} OpenThreads osg osgDB osgUtil osgGA osgViewer)
TARGET_COMPILE_OPTIONS(${LIB_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)
IF(BULLET_FOUND)
    TARGET_LINK_LIBRARIES(${LIB_NAME} debug BulletDynamics${VERSE_DEBUG_POSTFIX} optimized BulletDynamics)
    TARGET_LINK_LIBRARIES(${LIB_NAME} debug BulletCollision${VERSE_DEBUG_POSTFIX} optimized BulletCollision)
    TARGET_LINK_LIBRARIES(${LIB_NAME} debug LinearMath${VERSE_DEBUG_POSTFIX} optimized LinearMath)
ENDIF(BULLET_FOUND)

IF(EFFEKSEER_FOUND)
    TARGET_LINK_LIBRARIES(${LIB_NAME} debug Effekseer${VERSE_DEBUG_POSTFIX} optimized Effekseer)
    TARGET_LINK_LIBRARIES(${LIB_NAME} debug EffekseerRendererCommon${VERSE_DEBUG_POSTFIX}
                                      optimized EffekseerRendererCommon)
    TARGET_LINK_LIBRARIES(${LIB_NAME} debug EffekseerRendererGL${VERSE_DEBUG_POSTFIX}
                                      optimized EffekseerRendererGL)
    TARGET_LINK_LIBRARIES(${LIB_NAME} debug EffekseerSoundDSound${VERSE_DEBUG_POSTFIX}
                                      optimized EffekseerSoundDSound)
ENDIF(EFFEKSEER_FOUND)

INSTALL(TARGETS ${LIB_NAME} EXPORT ${LIB_NAME}
        RUNTIME DESTINATION ${INSTALL_BINDIR} COMPONENT libosgverse
        LIBRARY DESTINATION ${INSTALL_LIBDIR} COMPONENT libosgverse
        ARCHIVE DESTINATION ${INSTALL_ARCHIVEDIR} COMPONENT libosgverse-dev)
INSTALL(FILES ${LIBRARY_INCLUDE_FILES} DESTINATION ${INSTALL_INCDIR}/osgVerse/animation COMPONENT libosgverse-dev)

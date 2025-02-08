IF(VERSE_STATIC_BUILD)
    GET_PROPERTY(PLUGIN_LIBRARIES_TO_LINK GLOBAL PROPERTY VERSE_PLUGIN_LIBRARIES)
    SET(EXTERNAL_LIBRARIES ${EXTERNAL_LIBRARIES} ${PLUGIN_LIBRARIES_TO_LINK})

    IF(VERSE_USE_OSG_STATIC)
        SET(OSGPLUGIN_LIBRARIES osgdb_glsl osgdb_trans osgdb_rot osgdb_scale osgdb_osg osgdb_rgb osgdb_bmp)
        SET(OSGPLUGIN_LIBRARIES ${OSGPLUGIN_LIBRARIES}
            osgdb_deprecated_osg osgdb_deprecated_osgsim osgdb_deprecated_osgtext
            osgdb_deprecated_osgviewer osgdb_deprecated_osgterrain)
        SET(OSGPLUGIN_LIBRARIES ${OSGPLUGIN_LIBRARIES}
            osgdb_serializers_osg osgdb_serializers_osgtext osgdb_serializers_osgterrain osgdb_serializers_osgsim)
        RELINK_OSGPLUGINS(OSGPLUGIN_LIBRARIES2 ${OSGPLUGIN_LIBRARIES})
        SET(EXTERNAL_LIBRARIES ${EXTERNAL_LIBRARIES} ${OSGPLUGIN_LIBRARIES2})
    ENDIF(VERSE_USE_OSG_STATIC)
ENDIF(VERSE_STATIC_BUILD)

IF(NOT VERSE_USE_EXTERNAL_GLES)
    ADD_SUBDIRECTORY(viewer)
    ADD_SUBDIRECTORY(viewer_composite)

    IF(VERSE_BUILD_WITH_QT)
        IF(DEFINED ENV{QT5_DIR})
            SET(QT5_DIR $ENV{QT5_DIR})
        ENDIF()

        IF(DEFINED ENV{QT6_DIR})
            SET(QT6_DIR $ENV{QT6_DIR})
        ENDIF()

        SET(CMAKE_AUTOMOC OFF)
        FIND_PACKAGE(Qt5 COMPONENTS Core Gui Widgets OpenGL)
        FIND_PACKAGE(Qt6 COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)
        #########################################

        ADD_SUBDIRECTORY(qt_viewer)
        ADD_SUBDIRECTORY(qml_viewer)
    ENDIF()
ENDIF()
ADD_SUBDIRECTORY(earth_viewer)
ADD_SUBDIRECTORY(scene_editor)

IF(OSG_MAJOR_VERSION GREATER 2 AND OSG_MINOR_VERSION GREATER 5)
    ADD_SUBDIRECTORY(sdl_es_viewer)
ENDIF()

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
        IF(DEFINED ENV{QT6_DIR})
            SET(QT6_DIR $ENV{QT6_DIR})
        ENDIF()

        IF(DEFINED ENV{QT5_DIR})
            SET(QT5_DIR $ENV{QT5_DIR})
        ENDIF()

        SET(CMAKE_AUTOMOC OFF)
        FIND_PACKAGE(Qt6 QUIET COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets Qml)
        FIND_PACKAGE(Qt5 QUIET COMPONENTS Core Gui Widgets OpenGL Qml)

        IF(Qt6_FOUND)
            SET_PROPERTY(GLOBAL APPEND PROPERTY VERSE_DEPENDENCIES "QT6")
        ELSEIF(Qt5_FOUND)
            SET_PROPERTY(GLOBAL APPEND PROPERTY VERSE_DEPENDENCIES "QT5")
        ELSE()
            MESSAGE("[osgVerse] Dependency Qt5 and Qt6 not found. Some modules and functionalities will be ignored.")
        ENDIF()
        #########################################

        ADD_SUBDIRECTORY(qt_viewer)
        IF(Qt5Qml_DIR OR Qt6Qml_DIR)
            ADD_SUBDIRECTORY(qml_viewer)
        ENDIF()
    ENDIF()
ENDIF()
ADD_SUBDIRECTORY(scene_editor)
ADD_SUBDIRECTORY(earth_explorer)

IF(OSG_MAJOR_VERSION GREATER 2 AND OSG_MINOR_VERSION GREATER 5)
    ADD_SUBDIRECTORY(sdl_es_viewer)
    ADD_SUBDIRECTORY(osgearth_viewer)
ENDIF()

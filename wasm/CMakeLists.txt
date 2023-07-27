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
        SET(EXTERNAL_LIBRARIES ${EXTERNAL_LIBRARIES} ${OSGPLUGIN_LIBRARIES})
    ENDIF(VERSE_USE_OSG_STATIC)
ENDIF(VERSE_STATIC_BUILD)

SET(WASM_FLAGS_EX "")
IF(VERSE_WASM_USE_PTHREAD)
    SET(WASM_FLAGS_EX "-pthread -s PTHREAD_POOL_SIZE=4")
ENDIF(VERSE_WASM_USE_PTHREAD)

# https://emscripten.org/docs/optimizing/Optimizing-WebGL.html
IF(VERSE_USE_WEBGL2)
    # WebGL2 Browser Compatibility:
    # - IE     : none
    # - Edge   : 79.0
    # - Chrome : 56.0
    # - Firefox: 51.0
    # - Safari : 15.6 (iOS ver 15.2)
    # - Android: 97.0
    STRING(APPEND WASM_FLAGS_EX " -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2")
ELSE()
    # WebGL1 Browser Compatibility:
    # - IE     : 11.0
    # - Edge   : 12.0
    # - Chrome : 8.0
    # - Firefox: 4.0
    # - Safari : 5.1 (iOS ver 8.1)
    # - Android: 97.0
    STRING(APPEND WASM_FLAGS_EX " -s MIN_WEBGL_VERSION=1 -s MAX_WEBGL_VERSION=2")
ENDIF()

SET(WASM_FLAGS0 "--preload-file assets --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/shell.html -gsource-map -fsanitize=address")
SET(WASM_FLAGS1 "-O3 -s ASYNCIFY=1 -s USE_SDL=2 -s FULL_ES2 -s FULL_ES3 -s WASM=1 -s FETCH=1")
SET(WASM_FLAGS2 "-s ASSERTIONS=2 -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_MEMORY=2GB -s MAXIMUM_MEMORY=4GB")

ADD_SUBDIRECTORY(pbr_demo)
ADD_SUBDIRECTORY(script_demo)

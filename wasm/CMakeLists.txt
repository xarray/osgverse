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
    SET(WASM_FLAGS_EX "-pthread -s PTHREAD_POOL_SIZE=15 -s WASM_BIGINT -s TOTAL_MEMORY=1GB -s MAXIMUM_MEMORY=4GB")
ENDIF(VERSE_WASM_USE_PTHREAD)

# https://emscripten.org/docs/optimizing/Optimizing-WebGL.html
IF(VERSE_USE_EXTERNAL_GLES3)
    # WebGL2 Browser Compatibility:
    # - IE     : none
    # - Edge   : 79.0
    # - Chrome : 56.0
    # - Firefox: 51.0
    # - Safari : 15.6 (iOS ver 15.2)
    # - Android: 97.0
    STRING(APPEND WASM_FLAGS_EX " -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 -s FULL_ES3")
    SET(VERSE_WEBGL_NAME "webgl2")
ELSE()
    # WebGL1 Browser Compatibility:
    # - IE     : 11.0
    # - Edge   : 12.0
    # - Chrome : 8.0
    # - Firefox: 4.0
    # - Safari : 5.1 (iOS ver 8.1)
    # - Android: 97.0
    STRING(APPEND WASM_FLAGS_EX " -s MIN_WEBGL_VERSION=1 -s MAX_WEBGL_VERSION=2 -s FULL_ES2")
    SET(VERSE_WEBGL_NAME "webgl")
ENDIF()

CONFIGURE_FILE(shell.html.in "${CMAKE_CURRENT_BINARY_DIR}/shell.html" @ONLY)
SET(WASM_FLAGS0 "--preload-file assets --shell-file ${CMAKE_CURRENT_BINARY_DIR}/shell.html")
SET(WASM_FLAGS1 "-s ASYNCIFY_IMPORTS=[\"emscripten_sleep_using_raf\"] -s USE_SDL=2 -s FETCH=1 -s WASM=1 -s ASYNCIFY=1")

# https://developer.chrome.com/blog/faster-wasm-debugging
IF(VERSE_WASM_OPTIMIZE_SIZE)
    SET(WASM_FLAGS2 "-s ALLOW_MEMORY_GROWTH=1 -Oz")
ELSE(VERSE_WASM_OPTIMIZE_SIZE)
    SET(WASM_FLAGS2 "-fsanitize=address -gsource-map -s ALLOW_MEMORY_GROWTH=1 -s ASYNCIFY_STACK_SIZE=24576 -O0")
ENDIF(VERSE_WASM_OPTIMIZE_SIZE)

ADD_SUBDIRECTORY(pbr_demo)
ADD_SUBDIRECTORY(script_demo)
ADD_SUBDIRECTORY(osgearth_demo)

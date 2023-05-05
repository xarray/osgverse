SET(LIB_NAME osgdb_verse_cesium)
SET(LIBRARY_FILES
    ReaderWriterCesium.cpp
)

#########################################
FIND_PATH(CESIUMNATIVE_INCLUDE_DIR Cesium3DTilesReader/Library.h
    PATHS
    $ENV{OSG_ROOT}/include
    $ENV{OSG_DIR}/include
    /usr/include
    /usr/local/include
)

FIND_PATH(CESIUMNATIVE_LIB_DIR libCesium3DTilesReader.so Cesium3DTilesReader.lib
    PATHS
    $ENV{OSG_ROOT}/${FIND_LIB_POSTFIX}
    $ENV{OSG_DIR}/${FIND_LIB_POSTFIX}
    /usr/lib /usr/${FIND_LIB_POSTFIX}
    /usr/local/lib /usr/local/${FIND_LIB_POSTFIX}
)
#########################################

IF(NOT VERSE_SUPPORT_CPP17)

    MESSAGE("[osgVerse] C++ 17 standard is disabled at present. ${LIB_NAME} will be ignored.")

ELSEIF(CESIUMNATIVE_INCLUDE_DIR AND CESIUMNATIVE_LIB_DIR)

    SET(CMAKE_CXX_STANDARD 17)
    INCLUDE_DIRECTORIES(${CESIUMNATIVE_INCLUDE_DIR})
    LINK_DIRECTORIES(${CESIUMNATIVE_LIB_DIR})

    SET_PROPERTY(GLOBAL APPEND PROPERTY VERSE_PLUGIN_LIBRARIES "${LIB_NAME}")
    IF(VERSE_STATIC_BUILD)
        NEW_LIBRARY(${LIB_NAME} STATIC)
    ELSE()
        NEW_LIBRARY(${LIB_NAME} SHARED)
    ENDIF()

    SET_PROPERTY(TARGET ${LIB_NAME} PROPERTY FOLDER "PLUGINS")
    TARGET_COMPILE_OPTIONS(${LIB_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)
    TARGET_LINK_LIBRARIES(${LIB_NAME} osgVerseDependency async++ draco ktx_read spdlog
                          s2geometry modp_b64 tinyxml2 turbojpeg uriparser webpdecoder
                          CesiumAsync Cesium3DTilesReader Cesium3DTilesWriter CesiumUtility
                          Cesium3DTilesSelection CesiumGeometry CesiumGeospatial CesiumGltf
                          CesiumGltfReader CesiumGltfWriter CesiumJsonReader CesiumJsonWriter)
    LINK_OSG_LIBRARY(${LIB_NAME} OpenThreads osg osgDB osgUtil)

    IF(WIN32)
        # FIXME: really ignore these errors?
        TARGET_LINK_OPTIONS(${LIB_NAME} PRIVATE "/FORCE:MULTIPLE")
    ENDIF(WIN32)
    INSTALL(TARGETS ${LIB_NAME} EXPORT ${LIB_NAME}
            RUNTIME DESTINATION ${INSTALL_BINDIR} COMPONENT libosgverse
            LIBRARY DESTINATION ${INSTALL_LIBDIR} COMPONENT libosgverse
            ARCHIVE DESTINATION ${INSTALL_ARCHIVEDIR} COMPONENT libosgverse-dev)
    
ELSE()
    
    MESSAGE("[osgVerse] Dependency cesium-native not found. ${LIB_NAME} will be ignored.")
    
ENDIF()
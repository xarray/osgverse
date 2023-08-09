SET(LIB_NAME osgdb_verse_vdb)
SET(LIBRARY_FILES
    ReaderWriterVDB.cpp
)

#########################################
FIND_PATH(OPENVDB_INCLUDE_DIR openvdb/openvdb.h
    PATHS ${VERSE_3RDPARTY_PATH}/include
    /usr/include
    /usr/local/include
)

FIND_PATH(OPENVDB_BOOST_INCLUDE_DIR boost/type.hpp
    PATHS ${VERSE_3RDPARTY_PATH}/include
    /usr/include
    /usr/local/include
)

FIND_PATH(OPENVDB_TBB_INCLUDE_DIR tbb/blocked_range.h
    PATHS ${VERSE_3RDPARTY_PATH}/lib
    /usr/include
    /usr/local/include
)

FIND_PATH(OPENVDB_LIB_DIR libopenvdb.so openvdb.lib
    PATHS ${VERSE_3RDPARTY_PATH}/lib
    /usr/lib /usr/${FIND_LIB_POSTFIX}
    /usr/local/lib /usr/local/${FIND_LIB_POSTFIX}
)

FIND_PATH(OPENVDB_TBB_LIB_DIR libtbb.so tbb.lib
    PATHS ${VERSE_3RDPARTY_PATH}/lib
    /usr/lib /usr/${FIND_LIB_POSTFIX}
    /usr/local/lib /usr/local/${FIND_LIB_POSTFIX}
)
#########################################

IF(NOT VERSE_SUPPORT_CPP17)

    MESSAGE("[osgVerse] C++ 17 standard is disabled at present. ${LIB_NAME} will be ignored.")

ELSEIF(OPENVDB_INCLUDE_DIR AND OPENVDB_LIB_DIR)

    SET(CMAKE_CXX_STANDARD 17)
    INCLUDE_DIRECTORIES(${OPENVDB_INCLUDE_DIR}
                        ${OPENVDB_BOOST_INCLUDE_DIR} ${OPENVDB_TBB_INCLUDE_DIR})
    LINK_DIRECTORIES(${OPENVDB_LIB_DIR} ${OPENVDB_TBB_LIB_DIR})

    SET_PROPERTY(GLOBAL APPEND PROPERTY VERSE_PLUGIN_LIBRARIES "${LIB_NAME}")
    IF(VERSE_STATIC_BUILD)
        NEW_LIBRARY(${LIB_NAME} STATIC)
    ELSE()
        NEW_LIBRARY(${LIB_NAME} SHARED)
    ENDIF()

    SET_PROPERTY(TARGET ${LIB_NAME} PROPERTY FOLDER "PLUGINS")
    TARGET_COMPILE_OPTIONS(${LIB_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)
    TARGET_LINK_LIBRARIES(${LIB_NAME} osgVerseDependency osgVerseReaderWriter openvdb)
    LINK_OSG_LIBRARY(${LIB_NAME} OpenThreads osg osgDB osgUtil)

    INSTALL(TARGETS ${LIB_NAME} EXPORT ${LIB_NAME}
            RUNTIME DESTINATION ${INSTALL_BINDIR} COMPONENT libosgverse
            LIBRARY DESTINATION ${INSTALL_LIBDIR} COMPONENT libosgverse
            ARCHIVE DESTINATION ${INSTALL_ARCHIVEDIR} COMPONENT libosgverse-dev)

ELSE()

    MESSAGE("[osgVerse] Dependency OpenVDB not found. ${LIB_NAME} will be ignored.")

ENDIF()

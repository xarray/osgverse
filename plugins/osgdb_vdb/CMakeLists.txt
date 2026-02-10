SET(LIB_NAME osgdb_verse_vdb)
SET(LIBRARY_FILES
    ReaderWriterVDB.cpp
)

#########################################
FIND_PATH(OPENVDB_INCLUDE_DIR openvdb/openvdb.h
    PATHS ${THIRDPARTY_ROOT}/include
    /usr/include /usr/local/include
)

FIND_PATH(OPENVDB_BOOST_INCLUDE_DIR boost/type.hpp
    PATHS ${THIRDPARTY_ROOT}/include
    /usr/include /usr/local/include
)

FIND_PATH(OPENVDB_TBB_INCLUDE_DIR tbb/blocked_range.h
    PATHS ${THIRDPARTY_ROOT}/lib
    /usr/include /usr/local/include
)

FIND_PATH(OPENVDB_LIB_DIR libopenvdb.so openvdb.lib
    PATHS ${THIRDPARTY_ROOT}/lib
    /usr/lib /usr/lib/${FIND_LIB_POSTFIX} /usr/lib/${FIND_SUBLIB_POSTFIX}
    /usr/local/lib /usr/local/lib/${FIND_LIB_POSTFIX}
)

FIND_PATH(OPENVDB_TBB_LIB_DIR libtbb.so tbb.lib
    PATHS ${THIRDPARTY_ROOT}/lib
    /usr/lib /usr/lib/${FIND_LIB_POSTFIX} /usr/lib/${FIND_SUBLIB_POSTFIX}
    /usr/local/lib /usr/local/lib/${FIND_LIB_POSTFIX}
)
#########################################

IF(NOT VERSE_SUPPORT_CPP17)

    MESSAGE("[osgVerse] C++ 17 standard is disabled at present. ${LIB_NAME} will be ignored.")

ELSEIF(OPENVDB_BOOST_INCLUDE_DIR AND OPENVDB_INCLUDE_DIR AND OPENVDB_TBB_INCLUDE_DIR
       AND OPENVDB_LIB_DIR AND OPENVDB_TBB_LIB_DIR)

    INCLUDE_DIRECTORIES(${OPENVDB_BOOST_INCLUDE_DIR} ${OPENVDB_INCLUDE_DIR} ${OPENVDB_TBB_INCLUDE_DIR})
    LINK_DIRECTORIES(${OPENVDB_LIB_DIR} ${OPENVDB_TBB_LIB_DIR})

    SET_PROPERTY(GLOBAL APPEND PROPERTY VERSE_PLUGIN_LIBRARIES "${LIB_NAME}")
    SET_PROPERTY(GLOBAL APPEND PROPERTY VERSE_DEPENDENCIES "OPENVDB")
    IF(VERSE_STATIC_BUILD)
        NEW_PLUGIN(${LIB_NAME} STATIC)
    ELSE()
        NEW_PLUGIN(${LIB_NAME} SHARED)
    ENDIF()

    SET_PROPERTY(TARGET ${LIB_NAME} PROPERTY FOLDER "PLUGINS")
    TARGET_COMPILE_OPTIONS(${LIB_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)
    TARGET_LINK_LIBRARIES(${LIB_NAME} osgVerseDependency osgVerseAnimation openvdb)
    LINK_OSG_LIBRARY(${LIB_NAME} OpenThreads osg osgDB osgUtil osgText)

    INSTALL(TARGETS ${LIB_NAME} EXPORT ${LIB_NAME}
            RUNTIME DESTINATION ${INSTALL_PLUGINDIR} COMPONENT libosgverse
            LIBRARY DESTINATION ${INSTALL_LIBDIR} COMPONENT libosgverse
            ARCHIVE DESTINATION ${INSTALL_ARCHIVEDIR} COMPONENT libosgverse-dev)
    IF(NOT VERSE_STATIC_BUILD)
        IF(MSVC AND VERSE_INSTALL_PDB_FILES)
            INSTALL(FILES $<TARGET_PDB_FILE:${LIB_NAME}> DESTINATION ${INSTALL_PLUGINDIR} OPTIONAL)
        ENDIF()
    ENDIF()

ELSE()

    MESSAGE("[osgVerse] Dependency OpenVDB not found. ${LIB_NAME} will be ignored.")

ENDIF()

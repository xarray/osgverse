SET(LIB_NAME osgdb_verse_ept)
SET(LIBRARY_FILES
    ReaderWriterEPT.cpp PointCloudFileReader.cpp
)

ADD_DEFINITIONS(-DLASZIPDLL_EXPORTS)
SET_PROPERTY(GLOBAL APPEND PROPERTY VERSE_PLUGIN_LIBRARIES "${LIB_NAME}")
IF(VERSE_STATIC_BUILD)
    NEW_LIBRARY(${LIB_NAME} STATIC)
ELSE()
    NEW_LIBRARY(${LIB_NAME} SHARED)
ENDIF()

SET_PROPERTY(TARGET ${LIB_NAME} PROPERTY FOLDER "PLUGINS")
TARGET_COMPILE_OPTIONS(${LIB_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)
TARGET_LINK_LIBRARIES(${LIB_NAME} osgVerseDependency osgVerseReaderWriter)
LINK_OSG_LIBRARY(${LIB_NAME} OpenThreads osg osgDB osgUtil)

INSTALL(TARGETS ${LIB_NAME} EXPORT ${LIB_NAME}
        RUNTIME DESTINATION ${INSTALL_BINDIR} COMPONENT libosgverse
        LIBRARY DESTINATION ${INSTALL_LIBDIR} COMPONENT libosgverse
        ARCHIVE DESTINATION ${INSTALL_ARCHIVEDIR} COMPONENT libosgverse-dev)

SET(LIB_NAME osgVerseModeling)
SET(LIBRARY_INCLUDE_FILES
    MeshDeformer.h MeshTopology.h GeometryMerger.h GeometryMapper.h
    FFDModeler.h DynamicGeometry.h Math.h Octree.h Utilities.h)
SET(LIBRARY_FILES ${LIBRARY_INCLUDE_FILES}
    MeshDeformer.cpp MeshTopology.cpp FFDModeler.cpp Math.cpp
    GeometryMerger.cpp GeometryMapper.cpp DynamicGeometry.cpp
    Utilities.cpp UtilitiesEx.cpp
)

ADD_DEFINITIONS(-DUSINGZ)
NEW_LIBRARY(${LIB_NAME} STATIC)
TARGET_LINK_LIBRARIES(${LIB_NAME} osgVerseDependency)
LINK_OSG_LIBRARY(${LIB_NAME} OpenThreads osg osgDB osgUtil)
TARGET_COMPILE_OPTIONS(${LIB_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)

INSTALL(TARGETS ${LIB_NAME} EXPORT ${LIB_NAME}
        RUNTIME DESTINATION ${INSTALL_BINDIR} COMPONENT libosgverse
        LIBRARY DESTINATION ${INSTALL_LIBDIR} COMPONENT libosgverse
        ARCHIVE DESTINATION ${INSTALL_ARCHIVEDIR} COMPONENT libosgverse-dev)
INSTALL(FILES ${LIBRARY_INCLUDE_FILES} DESTINATION ${INSTALL_INCDIR}/osgVerse/modeling COMPONENT libosgverse-dev)

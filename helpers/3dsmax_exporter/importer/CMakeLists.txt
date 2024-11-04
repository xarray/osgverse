SET(LIB_NAME osgVerseMaxImporter)
SET(HEADERS
    importer.h utilities.h
    MaxConstructor.h ImportImplementor.h)
SET(SOURCES
    DllEntry.cpp
    SceneImportOSG.cpp
    MaxConstructor.cpp
    ImportImplementor.cpp
    Import_Geometry.cpp
    Import_ShapeDrawable.cpp
    Import_StateAttribute.cpp
    Import_TextureStateAttribute.cpp)
SET (RESOURCES importer.rc importer.def)

# Set the warning level back to 3, because the max sdk is dirty
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /GR")
REMOVE_DEFINITIONS(-DUNICODE -D_UNICODE -D_MBCS)
INCLUDE_DIRECTORIES(.. ${MAXSDK_INCLUDE_DIR})

ADD_LIBRARY(${LIB_NAME} SHARED ${HEADERS} ${SOURCES} ${RESOURCES})
SET_PROPERTY(TARGET ${LIB_NAME} PROPERTY FOLDER "EXPORTERS")
TARGET_COMPILE_OPTIONS(${LIB_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)
TARGET_LINK_LIBRARIES(${LIB_NAME} ${MAXSDK_LIBRARY} ${MAXSDK_core_LIBRARY} ${MAXSDK_geom_LIBRARY}
                      ${MAXSDK_mesh_LIBRARY} ${MAXSDK_maxutil_LIBRARY})
LINK_OSG_LIBRARY(${LIB_NAME} OpenThreads osg osgDB osgUtil)

# Set the suffix to 'dli' means that it's an importer plugin
SET_TARGET_PROPERTIES(${LIB_NAME} PROPERTIES SUFFIX ".dli")
SET_TARGET_PROPERTIES(${LIB_NAME} PROPERTIES CLEAN_DIRECT_OUTPUT 1)
INSTALL(TARGETS ${LIB_NAME} EXPORT ${LIB_NAME}
          RUNTIME DESTINATION ${INSTALL_BINDIR} COMPONENT libosgverse
          LIBRARY DESTINATION ${INSTALL_LIBDIR} COMPONENT libosgverse
          ARCHIVE DESTINATION ${INSTALL_ARCHIVEDIR} COMPONENT libosgverse-dev)

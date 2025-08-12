SET(EXE_NAME osgVerse_EarthExplorer)
SET(EXECUTABLE_FILES
    earth_main.cpp ui.cpp render_effects.cpp
    particle_data.cpp volume_data.cpp
    city_data.cpp internal.cpp ocean.cpp
    capture_callback.h
)

FIND_DEPENDENCE(NV_ENCODER "nvEncodeAPI.h" "libnvidia-encode.so;libnvencodeapi.so;nvencodeapi.lib" "")
IF(NV_ENCODER_FOUND AND CUDA_FOUND)
    ADD_DEFINITIONS(-DNV_ENCODER)
    INCLUDE_DIRECTORIES(${CUDA_TOOLKIT_INCLUDE})
ENDIF()

NEW_EXECUTABLE(${EXE_NAME} SHARED)
SET_PROPERTY(TARGET ${EXE_NAME} PROPERTY FOLDER "APPLICATIONS")
TARGET_LINK_LIBRARIES(${EXE_NAME} osgVerseReaderWriter osgVerseUI osgVersePipeline
                      osgVerseAnimation osgVerseWrappers osgVerseDependency)
IF(NV_ENCODER_FOUND)
    TARGET_LINK_LIBRARIES(${EXE_NAME} nvencodeapi)
ENDIF()
LINK_OSG_LIBRARY(${EXE_NAME} OpenThreads osg osgDB osgUtil osgGA osgText osgSim osgTerrain osgViewer)
TARGET_COMPILE_OPTIONS(${EXE_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)

IF(MSVC AND VERSE_INSTALL_PDB_FILES)
    INSTALL(FILES $<TARGET_PDB_FILE:${EXE_NAME}> DESTINATION ${INSTALL_BINDIR} OPTIONAL)
ENDIF()

SET(CMAKE_EXECUTABLE_SUFFIX ".html")
SET(EXE_NAME osgVerse_EarthWASM)
SET(EXECUTABLE_FILES
    osgearth_viewer.h osgearth_viewer.cpp
)

#########################################
IF(USE_WASM_OSGEARTH)
    ADD_DEFINITIONS(-DOSG_LIBRARY_STATIC -DOSGEARTH_LIBRARY_STATIC)
    INCLUDE_DIRECTORIES(${THIRDPARTY_ROOT}/include)
    LINK_DIRECTORIES(${THIRDPARTY_ROOT}/lib)
    SET(EXTERNAL_LIBRARIES ${EXTERNAL_LIBRARIES}
        expat proj iconv tiff tiffxx geotiff sharpyuv spatialite sqlite3 webp gdal geos geos_c
        osgEarth osgEarthUtil osgEarthSymbology osgdb_earth osgdb_osgearth_xyz osgdb_osgearth_engine_mp)

    SET(WASM_FLAGS2 "-s ALLOW_MEMORY_GROWTH=1")
    FILE(COPY "${CMAKE_SOURCE_DIR}/assets/shaders" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/assets")
    NEW_EXECUTABLE(${EXE_NAME} SHARED)
    SET_TARGET_PROPERTIES(${EXE_NAME} PROPERTIES LINK_FLAGS "${WASM_FLAGS0} ${WASM_FLAGS1} ${WASM_FLAGS2} ${WASM_FLAGS_EX}")

    SET_PROPERTY(TARGET ${EXE_NAME} PROPERTY FOLDER "APPLICATIONS")
    TARGET_LINK_LIBRARIES(${EXE_NAME} osgVerseDependency osgVerseAnimation osgVerseReaderWriter osgVersePipeline osgVerseScript)
    LINK_OSG_LIBRARY(${EXE_NAME} OpenThreads osg osgDB osgUtil osgGA osgText osgSim osgTerrain osgShadow osgViewer)
    TARGET_COMPILE_OPTIONS(${EXE_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)

ELSE(USE_WASM_OSGEARTH)

    MESSAGE("[osgVerse] Dependency osgEarth not found. ${EXE_NAME} will be ignored.")

ENDIF(USE_WASM_OSGEARTH)

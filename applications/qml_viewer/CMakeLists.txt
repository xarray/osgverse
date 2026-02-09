SET(EXE_NAME osgVerse_Viewer_Qml)
SET(EXECUTABLE_FILES
    viewer_main.cpp
)

# You need to copy <Qt>/<5.15.2>/<msvc2019_64>/<qml> to working folder, and set system path to Qt bin
IF(Qt6_FOUND)

    FIND_PACKAGE(Qt6 COMPONENTS Qml Quick)
    ADD_DEFINITIONS(-DUSE_QT6)
    QT6_ADD_RESOURCES(QT_RCC_SOURCES qt_interface.qrc)
    QT6_WRAP_CPP(QT_MOC_HPP qt_header.h)
    SET(EXECUTABLE_FILES ${EXECUTABLE_FILES}
        qt_header.h qt_events.cpp ${QT_MOC_HPP} ${QT_RCC_SOURCES} )

    NEW_EXECUTABLE(${EXE_NAME} SHARED)
    USE_MSVC_DEBUGGER(${EXE_NAME} "${THIRDPARTY_ROOT}/bin;${OSG_BUILD_INCLUDE_DIR}/../bin;$<TARGET_FILE_DIR:Qt6::Core>")
    SET_PROPERTY(TARGET ${EXE_NAME} PROPERTY FOLDER "APPLICATIONS")

    TARGET_LINK_LIBRARIES(${EXE_NAME} osgVerseDependency osgVerseReaderWriter osgVersePipeline
                          Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Qml Qt6::Quick)
    LINK_OSG_LIBRARY(${EXE_NAME} OpenThreads osg osgDB osgUtil osgGA osgText osgSim osgTerrain osgViewer)
    TARGET_COMPILE_OPTIONS(${EXE_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)

    IF(MSVC AND VERSE_INSTALL_PDB_FILES)
        INSTALL(FILES $<TARGET_PDB_FILE:${EXE_NAME}> DESTINATION ${INSTALL_BINDIR} OPTIONAL)
    ENDIF()

ELSEIF(Qt5_FOUND)

    FIND_PACKAGE(Qt5 COMPONENTS Qml Quick)
    ADD_DEFINITIONS(-DUSE_QT5)
    QT5_ADD_RESOURCES(QT_RCC_SOURCES qt_interface.qrc)
    QT5_WRAP_CPP(QT_MOC_HPP qt_header.h)
    SET(EXECUTABLE_FILES ${EXECUTABLE_FILES}
        qt_header.h qt_events.cpp ${QT_MOC_HPP} ${QT_RCC_SOURCES} )

    NEW_EXECUTABLE(${EXE_NAME} SHARED)
    USE_MSVC_DEBUGGER(${EXE_NAME} "${THIRDPARTY_ROOT}/bin;${OSG_BUILD_INCLUDE_DIR}/../bin;$<TARGET_FILE_DIR:Qt5::Core>")
    SET_PROPERTY(TARGET ${EXE_NAME} PROPERTY FOLDER "APPLICATIONS")

    TARGET_LINK_LIBRARIES(${EXE_NAME} osgVerseDependency osgVerseReaderWriter osgVersePipeline
                          Qt5::Core Qt5::Gui Qt5::Widgets Qt5::OpenGL Qt5::Qml Qt5::Quick)
    LINK_OSG_LIBRARY(${EXE_NAME} OpenThreads osg osgDB osgUtil osgGA osgText osgSim osgTerrain osgViewer)
    TARGET_COMPILE_OPTIONS(${EXE_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)

    IF(MSVC AND VERSE_INSTALL_PDB_FILES)
        INSTALL(FILES $<TARGET_PDB_FILE:${EXE_NAME}> DESTINATION ${INSTALL_BINDIR} OPTIONAL)
    ENDIF()

ELSE()

    MESSAGE("[osgVerse] Dependency Qt5 and Qt6 not found. ${EXE_NAME} will be ignored.")

ENDIF()

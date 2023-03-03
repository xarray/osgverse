SET(EXE_NAME osgVerse_QtViewer)
SET(EXECUTABLE_FILES
    viewer_main.cpp
)

#########################################
SET(CMAKE_AUTOMOC OFF)
IF(DEFINED ENV{QT5_DIR})
  SET(QT5_DIR $ENV{QT5_DIR})
ENDIF()

FIND_PACKAGE(Qt5 COMPONENTS Core Gui Widgets OpenGL)
#########################################

IF(Qt5_FOUND)

    #QT5_ADD_RESOURCES(QT_RCC_SOURCES ...)
    QT5_WRAP_CPP(QT_MOC_HPP qt_header.h)
    SET(EXECUTABLE_FILES ${EXECUTABLE_FILES}
        qt_header.h qt_widget.cpp ${QT_MOC_HPP} ${QT_RCC_SOURCES}
    )

    NEW_EXECUTABLE(${EXE_NAME} SHARED)
    SET_PROPERTY(TARGET ${EXE_NAME} PROPERTY FOLDER "APPLICATIONS")
    TARGET_LINK_LIBRARIES(${EXE_NAME} osgVerseDependency osgVerseReaderWriter osgVersePipeline
                          Qt5::Core Qt5::Gui Qt5::Widgets Qt5::OpenGL)
    LINK_OSG_LIBRARY(${EXE_NAME} OpenThreads osg osgDB osgUtil osgGA osgText osgSim osgTerrain osgViewer)
    TARGET_COMPILE_OPTIONS(${EXE_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)
    
ELSE(Qt5_FOUND)
    
    MESSAGE("[osgVerse] Dependency Qt5 not found. ${EXE_NAME} will be ignored.")
    
ENDIF(Qt5_FOUND)
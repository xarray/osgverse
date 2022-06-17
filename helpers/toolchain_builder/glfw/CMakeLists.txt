SET(LIBRARY_NAME glfw)
SET(GLFW_HEADERS include/GLFW/glfw3.h include/GLFW/glfw3native.h)
SET(GLFW_SOURCES src/context.c src/init.c src/input.c src/monitor.c src/vulkan.c src/window.c)

IF(WIN32)
    SET(GLFW_HEADERS ${GLFW_HEADERS}
        src/win32_platform.h src/win32_joystick.h src/wgl_context.h
        src/egl_context.h src/osmesa_context.h)
    SET(GLFW_SOURCES ${GLFW_SOURCES}
        src/win32_init.c src/win32_joystick.c src/win32_monitor.c src/win32_time.c
        src/win32_thread.c src/win32_window.c src/wgl_context.c
        src/egl_context.c src/osmesa_context.c)
    SET(_GLFW_WIN32 1)
    SET(_GLFW_USE_HYBRID_HPG 1)
ELSEIF(APPLE)
    SET(GLFW_HEADERS ${GLFW_HEADERS}
        src/cocoa_platform.h src/cocoa_joystick.h src/posix_thread.h
        src/nsgl_context.h src/egl_context.h src/osmesa_context.h)
    SET(GLFW_SOURCES ${GLFW_SOURCES}
        src/cocoa_init.m src/cocoa_joystick.m src/cocoa_monitor.m
        src/cocoa_window.m src/cocoa_time.c src/posix_thread.c
        src/nsgl_context.m src/egl_context.c src/osmesa_context.c)
    IF("${CMAKE_VERSION}" VERSION_LESS "3.16")
        SET(GLFW_SOURCES ${GLFW_SOURCES}
            src/cocoa_init.m src/cocoa_joystick.m src/cocoa_monitor.m
            src/cocoa_window.m src/nsgl_context.m)
    ENDIF()
    SET(_GLFW_COCOA 1)
ELSEIF(UNIX)
    FIND_PACKAGE(X11 REQUIRED)
    LIST(APPEND GLFW_INCLUDE_DIRS "${X11_X11_INCLUDE_PATH}")
    IF(X11_Xrandr_INCLUDE_PATH AND X11_Xinerama_INCLUDE_PATH AND X11_Xkb_INCLUDE_PATH AND
       X11_Xcursor_INCLUDE_PATH AND X11_Xi_INCLUDE_PATH AND X11_Xshape_INCLUDE_PATH)
    ELSE()
        MESSAGE(FATAL_ERROR "X11 headers missing, must install libxrandr/libxinerama/libxcursor/libxi/libxext")
    ENDIF()
    
    SET(GLFW_HEADERS ${GLFW_HEADERS}
        src/x11_platform.h src/xkb_unicode.h src/posix_time.h src/linux_joystick.h
        src/posix_thread.h src/glx_context.h src/egl_context.h src/osmesa_context.h)
    SET(GLFW_SOURCES ${GLFW_SOURCES}
        src/x11_init.c src/x11_monitor.c src/x11_window.c src/xkb_unicode.c src/posix_time.c
        src/posix_thread.c src/glx_context.c src/egl_context.c src/osmesa_context.c src/linux_joystick.c)
    SET(_GLFW_X11 1)
ENDIF()
CONFIGURE_FILE(glfw_config.h.in "${CMAKE_BINARY_DIR}/glfw_config.h" @ONLY)

INCLUDE_DIRECTORIES(. ${CMAKE_BINARY_DIR} ${GLFW_INCLUDE_DIRS})
ADD_DEFINITIONS(-D_GLFW_USE_CONFIG_H -DUNICODE -D_UNICODE)
ADD_LIBRARY(${LIBRARY_NAME} STATIC ${GLFW_SOURCES} ${GLFW_HEADERS})
TARGET_LINK_LIBRARIES(${LIBRARY_NAME} ${GLFW_LIBRARIES})

SET(LIBRARY_NAME glfw)
SET(GLFW_SOURCE_FILES
    glfw3.h glfw3native.h internal.h platform.h
    context.c init.c input.c monitor.c platform.c vulkan.c window.c
    egl_context.c osmesa_context.c null_platform.h null_joystick.h
    null_init.c null_monitor.c null_window.c null_joystick.c
)

IF(ANDROID OR IOS)
    # Ignore GLFW?
ELSEIF(APPLE)
    SET(GLFW_SOURCE_FILES ${GLFW_SOURCE_FILES} cocoa_time.h cocoa_time.c posix_thread.h
        posix_module.c posix_thread.c cocoa_platform.h cocoa_joystick.h cocoa_init.m
        cocoa_joystick.m cocoa_monitor.m cocoa_window.m nsgl_context.m)
    ADD_DEFINITIONS(-D_GLFW_COCOA)
ELSEIF(WIN32)
    SET(GLFW_SOURCE_FILES ${GLFW_SOURCE_FILES} win32_time.h win32_thread.h win32_module.c
        win32_time.c win32_thread.c win32_platform.h win32_joystick.h win32_init.c
        win32_joystick.c win32_monitor.c win32_window.c wgl_context.c)
    ADD_DEFINITIONS(-D_GLFW_WIN32)
ELSE()
    SET(GLFW_SOURCE_FILES ${GLFW_SOURCE_FILES}
        posix_time.h posix_thread.h posix_module.c posix_time.c posix_thread.c)
    IF(X11_FOUND)
        SET(GLFW_SOURCE_FILES ${GLFW_SOURCE_FILES} x11_platform.h xkb_unicode.h glx_context.c
            x11_init.c x11_monitor.c x11_window.c xkb_unicode.c posix_poll.h posix_poll.c)
        ADD_DEFINITIONS(-D_GLFW_X11)
    ELSEIF(WAYLAND_FOUND)
        SET(GLFW_SOURCE_FILES ${GLFW_SOURCE_FILES} wl_platform.h xkb_unicode.h wl_init.c
            wl_monitor.c wl_window.c xkb_unicode.c posix_poll.h posix_poll.c)
        ADD_DEFINITIONS(-D_GLFW_WAYLAND)
    ENDIF()
ENDIF()

INCLUDE_DIRECTORIES(BEFORE .)
ADD_LIBRARY(${LIBRARY_NAME} STATIC ${GLFW_SOURCE_FILES})
SET_PROPERTY(TARGET ${LIBRARY_NAME} PROPERTY FOLDER "THIRD_PARTY")

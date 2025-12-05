MACRO(COLLECT_GLFW_FILES DIR_PREFIX SRC_VAR DEF_VAR)

    SET(GLFW_SOURCE_FILES
        ${DIR_PREFIX}/glfw3.h ${DIR_PREFIX}/glfw3native.h ${DIR_PREFIX}/internal.h ${DIR_PREFIX}/platform.h
        ${DIR_PREFIX}/context.c ${DIR_PREFIX}/init.c ${DIR_PREFIX}/input.c ${DIR_PREFIX}/monitor.c
        ${DIR_PREFIX}/platform.c ${DIR_PREFIX}/vulkan.c ${DIR_PREFIX}/window.c ${DIR_PREFIX}/egl_context.c
        ${DIR_PREFIX}/osmesa_context.c ${DIR_PREFIX}/null_platform.h ${DIR_PREFIX}/null_joystick.h
        ${DIR_PREFIX}/null_init.c ${DIR_PREFIX}/null_monitor.c ${DIR_PREFIX}/null_window.c ${DIR_PREFIX}/null_joystick.c
    )

    SET(GLFW_DEFINITIONS )
    IF(ANDROID OR IOS)
        # Ignore GLFW?
    ELSEIF(APPLE)
        SET(GLFW_SOURCE_FILES ${GLFW_SOURCE_FILES} ${DIR_PREFIX}/cocoa_time.h ${DIR_PREFIX}/cocoa_time.c
            ${DIR_PREFIX}/posix_thread.h ${DIR_PREFIX}/posix_module.c ${DIR_PREFIX}/posix_thread.c ${DIR_PREFIX}/cocoa_platform.h
            ${DIR_PREFIX}/cocoa_joystick.h ${DIR_PREFIX}/cocoa_init.m ${DIR_PREFIX}/cocoa_joystick.m ${DIR_PREFIX}/cocoa_monitor.m
            ${DIR_PREFIX}/cocoa_window.m ${DIR_PREFIX}/nsgl_context.m)
        SET(GLFW_DEFINITIONS -D_GLFW_COCOA)
    ELSEIF(WIN32)
        SET(GLFW_SOURCE_FILES ${GLFW_SOURCE_FILES} ${DIR_PREFIX}/win32_time.h ${DIR_PREFIX}/win32_thread.h ${DIR_PREFIX}/win32_module.c
            ${DIR_PREFIX}/win32_time.c ${DIR_PREFIX}/win32_thread.c ${DIR_PREFIX}/win32_platform.h ${DIR_PREFIX}/win32_joystick.h
            ${DIR_PREFIX}/win32_init.c ${DIR_PREFIX}/win32_joystick.c ${DIR_PREFIX}/win32_monitor.c
            ${DIR_PREFIX}/win32_window.c ${DIR_PREFIX}/wgl_context.c)
        SET(GLFW_DEFINITIONS -D_GLFW_WIN32)
    ELSE()
        SET(GLFW_SOURCE_FILES ${GLFW_SOURCE_FILES} ${DIR_PREFIX}/posix_time.h ${DIR_PREFIX}/posix_thread.h
            ${DIR_PREFIX}/posix_module.c ${DIR_PREFIX}/posix_time.c ${DIR_PREFIX}/posix_thread.c)
        IF(X11_FOUND)
            SET(GLFW_SOURCE_FILES ${GLFW_SOURCE_FILES} ${DIR_PREFIX}/x11_platform.h ${DIR_PREFIX}/xkb_unicode.h
                ${DIR_PREFIX}/x11_init.c ${DIR_PREFIX}/x11_monitor.c ${DIR_PREFIX}/x11_window.c ${DIR_PREFIX}/xkb_unicode.c
                ${DIR_PREFIX}/posix_poll.h ${DIR_PREFIX}/posix_poll.c ${DIR_PREFIX}/glx_context.c)
            SET(GLFW_DEFINITIONS -D_GLFW_X11)
        ELSEIF(WAYLAND_FOUND)
            SET(GLFW_SOURCE_FILES ${GLFW_SOURCE_FILES} ${DIR_PREFIX}/wl_platform.h ${DIR_PREFIX}/xkb_unicode.h
                ${DIR_PREFIX}/wl_init.c ${DIR_PREFIX}/wl_monitor.c ${DIR_PREFIX}/wl_window.c ${DIR_PREFIX}/xkb_unicode.c
                ${DIR_PREFIX}/posix_poll.h ${DIR_PREFIX}/posix_poll.c)
            SET(GLFW_DEFINITIONS -D_GLFW_WAYLAND)
        ENDIF()
    ENDIF()

    SET(${SRC_VAR} ${GLFW_SOURCE_FILES} PARENT_SCOPE)
    SET(${DEF_VAR} ${GLFW_DEFINITIONS} PARENT_SCOPE)

ENDMACRO(COLLECT_GLFW_FILES)

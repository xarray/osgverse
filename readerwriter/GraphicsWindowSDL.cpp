#include "GraphicsWindowSDL.h"
#include <osg/DeleteHandler>
#if VERSE_WITH_SDL == 2
#   include <SDL.h>
#   include <SDL_syswm.h>
#else
#   include <SDL3/SDL.h>
#   include <SDL3/SDL_properties.h>
#endif
#include <iostream>

#if defined(SDL_VIDEO_DRIVER_COCOA)
extern "C" void* getViewFromWindow(NSWindow* window);
#endif

#if defined(OSG_GLES1_AVAILABLE) || defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
#   if !defined(VERSE_EMBEDDED) && !defined(__ANDROID__)
#       include <EGL/egl.h>
#       include <EGL/eglext.h>
#       include <EGL/eglext_angle.h>
#       define VERSE_GLES_DESKTOP 1
#       if VERSE_WITH_VULKAN
#           include <SDL_vulkan.h>
#           include "VulkanExtension.h"
#       endif
// EGL_VERSE_vulkan_objects
typedef void* (EGLAPIENTRYP PFNEGLGETVKOBJECTSVERSEPROC)(EGLDisplay dpy, EGLSurface surface);
#   endif
#endif

using namespace osgVerse;
#if VERSE_WITH_SDL == 2
#   include <SDL_syswm.h>
#   define SDL_EVENT_MOUSE_MOTION SDL_MOUSEMOTION
#   define SDL_EVENT_MOUSE_BUTTON_DOWN SDL_MOUSEBUTTONDOWN
#   define SDL_EVENT_MOUSE_BUTTON_UP SDL_MOUSEBUTTONUP
#   define SDL_EVENT_MOUSE_WHEEL SDL_MOUSEWHEEL
#   define SDL_EVENT_KEY_UP SDL_KEYUP
#   define SDL_EVENT_KEY_DOWN SDL_KEYDOWN
#   define SDL_EVENT_QUIT SDL_QUIT
#   define SDL_KEY_SYMBOL(ev) (ev).key.keysym.sym
#   define SDL_KEY_MODIFIER(ev) (ev).key.keysym.mod
#   define SDL_CREATE_WINDOW(H, title, x, y, w, h, fl) \
        { H = SDL_CreateWindow((title), (x), (y), (w), (h), (fl)); }
#   define SDL_GET_DISPLAYS(c) c = SDL_GetNumVideoDisplays();
#else
#   define SDL_KEY_SYMBOL(ev) (ev).key.key
#   define SDL_KEY_MODIFIER(ev) (ev).key.mod
#   define SDL_CREATE_WINDOW(H, title, x, y, w, h, fl) \
        { H = SDL_CreateWindow((title), (w), (h), (fl)); SDL_SetWindowPosition(H, (x), (y)); }
#   define SDL_GET_DISPLAYS(c) SDL_GetDisplays(&c);
#endif

#if defined(VERSE_GLES_DESKTOP)
static void EGLAPIENTRY eglErrorCallback(EGLenum error, const char* command, EGLint messageType,
                                         EGLLabelKHR threadLabel, EGLLabelKHR objectLabel, const char* msg)
{
    OSG_NOTICE << "[eglErrorCallback] Get message: " << msg << ", command = " << command << std::endl;
}
#endif

namespace
{
    static int getModKey()
    {
        SDL_Keymod modstates = SDL_GetModState();
#if VERSE_WITH_SDL == 2
        if (modstates & KMOD_LCTRL) return osgGA::GUIEventAdapter::KEY_Control_L;
        else if (modstates & KMOD_RCTRL) return osgGA::GUIEventAdapter::KEY_Control_R;
        else if (modstates & KMOD_LALT) return osgGA::GUIEventAdapter::KEY_Alt_L;
        else if (modstates & KMOD_RALT) return osgGA::GUIEventAdapter::KEY_Alt_R;
        else if (modstates & KMOD_LSHIFT) return osgGA::GUIEventAdapter::KEY_Shift_L;
        else if (modstates & KMOD_RSHIFT) return osgGA::GUIEventAdapter::KEY_Shift_R;
        else if (modstates & KMOD_CAPS) return osgGA::GUIEventAdapter::KEY_Caps_Lock;
        else if (modstates & KMOD_NUM) return osgGA::GUIEventAdapter::KEY_Num_Lock;
#else
        if (modstates & SDL_KMOD_LCTRL) return osgGA::GUIEventAdapter::KEY_Control_L;
        else if (modstates & SDL_KMOD_RCTRL) return osgGA::GUIEventAdapter::KEY_Control_R;
        else if (modstates & SDL_KMOD_LALT) return osgGA::GUIEventAdapter::KEY_Alt_L;
        else if (modstates & SDL_KMOD_RALT) return osgGA::GUIEventAdapter::KEY_Alt_R;
        else if (modstates & SDL_KMOD_LSHIFT) return osgGA::GUIEventAdapter::KEY_Shift_L;
        else if (modstates & SDL_KMOD_RSHIFT) return osgGA::GUIEventAdapter::KEY_Shift_R;
        else if (modstates & SDL_KMOD_CAPS) return osgGA::GUIEventAdapter::KEY_Caps_Lock;
        else if (modstates & SDL_KMOD_NUM) return osgGA::GUIEventAdapter::KEY_Num_Lock;
#endif
        else return 0;
    }

    static osgGA::GUIEventAdapter::KeySymbol getKey(SDL_Keycode key)
    {
        switch (key)
        {
        case SDLK_RETURN: return osgGA::GUIEventAdapter::KeySymbol::KEY_Return;
        case SDLK_ESCAPE: return osgGA::GUIEventAdapter::KeySymbol::KEY_Escape;
        case SDLK_BACKSPACE: return osgGA::GUIEventAdapter::KeySymbol::KEY_BackSpace;
        case SDLK_TAB: return osgGA::GUIEventAdapter::KeySymbol::KEY_Tab;
        case SDLK_SPACE: return osgGA::GUIEventAdapter::KeySymbol::KEY_Space;
        case SDLK_CAPSLOCK: return osgGA::GUIEventAdapter::KeySymbol::KEY_Caps_Lock;
        case SDLK_F1: return osgGA::GUIEventAdapter::KeySymbol::KEY_F1;
        case SDLK_F2: return osgGA::GUIEventAdapter::KeySymbol::KEY_F2;
        case SDLK_F3: return osgGA::GUIEventAdapter::KeySymbol::KEY_F3;
        case SDLK_F4: return osgGA::GUIEventAdapter::KeySymbol::KEY_F4;
        case SDLK_F5: return osgGA::GUIEventAdapter::KeySymbol::KEY_F5;
        case SDLK_F6: return osgGA::GUIEventAdapter::KeySymbol::KEY_F6;
        case SDLK_F7: return osgGA::GUIEventAdapter::KeySymbol::KEY_F7;
        case SDLK_F8: return osgGA::GUIEventAdapter::KeySymbol::KEY_F8;
        case SDLK_F9: return osgGA::GUIEventAdapter::KeySymbol::KEY_F9;
        case SDLK_F10: return osgGA::GUIEventAdapter::KeySymbol::KEY_F10;
        case SDLK_F11: return osgGA::GUIEventAdapter::KeySymbol::KEY_F11;
        case SDLK_F12: return osgGA::GUIEventAdapter::KeySymbol::KEY_F12;
        case SDLK_PRINTSCREEN: return osgGA::GUIEventAdapter::KeySymbol::KEY_Print;
        case SDLK_SCROLLLOCK: return osgGA::GUIEventAdapter::KeySymbol::KEY_Scroll_Lock;
        case SDLK_PAUSE: return osgGA::GUIEventAdapter::KeySymbol::KEY_Pause;
        case SDLK_INSERT: return osgGA::GUIEventAdapter::KeySymbol::KEY_Insert;
        case SDLK_HOME: return osgGA::GUIEventAdapter::KeySymbol::KEY_Home;
        case SDLK_PAGEUP: return osgGA::GUIEventAdapter::KeySymbol::KEY_Page_Up;
        case SDLK_DELETE: return osgGA::GUIEventAdapter::KeySymbol::KEY_Delete;
        case SDLK_END: return osgGA::GUIEventAdapter::KeySymbol::KEY_End;
        case SDLK_PAGEDOWN: return osgGA::GUIEventAdapter::KeySymbol::KEY_Page_Down;
        case SDLK_RIGHT: return osgGA::GUIEventAdapter::KeySymbol::KEY_Right;
        case SDLK_LEFT: return osgGA::GUIEventAdapter::KeySymbol::KEY_Left;
        case SDLK_DOWN: return osgGA::GUIEventAdapter::KeySymbol::KEY_Down;
        case SDLK_UP: return osgGA::GUIEventAdapter::KeySymbol::KEY_Up;
        default:
            //OSG_NOTICE << "[GraphicsWindowSDL] Unknown input key: " << key << std::endl;
            return (osgGA::GUIEventAdapter::KeySymbol)key;  // FIXME: cant check upper/lower
        }
    }
}

class SDLWindowingSystem : public osg::GraphicsContext::WindowingSystemInterface
{
public:
    SDLWindowingSystem()
    {
#if VERSE_WITH_SDL == 2
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
#else
        if (!SDL_Init(SDL_INIT_VIDEO))
#endif
        { OSG_WARN << "[GraphicsWindowSDL] Failed: " << SDL_GetError() << std::endl; return; }
    }

    virtual unsigned int getNumScreens(const osg::GraphicsContext::ScreenIdentifier& screenIdentifier =
                                       osg::GraphicsContext::ScreenIdentifier())
    { int count = 0; SDL_GET_DISPLAYS(count); return (unsigned int)count; }

    virtual void getScreenSettings(const osg::GraphicsContext::ScreenIdentifier& identifier,
                                   osg::GraphicsContext::ScreenSettings& resolution)
    {
#if VERSE_WITH_SDL == 2
        SDL_DisplayMode mode;
        if (SDL_GetCurrentDisplayMode(identifier.displayNum, &mode) == 0)
#else
        int count = 0; SDL_DisplayID* displays = SDL_GetDisplays(&count);
        SDL_DisplayID primaryID = (displays == NULL) ? 0 : displays[0];
        const SDL_DisplayMode* modePtr = SDL_GetCurrentDisplayMode(primaryID);
        if (!modePtr) { OSG_WARN << "[SDLWindowingSystem] getScreenSettings() failed: " << SDL_GetError() << "\n"; }

        const SDL_DisplayMode& mode = *modePtr;
        if (true)
#endif
        {
            resolution.width = mode.w; resolution.height = mode.h;
            resolution.refreshRate = mode.refresh_rate;
        }
        else
            OSG_WARN << "[SDLWindowingSystem] getScreenSettings() failed: " << SDL_GetError() << "\n";
    }

    virtual bool setScreenSettings(const osg::GraphicsContext::ScreenIdentifier& identifier,
                                   const osg::GraphicsContext::ScreenSettings& resolution)
    {
        OSG_WARN << "[SDLWindowingSystem] setScreenSettings() not implemented" << std::endl;
        return false;
    }

    virtual void enumerateScreenSettings(const osg::GraphicsContext::ScreenIdentifier& identifier,
                                         osg::GraphicsContext::ScreenSettingsList& resolutionList)
    {
#if VERSE_WITH_SDL == 2
        int numModes = SDL_GetNumDisplayModes(identifier.displayNum);
        for (int i = 0; i < numModes; ++i)
        {
            SDL_DisplayMode mode; osg::GraphicsContext::ScreenSettings settings;
            if (SDL_GetDisplayMode(i, 0, &mode) != 0) continue;
#else
        int numModes = 0;
        SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(identifier.displayNum, &numModes);
        for (int i = 0; i < numModes; ++i)
        {
            osg::GraphicsContext::ScreenSettings settings; if (modes == NULL) continue;
            SDL_DisplayMode& mode = *(modes[i]);
#endif

            settings.width = mode.w; settings.height = mode.h;
            settings.refreshRate = mode.refresh_rate;
            resolutionList.push_back(settings);
        }
    }

    virtual osg::GraphicsContext* createGraphicsContext(osg::GraphicsContext::Traits* traits)
    { return new GraphicsWindowSDL(traits); }

protected:
    virtual ~SDLWindowingSystem()
    {
        if (osg::Referenced::getDeleteHandler())
        {
            osg::Referenced::getDeleteHandler()->setNumFramesToRetainObjects(0);
            osg::Referenced::getDeleteHandler()->flushAll();
        }
        SDL_Quit();
    }
};

GraphicsWindowSDL::GraphicsWindowSDL(osg::GraphicsContext::Traits* traits)
:   _glContext(NULL), _glDisplay(NULL), _glSurface(NULL),
    _lastKey(0), _lastModKey(0), _gestureDistance(0.0f), _valid(false), _realized(false)
{
    _traits = traits; initialize();
    if (valid())
    {
        setState(new osg::State);
        getState()->setGraphicsContext(this);
        if (_traits.valid() && _traits->sharedContext != NULL)
        {
            getState()->setContextID(_traits->sharedContext->getState()->getContextID());
            incrementContextIDUsageCount(getState()->getContextID());
        }
        else
            getState()->setContextID(osg::GraphicsContext::createNewContextID());
    }
}

GraphicsWindowSDL::~GraphicsWindowSDL()
{
    releaseContextImplementation();
}

void GraphicsWindowSDL::initialize()
{
    WindowData* winData = static_cast<WindowData*>(_traits->inheritedWindowData.get());
#if defined(VERSE_EMBEDDED) && !defined(VERSE_WASM)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    OSG_NOTICE << "[GraphicsWindowSDL] Create EGL system for embedded device" << std::endl;
#elif defined(VERSE_GLES_DESKTOP)
    OSG_NOTICE << "[GraphicsWindowSDL] Create EGL system for desktop PC" << std::endl;
#elif defined(VERSE_WASM)
    if (winData)
    {
        SDL_SetHint(SDL_HINT_EMSCRIPTEN_ASYNCIFY, winData->sleepable ? "1" : "0");
        SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, winData->canvasElement.c_str());
    }
    OSG_NOTICE << "[GraphicsWindowSDL] Create EGL system for WebAssembly" << std::endl;
#else
    if (winData && winData->forceEGL)
    {
#  if defined(SDL_VIDEO_DRIVER_X11)
        SDL_SetHint(SDL_HINT_VIDEO_X11_FORCE_EGL, "1");
        OSG_NOTICE << "[GraphicsWindowSDL] Create EGL system (EGL forced on)" << std::endl;
#  else
        OSG_NOTICE << "[GraphicsWindowSDL] Create native windowing system (EGL disabled)" << std::endl;
#  endif
    }
    else
        { OSG_NOTICE << "[GraphicsWindowSDL] Create native windowing system" << std::endl; }
#endif

    int exMajor = 0, exMinor = 0;
    if (winData) { exMajor = winData->majorVersion; exMinor = winData->minorVersion; }

#if defined(VERSE_GLES_DESKTOP)
    EGLConfig config;
#elif defined(VERSE_EMBEDDED_GLES2)
    if (exMajor <= 0) exMajor = 2;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, exMajor);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, exMinor);
#elif defined(VERSE_EMBEDDED_GLES3)
    if (exMajor <= 0) exMajor = 3;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, exMajor);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, exMinor);
#endif

    // Create window
    int winX = 50, winY = 50, winW = 1280, winH = 720;
    unsigned int flags = SDL_WINDOW_OPENGL;
#if VERSE_WITH_SDL == 2
    flags |= SDL_WINDOW_SHOWN;
#endif
    if (_traits.valid())
    {
#if defined(SDL_VIDEO_DRIVER_COCOA)
        flags &= (~SDL_WINDOW_OPENGL); flags |= SDL_WINDOW_METAL;
#endif
        if (_traits->supportsResize) flags |= SDL_WINDOW_RESIZABLE;
        if (!_traits->windowDecoration) flags |= SDL_WINDOW_BORDERLESS;
        winX = _traits->x; winY = _traits->y;
        winW = _traits->width; winH = _traits->height;
#if !defined(VERSE_GLES_DESKTOP)
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, _traits->red);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, _traits->green);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, _traits->blue);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, _traits->depth);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, _traits->alpha);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, _traits->stencil);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, _traits->doubleBuffer ? 1 : 0);
        SDL_GL_SetAttribute(SDL_GL_STEREO, _traits->quadBufferStereo ? 1 : 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, _traits->sampleBuffers);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, _traits->samples);
        //SDL_GL_SetSwapInterval(_traits->vsync ? 1 : 0);
#endif

        const char* winName = _traits->windowName.empty() ? NULL : _traits->windowName.c_str();
        if (_traits->screenNum > 0)
        {
            int displayCount = 0; SDL_GET_DISPLAYS(displayCount);
            if (_traits->screenNum < displayCount)
            {
                int num = _traits->screenNum + 1;
                winX = SDL_WINDOWPOS_CENTERED_DISPLAY(num), winY = SDL_WINDOWPOS_CENTERED_DISPLAY(num);
                SDL_CREATE_WINDOW(_sdlWindow, winName, winX, winY, winW, winH, flags);
            }
        }
        else
            SDL_CREATE_WINDOW(_sdlWindow, winName, winX, winY, winW, winH, flags);

#if defined(VERSE_GLES_DESKTOP)
        // Config EGL by ourselves so that to set different backends of Google Angle!
        EGLDisplay display = EGL_NO_DISPLAY;
        EGLint configAttribList[] =
        {
#   if defined(OSG_GLES3_AVAILABLE)
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
#   else
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
#   endif
            EGL_RED_SIZE,       (int)_traits->red,
            EGL_GREEN_SIZE,     (int)_traits->green,
            EGL_BLUE_SIZE,      (int)_traits->blue,
            EGL_ALPHA_SIZE,     (int)_traits->alpha,
            EGL_DEPTH_SIZE,     (int)_traits->depth,
            EGL_STENCIL_SIZE,   (int)_traits->stencil,
            EGL_SAMPLE_BUFFERS, (int)_traits->sampleBuffers,
            EGL_SAMPLES,        (int)_traits->samples,
            EGL_NONE
        };

#  if VERSE_WITH_SDL == 2
        SDL_SysWMinfo sdlInfo; SDL_VERSION(&sdlInfo.version);
        SDL_GetWindowWMInfo(_sdlWindow, &sdlInfo);
#   if defined(SDL_VIDEO_DRIVER_WINDOWS)
        EGLNativeWindowType hWnd = (EGLNativeWindowType)sdlInfo.info.win.window;
#   elif defined(SDL_VIDEO_DRIVER_X11)
        EGLNativeWindowType hWnd = (EGLNativeWindowType)sdlInfo.info.x11.window;
#   elif defined(SDL_VIDEO_DRIVER_COCOA)
        EGLNativeWindowType hWnd = getViewFromWindow(sdlInfo.info.cocoa.window);
#   elif defined(SDL_VIDEO_DRIVER_ANDROID)
        EGLNativeWindowType hWnd = (EGLNativeWindowType)sdlInfo.info.android.window;
#   else
#       error "[GraphicsWindowSDL] Unsupported platform?"
#   endif
#  else
        SDL_PropertiesID props = SDL_GetWindowProperties(_sdlWindow);
#   if defined(SDL_PLATFORM_WINDOWS)
        EGLNativeWindowType hWnd = (EGLNativeWindowType)SDL_GetNumberProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, 0);
#   elif defined(SDL_PLATFORM_LINUX)
        EGLNativeWindowType hWnd = (EGLNativeWindowType)SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
#   elif defined(SDL_PLATFORM_MACOS) || defined(SDL_PLATFORM_IOS)
        EGLNativeWindowType hWnd = (EGLNativeWindowType)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
#   elif defined(SDL_PLATFORM_ANDROID)
        EGLNativeWindowType hWnd = (EGLNativeWindowType)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER, NULL);
#   else
#       error "[GraphicsWindowSDL] Unsupported platform?"
#   endif
#  endif

        PFNEGLDEBUGMESSAGECONTROLKHRPROC eglDebugMessageControlKHR =
            (PFNEGLDEBUGMESSAGECONTROLKHRPROC)(eglGetProcAddress("eglDebugMessageControlKHR"));
        PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT =
            (PFNEGLGETPLATFORMDISPLAYEXTPROC)(eglGetProcAddress("eglGetPlatformDisplayEXT"));
#   if false
        PFNEGLCREATEDEVICEANGLEPROC eglCreateDeviceANGLE =
            (PFNEGLCREATEDEVICEANGLEPROC)(eglGetProcAddress("eglCreateDeviceANGLE"));
        PFNEGLRELEASEDEVICEANGLEPROC eglReleaseDeviceANGLE =
            (PFNEGLRELEASEDEVICEANGLEPROC)(eglGetProcAddress("eglReleaseDeviceANGLE"));
#   endif
        if (eglDebugMessageControlKHR != NULL)
        {
            EGLAttrib controls[] = {
                EGL_DEBUG_MSG_CRITICAL_KHR, EGL_TRUE,
                EGL_DEBUG_MSG_ERROR_KHR, EGL_TRUE,
                EGL_DEBUG_MSG_WARN_KHR, EGL_TRUE,
                EGL_DEBUG_MSG_INFO_KHR, EGL_FALSE,
                EGL_NONE, EGL_NONE,
            };
            eglDebugMessageControlKHR(&eglErrorCallback, controls);
        }

        if (eglGetPlatformDisplayEXT != NULL)
        {
            const EGLint attrNewBackend[] = {
#   if defined(SDL_VIDEO_DRIVER_COCOA)
                EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE,
#   else
                EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE,
#   endif
                //EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
                //EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE, EGL_TRUE,  // for D3D11
                EGL_NONE, EGL_NONE
            };
            display = eglGetPlatformDisplayEXT(
                EGL_PLATFORM_ANGLE_ANGLE, (void*)EGL_DEFAULT_DISPLAY, attrNewBackend);
        }
        else
            OSG_WARN << "[GraphicsWindowSDL] eglGetPlatformDisplayEXT() not found" << std::endl;

        if (display == EGL_NO_DISPLAY)
        {
            display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
            if (display != EGL_NO_DISPLAY)
                OSG_NOTICE << "[GraphicsWindowSDL] Selected default backend as fallback" << std::endl;
        }

        if (display == EGL_NO_DISPLAY)
        { OSG_WARN << "[GraphicsWindowSDL] Failed to get EGL display" << std::endl; return; }

        // Initialize EGL
        EGLint majorVersion = 0, minorVersion = 0, numConfigs = 0;
        if (!eglInitialize(display, &majorVersion, &minorVersion))
        { OSG_WARN << "[GraphicsWindowSDL] Failed to initialize EGL display" << std::endl; return; }
        else if (!eglGetConfigs(display, NULL, 0, &numConfigs))
        { OSG_WARN << "[GraphicsWindowSDL] Failed to get EGL display config" << std::endl; return; }

        // Get an appropriate EGL framebuffer configuration
        if (!eglChooseConfig(display, configAttribList, &config, 1, &numConfigs))
        { OSG_WARN << "[GraphicsWindowSDL] Failed to choose EGL config" << std::endl; return; }

        // Configure the surface
        EGLint surfaceAttribList[] = { EGL_NONE, EGL_NONE };
        EGLSurface surface = eglCreateWindowSurface(
            display, config, (EGLNativeWindowType)hWnd, surfaceAttribList);

        
        if (surface != EGL_NO_SURFACE)
            { _glSurface = (void*)surface; _glDisplay = (void*)display; }
        else
        {
            OSG_WARN << "[GraphicsWindowSDL] Failed to create EGL surface: " << std::hex
                     << eglGetError() << std::dec << std::endl; return;
        }
#endif  // defined(VERSE_GLES_DESKTOP)
    }
    else
    {
        SDL_CREATE_WINDOW(_sdlWindow, "osgVerse::GraphicsWindowSDL",
                          winX, winY, winW, winH, flags | SDL_WINDOW_RESIZABLE);
    }

    if (_sdlWindow == NULL)
    { OSG_WARN << "[GraphicsWindowSDL] Failed: " << SDL_GetError() << std::endl; return; }

    // Create context
#if defined(VERSE_GLES_DESKTOP)
#  if defined(OSG_GLES3_AVAILABLE)
    if (exMajor <= 0) exMajor = 3;
    EGLint contextAttribList[] = {
        EGL_CONTEXT_CLIENT_VERSION, exMajor,
        EGL_CONTEXT_MINOR_VERSION, exMinor,
        EGL_NONE, EGL_NONE
    };
#  else
    if (exMajor <= 0) exMajor = 2;
    EGLint contextAttribList[] = { EGL_CONTEXT_CLIENT_VERSION, exMajor, EGL_NONE, EGL_NONE };
#  endif
    EGLDisplay display = (EGLDisplay)_glDisplay;
    EGLSurface surface = (EGLSurface)_glSurface;
    eglBindAPI(EGL_OPENGL_ES_API);  // Set rendering API

    // Create an EGL rendering context
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribList);
    if (context == EGL_NO_CONTEXT)
    { OSG_WARN << "[GraphicsWindowSDL] Failed to create EGL context" << std::endl; return; }

#  if VERSE_WITH_VULKAN
    // Check if Google Angle is modified for VERSE use?
    PFNEGLGETVKOBJECTSVERSEPROC eglGetVkObjectsVERSE =
        (PFNEGLGETVKOBJECTSVERSEPROC)(eglGetProcAddress("eglGetVkObjectsVERSE"));
    if (eglGetVkObjectsVERSE != NULL)
    {
        VulkanObjectInfo* infoData = (VulkanObjectInfo*)eglGetVkObjectsVERSE(display, surface);
        if (infoData != NULL)
        {
            _vulkanObjects = new VulkanManager(infoData);
#if true
            SDL_Vulkan_LoadLibrary(NULL);
            ((VulkanManager&)*_vulkanObjects).testValidation(SDL_Vulkan_GetVkGetInstanceProcAddr());
            SDL_Vulkan_UnloadLibrary();
#endif
        }
        OSG_NOTICE << "[GraphicsWindowSDL] eglGetVkObjectsVERSE() found. "
                   << "Retrieved Vulkan objects for integration uses." << std::endl;
    }
#  endif
#else
    SDL_GLContext context = SDL_GL_CreateContext(_sdlWindow);
    if (context == NULL)
    { OSG_WARN << "[GraphicsWindowSDL] Failed: " << SDL_GetError() << std::endl; return; }
#endif
    _glContext = (void*)context; _valid = true;
}

bool GraphicsWindowSDL::realizeImplementation()
{
    _realized = false; if (!_valid) initialize(); if (!_valid) return false;
#if OSG_VERSION_GREATER_THAN(3, 5, 0)
    getEventQueue()->syncWindowRectangleWithGraphicsContext();
#endif

    int windowWidth = 1280, windowHeight = 720;
    if (_traits.valid()) { windowWidth = _traits->width; windowHeight = _traits->height; }
#if VERSE_GLES_DESKTOP
    EGLDisplay display = (EGLContext)_glDisplay;
    EGLSurface surface = (EGLSurface)_glSurface;
    eglQuerySurface(display, surface, EGL_WIDTH, &windowWidth);
    eglQuerySurface(display, surface, EGL_HEIGHT, &windowHeight);
#endif
    getEventQueue()->windowResize(0, 0, windowWidth, windowHeight);
    resized(0, 0, windowWidth, windowHeight);

#if VERSE_GLES_DESKTOP
    getState()->setUseModelViewAndProjectionUniforms(true);
    getState()->setUseVertexAttributeAliasing(true);
#endif
    _realized = true; return true;
}

void GraphicsWindowSDL::closeImplementation()
{
    if (!_valid) return; else _valid = false;
#ifdef VERSE_GLES_DESKTOP
    EGLDisplay display = (EGLDisplay)_glDisplay;
    EGLContext context = (EGLContext)_glContext;
    EGLSurface surface = (EGLSurface)_glSurface;
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
#else
    SDL_GLContext context = (SDL_GLContext)_glContext;
#  if VERSE_WITH_SDL == 2
    SDL_GL_DeleteContext(context);
#  else
    SDL_GL_DestroyContext(context);
#  endif
#endif
    SDL_DestroyWindow(_sdlWindow);
}

bool GraphicsWindowSDL::makeCurrentImplementation()
{
    if (!_valid) return false;
#ifdef VERSE_GLES_DESKTOP
    EGLDisplay display = (EGLDisplay)_glDisplay;
    EGLContext context = (EGLContext)_glContext;
    EGLSurface surface = (EGLSurface)_glSurface;
    bool ok = (eglMakeCurrent(display, surface, surface, context) == EGL_TRUE);
    if (!ok) { OSG_WARN << "[GraphicsWindowSDL] Make current failed: " << eglGetError() << std::endl; }
    return ok;
#else
    SDL_GLContext context = (SDL_GLContext)_glContext;
#  if VERSE_WITH_SDL == 2
    return SDL_GL_MakeCurrent(_sdlWindow, context) == 0;
#  else
    return SDL_GL_MakeCurrent(_sdlWindow, context);
#  endif
#endif
}

bool GraphicsWindowSDL::releaseContextImplementation()
{
    if (!_valid) return false;
#ifdef VERSE_GLES_DESKTOP
    EGLDisplay display = (EGLDisplay)_glDisplay;
    return eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_TRUE;
#else
    return true;
#endif
}

void GraphicsWindowSDL::swapBuffersImplementation()
{
    if (!_valid) return;
#ifdef VERSE_GLES_DESKTOP
    EGLDisplay display = (EGLDisplay)_glDisplay;
    EGLSurface surface = (EGLSurface)_glSurface;
    eglSwapBuffers(display, surface);
#else
    SDL_GL_SwapWindow(_sdlWindow);
#endif
}

#if OSG_VERSION_GREATER_THAN(3, 1, 1)
bool GraphicsWindowSDL::checkEvents()
#else
void GraphicsWindowSDL::checkEvents()
#endif
{
    SDL_Event event;
#if OSG_VERSION_GREATER_THAN(3, 1, 1)
    if (!_realized) return false;
#else
    if (!_realized) return;
#endif

    while (SDL_PollEvent(&event))
    {
        osgGA::EventQueue* eq = getEventQueue();
        switch (event.type)
        {
        case SDL_EVENT_MOUSE_MOTION:
            if (event.motion.x > 0 || event.motion.y > 0)
                eq->mouseMotion(event.motion.x, event.motion.y); break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event.button.clicks == 2)
                eq->mouseDoubleButtonPress(event.button.x, event.button.y, event.button.button);
            else
                eq->mouseButtonPress(event.button.x, event.button.y, event.button.button); 
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            eq->mouseButtonRelease(event.button.x, event.button.y, event.button.button); break;
        case SDL_EVENT_MOUSE_WHEEL:
            if (event.wheel.y < 0) eq->mouseScroll(osgGA::GUIEventAdapter::ScrollingMotion::SCROLL_DOWN);
            else if (event.wheel.y > 0) eq->mouseScroll(osgGA::GUIEventAdapter::ScrollingMotion::SCROLL_UP); break;
        case SDL_EVENT_KEY_UP:
            {
                int key = getKey(SDL_KEY_SYMBOL(event)), state = SDL_KEY_MODIFIER(event); if (key == 0) break;
                if (state == 0) eq->getCurrentEventState()->setModKeyMask(0);
                eq->keyRelease((osgGA::GUIEventAdapter::KeySymbol)key, 0);  // modkey state will be kept if passed 0
                _lastKey = 0; _lastModKey = 0;
            } break;
        case SDL_EVENT_KEY_DOWN:
            {
                int key = getKey(SDL_KEY_SYMBOL(event)), mod = getModKey(); if (key == 0) break;
                if (key != _lastKey || mod != _lastModKey)
                {
                    eq->keyPress((osgGA::GUIEventAdapter::KeySymbol)key, mod);
                    _lastKey = key; _lastModKey = mod;
                }
            } break;
#if VERSE_WITH_SDL == 2
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                eq->windowResize(0, 0, event.window.data1, event.window.data2);
                resized(0, 0, event.window.data1, event.window.data2);
            }
            break;
        case SDL_MULTIGESTURE:
            if (fabs(event.mgesture.dTheta) > (osg::PI / 180.0f))
            {   // TODO: rotation?
            }
            else
            {
                // The distance change of the gesture is summed up and only turned into a zoom
                // step when it is big enough, otherwise every motion event of the gesture
                // would zoom by a whole step and the view would rush away. Fingers moving
                // apart (dDist > 0) zoom in and fingers moving together zoom out, so that the
                // content follows the fingers: this is the SCROLL_DOWN / SCROLL_UP pair since
                // the manipulators of OSG zoom out on SCROLL_UP (the default wheel zoom factor
                // of 0.1 lengthens the distance on that motion)
                int windowSize = (_traits.valid() && _traits->width > 0) ?
                                 ((_traits->width < _traits->height) ? _traits->width : _traits->height) : 720;
                _gestureDistance += event.mgesture.dDist;
                if (fabs(_gestureDistance) > windowSize * 0.02f)
                {
                    // FIXME: also send user-event?
                    eq->mouseScroll((_gestureDistance > 0.0f)  // Pinch open
                        ? osgGA::GUIEventAdapter::ScrollingMotion::SCROLL_DOWN
                        : osgGA::GUIEventAdapter::ScrollingMotion::SCROLL_UP);
                    _gestureDistance = 0.0f;
                }
            }
            break;
#else
        case SDL_EVENT_WINDOW_RESIZED:
            eq->windowResize(0, 0, event.window.data1, event.window.data2);
            resized(0, 0, event.window.data1, event.window.data2);
            break;
        case SDL_EVENT_FINGER_MOTION:
            // TODO
            break;
#endif
        case SDL_EVENT_QUIT: eq->closeWindow(); break;
        default: break;
        }
    }
#if OSG_VERSION_GREATER_THAN(3, 1, 1)
    return true;
#endif
}

void GraphicsWindowSDL::grabFocus()
{
#if !defined(VERSE_WASM)
#  if VERSE_WITH_SDL == 2
    if (_valid) SDL_SetWindowInputFocus(_sdlWindow);
#  else
    if (_valid) SDL_RaiseWindow(_sdlWindow);
#  endif
#endif
}

void GraphicsWindowSDL::grabFocusIfPointerInWindow()
{
#if !defined(VERSE_WASM)
#  if VERSE_WITH_SDL == 2
    if (_valid) SDL_SetWindowInputFocus(_sdlWindow);
#  else
    if (_valid) SDL_RaiseWindow(_sdlWindow);
#  endif
#endif
}

void GraphicsWindowSDL::raiseWindow()
{ if (_valid) SDL_RaiseWindow(_sdlWindow); }

void GraphicsWindowSDL::requestWarpPointer(float x, float y)
{ if (_valid) SDL_WarpMouseInWindow(_sdlWindow, x, y); }

bool GraphicsWindowSDL::setWindowDecorationImplementation(bool flag)
{
    if (!_valid) return false;
#if !defined(VERSE_WASM)
#  if VERSE_WITH_SDL == 2
    SDL_SetWindowBordered(_sdlWindow, flag ? SDL_TRUE : SDL_FALSE); return true;
#  else
    SDL_SetWindowBordered(_sdlWindow, flag); return true;
#  endif
#else
    return false;
#endif
}

bool GraphicsWindowSDL::setWindowRectangleImplementation(int x, int y, int width, int height)
{
    if (!_valid) return false;
    SDL_SetWindowSize(_sdlWindow, width, height);
    SDL_SetWindowPosition(_sdlWindow, x, y); return true;
}

void GraphicsWindowSDL::setWindowName(const std::string& name)
{ if (_valid) SDL_SetWindowTitle(_sdlWindow, name.c_str()); }

void GraphicsWindowSDL::setCursor(osgViewer::GraphicsWindow::MouseCursor cursor)
{
#if !defined(VERSE_WASM)
    SDL_Cursor* sdlCursor = NULL;
    switch (cursor)
    {
#if VERSE_WITH_SDL == 2
    case TextCursor: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM); break;
    case CrosshairCursor: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR); break;
    case WaitCursor: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT); break;
    case HandCursor: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND); break;
    case NoCursor: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO); break;
    case LeftRightCursor: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE); break;
    case UpDownCursor: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS); break;
    case TopLeftCorner: case BottomRightCorner: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE); break;
    case TopRightCorner: case BottomLeftCorner: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW); break;
    default: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW); break;
#else
    case TextCursor: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT); break;
    case CrosshairCursor: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR); break;
    case WaitCursor: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT); break;
    case HandCursor: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER); break;
    case NoCursor: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NOT_ALLOWED); break;
    case LeftRightCursor: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE); break;
    case UpDownCursor: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE); break;
    case TopLeftCorner: case BottomRightCorner: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NWSE_RESIZE); break;
    case TopRightCorner: case BottomLeftCorner: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NESW_RESIZE); break;
    default: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT); break;
#endif
    }
    SDL_SetCursor(sdlCursor); SDL_SetCursor(NULL);
#endif
}

void GraphicsWindowSDL::setSyncToVBlank(bool on)
{ SDL_GL_SetSwapInterval(on ? 1 : 0); }

GraphicsWindowHandle* GraphicsWindowSDL::getHandle() const
{
    osg::ref_ptr<GraphicsWindowHandle> handle = new GraphicsWindowHandle;
#if defined(OSG_GLES1_AVAILABLE) || defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
    handle->eglDisplay = _glDisplay; handle->eglSurface = _glSurface;
#endif
    handle->eglContext = _glContext; handle->nativeHandle = _sdlWindow; return handle.release();
}

extern "C" OSGVERSE_RW_EXPORT void graphicswindow_SDL(void) {}
#if OSG_VERSION_GREATER_THAN(3, 5, 1)
static osg::WindowingSystemInterfaceProxy<SDLWindowingSystem> s_proxy_SDLWindowingSystem("SDL");
#endif

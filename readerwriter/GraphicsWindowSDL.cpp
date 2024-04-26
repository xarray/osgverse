#include "GraphicsWindowSDL.h"
#include <SDL.h>
#include <SDL_syswm.h>
#if defined(OSG_GLES1_AVAILABLE) || defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
#   if !defined(VERSE_WEBGL1) && !defined(VERSE_WEBGL2)
#       include <EGL/egl.h>
#       include <EGL/eglext.h>
#       include <EGL/eglext_angle.h>
#       define VERSE_GLES_DESKTOP 1
#   endif
#endif
using namespace osgVerse;

GraphicsWindowSDL::GraphicsWindowSDL(osg::GraphicsContext::Traits* traits)
    : _glContext(NULL), _glDisplay(NULL), _glSurface(NULL), _valid(false), _realized(false)
{
    _traits = traits; initialize();
    if (valid())
    {
        setState(new osg::State);
        getState()->setGraphicsContext(this);
        if (_traits.valid() && _traits->sharedContext.valid())
        {
            getState()->setContextID(_traits->sharedContext->getState()->getContextID());
            incrementContextIDUsageCount(getState()->getContextID());
        }
        else
            getState()->setContextID(osg::GraphicsContext::createNewContextID());
    }
}

void GraphicsWindowSDL::initialize()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    { OSG_WARN << "[GraphicsWindowSDL] Failed: " << SDL_GetError() << std::endl; return; }

#if defined(VERSE_WEBGL1)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(VERSE_WEBGL2)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(VERSE_GLES_DESKTOP)
    EGLConfig config;
#endif

    // Create window
    int winX = 50, winY = 50, winW = 1280, winH = 720;
    if (_traits.valid())
    {
        unsigned int flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL
                           | SDL_WINDOW_KEYBOARD_GRABBED | SDL_WINDOW_MOUSE_GRABBED;
        if (_traits->supportsResize) flags |= SDL_WINDOW_RESIZABLE;
        if (!_traits->windowDecoration) flags |= SDL_WINDOW_BORDERLESS;
        winX = _traits->x; winY = _traits->y;
        winW = _traits->width; winH = _traits->height;
        _sdlWindow = SDL_CreateWindow(
            _traits->windowName.c_str(), winX, winY, winW, winH, flags);

#if defined(VERSE_GLES_DESKTOP)
        // Config EGL by ourselves so that to set different backends of Google Angle!
        SDL_SysWMinfo sdlInfo; SDL_VERSION(&sdlInfo.version);
        SDL_GetWindowWMInfo(_sdlWindow, &sdlInfo);
        EGLint configAttribList[] =
        {
#   if defined(OSG_GLES3_AVAILABLE)
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
#   else
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
#   endif
            EGL_RED_SIZE,       _traits->red,
            EGL_GREEN_SIZE,     _traits->green,
            EGL_BLUE_SIZE,      _traits->blue,
            EGL_ALPHA_SIZE,     _traits->alpha,
            EGL_DEPTH_SIZE,     _traits->depth,
            EGL_STENCIL_SIZE,   _traits->stencil,
            EGL_SAMPLE_BUFFERS, _traits->sampleBuffers,
            EGL_SAMPLES,        _traits->samples,
            EGL_NONE
        };

        EGLDisplay display = EGL_NO_DISPLAY;
        EGLNativeWindowType hWnd = sdlInfo.info.win.window;
        PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT =
            (PFNEGLGETPLATFORMDISPLAYEXTPROC)(eglGetProcAddress("eglGetPlatformDisplayEXT"));
        if (eglGetPlatformDisplayEXT != NULL)
        {
            const EGLint attrNewBackend[] = {
                EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE,
                //EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
                //EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE, EGL_TRUE,  // for D3D11
                EGL_NONE,
            };
            display = eglGetPlatformDisplayEXT(
                EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, attrNewBackend);
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
        if (surface != EGL_NO_SURFACE) { _glSurface = (void*)surface; _glDisplay = (void*)display; }
        else { OSG_WARN << "[GraphicsWindowSDL] Failed to create EGL surface" << std::endl; return; }
#else
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
#endif
        SDL_GL_SetSwapInterval(_traits->vsync ? 1 : 0);
    }
    else
    {
        _sdlWindow = SDL_CreateWindow(
            "osgVerse::GraphicsWindowSDL", winX, winY, winW, winH,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    }

    if (_sdlWindow == NULL)
    { OSG_WARN << "[GraphicsWindowSDL] Failed: " << SDL_GetError() << std::endl; return; }

    // Create context
#if defined(VERSE_GLES_DESKTOP)
#   if defined(OSG_GLES3_AVAILABLE)
    EGLint contextAttribList[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_CONTEXT_MINOR_VERSION, 0,
        EGL_NONE, EGL_NONE
    };
#   else
    EGLint contextAttribList[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE };
#   endif
    EGLDisplay display = (EGLDisplay)_glDisplay;
    eglBindAPI(EGL_OPENGL_ES_API);  // Set rendering API

    // Create an EGL rendering context
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribList);
    if (context == EGL_NO_CONTEXT)
    { OSG_WARN << "[GraphicsWindowSDL] Failed to create EGL context" << std::endl; return; }
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
    getEventQueue()->syncWindowRectangleWithGraphicsContext();

    int windowWidth = 1280, windowHeight = 720;
    if (_traits.valid()) { windowWidth = _traits->width; windowHeight = _traits->height; }
#if VERSE_GLES
    eglQuerySurface(display, surface, EGL_WIDTH, &windowWidth);
    eglQuerySurface(display, surface, EGL_HEIGHT, &windowHeight);
#endif
    getEventQueue()->windowResize(0, 0, windowWidth, windowHeight);
    resized(0, 0, windowWidth, windowHeight);
    _realized = true; return true;
}

void GraphicsWindowSDL::closeImplementation()
{
    if (!_valid) return;
#ifdef VERSE_GLES_DESKTOP
    EGLDisplay display = (EGLContext)_glDisplay;
    EGLContext context = (EGLContext)_glContext;
    EGLSurface surface = (EGLContext)_glSurface;
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
#else
    SDL_GLContext context = (SDL_GLContext)_glContext;
    SDL_GL_DeleteContext(context);
#endif
    SDL_DestroyWindow(_sdlWindow);
    SDL_Quit();
}

bool GraphicsWindowSDL::makeCurrentImplementation()
{
    if (!_valid) return false;
#ifdef VERSE_GLES_DESKTOP
    EGLDisplay display = (EGLContext)_glDisplay;
    EGLContext context = (EGLContext)_glContext;
    EGLSurface surface = (EGLContext)_glSurface;
    return eglMakeCurrent(display, surface, surface, context) == EGL_TRUE;
#else
    SDL_GLContext context = (SDL_GLContext)_glContext;
    return SDL_GL_MakeCurrent(_sdlWindow, context) == 0;
#endif
}

bool GraphicsWindowSDL::releaseContextImplementation()
{
    if (!_valid) return false;
#ifdef VERSE_GLES_DESKTOP
    EGLDisplay display = (EGLContext)_glDisplay;
    return eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_TRUE;
#else
    return true;
#endif
}

void GraphicsWindowSDL::swapBuffersImplementation()
{
    if (!_valid) return;
#ifdef VERSE_GLES_DESKTOP
    EGLDisplay display = (EGLContext)_glDisplay;
    EGLSurface surface = (EGLContext)_glSurface;
    eglSwapBuffers(display, surface);
#else
    SDL_GL_SwapWindow(_sdlWindow);
#endif
}

bool GraphicsWindowSDL::checkEvents()
{
    SDL_Event event;
    if (!_realized) return false;

    while (SDL_PollEvent(&event))
    {
        osgGA::EventQueue* eq = getEventQueue();
        switch (event.type)
        {
        case SDL_MOUSEMOTION:
            eq->mouseMotion(event.motion.x, event.motion.y); break;
        case SDL_MOUSEBUTTONDOWN:
            eq->mouseButtonPress(event.button.x, event.button.y, event.button.button); break;
        case SDL_MOUSEBUTTONUP:
            eq->mouseButtonRelease(event.button.x, event.button.y, event.button.button); break;
        case SDL_MOUSEWHEEL:
            if (event.wheel.y > 0) eq->mouseScroll(osgGA::GUIEventAdapter::ScrollingMotion::SCROLL_UP);
            else eq->mouseScroll(osgGA::GUIEventAdapter::ScrollingMotion::SCROLL_DOWN);
        case SDL_KEYUP:
            eq->keyRelease((osgGA::GUIEventAdapter::KeySymbol)event.key.keysym.sym); break;
        case SDL_KEYDOWN:
            eq->keyPress((osgGA::GUIEventAdapter::KeySymbol)event.key.keysym.sym); break;
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
            else if (fabs(event.mgesture.dDist) > 0.002f)
            {
                // FIXME: also send user-event?
                if (event.mgesture.dDist > 0.0f)  // Pinch open
                    eq->mouseScroll(osgGA::GUIEventAdapter::ScrollingMotion::SCROLL_UP);
                else  // Pinch close
                    eq->mouseScroll(osgGA::GUIEventAdapter::ScrollingMotion::SCROLL_DOWN);
            }
            break;
        case SDL_QUIT: eq->closeWindow(); break;
        default: break;
        }
    }
    return true;
}

void GraphicsWindowSDL::grabFocus()
{ if (_valid) SDL_SetWindowGrab(_sdlWindow, SDL_TRUE); }

void GraphicsWindowSDL::grabFocusIfPointerInWindow()
{ if (_valid) SDL_SetWindowGrab(_sdlWindow, SDL_TRUE); }

void GraphicsWindowSDL::raiseWindow()
{ if (_valid) SDL_RaiseWindow(_sdlWindow); }

void GraphicsWindowSDL::requestWarpPointer(float x, float y)
{ if (_valid) SDL_WarpMouseInWindow(_sdlWindow, x, y); }

bool GraphicsWindowSDL::setWindowDecorationImplementation(bool flag)
{
    if (!_valid) return false;
    SDL_SetWindowBordered(_sdlWindow, flag ? SDL_TRUE : SDL_FALSE); return true;
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
    SDL_Cursor* sdlCursor = NULL;
    switch (cursor)
    {
    case TextCursor: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM); break;
    case CrosshairCursor: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR); break;
    case WaitCursor: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT); break;
    case HandCursor: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND); break;
    case NoCursor: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO); break;
    case LeftRightCursor: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE); break;
    case UpDownCursor: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS); break;
    default: sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW); break;
    }
    SDL_SetCursor(sdlCursor); SDL_SetCursor(NULL);
}

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "GraphicsWindowGLFW.h"
#include <osg/DeleteHandler>
#include <iostream>
using namespace osgVerse;

namespace
{
    static int getModKey(int modstates)
    {
        if (modstates & GLFW_MOD_CONTROL) return osgGA::GUIEventAdapter::KEY_Control_L;
        else if (modstates & GLFW_MOD_ALT) return osgGA::GUIEventAdapter::KEY_Alt_L;
        else if (modstates & GLFW_MOD_SHIFT) return osgGA::GUIEventAdapter::KEY_Shift_L;
        else if (modstates & GLFW_MOD_CAPS_LOCK) return osgGA::GUIEventAdapter::KEY_Caps_Lock;
        else if (modstates & GLFW_MOD_NUM_LOCK) return osgGA::GUIEventAdapter::KEY_Num_Lock;
        else return 0;
    }

    static osgGA::GUIEventAdapter::KeySymbol getKey(int key, bool& treatAsChar)
    {
        switch (key)
        {
        case GLFW_KEY_ENTER: return osgGA::GUIEventAdapter::KeySymbol::KEY_Return;
        case GLFW_KEY_ESCAPE: return osgGA::GUIEventAdapter::KeySymbol::KEY_Escape;
        case GLFW_KEY_BACKSPACE: return osgGA::GUIEventAdapter::KeySymbol::KEY_BackSpace;
        case GLFW_KEY_TAB: return osgGA::GUIEventAdapter::KeySymbol::KEY_Tab;
        case GLFW_KEY_SPACE: return osgGA::GUIEventAdapter::KeySymbol::KEY_Space;
        case GLFW_KEY_CAPS_LOCK: return osgGA::GUIEventAdapter::KeySymbol::KEY_Caps_Lock;
        case GLFW_KEY_F1: return osgGA::GUIEventAdapter::KeySymbol::KEY_F1;
        case GLFW_KEY_F2: return osgGA::GUIEventAdapter::KeySymbol::KEY_F2;
        case GLFW_KEY_F3: return osgGA::GUIEventAdapter::KeySymbol::KEY_F3;
        case GLFW_KEY_F4: return osgGA::GUIEventAdapter::KeySymbol::KEY_F4;
        case GLFW_KEY_F5: return osgGA::GUIEventAdapter::KeySymbol::KEY_F5;
        case GLFW_KEY_F6: return osgGA::GUIEventAdapter::KeySymbol::KEY_F6;
        case GLFW_KEY_F7: return osgGA::GUIEventAdapter::KeySymbol::KEY_F7;
        case GLFW_KEY_F8: return osgGA::GUIEventAdapter::KeySymbol::KEY_F8;
        case GLFW_KEY_F9: return osgGA::GUIEventAdapter::KeySymbol::KEY_F9;
        case GLFW_KEY_F10: return osgGA::GUIEventAdapter::KeySymbol::KEY_F10;
        case GLFW_KEY_F11: return osgGA::GUIEventAdapter::KeySymbol::KEY_F11;
        case GLFW_KEY_F12: return osgGA::GUIEventAdapter::KeySymbol::KEY_F12;
        case GLFW_KEY_PRINT_SCREEN: return osgGA::GUIEventAdapter::KeySymbol::KEY_Print;
        case GLFW_KEY_SCROLL_LOCK: return osgGA::GUIEventAdapter::KeySymbol::KEY_Scroll_Lock;
        case GLFW_KEY_PAUSE: return osgGA::GUIEventAdapter::KeySymbol::KEY_Pause;
        case GLFW_KEY_INSERT: return osgGA::GUIEventAdapter::KeySymbol::KEY_Insert;
        case GLFW_KEY_HOME: return osgGA::GUIEventAdapter::KeySymbol::KEY_Home;
        case GLFW_KEY_PAGE_UP: return osgGA::GUIEventAdapter::KeySymbol::KEY_Page_Up;
        case GLFW_KEY_DELETE: return osgGA::GUIEventAdapter::KeySymbol::KEY_Delete;
        case GLFW_KEY_END: return osgGA::GUIEventAdapter::KeySymbol::KEY_End;
        case GLFW_KEY_PAGE_DOWN: return osgGA::GUIEventAdapter::KeySymbol::KEY_Page_Down;
        case GLFW_KEY_RIGHT: return osgGA::GUIEventAdapter::KeySymbol::KEY_Right;
        case GLFW_KEY_LEFT: return osgGA::GUIEventAdapter::KeySymbol::KEY_Left;
        case GLFW_KEY_DOWN: return osgGA::GUIEventAdapter::KeySymbol::KEY_Down;
        case GLFW_KEY_UP: return osgGA::GUIEventAdapter::KeySymbol::KEY_Up;
        default:
            treatAsChar = true; return (osgGA::GUIEventAdapter::KeySymbol)key;
        }
    }
}

class GLFWWindowingSystem : public osg::GraphicsContext::WindowingSystemInterface
{
public:
    GLFWWindowingSystem()
    {
        if (!glfwInit())
        {
            const char* errMsg = NULL; glfwGetError(&errMsg);
            OSG_WARN << "[GraphicsWindowGLFW] Failed: " << errMsg << std::endl;
        }
    }

    virtual unsigned int getNumScreens(const osg::GraphicsContext::ScreenIdentifier& screenIdentifier =
                                       osg::GraphicsContext::ScreenIdentifier())
    { int monitorCount = 0; glfwGetMonitors(&monitorCount); return monitorCount; }

    virtual void getScreenSettings(const osg::GraphicsContext::ScreenIdentifier& identifier,
                                   osg::GraphicsContext::ScreenSettings& resolution)
    {
        int monitorCount = 0; GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
        GLFWmonitor* primary = (identifier.screenNum < monitorCount) ?
            monitors[identifier.screenNum] : glfwGetPrimaryMonitor();

        const GLFWvidmode* mode = glfwGetVideoMode(primary);
        if (mode != NULL)
        {
            resolution.width = mode->width; resolution.height = mode->height;
            resolution.refreshRate = mode->refreshRate;
        }
        else
            OSG_WARN << "[GLFWWindowingSystem] getScreenSettings() failed" << std::endl;
    }

    virtual bool setScreenSettings(const osg::GraphicsContext::ScreenIdentifier& identifier,
                                   const osg::GraphicsContext::ScreenSettings& resolution)
    {
        OSG_WARN << "[GLFWWindowingSystem] setScreenSettings() not implemented" << std::endl;
        return false;
    }

    virtual void enumerateScreenSettings(const osg::GraphicsContext::ScreenIdentifier& identifier,
                                         osg::GraphicsContext::ScreenSettingsList& resolutionList)
    {
        int monitorCount = 0, modeCount = 0; GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
        GLFWmonitor* primary = (identifier.screenNum < monitorCount) ?
            monitors[identifier.screenNum] : glfwGetPrimaryMonitor();
        
        const GLFWvidmode* modes = glfwGetVideoModes(primary, &modeCount);
        for (int i = 0; i < modeCount; ++i)
        {
            osg::GraphicsContext::ScreenSettings settings;
            const GLFWvidmode& mode = modes[i];

            settings.width = mode.width; settings.height = mode.height;
            settings.refreshRate = mode.refreshRate;
            resolutionList.push_back(settings);
        }
    }

    virtual osg::GraphicsContext* createGraphicsContext(osg::GraphicsContext::Traits* traits)
    { return new GraphicsWindowGLFW(traits); }

protected:
    virtual ~GLFWWindowingSystem()
    {
        if (osg::Referenced::getDeleteHandler())
        {
            osg::Referenced::getDeleteHandler()->setNumFramesToRetainObjects(0);
            osg::Referenced::getDeleteHandler()->flushAll();
        }
        glfwTerminate();
    }
};

GraphicsWindowGLFW::GraphicsWindowGLFW(osg::GraphicsContext::Traits* traits)
:   _lastKey(0), _lastModKey(0), _lastChar(0), _valid(false), _realized(false)
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

GraphicsWindowGLFW::~GraphicsWindowGLFW()
{ releaseContextImplementation(); }

void GraphicsWindowGLFW::initialize()
{
    // FIXME: VERSE_WASM?
#if defined(VERSE_GLCORE)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    OSG_NOTICE << "[GraphicsWindowGLFW] Create native windowing system" << std::endl;
#elif defined(VERSE_EMBEDDED_GLES2)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    OSG_NOTICE << "[GraphicsWindowGLFW] Create EGL system with GLES2" << std::endl;
#elif defined(VERSE_EMBEDDED_GLES3)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    OSG_NOTICE << "[GraphicsWindowGLFW] Create EGL system with GLES3" << std::endl;
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    OSG_NOTICE << "[GraphicsWindowGLFW] Create native windowing system" << std::endl;
#endif

    // Create window
    int winX = 50, winY = 50, winW = 1280, winH = 720;
    GLFWmonitor* monitor = NULL; GLFWwindow* shared = NULL;
    if (_traits.valid())
    {
        glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
        glfwWindowHint(GLFW_RESIZABLE, _traits->supportsResize ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_DECORATED, _traits->windowDecoration ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_RED_BITS, _traits->red);
        glfwWindowHint(GLFW_GREEN_BITS, _traits->green);
        glfwWindowHint(GLFW_BLUE_BITS, _traits->blue);
        glfwWindowHint(GLFW_ALPHA_BITS, _traits->alpha);
        glfwWindowHint(GLFW_DEPTH_BITS, _traits->depth);
        glfwWindowHint(GLFW_STENCIL_BITS, _traits->stencil);
        glfwWindowHint(GLFW_DOUBLEBUFFER, _traits->doubleBuffer ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_STEREO, _traits->quadBufferStereo ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_SAMPLES, _traits->samples);

        winX = _traits->x; winY = _traits->y;
        winW = _traits->width; winH = _traits->height;
        if (_traits->screenNum > 0)
        {
            int monitorCount = 0; GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
            if (_traits->screenNum < monitorCount) monitor = monitors[_traits->screenNum];
        }
        if (_traits->sharedContext.valid())
        {
            GraphicsWindowGLFW* sharedWin = dynamic_cast<GraphicsWindowGLFW*>(_traits->sharedContext.get());
            shared = (sharedWin != NULL) ? sharedWin->_window : NULL;
        }
    }

    _window = glfwCreateWindow(winW, winH, "osgVerse::GraphicsWindowGLFW", monitor, shared);
    if (_window == NULL)
    {
        const char* errMsg = NULL; glfwGetError(&errMsg);
        OSG_WARN << "[GraphicsWindowGLFW] Failed: " << errMsg << std::endl; return;
    }
    else
    {
#define GET_EVENTER(w) \
    GraphicsWindowGLFW* gw = (GraphicsWindowGLFW*)glfwGetWindowUserPointer(w); osgGA::EventQueue* eq = gw->getEventQueue();

        glfwSetWindowUserPointer(_window, this);
        glfwSetWindowPosCallback(_window, [](GLFWwindow* w, int x, int y)
            { GET_EVENTER(w); eq->mouseMotion(x, y); });
        glfwSetWindowSizeCallback(_window, [](GLFWwindow* w, int width, int height)
            {
                GET_EVENTER(w); eq->windowResize(0, 0, width, height);
                gw->resized(0, 0, width, height);
            });
        glfwSetWindowCloseCallback(_window, [](GLFWwindow* w)
            { GET_EVENTER(w); eq->closeWindow(); });
        //glfwSetWindowFocusCallback(_window, [](GLFWwindow* w, int focused) {});
        //glfwSetWindowRefreshCallback(_window, [](GLFWwindow* w) {});

        glfwSetCursorPosCallback(_window, [](GLFWwindow* w, double x, double y)
            { GET_EVENTER(w); eq->mouseMotion(x, y); gw->_lastMousePosition.set(x, y); });
        glfwSetMouseButtonCallback(_window, [](GLFWwindow* w, int button, int action, int mods)
            {
                GET_EVENTER(w); int btn = 0;
                switch (button)
                {
                case GLFW_MOUSE_BUTTON_LEFT: btn = osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON; break;
                case GLFW_MOUSE_BUTTON_MIDDLE: btn = osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON; break;
                case GLFW_MOUSE_BUTTON_RIGHT: btn = 3/*osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON*/; break;
                default: btn = button; break;
                }
                if (action == GLFW_PRESS)
                    eq->mouseButtonPress(gw->_lastMousePosition[0], gw->_lastMousePosition[1], btn);
                else if (action == GLFW_RELEASE)
                    eq->mouseButtonRelease(gw->_lastMousePosition[0], gw->_lastMousePosition[1], btn);
            });
        glfwSetScrollCallback(_window, [](GLFWwindow* w, double x, double y)
            {
                GET_EVENTER(w);
                if (x < 0) eq->mouseScroll(osgGA::GUIEventAdapter::ScrollingMotion::SCROLL_LEFT);
                else if (x > 0) eq->mouseScroll(osgGA::GUIEventAdapter::ScrollingMotion::SCROLL_RIGHT);
                if (y < 0) eq->mouseScroll(osgGA::GUIEventAdapter::ScrollingMotion::SCROLL_DOWN);
                else if (y > 0) eq->mouseScroll(osgGA::GUIEventAdapter::ScrollingMotion::SCROLL_UP);
            });
        glfwSetKeyCallback(_window, [](GLFWwindow* w, int sym, int scancode, int action, int mods)
            {
                GET_EVENTER(w); bool asChar = false; int key = getKey(sym, asChar), mod = getModKey(mods);
                if (action == GLFW_PRESS)
                {
                    if (!asChar) eq->keyPress((osgGA::GUIEventAdapter::KeySymbol)key, mod);
                    gw->_lastKey = key; gw->_lastModKey = mod;
                }
                else if (action == GLFW_RELEASE)
                {
                    if (mods == 0) eq->getCurrentEventState()->setModKeyMask(0);
                    if (gw->_lastChar > 0) eq->keyRelease((osgGA::GUIEventAdapter::KeySymbol)gw->_lastChar, 0);
                    if (!asChar) eq->keyRelease((osgGA::GUIEventAdapter::KeySymbol)key, 0);
                    gw->_lastKey = 0; gw->_lastModKey = 0; gw->_lastChar = 0;
                }
            });
        glfwSetCharCallback(_window, [](GLFWwindow* w, unsigned int value)
            {
                GET_EVENTER(w); gw->_lastChar = value;
                eq->keyPress((osgGA::GUIEventAdapter::KeySymbol)value, gw->_lastModKey);
            });
    }
    _valid = true;
}

bool GraphicsWindowGLFW::realizeImplementation()
{
    _realized = false; if (!_valid) initialize(); if (!_valid) return false;
#if OSG_VERSION_GREATER_THAN(3, 5, 0)
    getEventQueue()->syncWindowRectangleWithGraphicsContext();
#endif

    int windowWidth = 1280, windowHeight = 720;
    if (_traits.valid()) { windowWidth = _traits->width; windowHeight = _traits->height; }
    glfwGetWindowSize(_window, &windowWidth, &windowHeight);

    getEventQueue()->windowResize(0, 0, windowWidth, windowHeight);
    resized(0, 0, windowWidth, windowHeight);

#if defined(OSG_GLES1_AVAILABLE) || defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
    getState()->setUseModelViewAndProjectionUniforms(true);
    getState()->setUseVertexAttributeAliasing(true);
#endif
    _realized = true; return true;
}

void GraphicsWindowGLFW::closeImplementation()
{
    if (!_valid) return; else _valid = false;
    glfwDestroyWindow(_window); _window = NULL;
}

bool GraphicsWindowGLFW::makeCurrentImplementation()
{
    if (!_valid) return false;
    glfwMakeContextCurrent(_window);
    return true;
}

bool GraphicsWindowGLFW::releaseContextImplementation()
{
    if (!_valid) return false;
    glfwMakeContextCurrent(NULL); return true;
}

void GraphicsWindowGLFW::swapBuffersImplementation()
{
    if (!_valid) return;
    glfwSwapBuffers(_window);
}

#if OSG_VERSION_GREATER_THAN(3, 1, 1)
bool GraphicsWindowGLFW::checkEvents()
#else
void GraphicsWindowGLFW::checkEvents()
#endif
{
#if OSG_VERSION_GREATER_THAN(3, 1, 1)
    if (!_realized) return false;
#else
    if (!_realized) return;
#endif
    glfwPollEvents();  // events will be handled in callbacks
#if OSG_VERSION_GREATER_THAN(3, 1, 1)
    return true;
#endif
}

void GraphicsWindowGLFW::grabFocus()
{
#if !defined(VERSE_WASM)
    if (_valid) glfwFocusWindow(_window);
#endif
}

void GraphicsWindowGLFW::grabFocusIfPointerInWindow()
{
#if !defined(VERSE_WASM)
    if (_valid) glfwFocusWindow(_window);
#endif
}

void GraphicsWindowGLFW::raiseWindow()
{
    if (_valid)
    {
        glfwSetWindowAttrib(_window, GLFW_FLOATING, GLFW_TRUE);
        glfwFocusWindow(_window);
        glfwSetWindowAttrib(_window, GLFW_FLOATING, GLFW_FALSE);
    }
}

void GraphicsWindowGLFW::requestWarpPointer(float x, float y)
{ if (_valid) glfwSetCursorPos(_window, x, y); }

bool GraphicsWindowGLFW::setWindowDecorationImplementation(bool flag)
{
    if (!_valid) return false;
#if !defined(VERSE_WASM)
    if (!flag) glfwSetWindowAttrib(_window, GLFW_DECORATED, GLFW_FALSE);
    else glfwSetWindowAttrib(_window, GLFW_DECORATED, GLFW_TRUE); return true;
#else
    return false;
#endif
}

bool GraphicsWindowGLFW::setWindowRectangleImplementation(int x, int y, int width, int height)
{
    if (!_valid) return false;
    glfwSetWindowPos(_window, x, y);
    glfwSetWindowSize(_window, width, height);
    return true;
}

void GraphicsWindowGLFW::setWindowName(const std::string& name)
{ if (_valid) glfwSetWindowTitle(_window, name.c_str()); }

void GraphicsWindowGLFW::setCursor(osgViewer::GraphicsWindow::MouseCursor cursor)
{
#if !defined(VERSE_WASM)
    GLFWcursor* glfwCursor = NULL;
    switch (cursor)
    {
    case TextCursor: glfwCursor = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR); break;
    case CrosshairCursor: glfwCursor = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR); break;
    case WaitCursor: glfwCursor = glfwCreateStandardCursor(GLFW_ARROW_CURSOR); break;
    case HandCursor: glfwCursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR); break;
    case NoCursor: glfwCursor = glfwCreateStandardCursor(GLFW_ARROW_CURSOR); break;
    case LeftRightCursor: glfwCursor = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR); break;
    case UpDownCursor: glfwCursor = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR); break;
    case TopLeftCorner: case BottomRightCorner: glfwCursor = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR); break;
    case TopRightCorner: case BottomLeftCorner: glfwCursor = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR); break;
    default: glfwCursor = glfwCreateStandardCursor(GLFW_ARROW_CURSOR); break;
    }
    glfwSetCursor(_window, glfwCursor);
#endif
}

void GraphicsWindowGLFW::setSyncToVBlank(bool on)
{ glfwSwapInterval(on ? 1 : 0); }

extern "C" OSGVERSE_RW_EXPORT void graphicswindow_GLFW(void) {}
#if OSG_VERSION_GREATER_THAN(3, 5, 1)
static osg::WindowingSystemInterfaceProxy<GLFWWindowingSystem> s_proxy_GLFWWindowingSystem("GLFW");
#endif

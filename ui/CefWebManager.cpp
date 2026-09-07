#include "CefWebManager.h"
#include <include/cef_app.h>
#include <include/cef_browser.h>
#include <include/cef_client.h>
#include <include/cef_render_handler.h>
#include <include/cef_life_span_handler.h>
#include <include/cef_load_handler.h>
#include <include/cef_request_handler.h>
#include <include/cef_v8.h>
#include <include/cef_command_line.h>
#include <include/wrapper/cef_helpers.h>

#include <OpenThreads/Mutex>
#include <OpenThreads/ScopedLock>
#include <osg/Image>
#include <osgViewer/View>
#include <algorithm>
#include <vector>
#ifdef _WIN32
#   include <direct.h>
#else
#   include <unistd.h>
#endif

#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT 0x80E1
#endif
using namespace osgVerse;

namespace osgVerse
{
    // Forward declarations
    class WebApp;
    class WebClient;

    std::string getCurrentDir()
    {
        char buf[4096];
    #ifdef _WIN32
        return _getcwd(buf, sizeof(buf)) ? std::string(buf) : std::string();
    #else
        return getcwd(buf, sizeof(buf)) ? std::string(buf) : std::string();
    #endif
    }

    // ========================================================================
    // JavaScript V8 Handler (executed in renderer process)
    // ========================================================================
    class JSV8Handler : public CefV8Handler
    {
    public:
        virtual bool Execute(const CefString& name, CefRefPtr<CefV8Value> object,
                             const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval,
                             CefString& exception) override
        {
            if (name == "query" && arguments.size() >= 2)
            {
                CefString cbName = arguments[0]->GetStringValue();
                CefString message = arguments[1]->GetStringValue();
                CefRefPtr<CefBrowser> browser = CefV8Context::GetCurrentContext()->GetBrowser();
                if (browser)
                {
                    CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("JSQuery");
                    CefRefPtr<CefListValue> args = msg->GetArgumentList();
                    args->SetString(0, cbName); args->SetString(1, message);
                    
                    //browser->SendProcessMessage(PID_BROWSER, msg);
                    CefRefPtr<CefFrame> frame = browser->GetMainFrame();
                    if (frame) frame->SendProcessMessage(PID_BROWSER, msg);
                }
                retval = CefV8Value::CreateBool(true);
                return true;
            }
            return false;
        }

        IMPLEMENT_REFCOUNTING(JSV8Handler);
    };

    // ========================================================================
    // CEF Render Process Handler (registers JS bindings)
    // ========================================================================
    class WebRenderProcessHandler : public CefRenderProcessHandler
    {
    public:
        virtual void OnContextCreated(CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame,
                                      CefRefPtr<CefV8Context> context) override
        {
            CefRefPtr<CefV8Value> obj = CefV8Value::CreateObject(nullptr, nullptr);
            CefRefPtr<CefV8Handler> handler = new JSV8Handler();
            CefRefPtr<CefV8Value> func = CefV8Value::CreateFunction("query", handler);
            obj->SetValue("query", func, V8_PROPERTY_ATTRIBUTE_NONE);
            context->GetGlobal()->SetValue("osgVerse", obj, V8_PROPERTY_ATTRIBUTE_NONE);
        }

        IMPLEMENT_REFCOUNTING(WebRenderProcessHandler);
    };

    // ========================================================================
    // CEF App
    // ========================================================================
    class WebApp : public CefApp
    {
    public:
        virtual CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override
        { return new WebRenderProcessHandler(); }

        virtual void OnBeforeCommandLineProcessing(const CefString& process_type,
                                                   CefRefPtr<CefCommandLine> command_line) override
        {   // Essential for off-screen rendering to get CPU-accessible pixel buffer
            command_line->AppendSwitch("no-sandbox");
            command_line->AppendSwitch("disable-gpu-sandbox");
            command_line->AppendSwitch("disable-gpu");
            command_line->AppendSwitch("disable-gpu-compositing");
            command_line->AppendSwitch("disable-software-rasterizer");
            command_line->AppendSwitch("in-process-gpu");
            command_line->AppendSwitch("enable-begin-frame-scheduling");
            command_line->AppendSwitchWithValue("autoplay-policy", "no-user-gesture-required");
        }

        IMPLEMENT_REFCOUNTING(WebApp);
    };

    // ========================================================================
    // PIMPL: CefWebViewImpl
    // ========================================================================
    class CefWebViewImpl
    {
    public:
        CefRefPtr<CefBrowser> browser;
        osg::ref_ptr<osg::Texture2D> texture;
        std::vector<unsigned char> pixelBuffer;
        OpenThreads::Mutex mutex;

        struct JSMessage { std::string name; std::string data; };
        std::vector<JSMessage> pendingJSMessages;
        std::map<std::string, std::function<void(const std::string&)>> jsCallbacks;

        int width, height;
        bool dirty, isLoading, canGoBack, canGoForward;

        CefWebViewImpl()
        : width(0), height(0), dirty(false), isLoading(false), canGoBack(false), canGoForward(false) {}
    };

    // ========================================================================
    // PIMPL: CefWebManagerImpl
    // ========================================================================
    class CefWebManagerImpl
    {
    public:
        std::vector<osg::ref_ptr<CefWebView>> views;
        CefRefPtr<WebApp> app;
        bool initialized;
        CefWebManagerImpl() : initialized(false) {}
    };

    // ========================================================================
    // WebClient: CEF Client implementation
    // ========================================================================
    class WebClient : public CefClient,
                      public CefRenderHandler,
                      public CefLifeSpanHandler,
                      public CefLoadHandler,
                      public CefRequestHandler
    {
    public:
        explicit WebClient(CefWebViewImpl* view) : _view(view) {}

        // --- CefClient ---
        virtual CefRefPtr<CefRenderHandler> GetRenderHandler() override { return this; }
        virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
        virtual CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
        virtual CefRefPtr<CefRequestHandler> GetRequestHandler() override { return this; }

        // --- CefRenderHandler (Off-Screen Rendering) ---
        virtual void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_view->mutex);
            rect = CefRect(0, 0, _view->width, _view->height);
        }

        virtual void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type,
                             const RectList& dirtyRects, const void* buffer,
                             int width, int height) override
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_view->mutex);
            size_t size = (size_t)width * height * 4;
            if (_view->pixelBuffer.size() != size) _view->pixelBuffer.resize(size);
            memcpy(_view->pixelBuffer.data(), buffer, size);
            _view->width = width; _view->height = height; _view->dirty = true;
        }

        // --- CefLifeSpanHandler ---
        virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) override {}
        virtual bool DoClose(CefRefPtr<CefBrowser> browser) override { return false; }

        virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) override
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_view->mutex);
            _view->browser = nullptr;
        }

        // --- CefLoadHandler ---
        virtual void OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading,
                                          bool canGoBack, bool canGoForward) override
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_view->mutex);
            _view->isLoading = isLoading;
            _view->canGoBack = canGoBack;
            _view->canGoForward = canGoForward;
        }

        // --- CefRequestHandler (Process Messages from Renderer) ---
        virtual bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                              CefProcessId source_process, CefRefPtr<CefProcessMessage> message) override
        {
            if (message->GetName() == "JSQuery")
            {
                std::string name = message->GetArgumentList()->GetString(0).ToString();
                std::string data = message->GetArgumentList()->GetString(1).ToString();
                OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_view->mutex);
                CefWebViewImpl::JSMessage msg; msg.name = name; msg.data = data;
                _view->pendingJSMessages.push_back(msg);
                return true;
            }
            return false;
        }

        IMPLEMENT_REFCOUNTING(WebClient);

    private:
        CefWebViewImpl* _view;
    };
}

// ========================================================================
// CefWebManager
// ========================================================================
CefWebManager* CefWebManager::s_instance = nullptr;

CefWebManager::CefWebManager() : _impl(new CefWebManagerImpl)
{ s_instance = this; }

CefWebManager::~CefWebManager()
{
    shutdown(); delete _impl;
    s_instance = nullptr;
}

CefWebManager* CefWebManager::instance()
{
    if (!s_instance) s_instance = new CefWebManager;
    return s_instance;
}

int CefWebManager::executeProcess(osg::ArgumentParser& arguments)
{
#ifdef _WIN32
    CefMainArgs main_args(GetModuleHandle(NULL));
#else
    CefMainArgs main_args(arguments.argc(), arguments.argv());
#endif
    CefRefPtr<WebApp> app(new WebApp);
    return CefExecuteProcess(main_args, app.get(), nullptr);
}

bool CefWebManager::initialize(osg::ArgumentParser& arguments, const std::string& cachePath)
{
    if (_impl->initialized) return true;
    else _impl->app = new WebApp;

    CefSettings settings;
    settings.no_sandbox = true;
    settings.windowless_rendering_enabled = true;
    settings.multi_threaded_message_loop = false;  // Manual message loop via CefDoMessageLoopWork
    if (!cachePath.empty())
        CefString(&settings.cache_path) = getCurrentDir() + "/" + cachePath;

#ifdef _WIN32
    CefMainArgs main_args(GetModuleHandle(NULL));
#else
    CefMainArgs main_args(arguments.argc(), arguments.argv());
#endif
    bool result = CefInitialize(main_args, settings, _impl->app.get(), nullptr);
    _impl->initialized = result; return result;
}

void CefWebManager::shutdown()
{
    if (!_impl->initialized) return;
    for (auto& view : _impl->views)
    {
        if (view->_impl && view->_impl->browser)
            view->_impl->browser->GetHost()->CloseBrowser(true);
    }
    _impl->views.clear(); CefShutdown();
    _impl->initialized = false;
}

bool CefWebManager::isInitialized() const
{ return _impl->initialized; }

CefWebView* CefWebManager::createView(int width, int height, const std::string& url)
{
    if (width < 1 || height < 1) return nullptr;
    if (!_impl->initialized) return nullptr;

    CefWebView* view = new CefWebView;
    view->_impl = new CefWebViewImpl;
    view->_impl->width = width;
    view->_impl->height = height;

    // Create placeholder texture
    view->_impl->texture = new osg::Texture2D;
    view->_impl->texture->setResizeNonPowerOfTwoHint(false);
    view->_impl->texture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
    view->_impl->texture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
    view->_impl->texture->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP_TO_EDGE);
    view->_impl->texture->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP_TO_EDGE);

    CefWindowInfo window_info; window_info.SetAsWindowless(0);
    CefBrowserSettings browser_info; browser_info.windowless_frame_rate = 60;

    CefRefPtr<WebClient> client(new WebClient(view->_impl));
    CefRefPtr<CefBrowser> browser = CefBrowserHost::CreateBrowserSync(
        window_info, client.get(), url, browser_info, nullptr, nullptr);

    if (browser) view->_impl->browser = browser;
    _impl->views.push_back(view); return view;
}

void CefWebManager::destroyView(CefWebView* view)
{
    if (!view) return;
    auto it = std::find(_impl->views.begin(), _impl->views.end(), view);
    if (it != _impl->views.end())
    {
        if (view->_impl && view->_impl->browser)
            view->_impl->browser->GetHost()->CloseBrowser(true);
        _impl->views.erase(it);
    }
}

void CefWebManager::update()
{
    if (!_impl->initialized) return; else CefDoMessageLoopWork();
    for (auto& view : _impl->views) view->updateTexture();
}

// ========================================================================
// CefWebView
// ========================================================================
CefWebView::CefWebView() : _impl(nullptr) {}
CefWebView::~CefWebView() { delete _impl; }

osg::Texture2D* CefWebView::getTexture() const
{ return _impl ? _impl->texture.get() : nullptr; }

void CefWebView::loadURL(const std::string& url)
{
    if (_impl && _impl->browser)
        _impl->browser->GetMainFrame()->LoadURL(url);
}

void CefWebView::loadString(const std::string& html)
{
    if (_impl && _impl->browser)
    {
        //_impl->browser->GetMainFrame()->LoadString(html, baseUrl);
        std::string dataUrl = "data:text/html;charset=utf-8," + html;
        _impl->browser->GetMainFrame()->LoadURL(dataUrl);
    }
}

void CefWebView::reload()
{ if (_impl && _impl->browser) _impl->browser->Reload(); }

void CefWebView::stopLoad()
{ if (_impl && _impl->browser) _impl->browser->StopLoad(); }

void CefWebView::goBack()
{ if (_impl && _impl->browser) _impl->browser->GoBack(); }

void CefWebView::goForward()
{ if (_impl && _impl->browser) _impl->browser->GoForward(); }

bool CefWebView::canGoBack() const
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_impl->mutex);
    return _impl ? _impl->canGoBack : false;
}

bool CefWebView::canGoForward() const
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_impl->mutex);
    return _impl ? _impl->canGoForward : false;
}

bool CefWebView::isLoading() const
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_impl->mutex);
    return _impl ? _impl->isLoading : false;
}

void CefWebView::setFocus(bool focus)
{ if (_impl && _impl->browser) _impl->browser->GetHost()->SetFocus(focus); }

void CefWebView::setSize(int width, int height)
{
    if (!_impl) return;
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_impl->mutex);
        _impl->width = width; _impl->height = height;
    }
    if (_impl->browser)
        _impl->browser->GetHost()->WasResized();
}

void CefWebView::executeJavaScript(const std::string& code)
{
    if (_impl && _impl->browser)
        _impl->browser->GetMainFrame()->ExecuteJavaScript(code, "", 0);
}

void CefWebView::bindJavaScriptCallback(const std::string& name, std::function<void(const std::string&)> cb)
{
    if (!_impl) return;
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_impl->mutex);
    _impl->jsCallbacks[name] = cb;
}

void CefWebView::sendMouseMove(int x, int y, bool leftDown)
{
    if (!_impl || !_impl->browser) return;
    CefMouseEvent event; event.x = x; event.y = y;
    if (leftDown) event.modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
    _impl->browser->GetHost()->SendMouseMoveEvent(event, false);
}

void CefWebView::sendMouseClick(int x, int y, int button, bool down, int clickCount)
{
    if (!_impl || !_impl->browser) return;
    CefMouseEvent event; event.x = x; event.y = y;
    CefBrowserHost::MouseButtonType cefButton =
        (button == 2) ? MBT_RIGHT : (button == 1) ? MBT_MIDDLE : MBT_LEFT;
    _impl->browser->GetHost()->SendMouseClickEvent(event, cefButton, !down, clickCount);
}

void CefWebView::sendMouseWheel(int x, int y, float deltaX, float deltaY)
{
    if (!_impl || !_impl->browser) return;
    CefMouseEvent event; event.x = x; event.y = y;
    _impl->browser->GetHost()->SendMouseWheelEvent(event, (int)deltaX, (int)deltaY);
}

void CefWebView::sendKeyEvent(int nativeKeyCode, bool down, int modifiers)
{
    if (!_impl || !_impl->browser) return;
    CefKeyEvent event; event.modifiers = modifiers;
    event.type = down ? KEYEVENT_RAWKEYDOWN : KEYEVENT_KEYUP;
    event.native_key_code = nativeKeyCode;
    _impl->browser->GetHost()->SendKeyEvent(event);
}

void CefWebView::sendCharEvent(int charCode)
{
    if (!_impl || !_impl->browser) return;
    CefKeyEvent event; event.type = KEYEVENT_CHAR;
    event.character = charCode; event.unmodified_character = charCode;
    _impl->browser->GetHost()->SendKeyEvent(event);
}

void CefWebView::updateTexture()
{
    // Process pending JS messages...
    if (!_impl) return;
    std::vector<CefWebViewImpl::JSMessage> messages;
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_impl->mutex);
        messages.swap(_impl->pendingJSMessages);
    }

    for (auto& msg : messages)
    {
        auto it = _impl->jsCallbacks.find(msg.name);
        if (it != _impl->jsCallbacks.end())
            it->second(msg.data);
    }
    if (!_impl->dirty) return;

    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_impl->mutex);
    if (!_impl->dirty) return;

    if (!_impl->texture)
    {
        _impl->texture = new osg::Texture2D;
        _impl->texture->setResizeNonPowerOfTwoHint(false);
        _impl->texture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        _impl->texture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        _impl->texture->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP_TO_EDGE);
        _impl->texture->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP_TO_EDGE);
    }
    
    osg::ref_ptr<osg::Image> image = _impl->texture->getImage();
    if (!image || (image.valid() && (image->s() != _impl->width || image->t() != _impl->height)))
    {
        image = new osg::Image; _impl->texture->setImage(image.get());
        image->allocateImage(_impl->width, _impl->height, 1, GL_BGRA_EXT, GL_UNSIGNED_BYTE);
        image->setInternalTextureFormat(GL_RGBA8);
    }

    if (!_impl->pixelBuffer.empty())
        memcpy(image->data(), _impl->pixelBuffer.data(), _impl->pixelBuffer.size());
    image->dirty(); _impl->dirty = false; 
}

// ========================================================================
// CefWebEventHandler
// ========================================================================
CefWebEventHandler::CefWebEventHandler(CefWebView* view)
:   _view(view), _lastX(0), _lastY(0), _width(0), _height(0) {}

void CefWebEventHandler::setViewportSize(int w, int h)
{ _width = w; _height = h; }

void CefWebEventHandler::handleMouseClick(int x, int y, int button, int action)
{
    bool down = (action == osgGA::GUIEventAdapter::PUSH) ||
                (action == osgGA::GUIEventAdapter::DOUBLECLICK);
    int cefButton = (button == osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON) ? 2 :
                    (button == osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON) ? 1 : 0;
    int clicks = (action == osgGA::GUIEventAdapter::DOUBLECLICK) ? 2 : 1;
    _view->sendMouseClick(x, y, cefButton, down, clicks);
    _lastX = x; _lastY = y;
}

void CefWebEventHandler::handleMouseMove(int x, int y, int button)
{
    bool leftDown = (button & osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON) != 0;
    _view->sendMouseMove(x, y, leftDown);
    _lastX = x; _lastY = y;
}

void CefWebEventHandler::handleWheel(float dx, float dy)
{ _view->sendMouseWheel(_lastX, _lastY, dx, dy); }

void CefWebEventHandler::handleKey(int key, int modkey, int action)
{
    bool down = (action == osgGA::GUIEventAdapter::KEYDOWN);
    int cefKey = mapOSGKeyToCEF(key); if (cefKey <= 0) return;
    int modifiers = mapOSGModifiersToCEF(modkey);
    _view->sendKeyEvent(cefKey, down, modifiers);
}

bool CefWebEventHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
    // OSG Y is bottom-up, CEF Y is top-down
    int x = (int)ea.getX(), y = _height - (int)ea.getY();
    if (!_view) return false;

    osgViewer::View* view = dynamic_cast<osgViewer::View*>(&aa);
    if (view && view->getCamera())
    {
        osg::Viewport* vp = view->getCamera()->getViewport();
        if (vp) { _width = (int)vp->width(); _height = (int)vp->height(); }
    }

    switch (ea.getEventType())
    {
    case osgGA::GUIEventAdapter::PUSH: case osgGA::GUIEventAdapter::RELEASE:
    case osgGA::GUIEventAdapter::DOUBLECLICK:
        handleMouseClick(x, y, ea.getButton(), ea.getEventType()); break;
    case osgGA::GUIEventAdapter::MOVE: case osgGA::GUIEventAdapter::DRAG:
        handleMouseClick(x, y, ea.getButtonMask(), ea.getEventType()); break;
    case osgGA::GUIEventAdapter::SCROLL:
        {
            float dy = 0.0f, dx = 0.0f;
            if (ea.getScrollingMotion() == osgGA::GUIEventAdapter::SCROLL_UP) dy = 100.0f;
            else if (ea.getScrollingMotion() == osgGA::GUIEventAdapter::SCROLL_DOWN) dy = -100.0f;
            else if (ea.getScrollingMotion() == osgGA::GUIEventAdapter::SCROLL_LEFT) dx = -100.0f;
            else if (ea.getScrollingMotion() == osgGA::GUIEventAdapter::SCROLL_RIGHT) dx = 100.0f;
            handleWheel(dx, dy); break;
        }
    case osgGA::GUIEventAdapter::KEYDOWN: case osgGA::GUIEventAdapter::KEYUP:
        handleKey(ea.getKey(), ea.getModKeyMask(), ea.getEventType()); break;
    default: break;
    }
    return false;
}

int CefWebEventHandler::mapOSGKeyToCEF(int osgKey)
{
    if (osgKey >= 'a' && osgKey <= 'z') return osgKey - 'a' + 'A';
    if (osgKey >= '0' && osgKey <= '9') return osgKey;
    switch (osgKey)
    {
    case osgGA::GUIEventAdapter::KEY_BackSpace: return 0x08;
    case osgGA::GUIEventAdapter::KEY_Tab:       return 0x09;
    case osgGA::GUIEventAdapter::KEY_Return:    return 0x0D;
    case osgGA::GUIEventAdapter::KEY_Escape:    return 0x1B;
    case osgGA::GUIEventAdapter::KEY_Space:     return 0x20;
    case osgGA::GUIEventAdapter::KEY_Left:      return 0x25;
    case osgGA::GUIEventAdapter::KEY_Up:        return 0x26;
    case osgGA::GUIEventAdapter::KEY_Right:     return 0x27;
    case osgGA::GUIEventAdapter::KEY_Down:      return 0x28;
    case osgGA::GUIEventAdapter::KEY_Delete:    return 0x2E;
    case osgGA::GUIEventAdapter::KEY_Home:      return 0x24;
    case osgGA::GUIEventAdapter::KEY_End:       return 0x23;
    case osgGA::GUIEventAdapter::KEY_Page_Up:   return 0x21;
    case osgGA::GUIEventAdapter::KEY_Page_Down: return 0x22;
    case osgGA::GUIEventAdapter::KEY_Shift_L:
    case osgGA::GUIEventAdapter::KEY_Shift_R:   return 0x10;
    case osgGA::GUIEventAdapter::KEY_Control_L:
    case osgGA::GUIEventAdapter::KEY_Control_R: return 0x11;
    case osgGA::GUIEventAdapter::KEY_Alt_L:
    case osgGA::GUIEventAdapter::KEY_Alt_R:     return 0x12;
    }
    return osgKey;
}

int CefWebEventHandler::mapOSGModifiersToCEF(int osgMod)
{
    int cefMod = 0;
    if (osgMod & osgGA::GUIEventAdapter::MODKEY_SHIFT) cefMod |= EVENTFLAG_SHIFT_DOWN;
    if (osgMod & osgGA::GUIEventAdapter::MODKEY_CTRL)  cefMod |= EVENTFLAG_CONTROL_DOWN;
    if (osgMod & osgGA::GUIEventAdapter::MODKEY_ALT)   cefMod |= EVENTFLAG_ALT_DOWN;
    return cefMod;
}

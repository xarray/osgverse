#ifndef OSGVERSE_CEFWEB_MANAGER_HPP
#define OSGVERSE_CEFWEB_MANAGER_HPP

#include <osg/ArgumentParser>
#include <osg/Texture2D>
#include <osgGA/GUIEventHandler>
#include <functional>
#include <string>
#include <map>

namespace osgVerse
{

    class CefWebView;

    /** libCEF DLL wrapper library should be compiled from its prebuilt package (with /MD under MSVC).
        After that, put all files in cefsimple (except cefsimple itself) to osgVerse executable folder
     */
    class CefWebManager : public osg::Referenced
    {
    public:
        CefWebManager();
        static CefWebManager* instance();

        /**
         * Execute CEF subprocess (renderer, GPU, etc.).
         * MUST be called at the very beginning of main() before any other CEF code.
         * If return value >= 0, this is a subprocess and should exit immediately with that code.
         */
        static int executeProcess(osg::ArgumentParser& arguments);

        /**
         * Initialize CEF browser process.
         * Call after executeProcess() in the main browser process.
         */
        bool initialize(osg::ArgumentParser& arguments, const std::string& cachePath = "");

        /** Shutdown CEF and release all resources */
        void shutdown();

        /** Check if CEF is initialized */
        bool isInitialized() const;

        /** Create an off-screen web view */
        CefWebView* createView(int width, int height, const std::string& url = "about:blank");

        /** Destroy a web view and release its browser instance */
        void destroyView(CefWebView* view);

        /** Call per frame in the main thread to drive CEF message loop and update textures */
        void update();

    protected:
        virtual ~CefWebManager();

        static CefWebManager* s_instance;
        class CefWebManagerImpl* _impl;
    };

    class CefWebView : public osg::Referenced
    {
    public:
        /** Get the OSG texture containing the latest rendered frame */
        osg::Texture2D* getTexture() const;

        /** Navigation */
        void loadURL(const std::string& url);
        void loadString(const std::string& html);
        void reload();
        void stopLoad();
        void goBack();
        void goForward();

        bool canGoBack() const;
        bool canGoForward() const;
        bool isLoading() const;

        /** Focus */
        void setFocus(bool focus);

        /** Resize the view (triggers re-rendering) */
        void setSize(int width, int height);

        /** Execute JavaScript code in the main frame (C++ -> JS) */
        void executeJavaScript(const std::string& code);

        /**
         * Bind a C++ callback to a JavaScript query.
         * In JS: window.osgVerse.query("name", "json message");
         * The callback will be triggered on the OSG main thread.
         */
        void bindJavaScriptCallback(const std::string& name, std::function<void(const std::string&)> cb);

        /** Input events (usually called by CefWebEventHandler, but can be used directly) */
        void sendMouseMove(int x, int y, bool leftDown = false);
        void sendMouseClick(int x, int y, int button, bool down, int clickCount = 1);
        void sendMouseWheel(int x, int y, float deltaX, float deltaY);
        void sendKeyEvent(int nativeKeyCode, bool down, int modifiers = 0);
        void sendCharEvent(int charCode);

        /** Update texture from CEF paint buffer. Called automatically by CefWebManager::update() */
        void updateTexture();

    protected:
        CefWebView();
        virtual ~CefWebView();

        friend class CefWebManager;
        class CefWebViewImpl* _impl;
    };

    class CefWebEventHandler : public osgGA::GUIEventHandler
    {
    public:
        CefWebEventHandler(CefWebView* view);
        void setViewportSize(int w, int h);

        void handleMouseClick(int x, int y, int button, int action);
        void handleMouseMove(int x, int y, int button);
        void handleWheel(float dx, float dy);
        void handleKey(int key, int modkey, int action);

        // Not recommended. Should better use handle*() functions after intersection test
        virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa);

    protected:
        static int mapOSGKeyToCEF(int osgKey);
        static int mapOSGModifiersToCEF(int osgMod);

        osg::observer_ptr<CefWebView> _view;
        int _lastX, _lastY, _width, _height;
    };

}

#endif

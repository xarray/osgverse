// Prevent GLES2/gl2.h to redefine gl* functions
#define GL_GLES_PROTOTYPES 0

#include <GL/glew.h>
#include <osg/Version>
#include <osg/Camera>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <imgui/imgui.h>
#if defined(OSG_GLES1_AVAILABLE) || defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
    // Specified GLES version of IMGUI backend?
#else
#   include <imgui/imgui_impl_opengl2.h>
#endif
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/ImGuizmo.h>
#include "ImGui.h"
#include "ImGui.Internal.h"
#include "ImGui.Styles.h"
#include "pipeline/Utilities.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
using namespace osgVerse;

extern void StyleColorsVisualStudio(ImGuiStyle* dst = (ImGuiStyle*)0);
extern void StyleColorsSonicRiders(ImGuiStyle* dst = (ImGuiStyle*)0);
extern void StyleColorsLightBlue(ImGuiStyle* dst = (ImGuiStyle*)0);
extern void StyleColorsTransparent(ImGuiStyle* dst = (ImGuiStyle*)0);
static bool s_useImguiLoaderGL3 = false;

void newImGuiFrame(osg::RenderInfo& renderInfo, double& time, std::function<void(ImGuiIO&)> func)
{
    ImGuiContext* context = ImGui::GetCurrentContext();
    if (!context) return; else if (time < 0.0f)
    {
        glewInit(); time = 0.0f;
        s_useImguiLoaderGL3 = glewIsSupported("GL_VERSION_3_0");
#if defined(OSG_GLES1_AVAILABLE) || defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
        ImGui_ImplOpenGL3_Init();
#else
        if (s_useImguiLoaderGL3) ImGui_ImplOpenGL3_Init();
        else ImGui_ImplOpenGL2_Init();
#endif
    }

#if defined(OSG_GLES1_AVAILABLE) || defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
    ImGui_ImplOpenGL3_NewFrame();
#else
    if (s_useImguiLoaderGL3) ImGui_ImplOpenGL3_NewFrame();
    else ImGui_ImplOpenGL2_NewFrame();
#endif

    ImGuiIO& io = ImGui::GetIO();
    if (renderInfo.getView() != NULL)
    {
        osg::Viewport* viewport = (renderInfo.getCurrentCamera() != NULL)
            ? renderInfo.getCurrentCamera()->getViewport() : NULL;
        if (!viewport) viewport = renderInfo.getView()->getCamera()->getViewport();
        if (!viewport) { OSG_FATAL << "[ImGuiManager] Empty viewport!\n"; return; }
        io.DisplaySize = ImVec2(viewport->width(), viewport->height());

        double currentTime = renderInfo.getView()->getFrameStamp()->getSimulationTime();
        io.DeltaTime = currentTime - time + 0.0000001;
        time = currentTime; func(io);
    }
    else
    {
        OSG_FATAL << "[ImGuiManager] No view provided!\n";
    }
    ImGui::NewFrame();

    ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
    ImGuizmo::BeginFrame();
}

void endImGuiFrame(osg::RenderInfo& renderInfo, ImGuiManager* manager,
    std::map<std::string, ImTextureID>& textureIdList,
    std::function<void(ImGuiContentHandler*, ImGuiContext*)> func)
{
    ImGuiContext* context = ImGui::GetCurrentContext();
    if (manager && context)
    {
        ImGuiContentHandler* v = manager->getContentHandler();
        if (v)
        {
            std::map<std::string, osg::ref_ptr<osg::Texture2D>>& tList = manager->getTextures();
            for (std::map<std::string, osg::ref_ptr<osg::Texture2D>>::iterator itr = tList.begin();
                 itr != tList.end(); ++itr)
            {
                osg::Texture2D* tex2D = itr->second.get();
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
                if (tex2D->isDirty(renderInfo.getContextID())) tex2D->apply(*renderInfo.getState());
#else
                if (tex2D->getTextureParameterDirty(renderInfo.getContextID()) > 0)
                    tex2D->apply(*renderInfo.getState());
#endif

                osg::Texture::TextureObject* tObj = tex2D->getTextureObject(renderInfo.getContextID());
                if (tObj) textureIdList[itr->first] = (ImTextureID)tObj->id();
            }
            func(v, context);
        }
    }
    else return;

    ImGui::Render();
#if defined(OSG_GLES1_AVAILABLE) || defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#else
    if (s_useImguiLoaderGL3)
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    else
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
#endif
}

void startImGuiContext(ImGuiManager* manager, std::map<std::string, ImFont*>& fonts)
{
    ImGui::CreateContext();
    int style = 1;  // FIXME
    switch (style)
    {
    case 1: ImGui::StyleColorsDark(); break;
    case 2: ImGui::StyleColorsLight(); break;
    case 10: StyleColorsVisualStudio(); break;
    case 11: StyleColorsSonicRiders(); break;
    case 12: StyleColorsLightBlue(); break;
    case 13: StyleColorsTransparent(); break;
    default: ImGui::StyleColorsClassic(); break;
    }

    static std::string s_iniFilename;
    ImGuiIO& io = ImGui::GetIO();
    s_iniFilename = ImGuiManager::defaultSettingsPath();
    if (!s_iniFilename.empty() && osgDB::makeDirectoryForFile(s_iniFilename))
        io.IniFilename = s_iniFilename.c_str();
    else
        io.IniFilename = NULL;
    fonts[""] = io.Fonts->AddFontDefault();

#if defined(__APPLE__)
    ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
    pio.Platform_SetClipboardTextFn = [](ImGuiContext*, const char* text)
        {
            FILE* p = popen("/usr/bin/pbcopy", "w");
            if (p) { if (text) fwrite(text, 1, strlen(text), p); pclose(p); }
        };

    pio.Platform_GetClipboardTextFn = [](ImGuiContext*) -> const char*
        {
            static std::string buffer; buffer.clear();
            FILE* p = popen("/usr/bin/pbpaste", "r"); if (!p) return NULL;
            char tmp[4096]; size_t n = 0;
            while ((n = fread(tmp, 1, sizeof(tmp), p)) > 0) buffer.append(tmp, n);
            pclose(p); return buffer.c_str();
        };
#endif

    std::string fontData = manager->getChineseSimplifiedFont();
    if (!fontData.empty())
    {
        fonts[osgDB::getStrippedName(fontData)] = io.Fonts->AddFontFromFileTTF(
            fontData.c_str(), 20.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
    }
    //io.Fonts->Build();

    /*unsigned char* pixels; int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    osg::Image* img = new osg::Image;
    img->setImage(width, height, 1, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, pixels, osg::Image::NO_DELETE);
    osgDB::writeImageFile(*img, "test.png");*/
}

namespace
{
    class ImGuiHandler : public osgGA::GUIEventHandler
    {
    public:
        std::map<std::string, ImFont*> _fonts;
        ImGuiInputQueue _input;
        ImGuiHandler() : _started(false), _released(false) {}

        void start(ImGuiManager* manager)
        { if (!_started) startImGuiContext(manager, _fonts); _started = true; }

        void release(ImGuiManager* manager)
        {
            // Must be idempotent: OSG may trigger releaseGLObjects() on the camera
            // more than once (per-context teardown + manager shutdown), and a second
            // ImGui_ImplOpenGL3_Shutdown() dereferences a deleted backend (crash).
            if (_released || !_started) return;
            _released = true;
#if defined(OSG_GLES1_AVAILABLE) || defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
            ImGui_ImplOpenGL3_Shutdown();
#else
            if (s_useImguiLoaderGL3) ImGui_ImplOpenGL3_Shutdown();
            else ImGui_ImplOpenGL2_Shutdown();
#endif
        }

        void drain(ImGuiIO& io)
        { ImGuiInputEvent::applyImGuiInputEvents(io, _input.takeAll()); }

        void publishCapture()
        {
            if (!ImGui::GetCurrentContext()) return; ImGuiIO& io = ImGui::GetIO();
            _input.publishCapture(io.WantCaptureMouse || ImGuizmo::IsUsing(), io.WantCaptureKeyboard);
        }

        virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
        {
            const bool wantCaptureMouse = _input.wantsMouse();
            const bool wantCaptureKeyboard = _input.wantsKeyboard();
            switch (ea.getEventType())
            {
            case osgGA::GUIEventAdapter::KEYDOWN:
            case osgGA::GUIEventAdapter::KEYUP:
                {
                    const bool isKeyDown = ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN;
                    _input.push(ImGuiInputEvent::keyEvent(ea.getKey(), isKeyDown, ea.getModKeyMask()));
                    return wantCaptureKeyboard;
                }
            case osgGA::GUIEventAdapter::DOUBLECLICK:
            case osgGA::GUIEventAdapter::RELEASE:
            case osgGA::GUIEventAdapter::PUSH:
                _input.push(ImGuiInputEvent::mouseButtonsEvent(ea.getX(), ea.getY(), ea.getButtonMask()));
                return wantCaptureMouse;
            case osgGA::GUIEventAdapter::DRAG:
            case osgGA::GUIEventAdapter::MOVE:
                _input.push(ImGuiInputEvent::mousePositionEvent(ea.getX(), ea.getY()));
                return wantCaptureMouse;
            case osgGA::GUIEventAdapter::SCROLL:
                _input.push(ImGuiInputEvent::mouseWheelEvent(ImGuiInputEvent::resolveImGuiWheelAmount(ea)));
                return wantCaptureMouse;
            default: return false;
            }
            return false;
        }

    protected:
        virtual ~ImGuiHandler()
        {
            // Shut the render backend down before destroying the context, otherwise
            // ImGui::Shutdown() asserts ("Forgot to shutdown Renderer backend?")
            release(NULL);
            ImGui::DestroyContext();
        }
        bool _started;
        bool _released;
    };

    struct ImGuiNewFrameCallback : public CameraDrawCallback
    {
        ImGuiNewFrameCallback(osgGA::GUIEventHandler* h) : _handler(h), _time(-1.0f) {}
        osg::observer_ptr<osgGA::GUIEventHandler> _handler;
        mutable double _time;

        virtual void operator()(osg::RenderInfo& renderInfo) const override
        {
            newImGuiFrame(renderInfo, _time, [&](ImGuiIO& io) {
                ImGuiHandler* handler = static_cast<ImGuiHandler*>(_handler.get());
                if (handler) handler->drain(io);
            });
        }
    };

    struct ImGuiRenderCallback : public CameraDrawCallback
    {
        ImGuiRenderCallback(ImGuiManager* m, osgGA::GUIEventHandler* h) : _handler(h), _manager(m) {}
        mutable std::map<std::string, ImTextureID> _textureIdList;
        osg::observer_ptr<osgGA::GUIEventHandler> _handler;
        // The manager may be destroyed before the viewer (declaration order in callers'
        // main()), while this callback can still be invoked during viewer teardown
        osg::observer_ptr<ImGuiManager> _manager;

        virtual void operator()(osg::RenderInfo& renderInfo) const override
        {
            ImGuiManager* manager = _manager.get();
            if (manager == NULL) return;
            endImGuiFrame(renderInfo, manager, _textureIdList,
                        [&](ImGuiContentHandler* v, ImGuiContext* context) {
                ImGuiHandler* handler = static_cast<ImGuiHandler*>(_handler.get());
                if (handler) v->ImGuiFonts = handler->_fonts;
                v->ImGuiTextures = _textureIdList; v->context = context;
                v->runInternal(manager);
            });

            ImGuiHandler* handler = static_cast<ImGuiHandler*>(_handler.get());
            if (handler) handler->publishCapture();
        }

        virtual void releaseGLObjects(osg::State* state) const
        {
            ImGuiHandler* handler = static_cast<ImGuiHandler*>(_handler.get());
            if (handler) handler->release(_manager.get());
        }
    };
}

////////////// ImGuiManager //////////////

ImGuiManager::ImGuiManager()
{}

ImGuiManager::~ImGuiManager()
{}

std::string ImGuiManager::defaultSettingsPath()
{
#if defined(_WIN32)
    const char* base = std::getenv("LOCALAPPDATA");
    return (base && base[0]) ? std::string(base) + "/osgVerse/imgui.ini" : std::string();
#elif defined(__APPLE__)
    const char* home = std::getenv("HOME");
    return (home && home[0]) ? std::string(home) +
        "/Library/Application Support/osgVerse/imgui.ini" : std::string();
#else
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && xdg[0]) return std::string(xdg) + "/osgVerse/imgui.ini";

    const char* home = std::getenv("HOME");
    return (home && home[0]) ? std::string(home) + "/.config/osgVerse/imgui.ini" : std::string();
#endif
}

void ImGuiManager::initializeEventHandler2D()
{
    _imguiHandler = new ImGuiHandler;
    static_cast<ImGuiHandler*>(_imguiHandler.get())->start(this);
}

void ImGuiManager::initialize(ImGuiContentHandler* cb, bool eventsFrom3D)
{
    _contentHandler = cb;
    if (eventsFrom3D) initializeEventHandler3D();
    else initializeEventHandler2D();
}

void ImGuiManager::shutdown()
{
    if (_imguiHandler.valid())
        static_cast<ImGuiHandler*>(_imguiHandler.get())->release(this);
}

void ImGuiManager::addToView(osgViewer::View* view, osg::Camera* specCam)
{
    osg::Camera* cam = (specCam != NULL) ? specCam : view->getCamera();
    osg::ref_ptr<ImGuiNewFrameCallback> nfcb = new ImGuiNewFrameCallback(_imguiHandler.get());
    osg::ref_ptr<ImGuiRenderCallback> rcb = new ImGuiRenderCallback(this, _imguiHandler.get());
    nfcb->setup(cam, PRE_DRAW); rcb->setup(cam, POST_DRAW);
    if (view) view->addEventHandler(_imguiHandler.get());
}

void ImGuiManager::setGuiTexture(const std::string& name, const std::string& file)
{
    osg::ref_ptr<osg::Image> image = osgDB::readImageFile(file);
    setGuiTexture(name, new osg::Texture2D(image.get()));
}

void ImGuiManager::setGuiTexture(const std::string& name, osg::Texture2D* tex2D)
{ _textures[name] = tex2D; }

void ImGuiManager::removeGuiTexture(const std::string& name)
{
    std::map<std::string, osg::ref_ptr<osg::Texture2D>>::iterator itr = _textures.find(name);
    if (itr != _textures.end()) _textures.erase(itr);
}

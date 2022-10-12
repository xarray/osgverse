#include <GL/glew.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/ImGuizmo.h>
#include <osg/Version>
#include <osg/Camera>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include "ImGui.h"
#include "ImGui.Styles.h"
#include "pipeline//Utilities.h"
using namespace osgVerse;

extern void StyleColorsVisualStudio(ImGuiStyle* dst = (ImGuiStyle*)0);
extern void StyleColorsSonicRiders(ImGuiStyle* dst = (ImGuiStyle*)0);
extern void StyleColorsLightBlue(ImGuiStyle* dst = (ImGuiStyle*)0);
extern void StyleColorsTransparent(ImGuiStyle* dst = (ImGuiStyle*)0);

void newImGuiFrame(osg::RenderInfo& renderInfo, double& time, std::function<void(ImGuiIO&)> func)
{
    ImGuiContext* context = ImGui::GetCurrentContext();
    if (!context) return; else if (time < 0.0f)
    {
        glewInit(); time = 0.0f;
        ImGui_ImplOpenGL3_Init();
    }
    ImGui_ImplOpenGL3_NewFrame();

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
#if OSG_VERSION_GREATER_THAN(3, 1, 8)
                if (tex2D->isDirty(renderInfo.getContextID())) tex2D->apply(*renderInfo.getState());
#else
                if (tex2D->getTextureParameterDirty(renderInfo.getContextID()) > 0)
                    tex2D->apply(*renderInfo.getState());
#endif

                osg::Texture::TextureObject* tObj = tex2D->getTextureObject(renderInfo.getContextID());
                if (tObj) textureIdList[itr->first] = reinterpret_cast<ImTextureID>((long long)tObj->id());
            }
            func(v, context);
        }
    }
    else return;

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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

    ImGuiIO& io = ImGui::GetIO();
    fonts[""] = io.Fonts->AddFontDefault();

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

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array
    io.KeyMap[ImGuiKey_Tab] = ImGuiKey_Tab;
    io.KeyMap[ImGuiKey_LeftArrow] = ImGuiKey_LeftArrow;
    io.KeyMap[ImGuiKey_RightArrow] = ImGuiKey_RightArrow;
    io.KeyMap[ImGuiKey_UpArrow] = ImGuiKey_UpArrow;
    io.KeyMap[ImGuiKey_DownArrow] = ImGuiKey_DownArrow;
    io.KeyMap[ImGuiKey_PageUp] = ImGuiKey_PageUp;
    io.KeyMap[ImGuiKey_PageDown] = ImGuiKey_PageDown;
    io.KeyMap[ImGuiKey_Home] = ImGuiKey_Home;
    io.KeyMap[ImGuiKey_End] = ImGuiKey_End;
    io.KeyMap[ImGuiKey_Insert] = ImGuiKey_Insert;
    io.KeyMap[ImGuiKey_Delete] = ImGuiKey_Delete;
    io.KeyMap[ImGuiKey_Backspace] = ImGuiKey_Backspace;
    io.KeyMap[ImGuiKey_Space] = ImGuiKey_Space;
    io.KeyMap[ImGuiKey_Enter] = ImGuiKey_Enter;
    io.KeyMap[ImGuiKey_Escape] = ImGuiKey_Escape;
    io.KeyMap[ImGuiKey_KeyPadEnter] = ImGuiKey_KeyPadEnter;
    io.KeyMap[ImGuiKey_A] = osgGA::GUIEventAdapter::KEY_A;
    io.KeyMap[ImGuiKey_C] = osgGA::GUIEventAdapter::KEY_C;
    io.KeyMap[ImGuiKey_V] = osgGA::GUIEventAdapter::KEY_V;
    io.KeyMap[ImGuiKey_X] = osgGA::GUIEventAdapter::KEY_X;
    io.KeyMap[ImGuiKey_Y] = osgGA::GUIEventAdapter::KEY_Y;
    io.KeyMap[ImGuiKey_Z] = osgGA::GUIEventAdapter::KEY_Z;
}

int convertImGuiSpecialKey(int key)
{
    switch (key)
    {
    case osgGA::GUIEventAdapter::KEY_Tab: return ImGuiKey_Tab;
    case osgGA::GUIEventAdapter::KEY_Left: return ImGuiKey_LeftArrow;
    case osgGA::GUIEventAdapter::KEY_Right: return ImGuiKey_RightArrow;
    case osgGA::GUIEventAdapter::KEY_Up: return ImGuiKey_UpArrow;
    case osgGA::GUIEventAdapter::KEY_Down: return ImGuiKey_DownArrow;
    case osgGA::GUIEventAdapter::KEY_Page_Up: return ImGuiKey_PageUp;
    case osgGA::GUIEventAdapter::KEY_Page_Down: return ImGuiKey_PageDown;
    case osgGA::GUIEventAdapter::KEY_Home: return ImGuiKey_Home;
    case osgGA::GUIEventAdapter::KEY_End: return ImGuiKey_End;
    case osgGA::GUIEventAdapter::KEY_Delete: return ImGuiKey_Delete;
    case osgGA::GUIEventAdapter::KEY_Insert: return ImGuiKey_Insert;
    case osgGA::GUIEventAdapter::KEY_BackSpace: return ImGuiKey_Backspace;
    case osgGA::GUIEventAdapter::KEY_Space: return ImGuiKey_Space;
    case osgGA::GUIEventAdapter::KEY_Return: return ImGuiKey_Enter;
    case osgGA::GUIEventAdapter::KEY_Escape: return ImGuiKey_Escape;
    case osgGA::GUIEventAdapter::KEY_KP_Enter: return ImGuiKey_KeyPadEnter;
    default: return -1;
    }
}

class ImGuiHandler : public osgGA::GUIEventHandler
{
public:
    std::map<std::string, ImFont*> _fonts;
    bool _mousePressed[3];
    float _mouseWheel;

    ImGuiHandler() : _mouseWheel(0.0f)
    {
        _mousePressed[0] = false;
        _mousePressed[1] = false;
        _mousePressed[2] = false;
    }

    void start(ImGuiManager* manager)
    { startImGuiContext(manager, _fonts); }

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        ImGuiIO& io = ImGui::GetIO();
        bool wantCaptureMouse = io.WantCaptureMouse;
        bool wantCaptureKeyboard = io.WantCaptureKeyboard;
        wantCaptureMouse |= ImGuizmo::IsUsing();

        switch (ea.getEventType())
        {
        case osgGA::GUIEventAdapter::KEYDOWN:
        case osgGA::GUIEventAdapter::KEYUP:
            //if (wantCaptureKeyboard)
            {
                const bool isKeyDown = ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN;
                const int c = ea.getKey();
                const int special_key = convertImGuiSpecialKey(c);
                if (special_key > 0)
                {
                    io.KeysDown[special_key] = isKeyDown;
                    io.KeyCtrl = ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL;
                    io.KeyShift = ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT;
                    io.KeyAlt = ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_ALT;
                    io.KeySuper = ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SUPER;
                }
                else if (isKeyDown && c > 0 && c < 0xFF)
                    io.AddInputCharacter((unsigned short)c);
                return wantCaptureKeyboard;
            }
        case osgGA::GUIEventAdapter::DOUBLECLICK:
        case osgGA::GUIEventAdapter::RELEASE:
        case osgGA::GUIEventAdapter::PUSH:
            io.MousePos = ImVec2(ea.getX(), io.DisplaySize.y - ea.getY());
            _mousePressed[0] = ea.getButtonMask() & osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON;
            _mousePressed[1] = ea.getButtonMask() & osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON;
            _mousePressed[2] = ea.getButtonMask() & osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON;
            return wantCaptureMouse;
        case osgGA::GUIEventAdapter::DRAG:
        case osgGA::GUIEventAdapter::MOVE:
            io.MousePos = ImVec2(ea.getX(), io.DisplaySize.y - ea.getY());
            return wantCaptureMouse;
        case osgGA::GUIEventAdapter::SCROLL:
            if (wantCaptureMouse)
                _mouseWheel = (ea.getScrollingMotion() == osgGA::GUIEventAdapter::SCROLL_UP ? 1.0f : -1.0f);
            return wantCaptureMouse;
        default: return false;
        }
        return false;
    }

protected:
    virtual ~ImGuiHandler()
    {
        //ImGui_ImplOpenGL3_Shutdown();  // FIXME
        ImGui::DestroyContext();
    }
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
            if (handler)
            {
                io.MouseDown[0] = handler->_mousePressed[0];
                io.MouseDown[1] = handler->_mousePressed[1];
                io.MouseDown[2] = handler->_mousePressed[2];
                io.MouseWheel = handler->_mouseWheel; handler->_mouseWheel = 0.0f;
            }
        });
    }
};

struct ImGuiRenderCallback : public CameraDrawCallback
{
    ImGuiRenderCallback(ImGuiManager* m, osgGA::GUIEventHandler* h) : _handler(h), _manager(m) {}
    mutable std::map<std::string, ImTextureID> _textureIdList;
    osg::observer_ptr<osgGA::GUIEventHandler> _handler;
    ImGuiManager* _manager;

    virtual void operator()(osg::RenderInfo& renderInfo) const override
    {
        endImGuiFrame(renderInfo, _manager, _textureIdList,
                      [&](ImGuiContentHandler* v, ImGuiContext* context) {
            ImGuiHandler* handler = static_cast<ImGuiHandler*>(_handler.get());
            if (handler) v->ImGuiFonts = handler->_fonts;
            v->ImGuiTextures = _textureIdList; v->context = context;
            v->runInternal(_manager);
        });
    }
};

////////////// ImGuiManager //////////////

ImGuiManager::ImGuiManager()
{
}

ImGuiManager::~ImGuiManager()
{}

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

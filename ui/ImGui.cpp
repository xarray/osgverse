#include <GL/glew.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>
#include <osg/Camera>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include "ImGui.h"
#include "ImGui.Styles.h"
using namespace osgVerse;

extern void StyleColorsVisualStudio(ImGuiStyle* dst = (ImGuiStyle*)0);
extern void StyleColorsSonicRiders(ImGuiStyle* dst = (ImGuiStyle*)0);
extern void StyleColorsLightBlue(ImGuiStyle* dst = (ImGuiStyle*)0);
extern void StyleColorsTransparent(ImGuiStyle* dst = (ImGuiStyle*)0);

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
    {
        ImGui::CreateContext();
        int style = 13;  // FIXME
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
        _fonts[""] = io.Fonts->AddFontDefault();

        std::string fontData = manager->getChineseSimplifiedFont();
        if (!fontData.empty())
        {
            _fonts[osgDB::getStrippedName(fontData)] = io.Fonts->AddFontFromFileTTF(
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
        io.KeyMap[ImGuiKey_Delete] = ImGuiKey_Delete;
        io.KeyMap[ImGuiKey_Backspace] = ImGuiKey_Backspace;
        io.KeyMap[ImGuiKey_Enter] = ImGuiKey_Enter;
        io.KeyMap[ImGuiKey_Escape] = ImGuiKey_Escape;
        io.KeyMap[ImGuiKey_A] = osgGA::GUIEventAdapter::KEY_A;
        io.KeyMap[ImGuiKey_C] = osgGA::GUIEventAdapter::KEY_C;
        io.KeyMap[ImGuiKey_V] = osgGA::GUIEventAdapter::KeySymbol::KEY_V;
        io.KeyMap[ImGuiKey_X] = osgGA::GUIEventAdapter::KeySymbol::KEY_X;
        io.KeyMap[ImGuiKey_Y] = osgGA::GUIEventAdapter::KeySymbol::KEY_Y;
        io.KeyMap[ImGuiKey_Z] = osgGA::GUIEventAdapter::KeySymbol::KEY_Z;
    }

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        ImGuiIO& io = ImGui::GetIO();
        const bool wantCaptureMouse = io.WantCaptureMouse;
        const bool wantCaptureKeyboard = io.WantCaptureKeyboard;

        switch (ea.getEventType())
        {
        case osgGA::GUIEventAdapter::KEYDOWN:
        case osgGA::GUIEventAdapter::KEYUP:
        {
            const bool isKeyDown = ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN;
            const int c = ea.getKey();
            const int special_key = convertKey(c);
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

    int convertKey(int key)
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
        case osgGA::GUIEventAdapter::KEY_BackSpace: return ImGuiKey_Backspace;
        case osgGA::GUIEventAdapter::KEY_Return: return ImGuiKey_Enter;
        case osgGA::GUIEventAdapter::KEY_Escape: return ImGuiKey_Escape;
        default: return -1;
        }
    }
};

struct ImGuiNewFrameCallback : public osg::Camera::DrawCallback
{
    ImGuiNewFrameCallback(osgGA::GUIEventHandler* h) : _handler(h), _time(-1.0f) {}
    osg::observer_ptr<osgGA::GUIEventHandler> _handler;
    mutable double _time;

    virtual void operator()(osg::RenderInfo& renderInfo) const override
    {
        ImGuiContext* context = ImGui::GetCurrentContext();
        if (!context) return; else if (_time < 0.0f)
        {
            glewInit(); _time = 0.0f;
            ImGui_ImplOpenGL3_Init();
        }
        ImGui_ImplOpenGL3_NewFrame();

        ImGuiIO& io = ImGui::GetIO();
        {
            osg::Viewport* viewport = renderInfo.getCurrentCamera()->getViewport();
            io.DisplaySize = ImVec2(viewport->width(), viewport->height());

            double currentTime = renderInfo.getView()->getFrameStamp()->getSimulationTime();
            io.DeltaTime = currentTime - _time + 0.0000001;
            _time = currentTime;

            ImGuiHandler* handler = static_cast<ImGuiHandler*>(_handler.get());
            if (handler)
            {
                io.MouseDown[0] = handler->_mousePressed[0];
                io.MouseDown[1] = handler->_mousePressed[1];
                io.MouseDown[2] = handler->_mousePressed[2];
                io.MouseWheel = handler->_mouseWheel;
            }
        }
        ImGui::NewFrame();
    }
};

struct ImGuiRenderCallback : public osg::Camera::DrawCallback
{
    ImGuiRenderCallback(ImGuiManager* m, osgGA::GUIEventHandler* h) : _manager(m), _handler(h) {}
    mutable std::map<std::string, ImTextureID> _textureIdList;
    osg::observer_ptr<osgGA::GUIEventHandler> _handler;
    ImGuiManager* _manager;

    virtual void operator()(osg::RenderInfo& renderInfo) const override
    {
        ImGuiContext* context = ImGui::GetCurrentContext();
        if (_manager && context)
        {
            ImGuiContentHandler* v = _manager->getContentHandler();
            if (v)
            {
                std::map<std::string, osg::ref_ptr<osg::Texture2D>>& tList = _manager->getTextures();
                _textureIdList.clear();
                for (std::map<std::string, osg::ref_ptr<osg::Texture2D>>::iterator itr = tList.begin();
                    itr != tList.end(); ++itr)
                {
                    osg::Texture2D* tex2D = itr->second.get();
                    if (tex2D->isDirty(renderInfo.getContextID())) tex2D->apply(*renderInfo.getState());
                    
                    osg::Texture::TextureObject* tObj = tex2D->getTextureObject(renderInfo.getContextID());
                    if (tObj) _textureIdList[itr->first] = reinterpret_cast<ImTextureID>(tObj->id());
                }

                ImGuiHandler* handler = static_cast<ImGuiHandler*>(_handler.get());
                if (handler) v->ImGuiFonts = handler->_fonts;
                v->ImGuiTextures = _textureIdList;
                v->context = context;
                v->runInternal(_manager);
            }
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
};

////////////// ImGuiManager //////////////

ImGuiManager::ImGuiManager()
{
    _imguiHandler = new ImGuiHandler;
}

ImGuiManager::~ImGuiManager()
{}

void ImGuiManager::initialize(ImGuiContentHandler* cb)
{
    _contentHandler = cb;
    static_cast<ImGuiHandler*>(_imguiHandler.get())->start(this);
}

void ImGuiManager::addToView(osgViewer::View* view)
{
    osg::Camera* cam = view->getCamera();
    cam->setPreDrawCallback(new ImGuiNewFrameCallback(_imguiHandler.get()));
    cam->setPostDrawCallback(new ImGuiRenderCallback(this, _imguiHandler.get()));
    view->addEventHandler(_imguiHandler.get());
}

void ImGuiManager::removeFromView(osgViewer::View* view)
{
    if (view->getCamera())
    {
        osg::Camera* cam = view->getCamera();
        cam->setPreDrawCallback(NULL);
        cam->setPostDrawCallback(NULL);
        view->removeEventHandler(_imguiHandler.get());
    }
}

void ImGuiManager::updateGuiTexture(const std::string& name, const std::string& file)
{
    osg::ref_ptr<osg::Image> image = osgDB::readImageFile(file);
    osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D(image.get());
    _textures[name] = tex2D.get();
}

void ImGuiManager::removeGuiTexture(const std::string& name)
{
    std::map<std::string, osg::ref_ptr<osg::Texture2D>>::iterator itr = _textures.find(name);
    if (itr != _textures.end()) _textures.erase(itr);
}

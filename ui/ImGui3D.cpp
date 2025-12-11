#include <imgui/imgui.h>
#include <osg/Version>
#include <osg/Camera>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include "ImGui.h"
#include "pipeline/Pipeline.h"
#include "pipeline/Utilities.h"
using namespace osgVerse;

extern void newImGuiFrame(osg::RenderInfo& renderInfo, double& time, std::function<void(ImGuiIO&)> func);
extern void endImGuiFrame(osg::RenderInfo& renderInfo, ImGuiManager* manager,
                          std::map<std::string, ImTextureID>& textureIdList,
                          std::function<void(ImGuiContentHandler*, ImGuiContext*)> func);
extern void startImGuiContext(ImGuiManager* manager, std::map<std::string, ImFont*>& fonts);
extern int convertImGuiCharacterKey(int key);
extern int convertImGuiSpecialKey(int key);

class ImGuiHandler3D : public osgGA::GUIEventHandler
{
public:
    std::map<std::string, ImFont*> _fonts;
    ImVec2 _mousePosition;
    bool _mousePressed[3];
    float _mouseWheel;

    ImGuiHandler3D() : _mouseWheel(0.0f)
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

        switch (ea.getEventType())
        {
        case osgGA::GUIEventAdapter::KEYDOWN:
        case osgGA::GUIEventAdapter::KEYUP:
            //if (wantCaptureKeyboard)
            {
                const bool isKeyDown = ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN;
                const int c = ea.getKey(); const int special_key = convertImGuiSpecialKey(c);
                if (special_key > 0)
                {
                    io.AddKeyEvent((ImGuiKey)special_key, isKeyDown);
                    io.KeyCtrl = ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL;
                    io.KeyShift = ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT;
                    io.KeyAlt = ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_ALT;
                    io.KeySuper = ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SUPER;
                }
                else if (c > 0 && c < 0xFF)
                {
                    io.AddKeyEvent((ImGuiKey)convertImGuiCharacterKey(c), isKeyDown);
                    if (isKeyDown) io.AddInputCharacter((unsigned short)c);
                }
                return wantCaptureKeyboard;
            }
        case osgGA::GUIEventAdapter::SCROLL:
            if (wantCaptureMouse)
                _mouseWheel = (ea.getScrollingMotion() == osgGA::GUIEventAdapter::SCROLL_UP ? 1.0f : -1.0f);
            return wantCaptureMouse;
        default: return false;
        }
        return false;
    }

protected:
    virtual ~ImGuiHandler3D()
    {
        //ImGui_ImplOpenGL3_Shutdown();  // FIXME
        ImGui::DestroyContext();
    }
};

struct ImGuiDrawableCallback : public virtual osg::Drawable::DrawCallback
{
    ImGuiDrawableCallback(ImGuiManager* m, osgGA::GUIEventHandler* h)
        : _manager(m), _handler(h), _time(-1.0f) {}

    mutable std::map<std::string, ImTextureID> _textureIdList;
    osg::observer_ptr<osgGA::GUIEventHandler> _handler;
    ImGuiManager* _manager;
    mutable double _time;

    virtual void drawImplementation(osg::RenderInfo& renderInfo, const osg::Drawable* d) const
    {
        newImGuiFrame(renderInfo, _time, [&](ImGuiIO& io) {
            ImGuiHandler3D* handler = static_cast<ImGuiHandler3D*>(_handler.get());
            if (handler)
            {
                io.MousePos = handler->_mousePosition;
                io.MouseDown[0] = handler->_mousePressed[0];
                io.MouseDown[1] = handler->_mousePressed[1];
                io.MouseDown[2] = handler->_mousePressed[2];
                io.MouseWheel = handler->_mouseWheel; handler->_mouseWheel = 0.0f;
            }
        });

        //d->drawImplementation(renderInfo);
        endImGuiFrame(renderInfo, _manager, _textureIdList,
                      [&](ImGuiContentHandler* v, ImGuiContext* context) {
            ImGuiHandler3D* handler = static_cast<ImGuiHandler3D*>(_handler.get());
            if (handler) v->ImGuiFonts = handler->_fonts;
            v->ImGuiTextures = _textureIdList; v->context = context;
            v->runInternal(_manager);
        });
    }
};

osg::Texture* ImGuiManager::addToTexture(osg::Group* parentOfRtt, int w, int h)
{
    osg::Texture* rttTex = Pipeline::createTexture(Pipeline::RGBA_INT8, w, h);
    osg::ref_ptr<osg::Camera> rttCamera = createRTTCamera(
        osg::Camera::COLOR_BUFFER0, rttTex, NULL, false);
    rttCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    rttCamera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0));
    rttCamera->setViewMatrix(osg::Matrix::identity());

    osg::Geode* geode = createScreenQuad(osg::Vec3(), 1.0f, 1.0f, osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
    geode->getDrawable(0)->setUseDisplayList(false);
    geode->getDrawable(0)->setDrawCallback(new ImGuiDrawableCallback(this, _imguiHandler.get()));
    rttCamera->addChild(geode);
    if (parentOfRtt) parentOfRtt->addChild(rttCamera.get());
    return rttTex;
}

void ImGuiManager::initializeEventHandler3D()
{
    _imguiHandler = new ImGuiHandler3D;
    static_cast<ImGuiHandler3D*>(_imguiHandler.get())->start(this);
}

void ImGuiManager::setMouseInput(const osg::Vec2& pos, int button, float wheel)
{
    ImGuiHandler3D* handler = dynamic_cast<ImGuiHandler3D*>(_imguiHandler.get());
    if (!handler)
    {
        OSG_NOTICE << "[ImGuiManager] setMouseInput() is unsupported in 2D GUI mode\n";
    }
    else
    {
        ImGuiIO& io = ImGui::GetIO();
        handler->_mousePosition = ImVec2(
            io.DisplaySize[0] * pos[0], io.DisplaySize[1] * pos[1]);
        handler->_mousePressed[0] = button & osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON;
        handler->_mousePressed[1] = button & osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON;
        handler->_mousePressed[2] = button & osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON;
        handler->_mouseWheel = wheel;
    }
}

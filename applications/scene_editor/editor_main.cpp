#include <osgDB/ReadFile>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osg/Fog>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

#include "defines.h"
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
GlobalData g_data;

class ConsoleHandler : public osg::NotifyHandler
{
public:
    ConsoleHandler() {}

    virtual void notify(osg::NotifySeverity severity, const char* message)
    {
        // TODO
        std::cout << "Lv-" << severity << ": " << message;
    }

    std::string getDateTimeTick()
    {
        auto tick = std::chrono::system_clock::now();
        std::time_t posix = std::chrono::system_clock::to_time_t(tick);
        uint64_t millseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(tick.time_since_epoch()).count() -
            std::chrono::duration_cast<std::chrono::seconds>(tick.time_since_epoch()).count() * 1000;

        char buf[20], buf2[5];
        std::tm tp = *std::localtime(&posix);
        std::string dateTime{ buf, std::strftime(buf, sizeof(buf), "%F %T", &tp) };
        snprintf(buf2, 5, ".%03d", (int)millseconds);
        return dateTime + std::string(buf2);
    }
};

class MyViewer : public osgViewer::Viewer
{
public:
    MyViewer(osgVerse::Pipeline* p) : osgViewer::Viewer(), _pipeline(p) {}
    osg::ref_ptr<osgVerse::Pipeline> _pipeline;

protected:
    virtual osg::GraphicsOperation* createRenderer(osg::Camera* camera)
    {
        if (_pipeline.valid()) return _pipeline->createRenderer(camera);
        else return osgViewer::Viewer::createRenderer(camera);
    }
};

class SceneManipulator : public osgGA::TrackballManipulator
{
public:
    SceneManipulator() : osgGA::TrackballManipulator() {}
    virtual bool performMovement()
    {
        if (_ga_t0.get() == NULL || _ga_t1.get() == NULL) return false;
        double eventTimeDelta = _ga_t0->getTime() - _ga_t1->getTime();
        if (eventTimeDelta < 0.0) eventTimeDelta = 0.0;

        float dx = _ga_t0->getXnormalized() - _ga_t1->getXnormalized();
        float dy = _ga_t0->getYnormalized() - _ga_t1->getYnormalized();
        if (dx == 0.0 && dy == 0.0) return false;

        unsigned int bm = _ga_t1->getButtonMask(), mk = _ga_t1->getModKeyMask();
        bool modKeyDown = (mk & osgGA::GUIEventAdapter::MODKEY_ALT);
        if (bm == osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON && modKeyDown)
        {
            return performMovementRightMouseButton(eventTimeDelta, dx, dy);
        }
        else if ((bm == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON && modKeyDown) ||
            bm == osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON)
        {
            return performMovementLeftMouseButton(eventTimeDelta, dx, dy);
        }
        else if (bm == osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON)
        {
            return performMovementMiddleMouseButton(eventTimeDelta, dx, dy);
        }
        return false;
    }
};

EditorContentHandler::EditorContentHandler(osgViewer::View* view, osg::Group* root)
    : _uiFrameNumber(0)
{
    osgVerse::SerializerFactory* factory = osgVerse::SerializerFactory::instance();
    factory->registerBlacklist("UserDataContainer", NULL);
    factory->registerBlacklist("ComputeBoundingSphereCallback", NULL);
    factory->registerBlacklist("ComputeBoundingBoxCallback", NULL);
    factory->registerBlacklist("CullCallback", NULL);
    factory->registerBlacklist("DrawCallback", NULL);
    factory->registerBlacklist("SupportsDisplayList", NULL);
    factory->registerBlacklist("UseDisplayList", NULL);
    factory->registerBlacklist("UseVertexBufferObjects", NULL);

#ifdef ORIGIN_CODE
    _mainMenu = new osgVerse::MainMenuBar;
    _mainMenu->userData = this;
    createEditorMenu1();
    createEditorMenu2();
    createEditorMenu3();
#endif

    _hierarchyData = new osgVerse::SceneHierarchy;
    _hierarchyData->setViewer(view, root);
    _hierarchyData->setItemClickAction(
        [this](osgVerse::TreeView* tree, osgVerse::TreeView::TreeData* item) {
            // TODO: select in 3D view
            osg::Object* obj = osgVerse::SceneDataProxy::get<osg::Object*>(item->userData.get());
            if (obj != NULL)
            {
                _entry = osgVerse::SerializerFactory::instance()
                       ->createInterfaces(obj, NULL, _interfaces);
            }
            else
                _interfaces.clear();
        });
    _hierarchyData->setItemDoubleClickAction(
        [this](osgVerse::TreeView* tree, osgVerse::TreeView::TreeData* item) {
            // TODO: focus in 3D view?
            std::cout << "ITEM DOUBLE CLICKED " << item->name << std::endl;
        });

    _hierarchy = new osgVerse::Window(TR0("Hierarchy") + "##editor");
    _hierarchy->pos = osg::Vec2(0.0f, 0.0f);
    _hierarchy->size = osg::Vec2(0.15f, 0.75f);
    _hierarchy->alpha = 0.9f;
    _hierarchy->useMenuBar = false;
    _hierarchy->flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar;
    _hierarchy->userData = this;

    _properties = new osgVerse::Window(TR0("Properties") + "##editor");
    _properties->pos = osg::Vec2(0.6f, 0.0f);
    _properties->size = osg::Vec2(0.4f, 0.75f);
    _properties->alpha = 0.9f;
    _properties->useMenuBar = false;
    _properties->flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar;
    _properties->userData = this;

    // TEST
    root->addChild(osgDB::readNodeFile("cessna.osg"));
    _hierarchyData->addItem(NULL, root);
}

void EditorContentHandler::runInternal(osgVerse::ImGuiManager* mgr)
{
    ImGui::PushFont(ImGuiFonts["LXGWFasmartGothic"]);

    bool done = _hierarchy->show(mgr, this), edited = false;
    if (done)
    {
        _hierarchyData->show(mgr, this);
        _hierarchy->showEnd();
    }

    done |= _properties->show(mgr, this);
    if (done)
    {
        for (size_t i = 0; i < _interfaces.size(); ++i)
        {
            _interfaces[i]->show(mgr, this);
            edited |= _interfaces[i]->checkEdited();
        }
        if (edited) _hierarchyData->refreshItem();
        _properties->showEnd();
    }

    ImGui::PopFont();
    _uiFrameNumber++;
}

void EditorContentHandler::handleCommands()
{
#ifdef ORIGIN_CODE
    osgVerse::CommandData cmd;
    while (osgVerse::CommandBuffer::instance()->take(cmd, false))
    {
        switch (cmd.type)
        {
        case osgVerse::ResizeEditor:
            if (_hierarchy.valid()) _hierarchy->getWindow()->sizeApplied = false;
            if (_properties.valid()) _properties->getWindow()->sizeApplied = false;
            if (_sceneLogic.valid()) _sceneLogic->getWindow()->sizeApplied = false;
            break;
        case osgVerse::RefreshHierarchy:
            if (!_hierarchy->handleCommand(&cmd))
                OSG_WARN << "[EditorContentHandler] Failed to refresh hierarchy" << std::endl;
            break;
        case osgVerse::RefreshHierarchyItem:
            if (!_hierarchy->handleItemCommand(&cmd))
                OSG_WARN << "[EditorContentHandler] Failed to refresh hierarchy item" << std::endl;
            break;
        case osgVerse::RefreshProperties:
            if (!_properties->handleCommand(&cmd))
                OSG_WARN << "[EditorContentHandler] Failed to refresh properties" << std::endl;
            break;
        }
    }
#endif
}

int main(int argc, char** argv)
{
    osgVerse::globalInitialize(argc, argv);
    osgVerse::updateOsgBinaryWrappers();

    // Core scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->setName("SceneRoot");
    osgVerse::Pipeline::setPipelineMask(*sceneRoot, DEFERRED_SCENE_MASK | SHADOW_CASTER_MASK);

    osg::ref_ptr<osg::Group> auxRoot = new osg::Group;
    auxRoot->setName("AuxRoot");
    osgVerse::Pipeline::setPipelineMask(*auxRoot, FORWARD_SCENE_MASK);

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(sceneRoot.get());
    root->addChild(auxRoot.get());

    // Main light
    osg::ref_ptr<osgVerse::LightDrawable> light0 = new osgVerse::LightDrawable;
    light0->setColor(osg::Vec3(1.5f, 1.5f, 1.2f));
    light0->setDirection(osg::Vec3(0.02f, 0.1f, -1.0f));
    light0->setDirectional(true);

    osg::ref_ptr<osg::Geode> lightGeode = new osg::Geode;
    lightGeode->addDrawable(light0.get());
    root->addChild(lightGeode.get());

    // Pipeline initialization
    osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline;

    // Set-up the viewer
    MyViewer viewer(pipeline.get());
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    //viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setCameraManipulator(new SceneManipulator);
    viewer.setSceneData(root.get());
    //viewer.setKeyEventSetsDone(0);
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.setUpViewOnSingleScreen(0);

    osgVerse::StandardPipelineParameters params(SHADER_DIR, SKYBOX_DIR "sunset.png");
    setupStandardPipeline(pipeline.get(), &viewer, params);

    osgVerse::ShadowModule* shadow = static_cast<osgVerse::ShadowModule*>(pipeline->getModule("Shadow"));
    if (shadow)
    {
        if (shadow->getFrustumGeode())
        {
            osgVerse::Pipeline::setPipelineMask(*shadow->getFrustumGeode(), FORWARD_SCENE_MASK);
            root->addChild(shadow->getFrustumGeode());
        }
    }

    osgVerse::LightModule* light = static_cast<osgVerse::LightModule*>(pipeline->getModule("Light"));
    if (light) light->setMainLight(light0.get(), "Shadow");

    // Post-HUD displays and utilities
    osg::ref_ptr<osg::Camera> skyCamera = osgVerse::SkyBox::createSkyCamera();
    auxRoot->addChild(skyCamera.get());

    osg::ref_ptr<osgVerse::SkyBox> skybox = new osgVerse::SkyBox(pipeline.get());
    {
        skybox->setSkyShaders(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "skybox.vert.glsl"),
            osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "skybox.frag.glsl"));
        skybox->setEnvironmentMap(params.skyboxMap.get(), false);
        skyCamera->addChild(skybox.get());
    }

    osg::ref_ptr<osg::Camera> postCamera = new osg::Camera;
    postCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    postCamera->setRenderOrder(osg::Camera::POST_RENDER, 10000);
    postCamera->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
    auxRoot->addChild(postCamera.get());

    osg::ref_ptr<osgVerse::NodeSelector> selector = new osgVerse::NodeSelector;
    {
        selector->setMainCamera(pipeline->getForwardCamera());
        postCamera->addChild(selector->getAuxiliaryRoot());
        // TODO: also add to hud camera?
    }

    g_data.mainCamera = viewer.getCamera();
    g_data.sceneRoot = sceneRoot.get();
    g_data.auxiliaryRoot = auxRoot.get();
    g_data.selector = selector.get();
    g_data.view = &viewer;
    g_data.pipeline = pipeline.get();
    g_data.shadow = shadow;

    // UI settings
    osg::ref_ptr<osgVerse::ImGuiManager> imgui = new osgVerse::ImGuiManager;
    imgui->setChineseSimplifiedFont(MISC_DIR "LXGWFasmartGothic.otf");
    imgui->initialize(new EditorContentHandler(&viewer, sceneRoot.get()));
    imgui->addToView(&viewer, postCamera.get());
    return viewer.run();
}

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

EditorContentHandler::EditorContentHandler()
    : _uiFrameNumber(0)
{
    osgVerse::SerializerFactory* factory = osgVerse::SerializerFactory::instance();
    factory->registerBlacklist("DataVariance", NULL);
    factory->registerBlacklist("UserDataContainer", NULL);
    factory->registerBlacklist("ComputeBoundingSphereCallback", NULL);
    factory->registerBlacklist("ComputeBoundingBoxCallback", NULL);
    factory->registerBlacklist("CullCallback", NULL);
    factory->registerBlacklist("DrawCallback", NULL);
    factory->registerBlacklist("SupportsDisplayList", NULL);
    factory->registerBlacklist("UseDisplayList", NULL);
    factory->registerBlacklist("UseVertexBufferObjects", NULL);

    _mainMenu = new osgVerse::MainMenuBar;
    _mainMenu->userData = this;
    createEditorMenu1();
    createEditorMenu2();
    createEditorMenu3();

    _navigationData = new osgVerse::SceneNavigation;
    _navigationData->setCamera(g_data.view->getCamera());
    _navigationData->setTransformAction([this](osgVerse::SceneNavigation* nav, osg::Transform* t)
    {
        // Notify properties
        for (size_t i = 0; i < _interfaces.size(); ++i) _interfaces[i]->dirty();
    });

    _hierarchyData = new osgVerse::SceneHierarchy;
    _hierarchyData->setViewer(g_data.view.get(), g_data.sceneRoot.get());
    _hierarchyData->setItemClickAction(
        [this](osgVerse::TreeView* tree, osgVerse::TreeView::TreeData* item) {
            osg::Object* obj = osgVerse::SceneDataProxy::get<osg::Object*>(item->userData.get());
            g_data.selector->clearAllSelectedNodes();
            if (obj != NULL)
            {
                _entry = osgVerse::SerializerFactory::instance()
                       ->createInterfaces(obj, NULL, _interfaces);
                _navigationData->setSelection(dynamic_cast<osg::Transform*>(obj));
                g_data.selector->addSelectedNode(dynamic_cast<osg::Node*>(obj));
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
    _hierarchy->pos = osg::Vec2(0.0f, 0.02f);
    _hierarchy->size = osg::Vec2(0.15f, 0.75f);
    _hierarchy->alpha = 0.9f;
    _hierarchy->flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar;
    _hierarchy->userData = this;

    _navigation = new osgVerse::Window(TR0("Navigation") + "##editor");
    _navigation->pos = osg::Vec2(0.2f, 0.02f);
    _navigation->size = osg::Vec2(0.55f, 0.1f);
    _navigation->alpha = 0.0f; _navigation->withBorder = false;
    _navigation->flags = ImGuiWindowFlags_NoDecoration;
    _navigation->userData = this;

    _properties = new osgVerse::Window(TR0("Properties") + "##editor");
    _properties->pos = osg::Vec2(0.75f, 0.02f);
    _properties->size = osg::Vec2(0.25f, 0.75f);
    _properties->alpha = 0.9f;
    _properties->flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar;
    _properties->userData = this;

    // TEST
    g_data.sceneRoot->addChild(osgDB::readNodeFile("lz.osgt"));
    _hierarchyData->addItem(NULL, g_data.sceneRoot.get());
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

    done = _navigation->show(mgr, this);
    if (done)
    {
        _navigationData->show(mgr, this);
        _navigation->showEnd();
    }

    done = _properties->show(mgr, this);
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

    _mainMenu->show(mgr, this);
    ImGui::PopFont();
    _uiFrameNumber++;
}

int main(int argc, char** argv)
{
    osg::setNotifyHandler(new osgVerse::ConsoleHandler);
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

    osgVerse::StandardPipelineParameters params(SHADER_DIR, SKYBOX_DIR + "sunset.png");
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
        skybox->setSkyShaders(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "skybox.vert.glsl"),
            osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "skybox.frag.glsl"));
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
    imgui->setChineseSimplifiedFont(MISC_DIR + "LXGWFasmartGothic.otf");
    imgui->initialize(new EditorContentHandler());
    imgui->addToView(&viewer, postCamera.get());
    return viewer.run();
}

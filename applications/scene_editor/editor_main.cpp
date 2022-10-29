#include <osgDB/ReadFile>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include "hierarchy.h"
#include "properties.h"
#include "scenelogic.h"
#include "defines.h"
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
        { return performMovementRightMouseButton(eventTimeDelta, dx, dy); }
        else if ((bm == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON && modKeyDown) ||
                 bm == osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON)
        { return performMovementLeftMouseButton(eventTimeDelta, dx, dy); }
        else if (bm == osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON)
        { return performMovementMiddleMouseButton(eventTimeDelta, dx, dy); }
        return false;
    }
};

EditorContentHandler::EditorContentHandler()
    : _uiFrameNumber(0)
{
    _mainMenu = new osgVerse::MainMenuBar;
    _mainMenu->userData = this;
    createEditorMenu1();
    createEditorMenu2();
    createEditorMenu3();

    // Initialize components after menu items to reuse their callbacks
    _hierarchy = new Hierarchy(this);
    _properties = new Properties(this);
    _sceneLogic = new SceneLogic(this);
}

void EditorContentHandler::runInternal(osgVerse::ImGuiManager* mgr)
{
    ImGui::PushFont(ImGuiFonts["SourceHanSansHWSC-Regular"]);
    handleCommands();

    _mainMenu->show(mgr, this);
    ImGui::Separator();
    if (_uiFrameNumber > 0)
    {
        // Wait for the first frame to initialize ImGui work-size
        if (_hierarchy.valid()) _hierarchy->show(mgr, this);
        if (_properties.valid()) _properties->show(mgr, this);
        if (_sceneLogic.valid()) _sceneLogic->show(mgr, this);
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        // TODO: auto layout
        osg::Vec4 hSize = _hierarchy->getWindow()->getCurrentRectangle();
        osg::Vec4 pSize = _properties->getWindow()->getCurrentRectangle();
        osg::Vec4 lSize = _sceneLogic->getWindow()->getCurrentRectangle();
    }

    // Dialog management
    { std::string r; osgVerse::ImGuiComponentBase::showFileDialog(r); }
    { bool r = false; osgVerse::ImGuiComponentBase::showConfirmDialog(r); }

    ImGui::PopFont();
    _uiFrameNumber++;
}

void EditorContentHandler::handleCommands()
{
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
}

int main(int argc, char** argv)
{
    osgVerse::globalInitialize(argc, argv);

    // Core scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->setName("SceneRoot");
    sceneRoot->setNodeMask(DEFERRED_SCENE_MASK | SHADOW_CASTER_MASK);

    osg::ref_ptr<osg::Group> auxRoot = new osg::Group;
    auxRoot->setName("AuxRoot");
    auxRoot->setNodeMask(~DEFERRED_SCENE_MASK);

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(sceneRoot.get());
    root->addChild(auxRoot.get());

    // Main light
    osg::ref_ptr<osgVerse::LightDrawable> light0 = new osgVerse::LightDrawable;
    light0->setColor(osg::Vec3(4.0f, 4.0f, 3.8f));
    light0->setDirection(osg::Vec3(0.02f, 0.1f, -1.0f));
    light0->setDirectional(true);

    osg::ref_ptr<osg::Geode> lightGeode = new osg::Geode;
    lightGeode->addDrawable(light0.get());
    root->addChild(lightGeode.get());

    // Pipeline initialization
    osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline;
    MyViewer viewer(pipeline.get());
    setupStandardPipeline(pipeline.get(), &viewer,
                          osgVerse::StandardPipelineParameters(SHADER_DIR, SKYBOX_DIR "barcelona.hdr"));

    osgVerse::ShadowModule* shadow = static_cast<osgVerse::ShadowModule*>(pipeline->getModule("Shadow"));
    if (shadow)
    {
        if (shadow->getFrustumGeode())
        {
            shadow->getFrustumGeode()->setNodeMask(FORWARD_SCENE_MASK);
            root->addChild(shadow->getFrustumGeode());
        }
    }

    osgVerse::LightModule* light = static_cast<osgVerse::LightModule*>(pipeline->getModule("Light"));
    light->setMainLight(light0.get(), "Shadow");

    // Post-HUD displays and utilities
    osg::ref_ptr<osg::Camera> skyCamera = new osg::Camera;
    skyCamera->setClearMask(0);
    skyCamera->setRenderOrder(osg::Camera::POST_RENDER, 10000);
    skyCamera->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
    auxRoot->addChild(skyCamera.get());

    osg::ref_ptr<osg::Camera> postCamera = new osg::Camera;
    postCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    postCamera->setRenderOrder(osg::Camera::POST_RENDER, 10000);
    postCamera->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
    auxRoot->addChild(postCamera.get());

    osg::ref_ptr<osgVerse::SkyBox> skybox = new osgVerse::SkyBox;
    {
        skybox->setEnvironmentMap(osgDB::readImageFile(SKYBOX_DIR "sunset.hdr"));
        skyCamera->addChild(skybox.get());
    }

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
    imgui->setChineseSimplifiedFont(MISC_DIR "SourceHanSansHWSC-Regular.otf");
    imgui->initialize(new EditorContentHandler);
    imgui->addToView(&viewer, postCamera.get());

    // Start the viewer
    viewer.addEventHandler(new osgVerse::CommandHandler);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    //viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setCameraManipulator(new SceneManipulator);
    viewer.setSceneData(root.get());
    //viewer.setKeyEventSetsDone(0);

    // FIXME: how to avoid shadow problem...
    // If renderer->setGraphicsThreadDoesCull(false), which is used by DrawThreadPerContext & ThreadPerCamera,
    // Shadow will go jigger because the output texture is not sync-ed before lighting...
    // For SingleThreaded & CullDrawThreadPerContext it seems OK
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    return viewer.run();
}
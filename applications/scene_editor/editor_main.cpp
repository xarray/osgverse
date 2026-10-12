#include PREPENDED_HEADER
#include <osgDB/ReadFile>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <picojson.h>
#include "defines.h"
GlobalData g_data;

#ifdef OSG_LIBRARY_STATIC
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
USE_SERIALIZER_WRAPPER(DracoGeometry)
#endif
USE_GRAPICSWINDOW_IMPLEMENTATION(SDL)
USE_GRAPICSWINDOW_IMPLEMENTATION(GLFW)

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

/** Handle single left clicks in the 3D view: pick the object under the cursor and select
    the corresponding item (or the nearest parent item) in the scene hierarchy. */
class ScenePickHandler : public osgGA::GUIEventHandler
{
public:
    ScenePickHandler(osgVerse::SceneHierarchy* hierarchy, osg::Camera* cam, osg::Node* sceneRoot)
        : _hierarchy(hierarchy), _camera(cam), _sceneRoot(sceneRoot), _pushX(0.0f), _pushY(0.0f),
          _pushed(false) {}

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        if (ea.getEventType() == osgGA::GUIEventAdapter::PUSH &&
            ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
        {
            _pushX = ea.getX(); _pushY = ea.getY(); _pushed = true;
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::RELEASE && _pushed &&
                 ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
        {
            _pushed = false;

            // Never pick when the click belongs to the UI (or a gizmo): otherwise clicking a
            // hierarchy row would also pick the scene and the two selections would fight.
            if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_ALT) return false;
            if (ImGui::GetCurrentContext() != NULL)
            {
                ImGuiIO& io = ImGui::GetIO();
                if (io.WantCaptureMouse || ImGuizmo::IsUsing()) return false;
            }

            // Only accept a click, not a camera drag
            float dx = ea.getX() - _pushX, dy = ea.getY() - _pushY;
            if (dx * dx + dy * dy > 16.0f) return false;

            osg::Camera* cam = _camera.get(); osg::Node* root = _sceneRoot.get();
            if (cam == NULL || root == NULL || !_hierarchy.valid()) return false;

            // Build the world-space pick ray from the display camera and intersect the scene
            osg::Matrix invVP = osg::Matrix::inverse(
                cam->getViewMatrix() * cam->getProjectionMatrix());
            osg::Vec3d start = osg::Vec3d(ea.getXnormalized(), ea.getYnormalized(), -1.0) * invVP;
            osg::Vec3d end = osg::Vec3d(ea.getXnormalized(), ea.getYnormalized(), 1.0) * invVP;

            osgVerse::IntersectionResult result =
                osgVerse::findNearestIntersection(root, start, end, &_condition);

            // Select the nearest mapped *node* (walking up from the picked drawable). NodeSelector
            // can only bound osg::Node, so picking the drawable itself would leave no box to show.
            osgVerse::TreeView::TreeData* item = NULL;
            if (result.drawable.valid())
            {
                for (int i = (int)result.nodePath.size() - 1; i >= 0 && item == NULL; --i)
                    item = _hierarchy->findItem(result.nodePath[i]);
            }

            // Nothing hit (empty space, or the sky which is not part of sceneRoot): clear selection
            if (item != NULL) _hierarchy->selectItem(item, true);
            else _hierarchy->clearSelection(true);
        }
        return false;
    }

protected:
    osg::observer_ptr<osgVerse::SceneHierarchy> _hierarchy;
    osg::observer_ptr<osg::Camera> _camera;
    osg::observer_ptr<osg::Node> _sceneRoot;
    osgVerse::IntersectionCondition _condition;
    float _pushX, _pushY; bool _pushed;
};

EditorContentHandler::EditorContentHandler()
    : _uiFrameNumber(0)
{
    std::string err, file = BASE_DIR + "/misc/editor_defaults.json";
    osgVerse::SerializerFactory* factory = osgVerse::SerializerFactory::instance();

    std::ifstream in(file.c_str(), std::ios::in | std::ios::binary);
    picojson::value document; err = picojson::parse(document, in);
    if (err.empty())
    {
        picojson::value& propJson = document.get("property");
        picojson::array& blJson = propJson.get("blacklists").get<picojson::array>();
        for (size_t i = 0; i < blJson.size(); ++i)
            factory->registerPropertyHint(blJson[i].get<std::string>(), osgVerse::SerializerFactory::BLACKLISTED, NULL);

        // TODO: readonly properties
    }

    std::map<std::string, std::pair<std::string, GLenum>> glEnums = osgVerse::createGLEnumMapper();
    for (std::map<std::string, std::pair<std::string, GLenum>>::iterator it = glEnums.begin();
         it != glEnums.end(); ++it) { factory->registerGLEnum(it->second.first, it->first, it->second.second); }

    // Reuse a shared GL enum group for properties named differently (e.g. BlendFunc's
    // SourceRGB/DestinationRGB use the "Blending" group), otherwise their TypeSelector
    // combo would have no item to select and display as "???".
    std::map<std::string, std::string> glAliases = osgVerse::createGLEnumPropertyAliases();
    for (std::map<std::string, std::string>::iterator a = glAliases.begin();
         a != glAliases.end(); ++a)
    {
        for (std::map<std::string, std::pair<std::string, GLenum>>::iterator it = glEnums.begin();
             it != glEnums.end(); ++it)
        {
            if (it->second.first == a->second)
                factory->registerGLEnum(a->first, it->first, it->second.second);
        }
    }

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
            osg::Object* obj = (item != NULL) ?
                osgVerse::SceneDataProxy::get<osg::Object*>(item->userData.get()) : NULL;
            g_data.selector->clearAllSelectedNodes();
            if (obj != NULL)
            {
                _entry = osgVerse::SerializerFactory::instance()
                       ->createInterfaces(obj, NULL, _interfaces);
                _navigationData->setSelection(dynamic_cast<osg::Transform*>(obj));
                g_data.selector->addSelectedNode(dynamic_cast<osg::Node*>(obj));
            }
            else
            {
                // NULL item means "clear": empty the properties panel and the 3D selection
                _entry = NULL; _interfaces.clear();
                _navigationData->setSelection(NULL);
            }
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
    _navigation->size = osg::Vec2(0.38f, 0.115f);  // kept attached to the properties panel
    _navigation->alpha = 0.0f; _navigation->withBorder = false;
    _navigation->flags = ImGuiWindowFlags_NoDecoration;
    _navigation->userData = this;

    _properties = new osgVerse::Window(TR0("Properties") + "##editor");
    _properties->pos = osg::Vec2(0.75f, 0.02f);
    _properties->size = osg::Vec2(0.25f, 0.75f);
    _properties->alpha = 0.9f;
    _properties->flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar;
    _properties->userData = this;
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

    // Keep the navigation bar attached to the left side of the properties panel
    osg::Vec4 propRect = _properties->getCurrentRectangle();
    if (propRect[2] > 0.0f && propRect[3] > 0.0f)
    {
        _navigation->pos = osg::Vec2(propRect[0] - _navigation->size[0] - 0.005f, propRect[1]);
        _navigation->sizeApplied = false;  // apply the new position at each frame
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

    bool confirmed = false;
    osgVerse::ImGuiComponentBase::showConfirmDialog(confirmed);
    osgVerse::ImGuiComponentBase::showFloatingWindows(mgr, this);

    ImGui::PopFont();
    _uiFrameNumber++;
}

int main(int argc, char** argv)
{
    osg::setNotifyHandler(new osgVerse::ConsoleHandler);
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());
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
    int screenNo = 0; arguments.read("--screen", screenNo);

    // Set-up the viewer
    MyViewer viewer(pipeline.get());
    viewer.addEventHandler(new osgViewer::StatsHandler);
    //viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    //viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setCameraManipulator(new SceneManipulator);
    viewer.setSceneData(root.get());
    //viewer.setKeyEventSetsDone(0);
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.setUpViewOnSingleScreen(screenNo);

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
#if !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE)
    osg::ref_ptr<osg::Camera> skyCamera = osgVerse::SkyBox::createSkyCamera();
    auxRoot->addChild(skyCamera.get());

    osg::ref_ptr<osgVerse::SkyBox> skybox = new osgVerse::SkyBox(pipeline.get());
    {
        skybox->setSkyShaders(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "skybox.vert.glsl"),
                              osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "skybox.frag.glsl"));
        skybox->setEnvironmentMap(params.skyboxMap.get(), false);
        skyCamera->addChild(skybox.get());
    }
#endif

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
    osg::ref_ptr<EditorContentHandler> editorCore = new EditorContentHandler;

    osg::ref_ptr<osgVerse::ImGuiManager> imgui = new osgVerse::ImGuiManager;
    imgui->setChineseSimplifiedFont(MISC_DIR + "LXGWFasmartGothic.otf");
    imgui->initialize(editorCore.get());
    imgui->addToView(&viewer, postCamera.get());
    imgui->setGuiTexture("Navigation", BASE_DIR + "/textures/navigation.png");

    // Clicking an object in the 3D view selects the matching hierarchy item (and vice versa)
    viewer.addEventHandler(new ScenePickHandler(editorCore->getHierarchyData(),
                                                pipeline->getForwardCamera(), sceneRoot.get()));

    // Start with test scene
    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFiles(arguments);
    if (!scene) scene = osgDB::readNodeFile(BASE_DIR + "/models/Sponza.osgb");
    g_data.sceneRoot->addChild(scene.get());
    g_data.view->getCameraManipulator()->home(0.0);

    editorCore->getHierarchyData()->addItem(NULL, g_data.sceneRoot.get());
    int result = viewer.run();

    // Explicit teardown while the viewer (and its GL context) is still alive:
    // make sure all rendering threads have joined (the ImGui PRE/POST_DRAW
    // callbacks run in the drawing thread and must not race the cleanup),
    // then shutdown ImGui first (idempotent), then release the scene graph
    // before the static singletons (TileManager, LightGlobalManager...)
    // get destructed
    viewer.stopThreading();
    imgui->shutdown();
    viewer.setSceneData(NULL);
    return result;
}

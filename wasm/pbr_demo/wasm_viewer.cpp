#include <emscripten.h>
#include <SDL2/SDL.h>
#include "wasm_viewer.h"

#define TEST_PIPELINE 1
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
USE_GRAPICSWINDOW_IMPLEMENTATION(SDL)

osg::ref_ptr<Application> g_app = new Application;
void loop() { g_app->frame(); }

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

// Server structure
/* - <assets>: User resource folder copied from master/assets
   - osgVerse_ViewerWASM.data: preload data (only shaders)
   - osgVerse_ViewerWASM.html: main HTML page
   - osgVerse_ViewerWASM.js: main Javascript file
   - osgVerse_ViewerWASM.wasm: main WASM file
   - osgVerse_ViewerWASM.wasm.map: source-map for debugging
*/
#define SERVER_ADDR "http://127.0.0.1:8000/assets"
int main(int argc, char** argv)
{
    osgVerse::globalInitialize(argc, argv);
    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile(SERVER_ADDR "/models/Sponza/Sponza.gltf");
    if (scene.valid())
    {
        // Add tangent/bi-normal arrays for normal mapping
        osgVerse::TangentSpaceVisitor tsv;
        scene->accept(tsv);
    }

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    if (scene.valid()) sceneRoot->addChild(scene.get());
    sceneRoot->setMatrix(osg::Matrix::rotate(osg::PI_2, osg::X_AXIS));
    osgVerse::Pipeline::setPipelineMask(*sceneRoot, DEFERRED_SCENE_MASK | SHADOW_CASTER_MASK);

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(sceneRoot.get());

    // Main light
    osg::ref_ptr<osgVerse::LightDrawable> light0 = new osgVerse::LightDrawable;
    light0->setColor(osg::Vec3(3.0f, 3.0f, 2.8f));
    light0->setDirection(osg::Vec3(0.02f, 0.1f, -1.0f));
    light0->setDirectional(true);

    osg::ref_ptr<osg::Geode> lightGeode = new osg::Geode;
    lightGeode->addDrawable(light0.get());
    root->addChild(lightGeode.get());

    // Create the pipeline
    osgVerse::StandardPipelineParameters params(SHADER_DIR, SERVER_ADDR "/skyboxes/sunset.png");
    osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline;

    // Post-HUD display
    // FIXME: No forward scene can be added for WebGL at present
#if false
    osg::ref_ptr<osg::Camera> postCamera = osgVerse::SkyBox::createSkyCamera();
    root->addChild(postCamera.get());

    osg::ref_ptr<osgVerse::SkyBox> skybox = new osgVerse::SkyBox(pipeline.get());
    {
        skybox->setSkyShaders(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "skybox.vert.glsl"),
                              osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "skybox.frag.glsl"));
        skybox->setEnvironmentMap(params.skyboxMap.get(), false);
        osgVerse::Pipeline::setPipelineMask(*skybox, FORWARD_SCENE_MASK);
        postCamera->addChild(skybox.get());
    }
#endif

    // Create the viewer
#if TEST_PIPELINE
    //osg::setNotifyLevel(osg::INFO);
    MyViewer* viewer = new MyViewer(pipeline.get());
    OSG_NOTICE << "PBR + deferred rendering mode\n";
#else
    //osg::setNotifyLevel(osg::INFO);
    root = new osg::Group;
    root->addChild(postCamera.get());
    root->addChild(sceneRoot.get());

    osgViewer::Viewer* viewer = new osgViewer::Viewer;
#endif
    viewer->addEventHandler(new osgViewer::StatsHandler);
    viewer->addEventHandler(new osgGA::StateSetManipulator(viewer->getCamera()->getOrCreateStateSet()));
    viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer->setSceneData(root.get());

    // Create the graphics window
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->x = 0; traits->y = 0; traits->width = 800; traits->height = 600;
    traits->alpha = 8; traits->depth = 24; traits->stencil = 8;
    traits->windowDecoration = true; traits->doubleBuffer = true;
    traits->readDISPLAY(); traits->setUndefinedScreenDetailsToDefaultScreen();
    traits->windowingSystemPreference = "SDL";

    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    viewer->getCamera()->setGraphicsContext(gc.get());
    viewer->getCamera()->setViewport(0, 0, traits->width, traits->height);
    viewer->getCamera()->setDrawBuffer(GL_BACK);
    viewer->getCamera()->setReadBuffer(GL_BACK);

    // Setup the pipeline
#if TEST_PIPELINE
    params.originWidth = traits->width; params.originHeight = traits->height;
    params.shadowNumber = 1; params.enableAO = false; params.enablePostEffects = true;
    queryOpenGLVersion(pipeline.get(), true);
    setupStandardPipeline(pipeline.get(), viewer, params);

    // Post pipeline settings
    osgVerse::ShadowModule* shadow = static_cast<osgVerse::ShadowModule*>(pipeline->getModule("Shadow"));
    if (shadow && shadow->getFrustumGeode())
    {
        osgVerse::Pipeline::setPipelineMask(*shadow->getFrustumGeode(), FORWARD_SCENE_MASK);
        root->addChild(shadow->getFrustumGeode());
    }

    osgVerse::LightModule* light = static_cast<osgVerse::LightModule*>(pipeline->getModule("Light"));
    if (light) light->setMainLight(light0.get(), "Shadow");
#endif

    // Start the main loop
    atexit(SDL_Quit);
    g_app->setViewer(viewer);
    emscripten_set_main_loop(loop, -1, 0);
    return 0;
}

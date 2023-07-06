#include <emscripten.h>
#include <SDL2/SDL.h>
#include "wasm_viewer.h"

#define TEST_PIPELINE 0
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()

Application* g_app = new Application;;
void loop()
{
    SDL_Event e;
    while (SDL_PollEvent(&e))
    { if (g_app) g_app->handleEvent(e); }
    if (g_app) g_app->frame();
}

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

int main(int argc, char** argv)
{
    osgVerse::globalInitialize(argc, argv);
    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile(BASE_DIR "/models/Sponza/Sponza.gltf");
    if (scene.valid())
    {
        // Add tangent/bi-normal arrays for normal mapping
        osgVerse::TangentSpaceVisitor tsv;
        scene->accept(tsv);

        VBOSetupVisitor vsv;
        scene->accept(vsv);
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
    osgVerse::StandardPipelineParameters params(SHADER_DIR, SKYBOX_DIR "sunset.png");
    osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline;

    // Post-HUD display
    osg::ref_ptr<osg::Camera> postCamera = osgVerse::SkyBox::createSkyCamera();
#if defined(VERSE_WEBGL2)
    root->addChild(postCamera.get());
#elif defined(VERSE_WEBGL1)
    // No forward scene can be added for WebGL1
#endif

    osg::ref_ptr<osgVerse::SkyBox> skybox = new osgVerse::SkyBox(pipeline.get());
    {
        skybox->setSkyShaders(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "skybox.vert.glsl"),
                              osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "skybox.frag.glsl"));
        skybox->setEnvironmentMap(params.skyboxMap.get(), false);
        osgVerse::Pipeline::setPipelineMask(*skybox, FORWARD_SCENE_MASK);
        postCamera->addChild(skybox.get());
    }

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

    // Create the application object
    int width = 800, height = 600;
    g_app->setViewer(viewer, width, height);
    viewer->getCamera()->setDrawBuffer(GL_BACK);
    viewer->getCamera()->setReadBuffer(GL_BACK);

    // Start SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("[osgVerse] Could not init SDL: '%s'\n", SDL_GetError());
        return 1;
    }

    atexit(SDL_Quit);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Window* window = SDL_CreateWindow(
        "osgVerse_ViewerWASM", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, SDL_WINDOW_OPENGL);
    if (!window)
    {
        printf("[osgVerse] Could not create window: '%s'\n", SDL_GetError());
        return 1;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (context == NULL)
    {
        printf("[osgVerse] Could not create SDL context: '%s'\n", SDL_GetError());
        return 1;
    }

    // Setup the pipeline
#if TEST_PIPELINE
    params.originWidth = width; params.originHeight = height;
    params.shadowNumber = 0;
    params.enableAO = false; params.enablePostEffects = false;
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
    emscripten_set_main_loop(loop, -1, 0);
    return 0;
}

#include <emscripten.h>
#include <SDL2/SDL.h>
#include "wasm_viewer.h"

#define TEST_PIPELINE 1
#define TEST_SHADOW_MAP 0

USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()

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
    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile(
        argc > 1 ? argv[1] : BASE_DIR "/models/Sponza/Sponza.gltf");
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
    osgVerse::StandardPipelineParameters params(SHADER_DIR, SKYBOX_DIR "barcelona.hdr");
    osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline;
    
    // Post-HUD display
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

    // Start SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("[osgVerse] Could not init SDL: '%s'\n", SDL_GetError());
        return 1;
    }

    atexit(SDL_Quit);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    int width = 800, height = 600;
    SDL_Window* window = SDL_CreateWindow(
        "osgVerse_ViewerWASM", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, SDL_WINDOW_OPENGL);
    if (!window)
    {
        printf("[osgVerse] Could not create window: '%s'\n", SDL_GetError());
        return 1;
    }

    SDL_GL_CreateContext(window);

    // Create the viewer
#if TEST_PIPELINE
    //osg::setNotifyLevel(osg::INFO);
    MyViewer viewer(pipeline.get());
#else
    //osg::setNotifyLevel(osg::INFO);
    root = new osg::Group;
    root->addChild(postCamera.get());
    root->addChild(sceneRoot.get());

    osgViewer::Viewer viewer;
#endif
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.setSceneData(root.get());

    // Create the graphics window
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> gw =
        viewer.setUpViewerAsEmbeddedInWindow(0, 0, windowWidth, windowHeight);
    viewer.getCamera()->setDrawBuffer(GL_BACK);
    viewer.getCamera()->setReadBuffer(GL_BACK);

    // Setup the pipeline
#if TEST_PIPELINE
    params.enablePostEffects = false;
    queryOpenGLVersion(pipeline.get(), true);
    setupStandardPipeline(pipeline.get(), &viewer, params);

    // Post pipeline settings
    osgVerse::ShadowModule* shadow = static_cast<osgVerse::ShadowModule*>(pipeline->getModule("Shadow"));
    if (shadow && shadow->getFrustumGeode())
    {
        osgVerse::Pipeline::setPipelineMask(*shadow->getFrustumGeode(), FORWARD_SCENE_MASK);
        root->addChild(shadow->getFrustumGeode());
    }

#   if TEST_SHADOW_MAP
    osg::ref_ptr<osg::Camera> hudCamera = new osg::Camera;
    hudCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    hudCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    hudCamera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0));
    hudCamera->setViewMatrix(osg::Matrix::identity());
    hudCamera->setRenderOrder(osg::Camera::POST_RENDER, 10000);
    hudCamera->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
    osgVerse::Pipeline::setPipelineMask(*hudCamera, FORWARD_SCENE_MASK);

    float quadY = 0.0f;
    for (int i = 0; i < shadow->getShadowNumber(); ++i)
    {
        osg::Node* quad = osgVerse::createScreenQuad(
            osg::Vec3(0.0f, quadY, 0.0f), 0.2f, 0.2f, osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
        quad->getOrCreateStateSet()->setTextureAttributeAndModes(0, shadow->getTexture(i));
        hudCamera->addChild(quad); quadY += 0.21f;
    }
    root->addChild(hudCamera.get());
#   endif

    osgVerse::LightModule* light = static_cast<osgVerse::LightModule*>(pipeline->getModule("Light"));
    if (light) light->setMainLight(light0.get(), "Shadow");
#endif

    // Start the main loop
    while (!viewer.done())
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            osgGA::EventQueue* eq = gw->getEventQueue();
            switch (event.type)
            {
            case SDL_MOUSEMOTION:
                eq->mouseMotion(event.motion.x, event.motion.y); break;
            case SDL_MOUSEBUTTONDOWN:
                eq->mouseButtonPress(event.button.x, event.button.y, event.button.button); break;
            case SDL_MOUSEBUTTONUP:
                eq->mouseButtonRelease(event.button.x, event.button.y, event.button.button); break;
            case SDL_KEYUP:
                eq->keyRelease((osgGA::GUIEventAdapter::KeySymbol)event.key.keysym.sym); break;
            case SDL_KEYDOWN:
                eq->keyPress((osgGA::GUIEventAdapter::KeySymbol)event.key.keysym.sym); break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                {
                    eq->windowResize(0, 0, event.window.data1, event.window.data2);
                    gw->resized(0, 0, event.window.data1, event.window.data2);
                }
                break;
            case SDL_QUIT:
                viewer.setDone(true); break;
            default: break;
            }
        }

        viewer.frame();
#if VERSE_GLES
        eglSwapBuffers(display, surface);
#else
        SDL_GL_SwapWindow(sdlWindow);
#endif
    }

#if VERSE_GLES
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
#else
    SDL_GL_DeleteContext(sdlContext);
#endif
    SDL_DestroyWindow(sdlWindow);
    SDL_Quit();
    return 0;
}

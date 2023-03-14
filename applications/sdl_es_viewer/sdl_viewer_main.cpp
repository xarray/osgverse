#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <SDL.h>
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

#include <pipeline/SkyBox.h>
#include <pipeline/Pipeline.h>
#include <pipeline/LightModule.h>
#include <pipeline/ShadowModule.h>
#include <pipeline/Utilities.h>
#include <iostream>
#include <sstream>

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
    if (!scene) { OSG_WARN << "Failed to load GLTF model"; return 1; }

    // Add tangent/bi-normal arrays for normal mapping
    osgVerse::TangentSpaceVisitor tsv;
    scene->accept(tsv);

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->addChild(scene.get());
    sceneRoot->setMatrix(osg::Matrix::rotate(osg::PI_2, osg::X_AXIS));
    osgVerse::Pipeline::setPipelineMask(*sceneRoot, DEFERRED_SCENE_MASK | SHADOW_CASTER_MASK);

    osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osg.15,15,1.scale.0,0,-300.trans");
    //osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osg.0,0,-250.trans");
    if (otherSceneRoot.valid())
        osgVerse::Pipeline::setPipelineMask(*otherSceneRoot, ~DEFERRED_SCENE_MASK);

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(otherSceneRoot.get());
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
    int requiredGLContext = 100;  // 100: Compatible, 300: GL3
    int requiredGLSL = 130;       // GLSL version: 120, 130, 140, ...
    osgVerse::StandardPipelineParameters params(SHADER_DIR, SKYBOX_DIR "barcelona.hdr");
    osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline(requiredGLContext, requiredGLSL);
    
    // Post-HUD display
    osg::ref_ptr<osg::Camera> postCamera = osgVerse::SkyBox::createSkyCamera();
    root->addChild(postCamera.get());

    osg::ref_ptr<osgVerse::SkyBox> skybox = new osgVerse::SkyBox(pipeline.get());
    {
        skybox->setSkyShaders(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "skybox.vert.glsl"),
            osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "skybox.frag.glsl"));
        skybox->setEnvironmentMap(params.skyboxMap.get(), false);
        osgVerse::Pipeline::setPipelineMask(*skybox, ~DEFERRED_SCENE_MASK);
        postCamera->addChild(skybox.get());
    }

    // Start SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        OSG_WARN << "Unable to init SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    unsigned int windowWidth = 1280, windowHeight = 720;
    SDL_Window* sdlWindow = SDL_CreateWindow("osgVerse_ViewerSDL", 50, 50, windowWidth, windowHeight,
                                             SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    if (sdlWindow == NULL)
    {
        OSG_WARN << "Unable to create SDL window: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_GLContext sdlContext = SDL_GL_CreateContext(sdlWindow);
    if (sdlContext == NULL)
    {
        OSG_WARN << "Unable to create SDL context: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create the viewer
    MyViewer viewer(pipeline.get());
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);

    // Create the graphics window
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> gw =
        viewer.setUpViewerAsEmbeddedInWindow(0, 0, windowWidth, windowHeight);
    viewer.getCamera()->setDrawBuffer(GL_BACK);
    viewer.getCamera()->setReadBuffer(GL_BACK);

    // Setup the pipeline
    params.enableVSync = false;
    setupStandardPipeline(pipeline.get(), &viewer, params);

    // Post pipeline settings
    osgVerse::ShadowModule* shadow = static_cast<osgVerse::ShadowModule*>(pipeline->getModule("Shadow"));
    if (shadow && shadow->getFrustumGeode())
    {
        osgVerse::Pipeline::setPipelineMask(*shadow->getFrustumGeode(), FORWARD_SCENE_MASK);
        root->addChild(shadow->getFrustumGeode());
    }

    osgVerse::LightModule* light = static_cast<osgVerse::LightModule*>(pipeline->getModule("Light"));
    if (light) light->setMainLight(light0.get(), "Shadow");

    // Start the main loop
    SDL_GL_SetSwapInterval(0);
    SDL_GL_MakeCurrent(sdlWindow, sdlContext);
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
                    eq->windowResize(0, 0, event.window.data1, event.window.data2);
                break;
            case SDL_QUIT:
                viewer.setDone(true); break;
            default: break;
            }
        }

        viewer.frame();
        SDL_GL_SwapWindow(sdlWindow);
    }

    SDL_GL_DeleteContext(sdlContext);
    SDL_DestroyWindow(sdlWindow);
    SDL_Quit();
    return 0;
}

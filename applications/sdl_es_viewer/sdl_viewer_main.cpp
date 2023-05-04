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

#if defined(OSG_GLES1_AVAILABLE) || defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
#   include <EGL/egl.h>
#   define VERSE_GLES 1
#   define TEST_PIPELINE 1
#else
#   define TEST_PIPELINE 1
#endif
#include <SDL.h>
#include <SDL_syswm.h>
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

    osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osg.15,15,1.scale.0,0,-300.trans");
    //osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osg.0,0,-250.trans");
    if (otherSceneRoot.valid())
        osgVerse::Pipeline::setPipelineMask(*otherSceneRoot, ~DEFERRED_SCENE_MASK);

    osg::ref_ptr<osg::Group> root = new osg::Group;
    //if (argc == 1) root->addChild(otherSceneRoot.get());
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

    SDL_SysWMinfo sdlInfo; SDL_VERSION(&sdlInfo.version);
    SDL_GetWindowWMInfo(sdlWindow, &sdlInfo);

#if VERSE_GLES
    EGLint configAttribList[] =
    {
        EGL_RED_SIZE,       8,
        EGL_GREEN_SIZE,     8,
        EGL_BLUE_SIZE,      8,
        EGL_ALPHA_SIZE,     8,
        EGL_DEPTH_SIZE,     24,
        EGL_STENCIL_SIZE,   8/*EGL_DONT_CARE*/,
        EGL_SAMPLE_BUFFERS, 0,
        EGL_NONE
    };

    EGLNativeWindowType hWnd = sdlInfo.info.win.window;
    EGLDisplay display = eglGetDisplay(GetDC(hWnd));
    if (display == EGL_NO_DISPLAY)
    { OSG_WARN << "Failed to get EGL display" << std::endl; return 1; }

    EGLint majorVersion = 0, minorVersion = 0;
    if (!eglInitialize(display, &majorVersion, &minorVersion))
    { OSG_WARN << "Failed to initialize EGL display" << std::endl; return 1; }

    EGLint numConfigs = 0;
    if (!eglGetConfigs(display, NULL, 0, &numConfigs))
    { OSG_WARN << "Failed to get EGL display config" << std::endl; return 1; }

    EGLConfig config;
    if (!eglChooseConfig(display, configAttribList, &config, 1, &numConfigs))
    { OSG_WARN << "Failed to choose EGL config" << std::endl; return 1; }

    EGLint surfaceAttribList[] = { EGL_NONE, EGL_NONE };
    EGLSurface surface = eglCreateWindowSurface(
        display, config, (EGLNativeWindowType)hWnd, surfaceAttribList);
    if (surface == EGL_NO_SURFACE)
    { OSG_WARN << "Failed to create EGL surface" << std::endl; return 1; }

    EGLint contextAttribList[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE };
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribList);
    if (context == EGL_NO_CONTEXT)
    { OSG_WARN << "Failed to create EGL context" << std::endl; return 1; }

    // Make context current. GLES drawing commands can work now 
    eglMakeCurrent(display, surface, surface, context);
#else
    SDL_GLContext sdlContext = SDL_GL_CreateContext(sdlWindow);
    if (sdlContext == NULL)
    {
        OSG_WARN << "Unable to create SDL context: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_GL_SetSwapInterval(0);
    SDL_GL_MakeCurrent(sdlWindow, sdlContext);
#endif

    // Create the viewer
#if TEST_PIPELINE
    //osg::setNotifyLevel(osg::INFO);
    MyViewer viewer(pipeline.get());
#else
    osg::setNotifyLevel(osg::INFO);
    root = new osg::Group;
    root->addChild(postCamera.get());
    //root->addChild(sceneRoot.get());
    osgViewer::Viewer viewer;
#endif
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
#if TEST_PIPELINE
    queryOpenGLVersion(pipeline.get(), true);
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
                    gw->resized(0, 0, event.window.data1, event.window.data2);
                    eq->windowResize(0, 0, event.window.data1, event.window.data2);
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

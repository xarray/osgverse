#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <pipeline/Pipeline.h>
#include <pipeline/LightModule.h>
#include <pipeline/ShadowModule.h>
#include <pipeline/Utilities.h>
#include <readerwriter/GraphicsWindowSDL.h>
#include <readerwriter/Utilities.h>
#include <iostream>
#include <sstream>
#include <android/log.h>
#include <SDL.h>

// The plugins are registered explicitly instead of with USE_OSG_PLUGINS()/USE_VERSE_PLUGINS():
// those macros only do something when OSG_LIBRARY_STATIC and VERSE_STATIC_BUILD are defined for
// this module, and they would also name plugins which the Android build does not contain yet
USE_OSGPLUGIN(glsl)          // shaders (std_gbuffer.glsl and the other pipeline stages)
USE_OSGPLUGIN(verse_gltf)    // the Sponza scene
USE_OSGPLUGIN(verse_image)   // jpg/png images used by the scene and the skybox
USE_GRAPICSWINDOW_IMPLEMENTATION(SDL)

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

namespace
{
    /** OSG prints its messages to stdout, which Android throws away. This keeps the message
        mapping of osgVerse::ConsoleHandler but sends the messages to the Android log */
    class LogcatHandler : public osgVerse::ConsoleHandler
    {
    public:
        virtual void notifyLevel0(osg::NotifySeverity severity, const std::string& message)
        { __android_log_write(ANDROID_LOG_ERROR, "osgVerse", message.c_str()); }

        virtual void notifyLevel1(osg::NotifySeverity severity, const std::string& message)
        { __android_log_write(ANDROID_LOG_WARN, "osgVerse", message.c_str()); }

        virtual void notifyLevel2(osg::NotifySeverity severity, const std::string& message)
        { __android_log_write(ANDROID_LOG_INFO, "osgVerse", message.c_str()); }

        virtual void notifyLevel3(osg::NotifySeverity severity, const std::string& message)
        { __android_log_write(ANDROID_LOG_DEBUG, "osgVerse", message.c_str()); }
    };

    /** SDL resizes its window to the surface provided by the activity, whatever size was
        requested when creating it, so the real drawing area is queried back here */
    void queryWindowSize(osgVerse::GraphicsWindowSDL* sdlWin, int& width, int& height)
    {
        if (!sdlWin) return;

        // getHandle() hands over the ownership of a newly created handle to the caller
        osgVerse::GraphicsWindowHandle* handle = sdlWin->getHandle();
        if (!handle) return;
        if (handle->nativeHandle)
            SDL_GetWindowSize((SDL_Window*)handle->nativeHandle, &width, &height);
        handle->unref();

        sdlWin->resized(0, 0, width, height);
    }
}

int SDL_main(int argc, char* argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    __android_log_print(ANDROID_LOG_VERBOSE, "osgVerse", "SDL_main() started");

    // The assets of the APK are unpacked to <application directory>/assets by the Java side
    // (see AssetExtractor.java), because the file readers of OSG and osgVerse have no way to
    // look inside an APK. That directory is therefore used as the base directory here
    const char* storagePath = SDL_AndroidGetInternalStoragePath();
    std::string dataDir = std::string(storagePath ? storagePath : "") + "/assets";
    osgVerse::InitParameters initParams = osgVerse::defaultInitParameters();
    initParams.verseBaseDir = dataDir;
    osgVerse::globalInitialize(argc, argv, initParams);
    osg::setNotifyHandler(new LogcatHandler);
    SHADER_DIR = BASE_DIR + "/shaders/";
    SKYBOX_DIR = BASE_DIR + "/skyboxes/";

    // The scene graph
    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile(BASE_DIR + "/models/Sponza/Sponza.gltf");
    if (!scene.valid()) OSG_WARN << "[AndroidDemo] Failed to load the Sponza scene" << std::endl;

    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    if (scene.valid()) sceneRoot->addChild(scene.get());
    sceneRoot->setName("PbrSceneRoot");
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

    // Create the pipeline and the viewer
    osgVerse::StandardPipelineParameters params(SHADER_DIR, SKYBOX_DIR + "sunset.png");
    osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline;
    osg::ref_ptr<MyViewer> viewer = new MyViewer(pipeline.get());

    viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer->setSceneData(root.get());

    // Create the graphics window. GraphicsWindowSDL opens and manages the SDL window itself,
    // so the application does not need an SDL event loop of its own: the events are collected
    // by GraphicsWindowSDL::checkEvents() and delivered to OSG's event queue
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->x = 0; traits->y = 0;
    traits->alpha = 8; traits->depth = 24; traits->stencil = 8;
    traits->windowDecoration = false; traits->doubleBuffer = true;
    traits->windowingSystemPreference = "SDL";

    // Start from the size of the display: SDL derives the requested orientation of the
    // activity from it, and adjusts the window to the surface of the activity anyway
    SDL_DisplayMode displayMode;
    if (SDL_GetCurrentDisplayMode(0, &displayMode) == 0)
        { traits->width = displayMode.w; traits->height = displayMode.h; }
    else
        { traits->width = 1280; traits->height = 720; }

    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    if (!gc.valid())
    {
        OSG_FATAL << "[AndroidDemo] Failed to create the graphics context" << std::endl;
        return 1;
    }

    // The size of the drawing area has to be known before the pipeline allocates its buffers
    gc->realize();
    int width = traits->width, height = traits->height;
    queryWindowSize(dynamic_cast<osgVerse::GraphicsWindowSDL*>(gc.get()), width, height);
    OSG_NOTICE << "[AndroidDemo] Drawing area: " << width << " x " << height << std::endl;

    viewer->getCamera()->setGraphicsContext(gc.get());
    viewer->getCamera()->setViewport(0, 0, width, height);
    viewer->getCamera()->setProjectionMatrixAsPerspective(
        30.0f, static_cast<double>(width) / static_cast<double>(height), 1.0f, 10000.0f);
    viewer->getCamera()->setDrawBuffer(GL_BACK);
    viewer->getCamera()->setReadBuffer(GL_BACK);

    // Setup the deferred pipeline
    queryOpenGLVersion(pipeline.get(), true, gc.get());
    params.enablePostEffects = true; params.enableAO = true;
    params.shadowNumber = 3;
    params.originWidth = width; params.originHeight = height;
    if (!setupStandardPipeline(pipeline.get(), viewer.get(), params)) return 1;

    osgVerse::ShadowModule* shadow = static_cast<osgVerse::ShadowModule*>(pipeline->getModule("Shadow"));
    if (shadow && shadow->getFrustumGeode())
    {
        osgVerse::Pipeline::setPipelineMask(*shadow->getFrustumGeode(), FORWARD_SCENE_MASK);
        root->addChild(shadow->getFrustumGeode());
    }

    osgVerse::LightModule* light = static_cast<osgVerse::LightModule*>(pipeline->getModule("Light"));
    if (light) light->setMainLight(light0.get(), "Shadow");

    // Start the main loop
    while (!viewer->done())
    {
        viewer->frame();

        // Leave the loop as soon as the window is gone, which happens when the activity is
        // finished (the Android back button, for example). SDLActivity waits for this thread
        // in onDestroy(), so it must not keep running after that
        if (!gc->valid()) { viewer->setDone(true); break; }
    }

    __android_log_print(ANDROID_LOG_VERBOSE, "osgVerse", "SDL_main() finished");
    return 0;
}

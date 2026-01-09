#include PREPENDED_HEADER
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
#define TEST_PIPELINE 1
#define TEST_SHADOW_MAP 0

#ifdef VERSE_WITH_SDL
#   if VERSE_APPLE
#      define TEST_VULKAN_IMPLEMENTATION 0
#   elif defined(OSG_GLES1_AVAILABLE) || defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
#      define TEST_VULKAN_IMPLEMENTATION 1
#   else
#      define TEST_VULKAN_IMPLEMENTATION 0
#endif

#   include <SDL.h>
#   include <SDL_syswm.h>
USE_GRAPICSWINDOW_IMPLEMENTATION(SDL)
#endif
USE_GRAPICSWINDOW_IMPLEMENTATION(GLFW)

#if TEST_VULKAN_IMPLEMENTATION
// TODO
#endif

#include <pipeline/SkyBox.h>
#include <pipeline/Pipeline.h>
#include <pipeline/LightModule.h>
#include <pipeline/ShadowModule.h>
#include <pipeline/Utilities.h>
#include <readerwriter/Utilities.h>
#include <iostream>
#include <sstream>

#ifdef OSG_LIBRARY_STATIC
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
#endif

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
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osg::setNotifyHandler(new osgVerse::ConsoleHandler);

    bool useGLFW = arguments.read("--use-glfw"), useWin32Ex = arguments.read("--use-win32ex");
    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFiles(arguments);
    if (!scene) scene = osgDB::readNodeFile(BASE_DIR + "/models/Sponza/Sponza.gltf");
    if (scene.valid())
    {
        // Add tangent/bi-normal arrays for normal mapping
        osgVerse::TangentSpaceVisitor tsv; scene->accept(tsv);
        osgVerse::FixedFunctionOptimizer ffo; scene->accept(ffo);
    }

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    if (scene.valid()) sceneRoot->addChild(scene.get());
    sceneRoot->setMatrix(osg::Matrix::scale(125.0f, 125.0f, 125.0f));
    osgVerse::Pipeline::setPipelineMask(*sceneRoot, DEFERRED_SCENE_MASK | SHADOW_CASTER_MASK);

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(sceneRoot.get());

    osg::ref_ptr<osg::Camera> postCamera = osgVerse::SkyBox::createSkyCamera();
    root->addChild(postCamera.get());

    // Main light
    osg::ref_ptr<osgVerse::LightDrawable> light0 = new osgVerse::LightDrawable;
    light0->setColor(osg::Vec3(1.5f, 1.5f, 1.2f));
    light0->setDirection(osg::Vec3(0.02f, 0.1f, -1.0f));
    light0->setDirectional(true);

    osg::ref_ptr<osg::Geode> lightGeode = new osg::Geode;
    lightGeode->addDrawable(light0.get());
    root->addChild(lightGeode.get());

    // Create the pipeline
    osgVerse::StandardPipelineParameters params(SHADER_DIR, SKYBOX_DIR + "barcelona.hdr");
    osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline;
    
    // Create the viewer
#if TEST_PIPELINE
    MyViewer viewer(pipeline.get());
#else
    root = new osg::Group;
    root->addChild(postCamera.get());
    root->addChild(sceneRoot.get());

    osgViewer::Viewer viewer;
#endif
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);

    // Create the graphics window
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->x = 50; traits->y = 50; traits->width = 1280; traits->height = 720;
    traits->alpha = 8; traits->depth = 24; traits->stencil = 8;
    traits->windowDecoration = true; traits->doubleBuffer = true;
    traits->readDISPLAY(); traits->setUndefinedScreenDetailsToDefaultScreen();

#if OSG_VERSION_GREATER_THAN(3, 4, 0)
#   ifdef VERSE_WITH_SDL
    traits->windowingSystemPreference = "SDL";
#   endif
    if (useGLFW) traits->windowingSystemPreference = "GLFW";
    else if (useWin32Ex) traits->windowingSystemPreference = "Win32NV";  // FIXME: will crash currently...
#endif

    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    viewer.getCamera()->setGraphicsContext(gc.get());
    viewer.getCamera()->setViewport(0, 0, traits->width, traits->height);
    viewer.getCamera()->setDrawBuffer(GL_BACK);
    viewer.getCamera()->setReadBuffer(GL_BACK);
    viewer.getCamera()->setProjectionMatrixAsPerspective(
        30.0f, static_cast<double>(traits->width) / static_cast<double>(traits->height), 1.0f, 10000.0f);
    
    // Setup the pipeline
#if TEST_PIPELINE
    //params.enablePostEffects = false;
    queryOpenGLVersion(pipeline.get(), true, gc.get());
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

    // Post-HUD display
    osg::ref_ptr<osgVerse::SkyBox> skybox = new osgVerse::SkyBox(pipeline.get());
    {
        skybox->setSkyShaders(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "skybox.vert.glsl"),
            osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "skybox.frag.glsl"));
        skybox->setEnvironmentMap(params.skyboxMap.get(), false);
        osgVerse::Pipeline::setPipelineMask(*skybox, FORWARD_SCENE_MASK);
        postCamera->addChild(skybox.get());
    }

    // Start the main loop
    while (!viewer.done())
    {
        viewer.frame();
    }
    return 0;
}

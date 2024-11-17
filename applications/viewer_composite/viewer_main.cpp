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
#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

// Change the macro to switch between composite viewer / viewer with slaves
#define USE_COMPOSITE_VIEWER 1

#include <pipeline/SkyBox.h>
#include <pipeline/Pipeline.h>
#include <pipeline/LightModule.h>
#include <pipeline/ShadowModule.h>
#include <pipeline/Utilities.h>
#include <readerwriter/Utilities.h>
#include <iostream>
#include <sstream>

USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()

#if USE_COMPOSITE_VIEWER
class MyView : public osgViewer::View
{
public:
    MyView(osgVerse::Pipeline* p) : osgViewer::View(), _pipeline(p) {}
    osg::ref_ptr<osgVerse::Pipeline> _pipeline;

protected:
    virtual osg::GraphicsOperation* createRenderer(osg::Camera* camera)
    {
        if (_pipeline.valid()) return _pipeline->createRenderer(camera);
        else return osgViewer::View::createRenderer(camera);
    }
};
#else
class MyViewer : public osgViewer::Viewer
{
public:
    MyViewer() : osgViewer::Viewer() {}
    std::vector<osg::ref_ptr<osgVerse::Pipeline>> pipelines;

protected:
    virtual osg::GraphicsOperation* createRenderer(osg::Camera* camera)
    {
        for (size_t i = 0; i < pipelines.size(); ++i)
        {
            osgVerse::Pipeline* pipeline = pipelines[i].get();
            if (pipeline->isValidCamera(camera))
                return pipeline->createRenderer(camera);
        }
        return osgViewer::Viewer::createRenderer(camera);
    }
};
#endif

int main(int argc, char** argv)
{
    osgVerse::globalInitialize(argc, argv);
    osg::setNotifyHandler(new osgVerse::ConsoleHandler);
    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile(
        argc > 1 ? argv[1] : BASE_DIR + "/models/Sponza/Sponza.gltf.125,125,125.scale");
    if (!scene) { OSG_WARN << "Failed to load GLTF model"; return 1; }

    // Add tangent/bi-normal arrays for normal mapping
    osgVerse::TangentSpaceVisitor tsv; scene->accept(tsv);
    osgVerse::FixedFunctionOptimizer ffo; scene->accept(ffo);

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->addChild(scene.get());
    sceneRoot->setMatrix(osg::Matrix::rotate(osg::PI_2, osg::X_AXIS));
    osgVerse::Pipeline::setPipelineMask(*sceneRoot, DEFERRED_SCENE_MASK | SHADOW_CASTER_MASK);

    osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osg.15,15,1.scale.0,0,-300.trans");
    //osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osg.0,0,-250.trans");
    if (otherSceneRoot.valid())
        osgVerse::Pipeline::setPipelineMask(*otherSceneRoot, FORWARD_SCENE_MASK);

    osg::ref_ptr<osg::Group> root = new osg::Group;
    if (argc == 1) root->addChild(otherSceneRoot.get());
    root->addChild(sceneRoot.get());

    // Main light
    osg::ref_ptr<osgVerse::LightDrawable> light0 = new osgVerse::LightDrawable;
    light0->setColor(osg::Vec3(1.5f, 1.5f, 1.2f));
    light0->setDirection(osg::Vec3(0.02f, 0.1f, -1.0f));
    light0->setDirectional(true);

    osg::ref_ptr<osg::Geode> lightGeode = new osg::Geode;
    lightGeode->addDrawable(light0.get());
    root->addChild(lightGeode.get());

    // Set the pipeline parameters
    int requiredGLContext = 100;  // 100: Compatible, 300: GL3
    int requiredGLSL = 130;       // GLSL version: 120, 130, 140, ...
    osgVerse::StandardPipelineParameters params(SHADER_DIR, SKYBOX_DIR + "barcelona.hdr");

    // Post-HUD display
    osg::ref_ptr<osg::Camera> postCamera = osgVerse::SkyBox::createSkyCamera();
    root->addChild(postCamera.get());

    osg::ref_ptr<osgVerse::SkyBox> skybox = new osgVerse::SkyBox;
    {
        skybox->setSkyShaders(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "skybox.vert.glsl"),
            osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "skybox.frag.glsl"));
        skybox->setEnvironmentMap(params.skyboxMap.get(), false);
        osgVerse::Pipeline::setPipelineMask(*skybox, FORWARD_SCENE_MASK);
        postCamera->addChild(skybox.get());
    }

    // Use multiple views
    const static int numViews = 2;

#if USE_COMPOSITE_VIEWER
    osg::ref_ptr<MyView> views[numViews];
    for (int i = 0; i < numViews; ++i)
    {
        // Create the pipeline
        osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline(requiredGLContext, requiredGLSL);

        // Realize the viewer
        views[i] = new MyView(pipeline.get());
        views[i]->addEventHandler(new osgViewer::StatsHandler);
        views[i]->addEventHandler(new osgViewer::WindowSizeHandler);
        views[i]->setCameraManipulator(new osgGA::TrackballManipulator);
        views[i]->setSceneData(root.get());
        views[i]->setUpViewInWindow(50 + 640 * i, 50, 640, 480);

        // Setup the pipeline... Always call viewer.setUp*() before setupStandardPipeline()!
        setupStandardPipeline(pipeline.get(), views[i].get(), params);

        // Post pipeline settings
        osgVerse::ShadowModule* shadow = static_cast<osgVerse::ShadowModule*>(pipeline->getModule("Shadow"));
        if (shadow && shadow->getFrustumGeode())
        {
            osgVerse::Pipeline::setPipelineMask(*shadow->getFrustumGeode(), FORWARD_SCENE_MASK);
            root->addChild(shadow->getFrustumGeode());
        }

        osgVerse::LightModule* light = static_cast<osgVerse::LightModule*>(pipeline->getModule("Light"));
        if (light) light->setMainLight(light0.get(), "Shadow");
    }

    // Start the composite viewer
    osgViewer::CompositeViewer viewer;
    for (int i = 0; i < numViews; ++i)
        viewer.addView(views[i].get());
#else
    MyViewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());

    // FIXME: how to avoid shadow problem...
    // If renderer->setGraphicsThreadDoesCull(false), which is used by DrawThreadPerContext & ThreadPerCamera,
    // Shadow will go jigger because the output texture is not sync-ed before lighting...
    // For SingleThreaded & CullDrawThreadPerContext it seems OK
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);

    double viewPos = (double)numViews - 1.0;
    for (int i = 0; i < numViews; ++i, viewPos -= 2.0)
    {
        osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
        traits->screenNum = 0;
        traits->x = 50 + 640 * i; traits->y = 50;
        traits->width = 640; traits->height = 480;
        traits->windowDecoration = true; traits->doubleBuffer = true;
        traits->sharedContext = 0; traits->readDISPLAY();
        traits->setUndefinedScreenDetailsToDefaultScreen();

        osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
        if (!gc) return -1;

        osg::ref_ptr<osg::Camera> camera = new osg::Camera;
        camera->setName("SlaveCamera" + std::to_string(i));
        camera->setGraphicsContext(gc.get());
        camera->setViewport(new osg::Viewport(0, 0, 640, 480));
        camera->setDrawBuffer(traits->doubleBuffer ? GL_BACK : GL_FRONT);
        camera->setReadBuffer(traits->doubleBuffer ? GL_BACK : GL_FRONT);

        osg::Matrix projOffset = osg::Matrix::scale((double)numViews, 1.0, 1.0)
                               * osg::Matrix::translate(viewPos, 0.0, 0.0);
        viewer.addSlave(camera.get(), projOffset, osg::Matrix());

        // Create the pipeline
        osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline(requiredGLContext, requiredGLSL);
        viewer.pipelines.push_back(pipeline);

        // Setup the pipeline
        setupStandardPipelineEx(pipeline.get(), &viewer, camera.get(), params);

        // Post pipeline settings
        osgVerse::ShadowModule* shadow = static_cast<osgVerse::ShadowModule*>(pipeline->getModule("Shadow"));
        if (shadow && shadow->getFrustumGeode())
        {
            osgVerse::Pipeline::setPipelineMask(*shadow->getFrustumGeode(), FORWARD_SCENE_MASK);
            root->addChild(shadow->getFrustumGeode());
        }

        osgVerse::LightModule* light = static_cast<osgVerse::LightModule*>(pipeline->getModule("Light"));
        if (light) light->setMainLight(light0.get(), "Shadow");
    }
#endif

    // FIXME: how to avoid shadow problem...
    // If renderer->setGraphicsThreadDoesCull(false), which is used by DrawThreadPerContext & ThreadPerCamera,
    // Shadow will go jigger because the output texture is not sync-ed before lighting...
    // For SingleThreaded & CullDrawThreadPerContext it seems OK
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);

    // Start the main loop
    while (!viewer.done())
    {
        viewer.frame();
    }
    return 0;
}

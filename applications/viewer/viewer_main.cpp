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

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#define TRANSPARENT_OBJECT_TEST 0
#define INDICATOR_TEST 0

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

class SetPipelineHandler : public osgGA::GUIEventHandler
{
public:
    SetPipelineHandler(osgVerse::Pipeline* p, osgVerse::LightDrawable* l)
        : _pipeline(p), _mainLight(l) {}

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        switch (ea.getEventType())
        {
        case osgGA::GUIEventAdapter::KEYUP:
            if (ea.getKey() == 'X')
            {
                OSG_NOTICE << "*** Changing to standard pipeline" << std::endl;
                _pipeline->clearStagesFromView(view);

                osgVerse::StandardPipelineParameters params(SHADER_DIR, SKYBOX_DIR "barcelona.hdr");
                setupStandardPipeline(_pipeline.get(), view, params);

                osgVerse::LightModule* light = static_cast<osgVerse::LightModule*>(_pipeline->getModule("Light"));
                if (light) light->setMainLight(_mainLight.get(), "Shadow");
            }
            else if (ea.getKey() == 'Z')
            {
                OSG_NOTICE << "*** Changing to fixed pipeline" << std::endl;
                _pipeline->clearStagesFromView(view);
            }
            break;
        }
        return false;
    }

protected:
    osg::observer_ptr<osgVerse::Pipeline> _pipeline;
    osg::observer_ptr<osgVerse::LightDrawable> _mainLight;
};

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
    osg::ref_ptr<osg::Node> scene = (argc > 1) ?
        osgDB::readNodeFiles(arguments) : osgDB::readNodeFile(BASE_DIR "/models/Sponza/Sponza.gltf");
    if (!scene) { OSG_WARN << "Failed to load GLTF model"; return 1; }

    // Add tangent/bi-normal arrays for normal mapping
    osgVerse::TangentSpaceVisitor tsv;
    scene->accept(tsv);

    if (arguments.read("--save"))
    {
        osg::ref_ptr<osgDB::Options> options = new osgDB::Options;
        options->setPluginStringData("TargetFileVersion", "91");  // the first version

        // Compress and optimize textures (it may take a while)
        // With op: CPU memory = 167.5MB, GPU memory = 0.8GB
        // Without: CPU memory = 401.8MB, GPU memory = 2.1GB
        osgVerse::TextureOptimizer texOp; scene->accept(texOp);
        osgDB::writeNodeFile(*scene, "pbr_scene.osgb", options.get());
        return 0;
    }

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->setName("PbrSceneRoot");
    sceneRoot->addChild(scene.get());
    sceneRoot->setMatrix(osg::Matrix::rotate(osg::PI_2, osg::X_AXIS));
    osgVerse::Pipeline::setPipelineMask(*sceneRoot, DEFERRED_SCENE_MASK);

    osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osg.15,15,1.scale.0,0,-300.trans");
    //osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osg.0,0,-250.trans");
    if (otherSceneRoot.valid())
        osgVerse::Pipeline::setPipelineMask(*otherSceneRoot, FORWARD_SCENE_MASK);

    osg::ref_ptr<osg::Group> root = new osg::Group;
    if (argc == 1) root->addChild(otherSceneRoot.get());
    root->addChild(sceneRoot.get());
    root->setName("Root");

#if TRANSPARENT_OBJECT_TEST
    // Test transparent object
    osg::BoundingSphere bs = sceneRoot->getBound();
    osg::ShapeDrawable* shape = new osg::ShapeDrawable(
        new osg::Sphere(bs.center() + osg::X_AXIS * bs.radius() * 1.5f, bs.radius() * 0.4f));
    shape->setColor(osg::Vec4(1.0f, 1.0f, 0.0f, 0.5f));
    shape->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    shape->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    osgVerse::Pipeline::setPipelineMask(*geode, FORWARD_SCENE_MASK & (~FIXED_SHADING_MASK));
    geode->addDrawable(shape);

    // Add tangent/bi-normal arrays for normal mapping
    tsv.reset(); geode->accept(tsv);
    root->addChild(geode.get());
#endif

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
        osgVerse::Pipeline::setPipelineMask(*skybox, FORWARD_SCENE_MASK);
        postCamera->addChild(skybox.get());
    }

    // Realize the viewer
    MyViewer viewer(pipeline.get());
    viewer.addEventHandler(new SetPipelineHandler(pipeline.get(), light0.get()));
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
    viewer.setUpViewOnSingleScreen(0);  // Always call viewer.setUp*() before setupStandardPipeline()!

    // Setup the pipeline
    params.enablePostEffects = true; params.enableAO = true;
    setupStandardPipeline(pipeline.get(), &viewer, params);

    // How to use clear color instead of skybox...
    //postCamera->removeChild(skybox.get());
    //pipeline->getStage("GBuffer")->camera->setClearColor(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));

    // Post pipeline settings
    osgVerse::ShadowModule* shadow = static_cast<osgVerse::ShadowModule*>(pipeline->getModule("Shadow"));
    if (shadow && shadow->getFrustumGeode())
    {
        osgVerse::Pipeline::setPipelineMask(*shadow->getFrustumGeode(), FORWARD_SCENE_MASK);
        root->addChild(shadow->getFrustumGeode());
    }

    osgVerse::LightModule* light = static_cast<osgVerse::LightModule*>(pipeline->getModule("Light"));
    if (light) light->setMainLight(light0.get(), "Shadow");

#if INDICATOR_TEST
    // Experimental: select model and show a highlight outline
    osgVerse::Pipeline::setModelIndicator(scene.get(), osgVerse::Pipeline::SelectIndicator);
#endif

    // Start the main loop
    while (!viewer.done())
    {
        viewer.frame();
    }
    return 0;
}

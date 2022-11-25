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

#define FOG_TEST 0
#define PARTICLE_TEST 0
#define INDICATOR_TEST 0

#if FOG_TEST
#   include <osg/Fog>
#endif
#if PARTICLE_TEST
#   include <osgParticle/PrecipitationEffect>
#endif

#include <pipeline/SkyBox.h>
#include <pipeline/Pipeline.h>
#include <pipeline/LightModule.h>
#include <pipeline/ShadowModule.h>
#include <pipeline/Utilities.h>
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
    sceneRoot->setNodeMask(DEFERRED_SCENE_MASK | SHADOW_CASTER_MASK);
    sceneRoot->setMatrix(osg::Matrix::rotate(osg::PI_2, osg::X_AXIS));

    osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osgt.15,15,1.scale.0,0,-300.trans");
    //osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osgt.0,0,-250.trans");
    if (otherSceneRoot.valid()) otherSceneRoot->setNodeMask(~DEFERRED_SCENE_MASK);

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

    // Start the pipeline
    int requiredGLContext = 100;  // 100: Compatible, 300: GL3
    int requiredGLSL = 130;       // GLSL version: 120, 130, 140, ...
    osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline(requiredGLContext, requiredGLSL);

    MyViewer viewer(pipeline.get());
    osgVerse::StandardPipelineParameters params(SHADER_DIR, SKYBOX_DIR "barcelona.hdr");
    setupStandardPipeline(pipeline.get(), &viewer, params);

    osgVerse::ShadowModule* shadow = static_cast<osgVerse::ShadowModule*>(pipeline->getModule("Shadow"));
    if (shadow && shadow->getFrustumGeode())
    {
        shadow->getFrustumGeode()->setNodeMask(FORWARD_SCENE_MASK);
        root->addChild(shadow->getFrustumGeode());
    }

    osgVerse::LightModule* light = static_cast<osgVerse::LightModule*>(pipeline->getModule("Light"));
    if (light) light->setMainLight(light0.get(), "Shadow");

    // Post-HUD display
    osg::ref_ptr<osg::Camera> postCamera = osgVerse::SkyBox::createSkyCamera();
    root->addChild(postCamera.get());

    osg::ref_ptr<osgVerse::SkyBox> skybox = new osgVerse::SkyBox(pipeline.get());
    {
        skybox->setSkyShaders(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "skybox.vert.glsl"),
                              osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "skybox.frag.glsl"));
        skybox->setEnvironmentMap(params.skyboxMap.get(), false);
        skybox->setNodeMask(~DEFERRED_SCENE_MASK);
        postCamera->addChild(skybox.get());
    }

#if INDICATOR_TEST
    // Experimental: select model and show a highlight outline
    osgVerse::Pipeline::setModelIndicator(scene.get(), osgVerse::Pipeline::SelectIndicator);
#endif

#if FOG_TEST
    osg::Vec2 fogRange(1500.0f, 20000.0f);
    osg::Vec3 fogColor(0.5f, 0.5f, 0.55f);
    pipeline->getStage("Final")->getUniform("FogDistance")->set(fogRange);
    pipeline->getStage("Final")->getUniform("FogColor")->set(fogColor);

    osg::Fog* fog = new osg::Fog;
    fog->setMode(osg::Fog::LINEAR);
    fog->setStart(fogRange[0]); fog->setEnd(fogRange[1]);
    fog->setColor(osg::Vec4(fogColor, 1.0f));
    otherSceneRoot->getOrCreateStateSet()->setAttributeAndModes(fog);
#endif

#if PARTICLE_TEST
    osg::ref_ptr<osg::Camera> particleCamera = new osg::Camera;
    particleCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    particleCamera->setRenderOrder(osg::Camera::POST_RENDER, 10000);
    particleCamera->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
    root->addChild(particleCamera.get());

    osgParticle::PrecipitationEffect* precipitationEffect = new osgParticle::PrecipitationEffect;
    precipitationEffect->setParticleSize(10.0f);
    precipitationEffect->setWind(osg::Vec3(1.0f, 1.0f, 0.0f));
    precipitationEffect->rain(1.0f);
    precipitationEffect->setNodeMask(~DEFERRED_SCENE_MASK);
    particleCamera->addChild(precipitationEffect);
#endif

    // Start the viewer
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
    while (!viewer.done())
    {
        //std::cout << sceneRoot->getBound().center() << "; " << sceneRoot->getBound().radius() << "\n";
        viewer.frame();
    }
    return 0;
}

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

#include <osgEarth/Version>
#include <osgEarth/Notify>
#include <osgEarth/GeoTransform>
#include <osgEarth/MapNode>
#include <osgEarth/GLUtils>
#if OSGEARTH_VERSION_GREATER_THAN(2, 10, 1)
#   include <osgEarth/EarthManipulator>
#   include <osgEarth/AutoClipPlaneHandler>
#   include <osgEarth/Sky>
#else
#   include <osgEarthUtil/EarthManipulator>
#   include <osgEarthUtil/AutoClipPlaneHandler>
#   include <osgEarthUtil/Sky>
#endif

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <pipeline/Pipeline.h>
#include <pipeline/ShadowModule.h>
#include <pipeline/LightModule.h>
#include <pipeline/Utilities.h>
#include <iostream>
#include <sstream>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()

#if OSGEARTH_VERSION_GREATER_THAN(2, 10, 1)
#   define EarthManipulator osgEarth::EarthManipulator
#   define AutoClipPlaneCullCallback osgEarth::AutoClipPlaneCullCallback
#else
#   define EarthManipulator osgEarth::Util::EarthManipulator
#   define AutoClipPlaneCullCallback osgEarth::Util::AutoClipPlaneCullCallback
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

class InteractiveHandler : public osgGA::GUIEventHandler
{
public:
    InteractiveHandler(EarthManipulator* em) : _manipulator(em), _viewpointSet(false) {}
    void addViewpoint(const osgEarth::Viewpoint& vp) { _viewPoints.push_back(vp); }
    
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        if (ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN ||
            ea.getEventType() == osgGA::GUIEventAdapter::PUSH)
        {
            if (_viewpointSet)
            { _manipulator->clearViewpoint(); _viewpointSet = false; }
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::KEYUP)
        {
            if (ea.getKey() >= '1' && ea.getKey() <= '9')
            {
                int index = ea.getKey() - '1';
                if (index < _viewPoints.size())
                { _manipulator->setViewpoint(_viewPoints[index], 12.0); _viewpointSet = true; }
            }
        }
        return false;
    }

protected:
    osg::observer_ptr<EarthManipulator> _manipulator;
    std::vector<osgEarth::Viewpoint> _viewPoints;
    bool _viewpointSet;
};

osgEarth::Viewpoint createPlaceOnEarth(osg::Group* sceneRoot, osgEarth::MapNode* mapNode, const std::string& file,
                                       const osg::Matrix& baseT, double lng, double lat, double h, float heading)
{
    class UpdatePlacerCallback : public osg::NodeCallback
    {
    public:
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            osg::Matrix w = static_cast<osg::MatrixTransform*>(node)->getWorldMatrices()[0];
            if (_rotated) w = osg::Matrix::rotate(osg::PI_2, osg::X_AXIS) * w;
            if (_scene.valid()) _scene->setMatrix(w); traverse(node, nv);
        }

        UpdatePlacerCallback(osg::MatrixTransform* s, bool r) : _scene(s), _rotated(r) {}
        osg::observer_ptr<osg::MatrixTransform> _scene; bool _rotated;
    };

    // Create a scene and a placer on earth for sceneRoot to copy
    osg::ref_ptr<osg::MatrixTransform> scene = new osg::MatrixTransform;
    osg::ref_ptr<osg::MatrixTransform> placer = new osg::MatrixTransform;
    placer->setUpdateCallback(new UpdatePlacerCallback(scene.get(), true));

    osg::ref_ptr<osg::MatrixTransform> baseScene = new osg::MatrixTransform;
    {
        osg::ref_ptr<osg::Node> loadedModel = osgDB::readNodeFile(file);
        if (loadedModel.valid()) baseScene->addChild(loadedModel.get());
        baseScene->setMatrix(baseT); scene->addChild(baseScene.get());
    }

    osgVerse::TangentSpaceVisitor tsv;
    scene->accept(tsv);
    sceneRoot->addChild(scene.get());

    // Use a geo-transform node to place scene on earth
    osg::ref_ptr<osgEarth::GeoTransform> geo = new osgEarth::GeoTransform;
    geo->addChild(placer.get());
    mapNode->addChild(geo.get());

    // Set the geo-transform pose
    osgEarth::GeoPoint pos(osgEarth::SpatialReference::get("wgs84"),
                           lng, lat, h, osgEarth::ALTMODE_RELATIVE);
    geo->setPosition(pos);

    // Set the viewpoint
    osgEarth::Viewpoint vp;
    vp.setNode(geo.get());
    vp.heading()->set(heading, osgEarth::Units::DEGREES);
    vp.pitch()->set(-50.0, osgEarth::Units::DEGREES);
    vp.range()->set(scene->getBound().radius() * 10.0, osgEarth::Units::METERS);
    return vp;
}

int main(int argc, char** argv)
{
#if OSGEARTH_VERSION_GREATER_THAN(2, 10, 1)
    osgEarth::initialize();
#endif
    osgVerse::globalInitialize(argc, argv);
    osg::ArgumentParser arguments(&argc, argv);

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    osgVerse::Pipeline::setPipelineMask(*sceneRoot, DEFERRED_SCENE_MASK | SHADOW_CASTER_MASK);
    
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addChild(sceneRoot.get());

    // Main light
    osg::ref_ptr<osgVerse::LightDrawable> light0 = new osgVerse::LightDrawable;
    light0->setColor(osg::Vec3(4.0f, 4.0f, 3.8f));
    light0->setDirection(osg::Vec3(0.0f, -1.0f, -0.5f));
    light0->setDirectional(true);

    osg::ref_ptr<osg::Geode> lightGeode = new osg::Geode;
    lightGeode->addDrawable(light0.get());
    root->addChild(lightGeode.get());

    // Create the pipeline
    osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline;

    // Realize the viewer
    MyViewer viewer(pipeline.get());
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setSceneData(root.get());
    viewer.setThreadingModel(osgViewer::Viewer::CullDrawThreadPerContext);
    viewer.setUpViewOnSingleScreen(0);  // Always call viewer.setUp*() before setupStandardPipeline()!

    // osgEarth configuration
    osg::ref_ptr<osg::Node> earthRoot = osgDB::readNodeFile("F:/DataSet/osgEarthData/t2.earth");
    if (earthRoot.valid())
    {
        // Tell the database pager to not modify the unref settings
        viewer.getDatabasePager()->setUnrefImageDataAfterApplyPolicy(true, false);

        // install our default manipulator (do this before calling load)
        osg::ref_ptr<EarthManipulator> earthMani = new EarthManipulator(arguments);
        viewer.setCameraManipulator(earthMani.get());

        // read in the Earth file and open the map node
        osg::ref_ptr<osgEarth::MapNode> mapNode = osgEarth::MapNode::get(earthRoot.get());
#if OSGEARTH_VERSION_GREATER_THAN(2, 10, 1)
        if (!mapNode->open()) { OSG_WARN << "Failed to open earth map"; return 1; }
#else
        if (!mapNode.valid()) { OSG_WARN << "Failed to open earth map"; return 1; }
#endif

        // default uniform values and disable small feature culling
        osgVerse::Pipeline::Stage* gbufferStage = pipeline->getStage("GBuffer");
        osgEarth::GLUtils::setGlobalDefaults(pipeline->getForwardCamera()->getOrCreateStateSet());
        gbufferStage->camera->setSmallFeatureCullingPixelSize(-1.0f);
        gbufferStage->camera->addCullCallback(new AutoClipPlaneCullCallback(mapNode.get()));

        // thread-safe initialization of the OSG wrapper manager. Calling this here
        // prevents the "unsupported wrapper" messages from OSG
        osgDB::Registry::instance()->getObjectWrapperManager()->findWrapper("osg::Image");

        // sky initialization
        osgEarth::Util::Ephemeris* ephemeris = new osgEarth::Util::Ephemeris;
        osg::ref_ptr<osgEarth::Util::SkyNode> skyNode = osgEarth::Util::SkyNode::create();
        skyNode->setName("SkyNode");
        skyNode->setEphemeris(ephemeris);
        skyNode->setDateTime(osgEarth::DateTime(2022, 7, 1, 10));
        skyNode->attach(&viewer, 0);
        skyNode->setLighting(true);
        skyNode->addChild(mapNode.get());

        // Add earth root to scene graph
        osg::ref_ptr<osg::Group> earthParent = new osg::Group;
        earthParent->addChild(earthRoot.get());
        earthParent->addChild(skyNode.get());
        osgVerse::Pipeline::setPipelineMask(*earthParent, ~DEFERRED_SCENE_MASK);
        root->addChild(earthParent.get());

        // Create places
        osgEarth::Viewpoint vp0 = createPlaceOnEarth(
            sceneRoot.get(), mapNode.get(), "../models/Sponza/Sponza.gltf",
            osg::Matrix::scale(1.0, 1.0, 1.0) * osg::Matrix::rotate(0.1, osg::Z_AXIS),
            119.008f, 25.9f, 15.0f, -40.0f);

        osg::ref_ptr<InteractiveHandler> interacter = new InteractiveHandler(earthMani.get());
        interacter->addViewpoint(vp0);
        viewer.addEventHandler(interacter.get());
    }

    // Setup the pipeline
    setupStandardPipeline(pipeline.get(), &viewer,
                          osgVerse::StandardPipelineParameters(SHADER_DIR, SKYBOX_DIR "sunset.png"));

    // Post pipeline settings
    osgVerse::ShadowModule* shadow = static_cast<osgVerse::ShadowModule*>(pipeline->getModule("Shadow"));
    if (shadow)
    {
        if (shadow->getFrustumGeode())
        {
            osgVerse::Pipeline::setPipelineMask(*shadow->getFrustumGeode(), FORWARD_SCENE_MASK);
            root->addChild(shadow->getFrustumGeode());
        }
    }

    osgVerse::LightModule* light = static_cast<osgVerse::LightModule*>(pipeline->getModule("Light"));
    if (light) light->setMainLight(light0.get(), "Shadow");

    float lightZ = -1.0f; bool lightD = true;
    while (!viewer.done())
    {
        if (lightD) { if (lightZ > -0.9f) lightD = false; else lightZ += 0.001f; }
        else { if (lightZ < -4.0f) lightD = true; else lightZ -= 0.001f; }
        light0->setDirection(osg::Vec3(0.0f, lightZ, -0.8f));
        viewer.frame();
    }
    return 0;
}

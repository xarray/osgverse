#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/ConvertUTF>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <osgEarth/Version>
#include <osgEarth/Registry>
#include <osgEarth/Capabilities>
#include <osgEarth/Notify>
#include <osgEarth/GeoTransform>
#include <osgEarth/MapNode>
#include <osgEarth/GLUtils>
#if OSGEARTH_VERSION_GREATER_THAN(2, 10, 2)
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
#include <readerwriter/Utilities.h>
#include <iostream>
#include <sstream>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

//#define VERSE_FORCE_SDL 1
#define TEST_PIPELINE 1

#if defined(OSG_GLES1_AVAILABLE) || defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
#   define VERSE_GLES_DESKTOP 1
USE_GRAPICSWINDOW_IMPLEMENTATION(SDL)
#elif defined(VERSE_FORCE_SDL)
USE_GRAPICSWINDOW_IMPLEMENTATION(SDL)
#endif

#define EARTH_INPUT_MASK 0x00010000
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()

#if OSGEARTH_VERSION_GREATER_THAN(2, 10, 2)
#   define EarthManipulator osgEarth::EarthManipulator
#   define AutoClipPlaneCullCallback osgEarth::AutoClipPlaneCullCallback
#else
#   define EarthManipulator osgEarth::Util::EarthManipulator
#   define AutoClipPlaneCullCallback osgEarth::Util::AutoClipPlaneCullCallback
#endif

#if TEST_PIPELINE
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

    osgVerse::TangentSpaceVisitor tsv; scene->accept(tsv);
    osgVerse::FixedFunctionOptimizer ffo; scene->accept(ffo);
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
#endif

int main(int argc, char** argv)
{
#if OSGEARTH_VERSION_GREATER_THAN(2, 10, 2)
    osgEarth::initialize();
#endif
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osg::setNotifyHandler(new osgVerse::ConsoleHandler);

    osgEarth::setNotifyLevel(osg::INFO);
    osgEarth::Registry::instance()->getCapabilities();
#ifdef VERSE_GLES_DESKTOP
    osgEarth::Registry::instance()->overrideTerrainEngineDriverName() = "mp";
#endif

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    osgVerse::Pipeline::setPipelineMask(*sceneRoot, DEFERRED_SCENE_MASK | SHADOW_CASTER_MASK);
    
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addChild(sceneRoot.get());

#if TEST_PIPELINE
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
#else
    osgViewer::Viewer viewer;
#endif
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setSceneData(root.get());

    // Create the graphics window
#if defined(VERSE_GLES_DESKTOP) || defined(VERSE_FORCE_SDL)
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->x = 50; traits->y = 50; traits->width = 1280; traits->height = 720;
    traits->alpha = 8; traits->depth = 24; traits->stencil = 8;
    traits->windowDecoration = true; traits->doubleBuffer = true;
    traits->readDISPLAY(); traits->setUndefinedScreenDetailsToDefaultScreen();
    traits->windowingSystemPreference = "SDL";

    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    viewer.getCamera()->setGraphicsContext(gc.get());
    viewer.getCamera()->setViewport(0, 0, traits->width, traits->height);
    viewer.getCamera()->setDrawBuffer(GL_BACK);
    viewer.getCamera()->setReadBuffer(GL_BACK);
    viewer.getCamera()->setProjectionMatrixAsPerspective(
        30.0f, static_cast<double>(traits->width) / static_cast<double>(traits->height), 1.0f, 10000.0f);
#else
    viewer.setUpViewOnSingleScreen(0);  // Always call viewer.setUp*() before setupStandardPipeline()
#endif
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.realize();

#if TEST_PIPELINE
    // Setup the pipeline
    osgVerse::StandardPipelineParameters spp(SHADER_DIR, SKYBOX_DIR + "sunset.png");
    spp.enableUserInput = true;
#   if false
    spp.addUserInputStage("Forward2", EARTH_INPUT_MASK,
                          osgVerse::StandardPipelineParameters::BEFORE_FINAL_STAGE,
                          osgVerse::StandardPipelineParameters::DEPTH_PARTITION_BACK);
    spp.addUserInputStage("Forward", EARTH_INPUT_MASK,
                          osgVerse::StandardPipelineParameters::BEFORE_FINAL_STAGE,
                          osgVerse::StandardPipelineParameters::DEPTH_PARTITION_FRONT);
#   else
    spp.addUserInputStage("Forward", EARTH_INPUT_MASK,
                          osgVerse::StandardPipelineParameters::BEFORE_FINAL_STAGE);
#   endif

#   ifdef VERSE_GLES_DESKTOP
    spp.withEmbeddedViewer = true;
#   endif
    setupStandardPipeline(pipeline.get(), &viewer, spp);
#endif

    // osgEarth configuration
    std::stringstream ss;
    ss << "<map version=\"2\">\n"
       << "<image driver=\"xyz\" enabled=\"true\" name=\"gaode_sat\" profile=\"global-mercator\""
       << "       url=\"http://webst0[1234].is.autonavi.com/appmaptile?style=6&amp;x={x}&amp;y={y}&amp;z={z}\">"
       << "<cache_policy min_time=\"0\" usage=\"no_cache\"/></image></map>";
    osgDB::ReaderWriter* rw = osgDB::Registry::instance()->getReaderWriterForExtension("earth");
    if (!rw) OSG_WARN << "Failed to read earth data from stream!" << std::endl;
    
    osg::ref_ptr<osg::Node> earthRoot = rw ? rw->readNode(ss).getNode() : osgDB::readNodeFile("simple.earth");
    if (earthRoot.valid())
    {
        // Tell the database pager to not modify the unref settings
        viewer.getDatabasePager()->setUnrefImageDataAfterApplyPolicy(true, false);

        // install our default manipulator (do this before calling load)
        osg::ref_ptr<EarthManipulator> earthMani = new EarthManipulator(arguments);
        viewer.setCameraManipulator(earthMani.get());

        // read in the Earth file and open the map node
        osg::ref_ptr<osgEarth::MapNode> mapNode = osgEarth::MapNode::get(earthRoot.get());
#if OSGEARTH_VERSION_GREATER_THAN(2, 10, 2)
        if (!mapNode->open()) { OSG_WARN << "Failed to open earth map"; return 1; }
#else
        if (!mapNode.valid()) { OSG_WARN << "Failed to open earth map"; return 1; }
#endif

#if TEST_PIPELINE
        // default uniform values and disable small feature culling
        osgVerse::Pipeline::Stage* gbufferStage = pipeline->getStage("GBuffer");
        osgEarth::GLUtils::setGlobalDefaults(pipeline->getStage("Forward")->camera->getOrCreateStateSet());
        gbufferStage->camera->setSmallFeatureCullingPixelSize(-1.0f);
        gbufferStage->camera->addCullCallback(new AutoClipPlaneCullCallback(mapNode.get()));
#else
        osgEarth::GLUtils::setGlobalDefaults(viewer.getCamera()->getOrCreateStateSet());
        viewer.getCamera()->setSmallFeatureCullingPixelSize(-1.0f);
        viewer.getCamera()->addCullCallback(new AutoClipPlaneCullCallback(mapNode.get()));
#endif

        // thread-safe initialization of the OSG wrapper manager. Calling this here
        // prevents the "unsupported wrapper" messages from OSG
        osgDB::Registry::instance()->getObjectWrapperManager()->findWrapper("osg::Image");

        // sky initialization
        /*osgEarth::Util::Ephemeris* ephemeris = new osgEarth::Util::Ephemeris;
        osg::ref_ptr<osgEarth::Util::SkyNode> skyNode = osgEarth::Util::SkyNode::create();
        skyNode->setName("SkyNode");
        skyNode->setEphemeris(ephemeris);
        skyNode->setDateTime(osgEarth::DateTime(2022, 7, 1, 10));
        skyNode->attach(&viewer, 0);
        skyNode->setLighting(true);
        skyNode->addChild(mapNode.get());*/

        // Add earth root to scene graph
        osg::ref_ptr<osg::Group> earthParent = new osg::Group;
        earthParent->addChild(earthRoot.get());
        //earthParent->addChild(skyNode.get());
        osgVerse::Pipeline::setPipelineMask(*earthParent, EARTH_INPUT_MASK);
        root->addChild(earthParent.get());

#if TEST_PIPELINE
        // Create places
        osgEarth::Viewpoint vp0 = createPlaceOnEarth(
            sceneRoot.get(), mapNode.get(), BASE_DIR + "/models/Sponza/Sponza.gltf",
            osg::Matrix::scale(1.0, 1.0, 1.0) * osg::Matrix::rotate(0.1, osg::Z_AXIS),
            119.008f, 25.9f, 15.0f, -40.0f);

        osg::ref_ptr<InteractiveHandler> interacter = new InteractiveHandler(earthMani.get());
        interacter->addViewpoint(vp0);
        viewer.addEventHandler(interacter.get());
#endif
    }

#if TEST_PIPELINE
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
#endif

    //osg::setNotifyLevel(osg::INFO);
    while (!viewer.done())
    {
        viewer.frame();
    }
    return 0;
}

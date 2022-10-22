#include <osg/io_utils>
#include <osg/LightSource>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgEarth/Notify>
#include <osgEarth/EarthManipulator>
#include <osgEarth/GeoTransform>
#include <osgEarth/MapNode>
#include <osgEarth/Sky>
#include <osgEarth/GLUtils>
#include <osgEarth/AutoClipPlaneHandler>
#include <iostream>
#include <sstream>
#include <readerwriter/LoadSceneFBX.h>
#include <readerwriter/LoadSceneGLTF.h>
#include <pipeline/Pipeline.h>
#include <pipeline/Utilities.h>

class InteractiveHandler : public osgGA::GUIEventHandler
{
public:
    InteractiveHandler(osgEarth::EarthManipulator* em) : _manipulator(em), _viewpointSet(false) {}
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
                { _manipulator->setViewpoint(_viewPoints[index], 2.0); _viewpointSet = true; }
            }
        }
        return false;
    }

protected:
    osg::observer_ptr<osgEarth::EarthManipulator> _manipulator;
    std::vector<osgEarth::Viewpoint> _viewPoints;
    bool _viewpointSet;
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

osgEarth::Viewpoint createPlaceOnEarth(osg::Group* sceneRoot, osgEarth::MapNode* mapNode, const std::string& file,
                                       const osg::Matrix& baseT, double lng, double lat, double h, float heading)
{
    class UpdatePlacerCallback : public osg::NodeCallback
    {
    public:
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            osg::Matrix w = static_cast<osg::MatrixTransform*>(node)->getWorldMatrices()[0];
            if (_rotated) w = osg::Matrix::rotate(osg::PI_2, osg::Z_AXIS) * w;
            if (_scene.valid()) _scene->setMatrix(w); traverse(node, nv);
        }

        UpdatePlacerCallback(osg::MatrixTransform* s, bool r) : _scene(s), _rotated(r) {}
        osg::observer_ptr<osg::MatrixTransform> _scene; bool _rotated;
    };

    // Create a scene and a placer on earth for sceneRoot to copy
    osg::ref_ptr<osg::MatrixTransform> scene = new osg::MatrixTransform;
    osg::ref_ptr<osg::MatrixTransform> placer = new osg::MatrixTransform;
    placer->setUpdateCallback(new UpdatePlacerCallback(scene.get(), false));

    osg::ref_ptr<osg::MatrixTransform> baseScene = new osg::MatrixTransform;
    {
        osg::ref_ptr<osg::Node> fbxScene = osgVerse::loadFbx(file);
        if (fbxScene.valid()) baseScene->addChild(fbxScene.get());
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
    vp.pitch()->set(-20.0, osgEarth::Units::DEGREES);
    vp.range()->set(scene->getBound().radius() * 10.0, osgEarth::Units::METERS);
    return vp;
}

int main(int argc, char** argv)
{
    osgEarth::initialize();
    osg::ArgumentParser arguments(&argc, argv);

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->setNodeMask(DEFERRED_SCENE_MASK | SHADOW_CASTER_MASK);
    
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addChild(sceneRoot.get());

#if true
    osgViewer::Viewer viewer;
#else
    osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline;
    MyViewer viewer(pipeline.get());
    setupStandardPipeline(pipeline.get(), &viewer, SHADER_DIR, SKYBOX_DIR "barcelona.hdr", 1920, 1080);
#endif

    // osgEarth configuration
    osg::ref_ptr<osg::Node> earthRoot = osgDB::readNodeFile("F:/DataSet/osgEarthData/t2.earth");
    if (earthRoot.valid())
    {
        // Tell the database pager to not modify the unref settings
        viewer.getDatabasePager()->setUnrefImageDataAfterApplyPolicy(true, false);

        // install our default manipulator (do this before calling load)
        osg::ref_ptr<osgEarth::EarthManipulator> earthMani = new osgEarth::EarthManipulator(arguments);
        viewer.setCameraManipulator(earthMani.get());

        // read in the Earth file and open the map node
        osg::ref_ptr<osgEarth::MapNode> mapNode = osgEarth::MapNode::get(earthRoot.get());
        if (!mapNode->open()) { OSG_WARN << "Failed to open earth map"; return 1; }

#if true
        osgEarth::GLUtils::setGlobalDefaults(viewer.getCamera()->getOrCreateStateSet());
        viewer.getCamera()->setSmallFeatureCullingPixelSize(-1.0f);
        viewer.getCamera()->addCullCallback(new osgEarth::AutoClipPlaneCullCallback(mapNode.get()));
#else
        // default uniform values and disable small feature culling
        osgEarth::GLUtils::setGlobalDefaults(pipeline->getForwardCamera()->getOrCreateStateSet());
        pipeline->getForwardCamera()->setSmallFeatureCullingPixelSize(-1.0f);
        pipeline->getForwardCamera()->addCullCallback(new osgEarth::AutoClipPlaneCullCallback(mapNode.get()));
#endif

        // thread-safe initialization of the OSG wrapper manager. Calling this here
        // prevents the "unsupported wrapper" messages from OSG
        osgDB::Registry::instance()->getObjectWrapperManager()->findWrapper("osg::Image");

        // sky initialization
        osgEarth::Util::Ephemeris* ephemeris = new osgEarth::Util::Ephemeris;
        osg::ref_ptr<osgEarth::Util::SkyNode> skyNode = osgEarth::Util::SkyNode::create();
        skyNode->setName("SkyNode");
        skyNode->setEphemeris(ephemeris);
        skyNode->setDateTime(osgEarth::DateTime(2021, 7, 1, 10));
        skyNode->attach(&viewer, 0);
        skyNode->setLighting(true);
        skyNode->addChild(mapNode.get());

        // Add earth root to scene graph
        osg::ref_ptr<osg::Group> earthParent = new osg::Group;
        earthParent->addChild(earthRoot.get());
        earthParent->addChild(skyNode.get());
        earthParent->setNodeMask(~DEFERRED_SCENE_MASK);
        root->addChild(earthParent.get());

        // Create places
        osgEarth::Viewpoint vp0 = createPlaceOnEarth(
            sceneRoot.get(), mapNode.get(), "../Escher/·ïÇì²èÉ½/SM_ChaShan.fbx",
            osg::Matrix::scale(0.05, 0.05, 0.05) * osg::Matrix::rotate(0.1, osg::Z_AXIS),
            118.804f, 26.484f, -10.0f, 90.0f);
        osgEarth::Viewpoint vp1 = createPlaceOnEarth(
            sceneRoot.get(), mapNode.get(), "../Escher/ÓñÏª¸Ö³§/SM_YuXiGangChang.FBX",
            osg::Matrix::scale(0.01, 0.01, 0.01) * osg::Matrix::rotate(0.0, osg::Z_AXIS),
            119.008f, 25.9f, 0.0f, -45.0f);
        osgEarth::Viewpoint vp2 = createPlaceOnEarth(
            sceneRoot.get(), mapNode.get(), "../Escher/RanQiHeZuoShe/SM_RanQiHeZuoShe_L.fbx",
            osg::Matrix::rotate(0.0, osg::Z_AXIS), 119.018f, 25.465f, 0.0f, -45.0f);

        osg::ref_ptr<InteractiveHandler> interacter = new InteractiveHandler(earthMani.get());
        interacter->addViewpoint(vp0);
        interacter->addViewpoint(vp1);
        interacter->addViewpoint(vp2);
        viewer.addEventHandler(interacter.get());
    }

    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setSceneData(root.get());
    while (!viewer.done())
    {
        viewer.frame();
    }
    return 0;
}

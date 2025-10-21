#include PREPENDED_HEADER
#include <osg/io_utils>
#include <osg/CullFace>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/Archive>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgGA/EventVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <modeling/Math.h>
#include <readerwriter/EarthManipulator.h>
#include <readerwriter/TileCallback.h>
#include <readerwriter/FileCache.h>
#include <pipeline/Pipeline.h>
#include <VerseCommon.h>
#include <iostream>
#include <sstream>

#ifdef OSG_LIBRARY_STATIC
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
USE_SERIALIZER_WRAPPER(DracoGeometry)
#endif

#define EARTH_INTERSECTION_MASK 0xf0000000
extern std::vector<osg::Camera*> configureEarthRendering(
        osgViewer::View& viewer, osg::Group* root, osg::Node* earth, osgVerse::EarthAtmosphereOcean& eData,
        const std::string& mainFolder, unsigned int mask, int w, int h);
extern osg::Node* configureCityData(osgViewer::View& viewer, osg::Node* earthRoot,
                                    osgVerse::EarthAtmosphereOcean& earthRenderingUtils,
                                    const std::string& mainFolder, unsigned int mask);
extern osg::Camera* configureUI(osgViewer::View& viewer, osg::Group* root,
                                const std::string& mainFolder, int w, int h);

class EnvironmentHandler : public osgGA::GUIEventHandler
{
public:
    EnvironmentHandler(osgVerse::EarthAtmosphereOcean* eao, const std::string& folder)
    :   _earthData(eao), _mainFolder(folder), _pressingKey(0)
    {
    }

    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        if (ea.getEventType() == osgGA::GUIEventAdapter::FRAME)
        {
            //
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN) _pressingKey = ea.getKey();
        else if (ea.getEventType() == osgGA::GUIEventAdapter::KEYUP) _pressingKey = 0;

        if (_pressingKey > 0)
        {
            //
        }
        return false;
    }

protected:
    osgVerse::EarthAtmosphereOcean* _earthData;
    std::string _mainFolder;
    int _pressingKey;
};

static std::string createCustomPath(int type, const std::string& prefix, int x, int y, int z)
{
    if (type == osgVerse::TileCallback::ORTHOPHOTO)
    {
        if (z > 4)
        {
            std::string prefix2 = "https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}";
            return osgVerse::TileCallback::createPath(prefix2, x, pow(2, z) - y - 1, z);
        }
    }
    return osgVerse::TileCallback::createPath(prefix, x, y, z);
}

int main(int argc, char** argv)
{
    osgViewer::Viewer viewer;
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osg::setNotifyHandler(new osgVerse::ConsoleHandler(false));
    osgVerse::updateOsgBinaryWrappers();
    osgDB::Registry::instance()->addFileExtensionAlias("tif", "verse_tiff");

    std::string mainFolder = BASE_DIR + "/models"; arguments.read("--folder", mainFolder);
    std::string skirtRatio = "0.05"; arguments.read("--skirt", skirtRatio);
    int w = 1920, h = 1080; arguments.read("--resolution", w, h);

    // Create earth
    std::string earthURLs = " Orthophoto=mbtiles://" + mainFolder + "/Earth/DOM_lv4.mbtiles/{z}-{x}-{y}.jpg"
                            " Elevation=mbtiles://" + mainFolder + "/Earth/DEM_lv3.mbtiles/{z}-{x}-{y}.tif"
                            " OceanMask=mbtiles://" + mainFolder + "/Earth/Mask_lv3.mbtiles/{z}-{x}-{y}.tif"
                            /*" MaximumLevel=8"*/" UseWebMercator=1 UseEarth3D=1 OriginBottomLeft=1"
                            " TileElevationScale=3 TileSkirtRatio=" + skirtRatio;
    osg::ref_ptr<osgDB::Options> earthOptions = new osgDB::Options(earthURLs);
    earthOptions->setPluginData("UrlPathFunction", (void*)createCustomPath);

    osg::ref_ptr<osg::Node> earth = osgDB::readNodeFile("0-0-0.verse_tms", earthOptions.get());
    if (!earth) { OSG_FATAL << "Main earth scene is missing!\n"; return 1; }

    // Configure scene components
    osgVerse::EarthAtmosphereOcean earthRenderingUtils;
    osg::ref_ptr<osg::MatrixTransform> earthRoot = new osg::MatrixTransform;
    std::vector<osg::Camera*> cameras = configureEarthRendering(
        viewer, earthRoot.get(), earth.get(), earthRenderingUtils, mainFolder, EARTH_INTERSECTION_MASK, w, h);
    earthRoot->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    osg::Camera* sceneCamera = cameras[0];
    //osg::Camera* atmosphereCamera = cameras[1];
    //osg::Camera* oceanCamera = cameras[2];
    sceneCamera->setNearFarRatio(0.00001);
    sceneCamera->addChild(configureCityData(
        viewer, earthRoot.get(), earthRenderingUtils, mainFolder, EARTH_INTERSECTION_MASK));

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(earthRoot.get());
    //root->addChild(configureUI(viewer, earthRoot.get(), mainFolder, w, h));

    // Configure the manipulator
    osg::ref_ptr<osgVerse::EarthManipulator> earthManipulator = new osgVerse::EarthManipulator;
    earthManipulator->setIntersectionMask(EARTH_INTERSECTION_MASK);
    earthManipulator->setWorldNode(earth.get());
    earthManipulator->setThrowAllowed(false);

    //osg::Vec3d pos = osgVerse::Coordinate::convertLLAtoECEF(
    //    osg::Vec3d(osg::inDegrees(0.0), osg::inDegrees(120.0), 10000.0));
    //earthManipulator->moveTo(pos, 0.0, 120.0);

    // Start the viewer
    osg::ref_ptr<osgVerse::FileCache> fileCache = new osgVerse::FileCache("earth_cache");
    osgDB::Registry::instance()->setFileCache(fileCache.get());

    //osg::ref_ptr<osgVerse::EarthProjectionMatrixCallback> epmcb =
    //    new osgVerse::EarthProjectionMatrixCallback(sceneCamera, earth->getBound().center());
    //epmcb->setNearFirstModeThreshold(2000.0);
    //sceneCamera->setClampProjectionMatrixCallback(epmcb.get());

    viewer.addEventHandler(new EnvironmentHandler(&earthRenderingUtils, mainFolder));
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setRealizeOperation(new osgVerse::RealizeOperation);
    viewer.setCameraManipulator(earthManipulator.get());
    viewer.setSceneData(root.get());
    //viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);

    int screenNo = 0; arguments.read("--screen", screenNo);
    viewer.setUpViewOnSingleScreen(screenNo);
    return viewer.run();
}

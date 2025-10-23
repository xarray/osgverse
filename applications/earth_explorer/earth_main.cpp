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
#define SIMPLE_VERSION 0

extern std::vector<osg::Camera*> configureEarthRendering(
        osgViewer::View& viewer, osg::Group* root, osg::Node* earth, osgVerse::EarthAtmosphereOcean& eData,
        const std::string& mainFolder, unsigned int mask, int w, int h);
extern osg::Node* configureCityData(osgViewer::View& viewer, osg::Node* earthRoot,
                                    osgVerse::EarthAtmosphereOcean& earthRenderingUtils,
                                    const std::string& mainFolder, unsigned int mask, bool waitingMode);
extern osg::Camera* configureUI(osgViewer::View& viewer, osg::Group* root,
                                const std::string& mainFolder, int w, int h);

class EnvironmentHandler : public osgGA::GUIEventHandler
{
public:
    EnvironmentHandler(osgVerse::EarthAtmosphereOcean* eao, const std::string& folder)
    :   _earthData(eao), _mainFolder(folder), _pressingKey(0), _pathIndex(0), _sunAngle(0.0f)
    {
        _earthData->commonUniforms["OceanOpaque"]->set(0.0f);
        _earthData->commonUniforms["WorldSunDir"]->set(
            osg::Vec3(-1.0f, 0.0f, 0.0f) * osg::Matrix::rotate(_sunAngle, osg::Z_AXIS));
    }

    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        if (ea.getEventType() == osgGA::GUIEventAdapter::USER)
        {
            const osgDB::Options* ev = dynamic_cast<const osgDB::Options*>(ea.getUserData());
            std::string command = ev ? ev->getOptionString() : "";

            std::vector<std::string> commmandPair; osgDB::split(command, commmandPair, '/');
            handleCommand(view, commmandPair.front(), commmandPair.back());
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::FRAME)
        {
            for (std::map<std::string, bool>::iterator itr = _toggles.begin(); itr != _toggles.end(); ++itr)
            {
                const std::string& cmd = itr->first; if (!itr->second) continue;
                if (cmd == "light")
                {
                    _sunAngle += 0.01f;
                    _earthData->commonUniforms["WorldSunDir"]->set(
                        osg::Vec3(-1.0f, 0.0f, 0.0f) * osg::Matrix::rotate(_sunAngle, osg::Z_AXIS));
                    view->getEventQueue()->userEvent(new osgDB::Options("value/" + std::to_string(_sunAngle)));
                }
                else if (cmd == "auto_rotate")
                {
                    // TODO
                }
            }
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN) _pressingKey = ea.getKey();
        else if (ea.getEventType() == osgGA::GUIEventAdapter::KEYUP)
        {
            osgVerse::EarthManipulator* earthMani =
                static_cast<osgVerse::EarthManipulator*>(view->getCameraManipulator());
            if (ea.getKey() == 'o')  // set an animation-path frame
            {
                earthMani->insertControlPointFromCurrentView((float)_pathIndex); _pathIndex += 60;
            }
            else if (ea.getKey() == 'i')  // save the animation-path
            {
                std::ofstream out("../city_path.txt", std::ios::out);
                const osgVerse::EarthManipulator::ControlPointSet& path = earthMani->getControlPoints();
                for (osgVerse::EarthManipulator::ControlPointSet::const_iterator it = path.begin();
                     it != path.end(); ++it)
                {
                    osgVerse::EarthManipulator::ControlPoint* cp = (*it).get();
                    out << cp->_time << "," << cp->_rotation << "," << cp->_tilt << "," << cp->_distance << "\n";
                }
            }
            else if (ea.getKey() == 'p')  // load and play the animation-path
            {
                std::ifstream in("../city_path.txt", std::ios::in); std::string line;
                if (in)
                {
                    earthMani->clearControlPoints();
                    while (std::getline(in, line))
                    {
                        if (line.empty()) continue; else if (line[0] == '#') continue;
                        std::vector<std::string> values; osgDB::split(line, values, ',');

                        if (values.size() < 4) continue;
                        osg::Quat q; std::stringstream ss(values[1]); ss >> q;
                        double time = atof(values[0].c_str()), tilt = atof(values[2].c_str());
                        double distance = atof(values[3].c_str());
                        earthMani->insertControlPoint(
                            new osgVerse::EarthManipulator::ControlPoint(time, q, distance, tilt));
                    }

                    if (earthMani->isAnimationRunning()) earthMani->stopAnimation();
                    earthMani->startAnimation();
                }
            }
            _pressingKey = 0;
        }

        if (_pressingKey > 0)
        {
            switch (_pressingKey)
            {
            case ',': case '<':
                _sunAngle -= 0.00005f;
                _earthData->commonUniforms["WorldSunDir"]->set(
                    osg::Vec3(-1.0f, 0.0f, 0.0f) * osg::Matrix::rotate(_sunAngle, osg::Z_AXIS));
                view->getEventQueue()->userEvent(new osgDB::Options("value/" + std::to_string(_sunAngle))); break;
            case '.': case '>':
                _sunAngle += 0.00005f;
                _earthData->commonUniforms["WorldSunDir"]->set(
                    osg::Vec3(-1.0f, 0.0f, 0.0f) * osg::Matrix::rotate(_sunAngle, osg::Z_AXIS));
                view->getEventQueue()->userEvent(new osgDB::Options("value/" + std::to_string(_sunAngle))); break;
            }
        }
        return false;
    }

    void handleCommand(osgViewer::View* view, const std::string& type, const std::string& cmd)
    {
        if (type == "button")
        {
            osgVerse::EarthManipulator* earthMani =
                static_cast<osgVerse::EarthManipulator*>(view->getCameraManipulator());
            if (cmd == "light") _toggles[cmd] = !_toggles[cmd];
            else if (cmd == "auto_rotate") _toggles[cmd] = !_toggles[cmd];
            else if (cmd == "go_home") earthMani->home(0.0);
            else if (cmd == "ocean")
            {
                bool v = !_toggles[cmd]; _toggles[cmd] = v;
                _earthData->commonUniforms["OceanOpaque"]->set(v ? 1.0f : 0.0f);
            }
            /*else if (cmd == "globe")
            {
                bool v = !_toggles[cmd]; _toggles[cmd] = v;
                _earthData->commonUniforms["GlobalOpaque"]->set(v ? 0.5f : 1.0f);
            }*/
            else if (cmd == "zoom_in") earthMani->performScale(osgGA::GUIEventAdapter::SCROLL_UP);
            else if (cmd == "zoom_out") earthMani->performScale(osgGA::GUIEventAdapter::SCROLL_DOWN);
        }
        else if (type == "item")
        {
            osgVerse::TileManager* mgr = osgVerse::TileManager::instance();
            if (cmd.find("seg") != std::string::npos)
                mgr->setLayerPath(osgVerse::TileCallback::USER, _mainFolder + "/Tiles/" + cmd + "/{z}/{x}/{y}.png");
        }
    }

protected:
    std::map<std::string, bool> _toggles;
    osgVerse::EarthAtmosphereOcean* _earthData;
    std::string _mainFolder;
    int _pressingKey, _pathIndex;
    float _sunAngle;
};

static std::string createCustomPath(int type, const std::string& prefix, int x, int y, int z)
{
    if (type == osgVerse::TileCallback::ORTHOPHOTO)
    {
#if SIMPLE_VERSION
        if (z > 4)
#else
        if (z > 13)
#endif
        {
            std::string prefix2 = "https://mt1.google.com/vt/lyrs=s&x={x}&y={y}&z={z}";
            return osgVerse::TileCallback::createPath(prefix2, x, pow(2, z) - y - 1, z);
        }
    }
    else if (type == osgVerse::TileCallback::USER)
        return osgVerse::TileCallback::createPath(prefix, x, pow(2, z) - y - 1, z);
#if SIMPLE_VERSION
    else if (z > 3) return "";  // for elevation & ocean-mask, ignore deeper levels
#else
    else if (z > 8) return "";  // for elevation & ocean-mask, ignore deeper levels
#endif
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
    bool cityWaitingTiles = true, manipulatorCanThrow = false;
    if (arguments.read("--no-wait")) cityWaitingTiles = false;
    if (arguments.read("--thrown")) manipulatorCanThrow = true;

    // Create earth
#if !SIMPLE_VERSION
    std::string earthURLs = " Orthophoto=mbtiles://" + mainFolder + "/Earth/satellite-2017-jpg-z13.mbtiles/{z}-{x}-{y}.jpg"
                            " Elevation=mbtiles://" + mainFolder + "/Earth/elevation-google-tif-z8.mbtiles/{z}-{x}-{y}.tif"
                            " OceanMask=mbtiles://" + mainFolder + "/Earth/aspect-slope-tif-z8.mbtiles/{z}-{x}-{y}.tif"
#else
    std::string earthURLs = " Orthophoto=mbtiles://" + mainFolder + "/Earth/DOM_lv4.mbtiles/{z}-{x}-{y}.jpg"
                            " Elevation=mbtiles://" + mainFolder + "/Earth/DEM_lv3.mbtiles/{z}-{x}-{y}.tif"
                            " OceanMask=mbtiles://" + mainFolder + "/Earth/Mask_lv3.mbtiles/{z}-{x}-{y}.tif"
#endif
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
    sceneCamera->setLODScale(0.8f);
    sceneCamera->setNearFarRatio(0.00001);
    sceneCamera->setSmallFeatureCullingPixelSize(10.0f);
    sceneCamera->addChild(configureCityData(
        viewer, earthRoot.get(), earthRenderingUtils, mainFolder, EARTH_INTERSECTION_MASK, cityWaitingTiles));

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(earthRoot.get());
#if !SIMPLE_VERSION
    root->addChild(configureUI(viewer, earthRoot.get(), mainFolder, w, h));
#endif

    // Configure the manipulator
    osg::ref_ptr<osgVerse::EarthManipulator> earthManipulator = new osgVerse::EarthManipulator;
    earthManipulator->setIntersectionMask(EARTH_INTERSECTION_MASK);
    earthManipulator->setWorldNode(earth.get());
    earthManipulator->setThrowAllowed(manipulatorCanThrow);

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

    osgDB::DatabasePager* pager = new osgDB::DatabasePager;
    pager->setDrawablePolicy(osgDB::DatabasePager::USE_VERTEX_BUFFER_OBJECTS);
    pager->setDoPreCompile(true); pager->setUpThreads(6, 3);

    viewer.addEventHandler(new EnvironmentHandler(&earthRenderingUtils, mainFolder));
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setRealizeOperation(new osgVerse::RealizeOperation);
    viewer.setCameraManipulator(earthManipulator.get());
    viewer.setDatabasePager(pager);
    viewer.setSceneData(root.get());
    //viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);

    int screenNo = 0; arguments.read("--screen", screenNo);
    viewer.setUpViewOnSingleScreen(screenNo);
    return viewer.run();
}

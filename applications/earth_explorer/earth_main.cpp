#include PREPENDED_HEADER
#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
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
#include <pipeline/IncrementalCompiler.h>
#include <VerseCommon.h>
#include <iostream>
#include <sstream>

#ifdef OSG_LIBRARY_STATIC
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
USE_SERIALIZER_WRAPPER(DracoGeometry)
#endif

#define EARTH_INTERSECTION_MASK 0xf0000000
typedef std::pair<osg::Camera*, osg::Texture*> CameraTexturePair;
extern CameraTexturePair configureEarthAndAtmosphere(osgViewer::View& viewer, osg::Group* root, osg::Node* earth,
                                                     const std::string& mainFolder, int width, int height);
extern osg::Node* configureOcean(osgViewer::View& viewer, osg::Group* root, osg::Texture* sceneMaskTex,
                                 const std::string& mainFolder, int width, int height, unsigned int mask);
extern void configureParticleCloud(osgViewer::View& viewer, osg::Group* root, const std::string& mainFolder,
                                   unsigned int mask, bool withGeomShader);
extern osg::MatrixTransform* createVolumeBox(const std::string& vdbFile, double lat, double lon,
                                             double z, unsigned int mask);

class EnvironmentHandler : public osgGA::GUIEventHandler
{
public:
    EnvironmentHandler(osg::Camera* c, osg::StateSet* ss, osg::EllipsoidModel* em)
        : _rttCamera(c), _mainStateSets(ss), _ellipsoidModel(em), _sunAngle(0.1f), _pressingKey(0)
    {
        osg::Uniform* worldSunDir = ss->getOrCreateUniform("worldSunDir", osg::Uniform::FLOAT_VEC3);
        worldSunDir->set(osg::Vec3(-1.0f, 0.0f, 0.0f) * osg::Matrix::rotate(_sunAngle, osg::Z_AXIS));
    }

    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        if (ea.getEventType() == osgGA::GUIEventAdapter::FRAME)
        {
            osg::StateSet* ss = _mainStateSets.get();
            osg::Uniform* cameraToWorld = ss->getOrCreateUniform("cameraToWorld", osg::Uniform::FLOAT_MAT4);
            osg::Uniform* screenToCamera = ss->getOrCreateUniform("screenToCamera", osg::Uniform::FLOAT_MAT4);
            osg::Uniform* worldCameraPos = ss->getOrCreateUniform("worldCameraPos", osg::Uniform::FLOAT_VEC3);

            osg::Camera* mainCamera = view->getCamera();
            if (mainCamera)
            {
                osg::Matrixf invViewMatrix = mainCamera->getInverseViewMatrix();
                cameraToWorld->set(invViewMatrix);
                screenToCamera->set(osg::Matrixf::inverse(mainCamera->getProjectionMatrix()));
                worldCameraPos->set(osg::Vec3(invViewMatrix.getTrans()));
            }
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN) _pressingKey = ea.getKey();
        else if (ea.getEventType() == osgGA::GUIEventAdapter::KEYUP) _pressingKey = 0;

        if (_pressingKey > 0)
        {
            osg::StateSet* ss = _mainStateSets.get();
            osg::Uniform* worldSunDir = ss->getOrCreateUniform("worldSunDir", osg::Uniform::FLOAT_VEC3);
            osg::Uniform* opaqueValue = ss->getOrCreateUniform("globalOpaque", osg::Uniform::FLOAT);
            osg::Uniform* opaqueValue2 = ss->getOrCreateUniform("oceanOpaque", osg::Uniform::FLOAT);

            osg::Vec3 originDir(-1.0f, 0.0f, 0.0f); float opaque = 1.0f;
            switch (_pressingKey)
            {
            case osgGA::GUIEventAdapter::KEY_Left:
                _sunAngle -= 0.01f; worldSunDir->set(originDir * osg::Matrix::rotate(_sunAngle, osg::Z_AXIS)); break;
            case osgGA::GUIEventAdapter::KEY_Right:
                _sunAngle += 0.01f; worldSunDir->set(originDir * osg::Matrix::rotate(_sunAngle, osg::Z_AXIS)); break;
            case '[':
                opaqueValue->get(opaque); opaqueValue->set(osg::clampAbove(opaque - 0.01f, 0.0f)); break;
            case ']':
                opaqueValue->get(opaque); opaqueValue->set(osg::clampBelow(opaque + 0.01f, 1.0f)); break;
            case '-':
                opaqueValue2->get(opaque); opaqueValue2->set(osg::clampAbove(opaque - 0.01f, 0.0f)); break;
            case '=':
                opaqueValue2->get(opaque); opaqueValue2->set(osg::clampBelow(opaque + 0.01f, 1.0f)); break;
            }
        }
        return false;
    }

protected:
    osg::observer_ptr<osg::StateSet> _mainStateSets;
    osg::EllipsoidModel* _ellipsoidModel;
    osg::Camera* _rttCamera;
    float _sunAngle;
    int _pressingKey;
};

int main(int argc, char** argv)
{
    osgViewer::Viewer viewer;
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osg::setNotifyHandler(new osgVerse::ConsoleHandler(false));
    osgVerse::updateOsgBinaryWrappers();
    //osgDB::Registry::instance()->addFileExtensionAlias("tif", "verse_tiff");

    std::string mainFolder = "G:/DOM_DEM"; arguments.read("--folder", mainFolder);
    std::string skirtRatio = "0.05"; arguments.read("--skirt", skirtRatio);
    int w = 1920, h = 1080; arguments.read("--resolution", w, h);
    bool withGeomShader = true; if (arguments.read("--no-geometry-shader")) withGeomShader = false;

    // Create earth
    std::string earthURLs = " Orthophoto=mbtiles://F:/satellite-2017-jpg-z13.mbtiles/{z}-{x}-{y}.jpg"
                            " Elevation=mbtiles://F:/elevation-google-tif-z8.mbtiles/{z}-{x}-{y}.tif"
                            //" Orthophoto=" + mainFolder + "/EarthDOM/{z}/{x}/{y}.jpg"
                            //" Elevation=" + mainFolder + "/EarthDEM/{z}/{x}/{y}.tif"
                            " OceanMask=mbtiles://F:/elevation-google-tif-z8.mbtiles/{z}-{x}-{y}.tif"
                            " MaximumLevel=8 UseWebMercator=1 UseEarth3D=1 OriginBottomLeft=1"
                            " TileElevationScale=3 TileSkirtRatio=" + skirtRatio;
    osg::ref_ptr<osgDB::Options> earthOptions = new osgDB::Options(earthURLs);

    osg::ref_ptr<osg::Node> earth = osgDB::readNodeFile("0-0-0.verse_tms", earthOptions.get());
    earth->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    earth->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

    // Create the scene graph
    osg::ref_ptr<osg::Group> root = new osg::Group;
    CameraTexturePair camTexPair = configureEarthAndAtmosphere(viewer, root.get(), earth.get(), mainFolder, w, h);

    osg::ref_ptr<osg::Camera> sceneCamera = camTexPair.first;
    osg::ref_ptr<osg::Texture> sceneTexture = camTexPair.second;
    configureOcean(viewer, root.get(), sceneTexture.get(), mainFolder, w, h, ~EARTH_INTERSECTION_MASK);
    configureParticleCloud(viewer, sceneCamera.get(), mainFolder, ~EARTH_INTERSECTION_MASK, withGeomShader);

    //osg::MatrixTransform* vdb = createVolumeBox(
    //    mainFolder + "/test.vdb.verse_vdb", 0.0, 0.0, 0.0, ~EARTH_INTERSECTION_MASK);
    //if (vdb) sceneCamera->addChild(vdb);

    osg::ref_ptr<osgVerse::EarthProjectionMatrixCallback> epmcb =
        new osgVerse::EarthProjectionMatrixCallback(viewer.getCamera(), earth->getBound().center());
    epmcb->setNearFirstModeThreshold(2000.0);
    sceneCamera->setClampProjectionMatrixCallback(epmcb.get());

    osg::ref_ptr<osgVerse::EarthManipulator> earthManipulator = new osgVerse::EarthManipulator;
    earthManipulator->setIntersectionMask(EARTH_INTERSECTION_MASK);
    earthManipulator->setWorldNode(earth.get());
    earthManipulator->setThrowAllowed(false);

    // Realize the viewer
    viewer.addEventHandler(new EnvironmentHandler(
        sceneCamera.get(), root->getOrCreateStateSet(), earthManipulator->getEllipsoid()));
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setCameraManipulator(earthManipulator.get());
    viewer.setSceneData(root.get());
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.setUpViewOnSingleScreen(0);

    // Start the main loop
    while (!viewer.done())
    {
        viewer.frame();
    }
    return 0;
}

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
#include <readerwriter/DatabasePager.h>
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
extern osg::Camera* configureEarthAndAtmosphere(osgViewer::View& viewer, osg::Group* root, osg::Node* earth,
                                                const std::string& mainFolder, int width, int height);
extern void configureParticleCloud(osg::Group* root, const std::string& mainFolder,
                                   unsigned int mask, bool withGeomShader);

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
            osg::Uniform* opaqueValue = ss->getOrCreateUniform("opaque", osg::Uniform::FLOAT);

            osg::Vec3 originDir(-1.0f, 0.0f, 0.0f); float opaque = 1.0f;
            switch (_pressingKey)
            {
            case '-':
                _sunAngle -= 0.01f; worldSunDir->set(originDir * osg::Matrix::rotate(_sunAngle, osg::Z_AXIS)); break;
            case '=':
                _sunAngle += 0.01f; worldSunDir->set(originDir * osg::Matrix::rotate(_sunAngle, osg::Z_AXIS)); break;
            case '[':
                opaqueValue->get(opaque); opaqueValue->set(osg::clampAbove(opaque - 0.01f, 0.0f)); break;
            case ']':
                opaqueValue->get(opaque); opaqueValue->set(osg::clampBelow(opaque + 0.01f, 1.0f)); break;
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

    std::string mainFolder = "G:/DOM_DEM"; arguments.read("--folder", mainFolder);
    std::string skirtRatio = "0.05"; arguments.read("--skirt", skirtRatio);
    int w = 1920, h = 1080; arguments.read("--resolution", w, h);
    bool withGeomShader = true; if (arguments.read("--no-geometry-shader")) withGeomShader = false;

    // Create earth
    std::string earthURLs = " Orthophoto=" + mainFolder + "/EarthDOM/{z}/{x}/{y}.jpg OriginBottomLeft=1"
                            " Elevation=" + mainFolder + "/EarthDEM/{z}/{x}/{y}.tif UseWebMercator=0"
                            " UseEarth3D=1 TileSkirtRatio=" + skirtRatio;
    osg::ref_ptr<osgDB::Options> earthOptions = new osgDB::Options(earthURLs);

    osg::ref_ptr<osg::Node> earth = osgDB::readNodeFile("0-0-x.verse_tms", earthOptions.get());
    earth->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    earth->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

    // Create the scene graph
    osg::ref_ptr<osg::Group> root = new osg::Group;
    osg::ref_ptr<osg::Camera> sceneCamera =
        configureEarthAndAtmosphere(viewer, root.get(), earth.get(), mainFolder, w, h);
    configureParticleCloud(sceneCamera.get(), mainFolder, ~EARTH_INTERSECTION_MASK, withGeomShader);

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

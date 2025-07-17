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
extern osg::Camera* configureEarthAndAtmosphere(osg::Group* root, osg::Node* earth);

class EnvironmentHandler : public osgGA::GUIEventHandler
{
public:
    EnvironmentHandler(osg::Camera* c, osg::StateSet* ss, osg::EllipsoidModel* em)
        : _rttCamera(c), _mainStateSets(ss), _ellipsoidModel(em), _sunAngle(0.1f)
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

            osg::Uniform* worldSunDir = ss->getOrCreateUniform("worldSunDir", osg::Uniform::FLOAT_VEC3);
            osg::Uniform* hdrExposure = ss->getOrCreateUniform("hdrExposure", osg::Uniform::FLOAT);
            /*if (inputManager->getNumKeyboards() > 0)
            {
                osg::Vec3 originDir(-1.0f, 0.0f, 0.0f);
                if (inputManager->isKeyDown('-'))
                {
                    _sunAngle -= 0.01f;
                    worldSunDir->set(originDir * osg::Matrix::rotate(_sunAngle, osg::Z_AXIS));
                }
                if (inputManager->isKeyDown('='))
                {
                    _sunAngle += 0.01f;
                    worldSunDir->set(originDir * osg::Matrix::rotate(_sunAngle, osg::Z_AXIS));
                }
                if (inputManager->isKeyDown('['))
                {
                    float value = 0.0f; hdrExposure->get(value);
                    hdrExposure->set(value - 0.01f);
                }
                if (inputManager->isKeyDown(']'))
                {
                    float value = 0.0f; hdrExposure->get(value);
                    hdrExposure->set(value + 0.01f);
                }
            }*/
        }
        return false;
    }

protected:
    osg::observer_ptr<osg::StateSet> _mainStateSets;
    osg::EllipsoidModel* _ellipsoidModel;
    osg::Camera* _rttCamera;
    float _sunAngle;
};

int main(int argc, char** argv)
{
    osgViewer::Viewer viewer;
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osg::setNotifyHandler(new osgVerse::ConsoleHandler);
    osgVerse::updateOsgBinaryWrappers();

    // Create earth
    std::string earthURLs = "Orthophoto=https://webst01.is.autonavi.com/appmaptile?style%3d6&x%3d{x}&y%3d{y}&z%3d{z} "
                            //"Elevation=https://mt1.google.com/vt/lyrs%3dt&x%3d{x}&y%3d{y}&z%3d{z} UseWebMercator=1";
                            "UseWebMercator=1";
    osg::ref_ptr<osgDB::Options> earthOptions = new osgDB::Options(earthURLs + " UseEarth3D=1");
    osg::ref_ptr<osg::Node> earth = osgDB::readNodeFile("0-0-0.verse_tms", earthOptions.get());

    // Create the scene graph
    osg::ref_ptr<osg::Group> root = new osg::Group;
    osg::ref_ptr<osg::Camera> sceneCamera = configureEarthAndAtmosphere(root.get(), earth.get());

    osg::ref_ptr<osgVerse::EarthProjectionMatrixCallback> epmcb =
        new osgVerse::EarthProjectionMatrixCallback(viewer.getCamera(), earth->getBound().center());
    epmcb->setNearFirstModeThreshold(2000.0);
    sceneCamera->setClampProjectionMatrixCallback(epmcb.get());

    osg::ref_ptr<osgVerse::EarthManipulator> earthManipulator = new osgVerse::EarthManipulator;
    earthManipulator->setIntersectionMask(EARTH_INTERSECTION_MASK);
    earthManipulator->setWorldNode(earth.get());

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

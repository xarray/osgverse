#include <osg/io_utils>
#include <osg/LightSource>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>
#include <random>
#include <chrono>

#include <readerwriter/Utilities.h>
#include <pipeline/Utilities.h>
#include <pipeline/Global.h>
#include <pipeline/Pipeline.h>
#include <pipeline/Drawer2D.h>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

#ifdef OSG_LIBRARY_STATIC
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
#endif
USE_GRAPICSWINDOW_IMPLEMENTATION(SDL)
USE_GRAPICSWINDOW_IMPLEMENTATION(GLFW)

class HudTextHandler : public osgGA::GUIEventHandler
{
public:
    HudTextHandler(osg::Camera* cam, osgVerse::Drawer2D* drawer, int viewNo)
        : _mainCamera(cam), _drawer(drawer), _viewNum(viewNo) {}

    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        if (ea.getEventType() == osgGA::GUIEventAdapter::FRAME)
        {
            if (_mainCamera.valid())
                view->getCamera()->setViewMatrix(_mainCamera->getViewMatrix());
            if (_drawer.valid())
            {
                int frames = view->getFrameStamp()->getFrameNumber();
                _drawer->startInThread([this, frames](osgVerse::Drawer2D* drawer)
                    {
                        auto now = std::chrono::system_clock::now();
                        auto t = std::chrono::system_clock::to_time_t(now);
                        std::stringstream ss; ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
                        drawer->fillBackground(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));

                        std::mt19937 gen(std::random_device{}());
                        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
                        std::string text[] = {
                            "ID: TEST_TEXT_" + std::to_string(_viewNum),
                            "Frame: " + std::to_string(frames), "Time: " + ss.str(),
                            "Position: " + std::to_string(dist(gen) * 100.0) + ", " + std::to_string(dist(gen) * 100.0),
                            "Attitude: " + std::to_string(dist(gen) * 10000.0) + " m",
                            "Velocity: " + std::to_string(dist(gen) * 100.0) + ", " + std::to_string(dist(gen) * 100.0),
                            "Heading: " + std::to_string(dist(gen) * 360.0) + " deg",
                            "Pitch: " + std::to_string(dist(gen) * 360.0) + " deg",
                            "Roll: " + std::to_string(dist(gen) * 360.0) + " deg",
                            "Wind Direction: " + std::to_string(dist(gen) * 100.0) + ", " + std::to_string(dist(gen) * 100.0)
                        };

                        osg::Vec4 colors[] = {
                            osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f), osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f), osg::Vec4(1.0f, 0.0f, 1.0f, 1.0f),
                            osg::Vec4(0.0f, 1.0f, 0.0f, 1.0f), osg::Vec4(0.0f, 1.0f, 1.0f, 1.0f), osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f),
                            osg::Vec4(1.0f, 0.5f, 0.0f, 1.0f), osg::Vec4(1.0f, 0.0f, 0.5f, 1.0f), osg::Vec4(1.0f, 0.5f, 0.5f, 1.0f),
                            osg::Vec4(0.5f, 0.5f, 0.5f, 1.0f)
                        };

                        for (int i = 0; i < 60; ++i)
                        {
                            float textSize = (i > 30) ? (i - 20.0f) : (40.0f - i);
                            osg::Vec4 bbox = drawer->getUtf8TextBoundingBox(text[i % 10], textSize);
                            drawer->drawUtf8Text(osg::Vec2(bbox[0], i * 40.0f + bbox[1] + bbox[3]), textSize,
                                                 text[i % 10], "", osgVerse::DrawerStyleData(colors[i % 10], true));
                        }
                    }, false);
                _drawer->finish();
            }
        }
        return false;
    }

protected:
    osg::observer_ptr<osg::Camera> _mainCamera;
    osg::observer_ptr<osgVerse::Drawer2D> _drawer;
    int _viewNum;
};

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());
    osg::setNotifyHandler(new osgVerse::ConsoleHandler);

    int numViews = 1, winWidth = 800, winHeight = 600; bool withHUD = !arguments.read("--no-hud");
    arguments.read("--views", numViews); arguments.read("--size", winWidth, winHeight);

    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFiles(arguments);
    if (!scene) scene = osgDB::readNodeFile(BASE_DIR + "/models/Sponza/Sponza.gltf");

    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    if (scene.valid()) sceneRoot->addChild(scene.get());

    osg::ref_ptr<osgVerse::Drawer2D> drawer = new osgVerse::Drawer2D;
    drawer->allocateImage(1920, 1080, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    drawer->loadFont("default", MISC_DIR + "LXGWFasmartGothic.otf");
    drawer->setPixelBufferObject(new osg::PixelBufferObject(drawer.get()));

    osg::Camera* hudCamera = osgVerse::createHUDCamera(
        NULL, winWidth, winHeight, osg::Vec3(), 1.0f, 1.0f, osg::Vec4(0.0f, 1.0f, 1.0f, 0.0f), true, false);
    hudCamera->getOrCreateStateSet()->setTextureAttribute(0, osgVerse::createTexture2D(drawer.get()));
    hudCamera->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    hudCamera->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->getOrCreateStateSet()->setAttribute(osgVerse::createDefaultProgram("baseTexture"));
    root->getOrCreateStateSet()->addUniform(new osg::Uniform("baseTexture", (int)0));
    root->addChild(sceneRoot.get());
    root->addChild(hudCamera);

    osgViewer::CompositeViewer viewer;
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.setRealizeOperation(new osgVerse::RealizeOperation);
    for (int i = 0; i < numViews; ++i)
    {
        osgViewer::View* view = new osgViewer::View;
        if (i == 0)
        {
            view->setCameraManipulator(new osgGA::TrackballManipulator);
            view->addEventHandler(new osgViewer::StatsHandler);
        }
        view->addEventHandler(new HudTextHandler(
            (viewer.getNumViews() > 0 ? viewer.getView(0)->getCamera() : NULL),
            (i == 0 && withHUD ? drawer.get() : NULL), i));
        view->setSceneData(root.get());
        view->setUpViewInWindow(i * 50, 50, winWidth, winHeight);
        viewer.addView(view);
    }

    while (!viewer.done()) viewer.frame();
    return 0;
}

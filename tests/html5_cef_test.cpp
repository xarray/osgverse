#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <pipeline/Global.h>
#include <readerwriter/Utilities.h>
#include <ui/CefWebManager.h>
#include <iostream>
#include <sstream>

#ifndef _DEBUG
#include <backward.hpp>
namespace backward { backward::SignalHandling sh; }
#endif

// Update callback to drive CEF message loop every frame
class CefUpdateCallback : public osg::NodeCallback
{
public:
    CefUpdateCallback(osgVerse::CefWebManager* mgr, osgVerse::CefWebView* view)
        : _manager(mgr), _view(view), _timer(0.0) {}

    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        if (_manager.valid()) _manager->update();
        if (_view.valid() && nv->getFrameStamp())
        {   // Demo: execute JavaScript after 3 seconds
            double t = nv->getFrameStamp()->getSimulationTime();
            if (_timer < 3.0 && t >= 3.0)
            {
                _view->executeJavaScript(
                    "document.body.style.background = 'linear-gradient(to right, #1e3c72, #2a5298)';"
                    "document.body.innerHTML += '<h1 style=\"color:white;text-align:center;\">"
                    "Hello from osgVerse CEF!</h1>';");
            }
            _timer = t;
        }
        traverse(node, nv);
    }

protected:
    osg::observer_ptr<osgVerse::CefWebManager> _manager;
    osg::observer_ptr<osgVerse::CefWebView> _view;
    double _timer;
};

int main(int argc, char** argv)
{
    // STEP 1: Execute CEF subprocess (MUST be first)
    osg::ArgumentParser arguments(&argc, argv);
    int subExit = osgVerse::CefWebManager::executeProcess(arguments);
    if (subExit >= 0) return subExit;  // This is a renderer/GPU subprocess

    // STEP 2: Initialize CEF browser process
    osgVerse::CefWebManager* cefMgr = osgVerse::CefWebManager::instance();
    if (!cefMgr->initialize(arguments, "cef_cache"))
    { OSG_FATAL << "Failed to initialize CEF browser process.\n"; return 1; }

    // STEP 3: Setup OSG Viewer
    arguments = osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());
    osgViewer::Viewer viewer;
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;

    // Load or create a fallback scene
    osg::ref_ptr<osg::Node> scene = (argc > 1) ? osgDB::readNodeFiles(arguments)
                                               : osgDB::readNodeFile("cessna.osg");
    if (scene.valid()) root->addChild(scene.get());

    // STEP 4: Create CEF Web View
    int webW = 1280, webH = 720;
    osg::ref_ptr<osgVerse::CefWebView> webView =
        cefMgr->createView(webW, webH, "https://gitee.com/xarray/osgverse");
    if (!webView) { OSG_FATAL << "Failed to create web view.\n"; return 1; }

    // Bind a C++ callback for JS interaction demo
    webView->bindJavaScriptCallback("osgMessage", [](const std::string& msg)
    { OSG_NOTICE << "JS -> C++ Message: " << msg << std::endl; });

    // STEP 5: Create textured quad to display web content
    osg::ref_ptr<osg::Geometry> quad = osg::createTexturedQuadGeometry(
        osg::Vec3(), osg::X_AXIS * 16.0f, osg::Z_AXIS * 9.0f, 0.0f, 1.0f, 1.0f, 0.0f);
    quad->getOrCreateStateSet()->setTextureAttributeAndModes(0, webView->getTexture());
    quad->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
#if !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE) && !defined(OSG_GL3_AVAILABLE)
    quad->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
#endif

    osg::Geode* webGeode = new osg::Geode;
    webGeode->addDrawable(quad.get());
    root->addChild(webGeode);

    // STEP 6: Setup viewer with CEF event handler and update callback
    viewer.addEventHandler(new osgVerse::CefWebEventHandler(webView.get()));
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(0);
    viewer.setThreadingModel(osgViewer::ViewerBase::SingleThreaded);

    // Drive CEF message loop and texture updates
    root->addUpdateCallback(new CefUpdateCallback(cefMgr, webView.get()));
    viewer.run(); cefMgr->shutdown();
    return 0;
}

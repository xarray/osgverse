#ifndef MANA_APP_VIEWERWASM_HPP
#define MANA_APP_VIEWERWASM_HPP

#include <emscripten.h>
#include <emscripten/html5.h>

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

#include <pipeline/SkyBox.h>
#include <pipeline/Pipeline.h>
#include <pipeline/LightModule.h>
#include <pipeline/ShadowModule.h>
#include <pipeline/Utilities.h>
#include <iostream>
#include <sstream>

class NotifyLogger : public osg::NotifyHandler
{
public:
    virtual void notify(osg::NotifySeverity severity, const char* message)
    {
        printf("%s| %s", getLevel(severity).c_str(), message);
    }

    std::string getLevel(osg::NotifySeverity severity)
    {
        switch (severity)
        {
            case osg::DEBUG_FP: return "V";
            case osg::DEBUG_INFO: return "D";
            case osg::NOTICE: case osg::INFO: return "I";
            case osg::WARN: return "W";
            case osg::FATAL: case osg::ALWAYS: return "E";
        }
        return "?";
    }
};

class Application : public osg::Referenced
{
public:
    Application()
    {
        _logger = new NotifyLogger;
        osg::setNotifyHandler(_logger.get());
        osg::setNotifyLevel(osg::INFO);
    }

    ~Application()
    {
        osg::setNotifyHandler(NULL);
        _viewer = NULL; _logger = NULL;
    }

    static EM_BOOL canvasWindowResized(int eventType, const EmscriptenUiEvent* event, void* userData)
    {
        // FIXME: affected by fullScreenCallback?
        return EMSCRIPTEN_RESULT_SUCCESS;
    }

    static EM_BOOL fullScreenCallback(int eventType, const EmscriptenFullscreenChangeEvent* event, void* userData)
    {
        Application* app = (Application*)userData;
        int width = event->isFullscreen ? event->screenWidth : 800;
        int height = event->isFullscreen ? event->screenHeight : 600;
        app->_viewer->getCamera()->getGraphicsContext()->resized(0, 0, width, height);
        app->_viewer->getEventQueue()->windowResize(0, 0, width, height);
        std::cout << "fullScreenCallback: " << width << "x" << height << std::endl;
        return EMSCRIPTEN_RESULT_SUCCESS;
    }

    static EM_BOOL contextLostCallback(int eventType, const void* reserved, void* userData)
    {
        // TODO
        std::cout << "contextLostCallback: " << eventType << std::endl;
        return EMSCRIPTEN_RESULT_SUCCESS;
    }

    static EM_BOOL contextRestoredCallback(int eventType, const void* reserved, void* userData)
    {
        // TODO
        std::cout << "contextRestoredCallback: " << eventType << std::endl;
        return EMSCRIPTEN_RESULT_SUCCESS;
    }

    void setViewer(osgViewer::Viewer* v) { _viewer = v; }
    void frame() { _viewer->frame(); }

protected:
    osg::ref_ptr<NotifyLogger> _logger;
    osg::ref_ptr<osgViewer::Viewer> _viewer;
};

#endif

#ifndef MANA_APP_VIEWERWASM_HPP
#define MANA_APP_VIEWERWASM_HPP

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

    void handleEvent(SDL_Event& event)
    {
        osgGA::EventQueue* eq = _gw->getEventQueue();
        switch (event.type)
        {
        case SDL_MOUSEMOTION:
            eq->mouseMotion(event.motion.x, event.motion.y); break;
        case SDL_MOUSEBUTTONDOWN:
            eq->mouseButtonPress(event.button.x, event.button.y, event.button.button); break;
        case SDL_MOUSEBUTTONUP:
            eq->mouseButtonRelease(event.button.x, event.button.y, event.button.button); break;
        case SDL_KEYUP:
            eq->keyRelease((osgGA::GUIEventAdapter::KeySymbol)event.key.keysym.sym); break;
        case SDL_KEYDOWN:
            eq->keyPress((osgGA::GUIEventAdapter::KeySymbol)event.key.keysym.sym); break;
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                eq->windowResize(0, 0, event.window.data1, event.window.data2);
                _gw->resized(0, 0, event.window.data1, event.window.data2);
            }
            break;
        case SDL_QUIT:
            _viewer->setDone(true); break;
        default: break;
        }
    }

    void setViewer(osgViewer::Viewer* v, int width, int height)
    {
        _gw = v->setUpViewerAsEmbeddedInWindow(0, 0, width, height);
        _gw->getEventQueue()->windowResize(0, 0, width, height);
        _viewer = v;
    }

    void frame() { _viewer->frame(); }

protected:
    osg::ref_ptr<NotifyLogger> _logger;
    osg::ref_ptr<osgViewer::Viewer> _viewer;
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> _gw;
};

#endif

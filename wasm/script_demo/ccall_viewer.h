#ifndef MANA_APP_JSCALLERWASM_HPP
#define MANA_APP_JSCALLERWASM_HPP

#include <osg/io_utils>
#include <osg/Material>
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
#include <script/JsonScript.h>
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
        _scripter = new osgVerse::JsonScript;
        _logger = new NotifyLogger;
        osg::setNotifyHandler(_logger.get());
        osg::setNotifyLevel(osg::INFO);
    }

    ~Application()
    {
        osg::setNotifyHandler(NULL);
        _viewer = NULL; _logger = NULL; _scripter = NULL;
    }

    osgVerse::JsonScript* scripter() { return _scripter.get(); }
    void setViewer(osgViewer::Viewer* v) { _viewer = v; }
    void frame() { _viewer->frame(); }

protected:
    osg::ref_ptr<NotifyLogger> _logger;
    osg::ref_ptr<osgViewer::Viewer> _viewer;
    osg::ref_ptr<osgVerse::JsonScript> _scripter;
};

#endif

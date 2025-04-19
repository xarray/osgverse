#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <osg/LightSource>
#include <osg/Texture2D>
#include <osg/ShapeDrawable>
#include <osg/MatrixTransform>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <pipeline/Utilities.h>
#include <ai/McpServer.h>
#include <iostream>
#include <sstream>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

int main(int argc, char** argv)
{
    osg::ref_ptr<osgVerse::McpTool> tool0 = new osgVerse::McpTool("build_wall", "Build a wall in 3D world");
    tool0->addProperty("center", osgVerse::McpTool::NumberArrayType, true);
    tool0->addProperty("extent", osgVerse::McpTool::NumberArrayType, true);

    osg::ref_ptr<osgVerse::McpTool> tool1 = new osgVerse::McpTool("build_roof", "Build a roof in 3D world");
    tool1->addProperty("center", osgVerse::McpTool::NumberArrayType, true);
    tool1->addProperty("extent", osgVerse::McpTool::NumberArrayType, true);

    osg::ref_ptr<osgVerse::McpServer> server = new osgVerse::McpServer;
    server->registerTool(tool0.get(), [](const picojson::value& params, const std::string& id)
    {
        OSG_NOTICE << "build_wall: " << params.serialize(false) << std::endl;
        return osgVerse::McpServer::methodResult("OK");
    });
    server->registerTool(tool1.get(), [](const picojson::value& params, const std::string& id)
    {
        OSG_NOTICE << "build_roof: " << params.serialize(false) << std::endl;
        return osgVerse::McpServer::methodResult("OK");
    });
    server->start("0.0.0.0", 1280);

    // Scene root
    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->setName("Root");

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.setSceneData(root.get());

    int code = viewer.run();
    server->stop();
    return code;
}

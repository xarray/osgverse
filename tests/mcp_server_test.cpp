#include <osg/io_utils>
#include <osg/MatrixTransform>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgUtil/SmoothingVisitor>
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
    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->setName("Root");

    osg::ref_ptr<osgVerse::McpTool> tool0 = new osgVerse::McpTool("create_triangles", "Create a list of triangles in 3D world");
    tool0->addProperty("vertices", "A list of 3D vertices to construct a geometry shape",
                       osgVerse::McpTool::NumberType | osgVerse::McpTool::ArrayType | osgVerse::McpTool::RequiredType);
    tool0->addProperty("indices", "A list of vertex indices to construct triangles from given vertices",
                       osgVerse::McpTool::NumberType | osgVerse::McpTool::ArrayType | osgVerse::McpTool::RequiredType);

    osg::ref_ptr<osgVerse::McpServer> server = new osgVerse::McpServer;
    server->registerTool(tool0.get(), [root](const picojson::value& params, const std::string& id)
    {
        //OSG_NOTICE << "create_triangles: " << params.serialize(false) << std::endl;
        if (params.contains("vertices") && params.contains("indices"))
        {
            picojson::array vertices = params.get("vertices").get<picojson::array>();
            picojson::array indices = params.get("indices").get<picojson::array>();

            osg::Vec3Array* va = new osg::Vec3Array;
            for (size_t i = 0; i < vertices.size(); i += 3)
            {
                va->push_back(osg::Vec3(vertices[i].get<double>(),
                                        vertices[i + 1].get<double>(), vertices[i + 2].get<double>()));
            }

            osg::DrawElementsUShort* de = new osg::DrawElementsUShort(GL_TRIANGLES);
            for (size_t i = 0; i < indices.size(); ++i)
                de->push_back((unsigned short)indices[i].get<double>());

            osg::Geometry* geom = new osg::Geometry;
            geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
            geom->setVertexArray(va); geom->addPrimitiveSet(de);

            osg::Geode* geode = new osg::Geode; geode->addDrawable(geom);
            osgUtil::SmoothingVisitor smv; geode->accept(smv);

            root->removeChildren(0, root->getNumChildren());  // FIXME: just because it is a test...
            root->addChild(geode);
            return osgVerse::McpServer::methodResult("OK");
        }
        else
            return osgVerse::McpServer::methodResult("Invalid call");
    });
    server->start("0.0.0.0", 1280);

    // Scene root
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

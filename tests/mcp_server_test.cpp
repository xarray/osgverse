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
    osg::ref_ptr<osgGA::TrackballManipulator> trackball = new osgGA::TrackballManipulator;

    osg::ref_ptr<osgVerse::McpTool> tool0 = new osgVerse::McpTool("create_triangles", "Create a list of triangles in 3D world");
    tool0->addProperty("vertices", "A list of 3D vertices to construct a geometry shape",
                       osgVerse::McpTool::NumberType | osgVerse::McpTool::ArrayType | osgVerse::McpTool::RequiredType);
    tool0->addProperty("indices", "A list of vertex indices to construct triangles from given vertices",
                       osgVerse::McpTool::NumberType | osgVerse::McpTool::ArrayType | osgVerse::McpTool::RequiredType);

    osg::ref_ptr<osgVerse::McpTool> tool1 = new osgVerse::McpTool("create_earth", "Create an earth model in 3D world");

    osg::ref_ptr<osgVerse::McpTool> tool2 = new osgVerse::McpTool("view_earth", "View part of the earth");
    tool2->addProperty("cameraPos", "Current camera position (Latitude in radians, Longitude in radians, altitude in meters)",
                       osgVerse::McpTool::NumberType | osgVerse::McpTool::ArrayType | osgVerse::McpTool::RequiredType);
    tool2->addProperty("targetPos", "Current view target position (Latitude in radians, Longitude in radians, altitude in meters)",
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

    server->registerTool(tool1.get(), [root](const picojson::value& params, const std::string& id)
    {
        //OSG_NOTICE << "create_earth: " << params.serialize(false) << std::endl;
        osg::Node* earth = osgDB::readNodeFile("0-0-0.verse_tms", new osgDB::Options(
            "URL=https://webst01.is.autonavi.com/appmaptile?style%3d6&x%3d{x}&y%3d{y}&z%3d{z} UseWebMercator=1 UseEarth3D=1"));
        root->removeChildren(0, root->getNumChildren());  // FIXME: just because it is a test...
        root->addChild(earth);
        return osgVerse::McpServer::methodResult("OK");
    });

    server->registerTool(tool2.get(), [trackball](const picojson::value& params, const std::string& id)
    {
        //OSG_NOTICE << "view_earth: " << params.serialize(false) << std::endl;
        if (params.contains("cameraPos") && params.contains("targetPos"))
        {
            osg::EllipsoidModel em;
            picojson::array cameraPos = params.get("cameraPos").get<picojson::array>();
            picojson::array targetPos = params.get("targetPos").get<picojson::array>();

            osg::Vec3d camera, target; osg::Matrix matrix;
            em.computeLocalToWorldTransformFromLatLongHeight(
                cameraPos[0].get<double>(), cameraPos[1].get<double>(), cameraPos[2].get<double>(), matrix);
            camera = matrix.getTrans();
            em.computeLocalToWorldTransformFromLatLongHeight(
                targetPos[0].get<double>(), targetPos[1].get<double>(), targetPos[2].get<double>(), matrix);
            target = matrix.getTrans();

            osg::Vec3d side, up = osg::Y_AXIS, forward = target - camera; forward.normalize();
            if (abs(forward * up) > 0.999) up = osg::Z_AXIS;
            side = up ^ forward; up = forward ^ side;

            trackball->setByInverseMatrix(osg::Matrix::lookAt(camera, target, up));
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
    viewer.setCameraManipulator(trackball.get());
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.setSceneData(root.get());

    int code = viewer.run();
    server->stop();
    return code;
}

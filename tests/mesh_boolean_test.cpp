#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/PagedLOD>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>
#include <pipeline/Utilities.h>
#include <modeling/Utilities.h>
#include <modeling/CsgBoolean.h>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

int main(int argc, char** argv)
{
    osg::ref_ptr<osg::Node> nodeA = osgDB::readNodeFile("cow.osg");
    //osg::ref_ptr<osg::Node> nodeB = osgDB::readNodeFile("cow.osg.(-1,-0.5,0).trans");
    osg::ref_ptr<osg::Geode> nodeB = new osg::Geode;
    nodeB->addDrawable(osgVerse::createEllipsoid(osg::Vec3(0.0f, -0.5f, 0.0f), 1.0f, 1.0f, 3.0f));

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(osgVerse::CsgBoolean::process(
        osgVerse::CsgBoolean::A_NOT_B, nodeA.get(), nodeB.get()));

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}

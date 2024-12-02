#include <osg/io_utils>
#include <osg/PolygonMode>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgUtil/SmoothingVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <modeling/Math.h>
#include <modeling/Utilities.h>
#include <pipeline/Utilities.h>
#include <iostream>
#include <sstream>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

int main(int argc, char** argv)
{
    osgVerse::PointList2D polygon0;
    polygon0.push_back(osgVerse::PointType2D(osg::Vec2d(348, 257), 0));
    polygon0.push_back(osgVerse::PointType2D(osg::Vec2d(364, 148), 1));
    polygon0.push_back(osgVerse::PointType2D(osg::Vec2d(362, 148), 2));
    polygon0.push_back(osgVerse::PointType2D(osg::Vec2d(326, 241), 3));
    polygon0.push_back(osgVerse::PointType2D(osg::Vec2d(295, 219), 4));
    polygon0.push_back(osgVerse::PointType2D(osg::Vec2d(258, 88), 5));
    polygon0.push_back(osgVerse::PointType2D(osg::Vec2d(440, 129), 6));
    polygon0.push_back(osgVerse::PointType2D(osg::Vec2d(370, 196), 7));
    polygon0.push_back(osgVerse::PointType2D(osg::Vec2d(372, 275), 8));

    osgVerse::PointList2D polygon1;
    polygon1.push_back(osgVerse::PointType2D(osg::Vec2d(100, 50), 0));
    polygon1.push_back(osgVerse::PointType2D(osg::Vec2d(10, 79), 1));
    polygon1.push_back(osgVerse::PointType2D(osg::Vec2d(65, 2), 2));
    polygon1.push_back(osgVerse::PointType2D(osg::Vec2d(65, 98), 3));
    polygon1.push_back(osgVerse::PointType2D(osg::Vec2d(10, 21), 4));

    std::vector<osgVerse::PointList2D> offsetResults =
        osgVerse::GeometryAlgorithm::expandPolygon2D(polygon0, -7.0);

    osg::ref_ptr<osg::Geode> root = new osg::Geode;
    root->addDrawable(
        osgVerse::createPointListGeometry(polygon0, osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f), false, true));
    for (size_t i = 0; i < offsetResults.size(); ++i)
        root->addDrawable(osgVerse::createPointListGeometry(
            offsetResults[i], osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f), false, true));

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(0);
    return viewer.run();
}

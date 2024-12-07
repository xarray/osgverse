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

    osgVerse::PointList2D polygonSub;
    polygonSub.push_back(osgVerse::PointType2D(osg::Vec2d(100, 50), 0));
    polygonSub.push_back(osgVerse::PointType2D(osg::Vec2d(10, 79), 1));
    polygonSub.push_back(osgVerse::PointType2D(osg::Vec2d(65, 2), 2));
    polygonSub.push_back(osgVerse::PointType2D(osg::Vec2d(65, 98), 3));
    polygonSub.push_back(osgVerse::PointType2D(osg::Vec2d(10, 21), 4));

    osgVerse::PointList2D polygonClip;
    polygonClip.push_back(osgVerse::PointType2D(osg::Vec2d(98, 63), 0));
    polygonClip.push_back(osgVerse::PointType2D(osg::Vec2d(4, 68), 1));
    polygonClip.push_back(osgVerse::PointType2D(osg::Vec2d(77, 8), 2));
    polygonClip.push_back(osgVerse::PointType2D(osg::Vec2d(52, 100), 3));
    polygonClip.push_back(osgVerse::PointType2D(osg::Vec2d(19, 12), 4));
    
    std::vector<osgVerse::PointList2D> offsetResults =
        osgVerse::GeometryAlgorithm::expandPolygon2D(polygon0, -7.0);
    std::vector<osgVerse::PointList2D> clippedResults =
        osgVerse::GeometryAlgorithm::clipPolygon2D(
            std::vector<osgVerse::PointList2D>{polygonSub},
            std::vector<osgVerse::PointList2D>{polygonClip},
            osgVerse::GeometryAlgorithm::BOOL_Intersection, false);

    osg::ref_ptr<osg::Geode> dataNode = new osg::Geode;
    dataNode->addDrawable(
        osgVerse::createPointListGeometry(polygon0, osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f), false, true));
    dataNode->addDrawable(
        osgVerse::createPointListGeometry(polygonSub, osg::Vec4(0.8f, 0.8f, 1.0f, 1.0f), false, true));
    dataNode->addDrawable(
        osgVerse::createPointListGeometry(polygonClip, osg::Vec4(0.4f, 0.4f, 0.6f, 1.0f), false, true));

    osg::ref_ptr<osg::Geode> resultNode = new osg::Geode;
    for (size_t i = 0; i < offsetResults.size(); ++i)
        resultNode->addDrawable(osgVerse::createPointListGeometry(
            offsetResults[i], osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f), false, true));
    for (size_t i = 0; i < clippedResults.size(); ++i)
        resultNode->addDrawable(osgVerse::createPointListGeometry(
            clippedResults[i], osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f), false, true));

    osg::ref_ptr<osg::MatrixTransform> resultMT = new osg::MatrixTransform;
    resultMT->setMatrix(osg::Matrix::translate(0.0f, 0.0f, 0.1f));
    resultMT->addChild(resultNode.get());

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(dataNode.get()); root->addChild(resultMT.get());

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(0);
    return viewer.run();
}

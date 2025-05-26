#include <osg/io_utils>
#include <osg/LightSource>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>

#include <modeling/Math.h>
#include <readerwriter/DatabasePager.h>
#include <VerseCommon.h>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

class MyDatabasePager : public osgVerse::DatabasePager
{
public:
    virtual void removeExpiredSubgraphs(const osg::FrameStamp& fs)
    {
        unsigned int numPagedLODs = _activePagedLODList->size();
        //std::cout << "removeExpiredSubgraphs " << numPagedLODs << "...\n";

        osgVerse::DatabasePager::removeExpiredSubgraphs(fs);
    }
};

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osgVerse::updateOsgBinaryWrappers();

    osg::ref_ptr<osg::Node> earth = osgDB::readNodeFile("0-0-0.verse_tms", new osgDB::Options(
            "URL=https://webst01.is.autonavi.com/appmaptile?style%3d6&x%3d{x}&y%3d{y}&z%3d{z} UseWebMercator=1 UseEarth3D=1"));
            //"URL=https://mt1.google.com/vt/lyrs%3ds&x%3d{x}&y%3d{y}&z%3d{z} UseWebMercator=1 UseEarth3D=1"));
    osg::ref_ptr<osg::Node> tiles = osgDB::readNodeFile("E:/model/b3dm/tileset.json.verse_tiles");

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    root->addChild(earth.get());
    
    osg::Vec3 dir = tiles->getBound().center(); dir.normalize();
    osg::ref_ptr<osg::MatrixTransform> tileMT = new osg::MatrixTransform;
    tileMT->setMatrix(osg::Matrix::translate(-dir * 380.0f));
    tileMT->addChild(tiles.get());

    osg::ref_ptr<osg::EllipsoidModel> em = new osg::EllipsoidModel;
    osg::Vec3d ecef = tiles->getBound().center(), newEcef, lla;
    em->convertXYZToLatLongHeight(ecef[0], ecef[1], ecef[2], lla[0], lla[1], lla[2]);

    osg::Vec3d newLLA = osgVerse::Coordinate::convertWGS84toGCJ02(lla);
    std::cout << "WGS84: " << osg::RadiansToDegrees(lla[1]) << ", " << osg::RadiansToDegrees(lla[0]) << "; "
              << "GCJ02: " << osg::RadiansToDegrees(newLLA[1]) << ", " << osg::RadiansToDegrees(newLLA[0]) << std::endl;
    em->convertLatLongHeightToXYZ(newLLA[0], newLLA[1], newLLA[2], newEcef[0], newEcef[1], newEcef[2]);

    osg::ref_ptr<osg::MatrixTransform> tileOffset = new osg::MatrixTransform;
    tileOffset->setMatrix(osg::Matrix::translate(newEcef - ecef));
    tileOffset->addChild(tileMT.get());
    root->addChild(tileOffset.get());

    osg::BoundingSphere bs = tiles->getBound(); double r = bs.radius() * 10.0;
    osg::ref_ptr<osgGA::TrackballManipulator> trackball = new osgGA::TrackballManipulator;
    trackball->setHomePosition(bs.center() + osg::Z_AXIS * r, bs.center(), osg::Y_AXIS);

    osgViewer::Viewer viewer;
    viewer.getCamera()->setNearFarRatio(0.00001);
    //viewer.setDatabasePager(new MyDatabasePager);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(trackball.get());
    viewer.setSceneData(root.get());
    return viewer.run();
}

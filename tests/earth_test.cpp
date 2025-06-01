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
#include <pipeline/IncrementalCompiler.h>
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

static std::string replace(std::string& src, const std::string& match, const std::string& v, bool& c)
{
    size_t levelPos = src.find(match); if (levelPos == std::string::npos) { c = false; return src; }
    src.replace(levelPos, match.length(), v); c = true; return src;
}

static std::string createCustomPath(const std::string& prefix, int x, int y, int z)
{
    std::string path = prefix; bool changed = false;
    //path = replace(path, "{x16}", std::to_string(x / 16), changed);
    //path = replace(path, "{y16}", std::to_string(y / 16), changed);
    //y = (int)pow((double)z, 2.0) - y - 1;
    path = replace(path, "{z}", std::to_string(z), changed);
    path = replace(path, "{x}", std::to_string(x), changed);
    path = replace(path, "{y}", std::to_string(y), changed); return path;
}

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osgVerse::updateOsgBinaryWrappers();

    osg::ref_ptr<osgDB::Options> earthOptions = new osgDB::Options(
        "URL=https://webst01.is.autonavi.com/appmaptile?style%3d6&x%3d{x}&y%3d{y}&z%3d{z} UseWebMercator=1 UseEarth3D=1");
        //"URL=https://mt1.google.com/vt/lyrs%3ds&x%3d{x}&y%3d{y}&z%3d{z} UseWebMercator=1 UseEarth3D=1");
        //"URL=http://p0.map.gtimg.com/sateTiles/{z}/{x16}/{y16}/{x}_{y}.jpg UseWebMercator=1 UseEarth3D=1");
    earthOptions->setPluginData("UrlPathFunction", (void*)createCustomPath);

    osg::ref_ptr<osg::Node> earth = osgDB::readNodeFile("0-0-0.verse_tms", earthOptions.get());
    osg::ref_ptr<osg::Node> tiles = osgDB::readNodeFiles(arguments, new osgDB::Options("DisabledPBR=1"));
    if (!earth || !tiles) return 1;

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

    osg::ref_ptr<osgVerse::IncrementalCompiler> incrementalCompiler = new osgVerse::IncrementalCompiler;
    incrementalCompiler->setCompileCallback(new osgVerse::IncrementalCompileCallback);

    osgViewer::Viewer viewer;
    viewer.getCamera()->setNearFarRatio(0.00001);
    viewer.setDatabasePager(new MyDatabasePager);
    viewer.setIncrementalCompileOperation(incrementalCompiler.get());
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(trackball.get());
    viewer.setSceneData(root.get());
    return viewer.run();
}

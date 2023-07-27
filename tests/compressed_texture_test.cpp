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

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

int main(int argc, char** argv)
{
    osgViewer::Viewer viewer;
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;

#if 1
    osgDB::ReaderWriter* rw = osgDB::Registry::instance()->getReaderWriterForExtension("verse_ktx");
    if (rw)
    {
        osg::ref_ptr<osg::Image> image0 = osgDB::readImageFile("Images/clockface.jpg");
        image0->setInternalTextureFormat(GL_RGB8);
        rw->writeImage(*image0, "clockface.ktx");
        OSG_NOTICE << "KTX file saved!\n";
    }
    else
    {
        OSG_WARN << "No KTX plugin!\n";
        return 1;
    }

    osg::ref_ptr<osg::Image> image = rw->readImage("clockface.ktx").getImage();
    if (image.valid())
    {
        osg::Geode* geode = osg::createGeodeForImage(image.get());
        root->addChild(geode);
    }
#else
    osg::ref_ptr<osg::Image> image = osgDB::readImageFile("Images/clockface.jpg");
    osg::Geode* geode = osg::createGeodeForImage(image.get());

    osg::Texture* tex = static_cast<osg::Texture*>(
        geode->getDrawable(0)->getStateSet()->getTextureAttribute(0, osg::StateAttribute::TEXTURE));
    if (tex) tex->setInternalFormatMode(osg::Texture::USE_S3TC_DXT1_COMPRESSION);
    root->addChild(geode);
#endif

    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}

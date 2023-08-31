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

#include <readerwriter/DracoProcessor.h>
#ifdef OSG_LIBRARY_STATIC
USE_SERIALIZER_WRAPPER(DracoGeometry)
#endif

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

#define COMPRESSING_GEOMETRY 0
#define COMPRESSING_TEXTURE 1

int main(int argc, char** argv)
{
    osgViewer::Viewer viewer;
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;

#if COMPRESSING_GEOMETRY
    {
        osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile("cow.osg");
        if (!scene.valid()) return 1;

        osg::Geode* parent = scene->asGroup()->getChild(0)->asGeode();
        osg::Geometry* geom = parent->getDrawable(0)->asGeometry();
        if (geom)
        {
#if 1
            // Without Draco: cow.osgb = 297kb
            // With Draco   : cow.osgb = 81kb
            osg::ref_ptr<osgVerse::DracoGeometry> geom2 = new osgVerse::DracoGeometry(*geom);
            parent->replaceDrawable(geom, geom2.get());
            osgDB::writeNodeFile(*scene, "draco_cow.osgb");

            scene = osgDB::readNodeFile("draco_cow.osgb");
            root->addChild(scene.get());
#else
            osgVerse::DracoProcessor dp;
            std::ofstream out("draco_geom.bin", std::ios::out | std::ios::binary);
            if (dp.encodeDracoData(out, geom))
            {
                out.close();
                std::ifstream in("draco_geom.bin", std::ios::in | std::ios::binary);
                osg::ref_ptr<osg::Geometry> new_geom = dp.decodeDracoData(in);
                if (new_geom.valid())
                {
                    osg::Geode* geode = new osg::Geode;
                    geode->addDrawable(new_geom.get());
                    root->addChild(geode);
                    std::cout << "Draco data added\n";
                }
            }
            else return 1;
#endif
        }
    }

#elif COMPRESSING_TEXTURE
    osgDB::ReaderWriter* rw = osgDB::Registry::instance()->getReaderWriterForExtension("verse_ktx");
    if (rw)
    {
        osg::ref_ptr<osg::Image> image0 = osgDB::readImageFile("Images/clockface.jpg");
        rw->writeImage(*image0, "clockface.ktx");
        OSG_NOTICE << "KTX file saved!\n";
    }
    else
    {
        OSG_WARN << "No KTX plugin!\n";
        return 1;
    }

    // No compressed RGBA32: CPU memory = 1.06GB, GPU memory = 1.4GB
    // DXT BC1 / BC3: CPU memory = 207MB, GPU memory = 0.6GB
    // KTX ETC1 / ETC2: CPU memory = 209MB, GPU memory = 1.4GB (NV drivers may not support it)
    for (int i = 0; i < 1000; ++i)
    {
        osg::ref_ptr<osg::Image> image = rw->readImage("clockface.ktx").getImage();
        if (image.valid())
        {
            osg::Geode* geode = osg::createGeodeForImage(image.get());
            root->addChild(geode);
        }
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

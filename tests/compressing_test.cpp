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

#include <VerseCommon.h>
#include <modeling/Utilities.h>
#include <readerwriter/Utilities.h>
#include <readerwriter/DracoProcessor.h>
#ifdef OSG_LIBRARY_STATIC
USE_SERIALIZER_WRAPPER(DracoGeometry)
#endif

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

class SceneDataOptimizer : public osgVerse::TextureOptimizer
{
public:
    SceneDataOptimizer()
        : osgVerse::TextureOptimizer(true, "optimize_tex_compress_test") {}

protected:
    virtual void applyTexture(osg::Texture* tex, unsigned int unit)
    {
        osg::Image* image = tex->getImage(0);
        if (image && image->valid() && !image->isMipmap())
        {
            image->ensureValidSizeForTexturing(2048);
            osgVerse::generateMipmaps(*image, false);
        }

        osgVerse::TextureOptimizer::applyTexture(tex, unit);
        tex->setClientStorageHint(false);
        tex->setUnRefImageDataAfterApply(true);
    }
};

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    osgVerse::updateOsgBinaryWrappers();

#if true
    osg::ref_ptr<osg::Image> image = osgDB::readImageFile("Images/osg256.png");
    osg::ref_ptr<osg::Image> dds = osgVerse::compressImage(*image);
    root->addChild(osg::createGeodeForImage(dds.get()));
    root->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
#elif false
    osg::ref_ptr<osg::Node> node = osgDB::readNodeFiles(arguments);
    if (node)
    {
        osgVerse::MeshCollector mc; node->accept(mc);
        std::vector<osg::ref_ptr<osg::Geometry>> geomList = mc.output();

        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        for (size_t i = 0; i < geomList.size(); ++i)
        {
            osgVerse::MeshOptimizer optimizer;
            optimizer.optimize(geomList[i].get());
            geode->addDrawable(geomList[i].get());
        }
        root->addChild(geode.get());
    }
#else
    std::string outFile = "result.osgb";
    if (arguments.read("--out"))
    {
        osg::ref_ptr<osg::Node> node = osgDB::readNodeFiles(arguments);
        if (node)
        {
            SceneDataOptimizer sdo; node->accept(sdo);
            sdo.deleteSavedTextures();

            osg::ref_ptr<osgDB::Options> options = new osgDB::Options("WriteImageHint=IncludeFile");
            options->setPluginStringData("UseBASISU", "1");
            arguments.read("--filename", outFile);
            osgDB::writeNodeFile(*node, outFile, options.get());
            root->addChild(node.get());
        }
    }
    else
        root->addChild(osgDB::readNodeFiles(arguments));
#endif

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}

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

#include <pipeline/Global.h>
#include <readerwriter/Utilities.h>
#include <readerwriter/DracoProcessor.h>
#ifdef OSG_LIBRARY_STATIC
USE_SERIALIZER_WRAPPER(DracoGeometry)
#endif

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

class SceneDataOptimizer : public osgVerse::TextureOptimizer
{
public:
    SceneDataOptimizer() : osgVerse::TextureOptimizer() {}

protected:
    virtual void applyTexture(osg::Texture* tex, unsigned int unit)
    {
        osgVerse::TextureOptimizer::applyTexture(tex, unit);
        tex->setClientStorageHint(false);
        tex->setUnRefImageDataAfterApply(true);
    }
};

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osgVerse::updateOsgBinaryWrappers();

    std::string outFile = "result.osgb";
    if (arguments.read("--out"))
    {
        osg::ref_ptr<osg::Node> node = osgDB::readNodeFiles(arguments);
        if (node)
        {
            SceneDataOptimizer sdo; node->accept(sdo);
            arguments.read("--filename", outFile);
            osgDB::writeNodeFile(
                *node, outFile, new osgDB::Options("WriteImageHint=IncludeFile"));
        }
        return 0;
    }

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addChild(osgDB::readNodeFiles(arguments));

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}

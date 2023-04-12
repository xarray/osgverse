#include <osg/io_utils>
#include <osg/LightSource>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/Archive>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <pipeline/Pipeline.h>
#include <iostream>
#include <sstream>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

class CaptureCallback : public osg::Camera::DrawCallback
{
public:
    CaptureCallback(bool b)
    {
        _msWriter = osgDB::Registry::instance()->getReaderWriterForExtension("verse_ms");
        if (_msWriter.valid())
            _msServer = _msWriter->openArchive("TestServer", osgDB::ReaderWriter::CREATE).getArchive();
    }

    virtual ~CaptureCallback()
    {
        if (_msServer.valid()) _msServer->close();
    }
    
    virtual void operator()(osg::RenderInfo& renderInfo) const
    {
        glReadBuffer(GL_BACK);  // read from back buffer (gc must be double-buffered)
        if (_msWriter.valid())
        {
            osg::GraphicsContext* gc = renderInfo.getCurrentCamera()->getGraphicsContext();
            int width = 800, height = 600;
            if (gc->getTraits())
            {
                width = gc->getTraits()->width;
                height = gc->getTraits()->height;
            }

            osg::ref_ptr<osg::Image> image = new osg::Image;
            image->readPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE);
            _msWriter->writeImage(*image, "rtmp://127.0.0.1:1935/live/stream");
            //osgDB::writeImageFile(*image, "capture.png");
        }
        else
            OSG_WARN << "Invalid readerwriter verse_ms?\n";
    }

protected:
    osg::ref_ptr<osgDB::ReaderWriter> _msWriter;
    osg::ref_ptr<osgDB::Archive> _msServer;
};

int main(int argc, char** argv)
{
    osgDB::Registry::instance()->loadLibrary(
        osgDB::Registry::instance()->createLibraryNameForExtension("verse_ms"));
    osg::setNotifyLevel(osg::NOTICE);

    osg::ref_ptr<osg::Node> scene =
        (argc < 2) ? osgDB::readNodeFile("cessna.osg") : osgDB::readNodeFile(argv[1]);
    if (!scene) { OSG_WARN << "Failed to load " << (argc < 2) ? "" : argv[1]; return 1; }

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->addChild(scene.get());

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(sceneRoot.get());
    viewer.setUpViewInWindow(50, 50, 800, 600);

    CaptureCallback* cap = new CaptureCallback(true);
    viewer.getCamera()->setFinalDrawCallback(cap);
    return viewer.run();
}

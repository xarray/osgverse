#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <osg/LightSource>
#include <osg/Texture2D>
#include <osg/ShapeDrawable>
#include <osg/MatrixTransform>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgUtil/CullVisitor>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <readerwriter/Utilities.h>
#include <pipeline/Utilities.h>
#include <iostream>
#include <sstream>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;

    osg::ref_ptr<osgVerse::MultiModelClient> client = new osgVerse::MultiModelClient("http://127.0.0.1:5000");
    client->registerFunction(
        "text", "def handler(text: str, metadata):\\n  print(text)\\n  return {'status': 'success', 'handler': 1}");
    client->registerFunction(
        "json", "def handler(text: str, metadata):\\n  print(text)\\n  return {'status': 'success', 'handler': 2}");
    client->registerFunction(
        "image", "def handler(image: 'Image.Image', metadata):\\n  from PIL import Image\\n"
                 "  print(image.format)\\n  return {'status': 'success', 'handler': 3}");
    client->registerFunction(
        "shm", "def handler(data: bytes, metadata):\\n  return data[::-1]");

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewInWindow(0, 0, 640, 480);
    viewer.realize();

    osgVerse::QuickEventHandler* handler = new osgVerse::QuickEventHandler;
    handler->addKeyUpCallback('1', [&](int key) { client->sendText("HELLO WORLD", false); });
    handler->addKeyUpCallback('2', [&](int key) { client->sendText("{\"msg\": \"HELLO WORLD\"}", true); });
    handler->addKeyUpCallback('3', [&](int key)
    {
        osg::ref_ptr<osg::Image> img = osgDB::readImageFile("Images/osg256.png");
        if (img.valid()) client->sendImage(*img, true);
    });
    viewer.addEventHandler(handler);

    // Test SHM usage
    std::vector<unsigned char> testData(256);
    for (size_t i = 0; i < testData.size(); ++i) testData[i] = 255 - i;
    client->sendShm("test", testData.data(), testData.size(), true);

    while (!viewer.done())
    {
        viewer.frame();
        if (!(viewer.getFrameStamp()->getFrameNumber() % 60))
        {
            std::vector<unsigned char> rcv = client->receiveShm("test");
            for (size_t i = 0; i < osg::minimum(rcv.size(), (size_t)10); ++i) std::cout << (int)rcv[i] << " ";
            std::cout << "... Total size = " << rcv.size() << std::endl;
        }
    }
    client->cleanupShm("test");
    return 0;
}

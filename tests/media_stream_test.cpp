#include <osg/io_utils>
#include <osg/ImageStream>
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

#include <libhv/all/server/WebSocketServer.h>
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#define MEDIA_PULLER 1
#define MEDIA_SERVER 0
#define MEDIA_WEBRTC_COMMANDS 1

class HttpApiCallback : public osgVerse::UserCallback
{
public:
    HttpApiCallback(osgViewer::View* v, const std::string& name)
        : osgVerse::UserCallback(name), _view(v) {}

    virtual bool run(osg::Object* object, Parameters& in, Parameters& out) const
    {
        if (in.empty()) return false;
        osgVerse::StringObject* so = static_cast<osgVerse::StringObject*>(in[0].get());
        if (!so || (so && so->values.size() < 2)) return false;

        // TODO
        std::cout << so->values[0] << ", " << so->values[1] << "\n";
        return true;
    }

protected:
    osgViewer::View* _view;
};

class CaptureCallback : public osg::Camera::DrawCallback
{
public:
    CaptureCallback(osgViewer::View* v, bool b)
    {
        _msWriter = osgDB::Registry::instance()->getReaderWriterForExtension("verse_ms");
#if MEDIA_SERVER
        if (_msWriter.valid())
        {
            osg::ref_ptr<osgDB::Options> options = new osgDB::Options;
#   if MEDIA_SERVER_WEBRTC
            options->setPluginStringData("http", "80");
            options->setPluginStringData("rtsp", "554");
            options->setPluginStringData("rtmp", "1935");
            options->setPluginStringData("rtc", "8000");  // set RTC port: 8000
#   endif
            _msServer = _msWriter->openArchive(
                "TestServer", osgDB::ReaderWriter::CREATE, 4096, options.get()).getArchive();
            _msServer->getOrCreateUserDataContainer()->addUserObject(new HttpApiCallback(v, "HttpAPI"));
        }
#endif
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
            image->flipVertical();  // low-performance, just for example here
            _msWriter->writeImage(*image, "rtmp://127.0.0.1:1935/live/stream");
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

#if MEDIA_PULLER
    CaptureCallback* cap = new CaptureCallback(&viewer, true);
    viewer.getCamera()->setFinalDrawCallback(cap);
#else
    osg::ImageStream* is = dynamic_cast<osg::ImageStream*>(
        osgDB::readImageFile("rtmp://ns8.indexforce.com/home/mystream.verse_ms"));
    if (is) is->play(); else return 1;

    osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
    mt->addChild(osg::createGeodeForImage(is));
    mt->setMatrix(osg::Matrix::scale(4.0f, 4.0f, 4.0f) *
                  osg::Matrix::translate(0.0f, 0.0f, 5.0f));
    sceneRoot->addChild(mt.get());
#endif

#if MEDIA_WEBRTC_COMMANDS
    // Example command from WebRTC puller
    hv::WebSocketService ws;
    ws.onmessage = [&viewer](const WebSocketChannelPtr& channel, const std::string& msg)
    {
        //printf("onmessage: %.*s\n", (int)msg.size(), msg.data());
        osgDB::StringList values;
        osgDB::split(msg, values, ',');
        if (values.size() < 3) return;  // cmd,x,y

        float x = atof(values[1].c_str()) * 800.0f;
        float y = atof(values[2].c_str()) * 600.0f;
        if (values[0] == "move")
            viewer.getEventQueue()->mouseMotion(x, y);
        else if (values[0] == "press")
            viewer.getEventQueue()->mouseButtonPress(x, y, osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON);
        else if (values[0] == "release")
            viewer.getEventQueue()->mouseButtonRelease(x, y, osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON);
    };

    hv::WebSocketServer server;
    server.registerWebSocketService(&ws);
    server.setPort(9999);
    server.setThreadNum(4);
    server.start();
#endif
    return viewer.run();
}

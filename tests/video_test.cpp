#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <pipeline/ExternalTexture2D.h>
#include <pipeline/Utilities.h>
#include <readerwriter/Utilities.h>
#include <iostream>
#include <sstream>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());
    std::string file; bool recordeMode = arguments.read("--record");
    if (!arguments.read("--file", file))
    {
        std::cout << "Please specify a movie file name or stream URL with --file."
                  << std::endl; return 1;
    }
    if (file.empty())
    { file = "record.mp4.verse_ffmpeg"; recordeMode = true; }

    CUcontext cuContext = osgVerse::CudaAlgorithm::initializeContext(0);
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    osg::ref_ptr<osgVerse::ExternalTexture2D> videoTexture;
    osg::ref_ptr<osgVerse::GpuResourceDemuxerMuxerContainer> videoRecorder;

    osgDB::Options* opt = new osgDB::Options; opt->setPluginData("Context", cuContext);
    if (recordeMode)
    {
        osgVerse::GpuResourceReaderWriterContainer* container =
            dynamic_cast<osgVerse::GpuResourceReaderWriterContainer*>(osgDB::readObjectFile("encoder.codec_nv", opt));
        if (!container)
        {
            OSG_WARN << "No encoder found for video recording" << std::endl;
            return 0;
        }

        videoRecorder = new osgVerse::GpuResourceDemuxerMuxerContainer;
        container->getWriter()->openResource(videoRecorder.get());
        // Use osgDB::writeObjectFile(*videoRecorder, name) to create muxer and save H264 frames

        // Set up scene graph
        // TODO: add container1->getWriter() to camera drawcallback
    }
    else
    {
        osgVerse::GpuResourceReaderWriterContainer* container =
            dynamic_cast<osgVerse::GpuResourceReaderWriterContainer*>(osgDB::readObjectFile("decoder.codec_nv", opt));
        if (!container)
        {
            OSG_WARN << "No decoder found for video playing" << std::endl;
            return 0;
        }

        osgVerse::GpuResourceDemuxerMuxerContainer* videoReader =
            dynamic_cast<osgVerse::GpuResourceDemuxerMuxerContainer*>(osgDB::readObjectFile(file));
        if (!videoReader)
        {
            OSG_WARN << "No demuxer found for video file: " << file << std::endl;
            return 0;
        }

        videoTexture = new osgVerse::ExternalTexture2D(cuContext);
        videoTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        videoTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        videoTexture->setResourceReader(container->getReader());
        if (!container->getReader()->openResource(videoReader))
            OSG_WARN << "Failed to open video demuxer: " << file << std::endl;

        // Add audio container if needed
        container->getReader()->setAudioContainer(osgVerse::AudioPlayer::instance());

        // Set up scene graph
        osg::Geometry* quad = osg::createTexturedQuadGeometry(
            osg::Vec3(), osg::X_AXIS * 1.6f, osg::Z_AXIS * 0.9f, 0.0f, 1.0f, 1.0f, 0.0f);
        quad->getOrCreateStateSet()->setTextureAttributeAndModes(0, videoTexture.get());

        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable(quad); root->addChild(geode.get());
    }

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    while (!viewer.done())
    {
        viewer.frame();
        if (videoRecorder.valid()) osgDB::writeObjectFile(*videoRecorder, file);
    }

    if (videoTexture.valid()) videoTexture->releaseGpuData();
    osgVerse::CudaAlgorithm::deinitializeContext(cuContext);
    return 0;
}

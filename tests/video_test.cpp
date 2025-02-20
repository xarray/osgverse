#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <pipeline/CudaTexture2D.h>
#include <pipeline/Utilities.h>
#include <cuda.h>
#include <iostream>
#include <sstream>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

static void createCudaContext(CUcontext* cuContext, int iGpu, unsigned int flags)
{
    CUdevice cuDevice = 0;
    char deviceName[80];

    cuDeviceGet(&cuDevice, iGpu);
    cuDeviceGetName(deviceName, sizeof(deviceName), cuDevice);
    cuCtxCreate(cuContext, flags, cuDevice);
    OSG_NOTICE << "GPU in use: " << deviceName << std::endl;
}

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    std::string file; if (!arguments.read("--file", file))
    {
        std::cout << "Please specify a movie file name or stream URL."
                  << std::endl; return 1;
    }

    int numGpu = 0, idGpu = 0;
    cuInit(0); cuDeviceGetCount(&numGpu);
    if (idGpu < 0 || idGpu >= numGpu) return 1;

    CUcontext cuContext = NULL;
    createCudaContext(&cuContext, idGpu, CU_CTX_SCHED_BLOCKING_SYNC);

    osgDB::Options* opt = new osgDB::Options; opt->setPluginData("Context", cuContext);
    osgVerse::CudaResourceReaderWriterContainer* container1 =
        dynamic_cast<osgVerse::CudaResourceReaderWriterContainer*>(osgDB::readObjectFile("decoder.codec_nv", opt));
    if (!container1)
    {
        OSG_WARN << "No decoder found for video playing" << std::endl;
        return 0;
    }

    osgVerse::CudaResourceDemuxerMuxerContainer* container2 =
        dynamic_cast<osgVerse::CudaResourceDemuxerMuxerContainer*>(osgDB::readObjectFile(file));
    if (!container2)
    {
        OSG_WARN << "No demuxer found for video file: " << file << std::endl;
        return 0;
    }
    
    osg::ref_ptr<osgVerse::CudaTexture2D> tex = new osgVerse::CudaTexture2D(cuContext);
    tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    tex->setResourceReader(container1->getReader());
    container1->getReader()->openResource(container2->getDemuxer());

    // Scene graph and simulation loop
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    {
        osg::Geometry* quad = osg::createTexturedQuadGeometry(
            osg::Vec3(), osg::X_AXIS * 1.6f, osg::Z_AXIS * 0.9f, 0.0f, 1.0f, 1.0f, 0.0f);
        quad->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex.get());

        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable(quad); root->addChild(geode.get());
    }

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.run();

    tex->releaseCudaData();
    cuCtxDestroy(cuContext);
    return 0;
}

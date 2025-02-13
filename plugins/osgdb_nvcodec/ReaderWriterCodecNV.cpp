#include <osg/io_utils>
#include <osg/UserDataContainer>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/ImageStream>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgDB/Archive>

#include "pipeline/CudaUtils/ColorSpace.h"
#include "pipeline/CudaTexture2D.h"
#include "NvDecoder/NvDecoder.h"
#include "Utils/NvCodecUtils.h"
#include "Utils/FFmpegDemuxer.h"
#include "Common/AppDecUtils.h"
simplelogger::Logger* logger = simplelogger::LoggerFactory::CreateConsoleLogger();

class CuvidResourceReader : public osgVerse::CudaResourceReaderBase
{
public:
    CuvidResourceReader(CUcontext cu)
        : osgVerse::CudaResourceReaderBase(cu), _numFrames(0)
    { _demuxer = NULL; _decoder = NULL; }

    virtual ~CuvidResourceReader()
    {
        if (_decoder != NULL) delete _decoder;
        if (_demuxer != NULL) delete _demuxer;
    }

    virtual bool openResource(const std::string& name)
    {
        if (_decoder != NULL) delete _decoder;
        if (_demuxer != NULL) delete _demuxer;
        
        _demuxer = new FFmpegDemuxer(name.c_str(), 1000i64);
        _decoder = new NvDecoder(_cuContext, true, FFmpeg2NvCodecId(_demuxer->GetVideoCodec()));
        _width = (_demuxer->GetWidth() + 1) & ~1; _height = _demuxer->GetHeight();
        _numFrames = 0; return true;
    }

    virtual void releaseCuda()
    {
        _mutex.lock();
        ck(cuMemFree(_deviceFrame)); _pbo = 0;
        if (_decoder != NULL) delete _decoder; _decoder = NULL;
        if (_demuxer != NULL) delete _demuxer; _demuxer = NULL;
        _mutex.unlock();
    }

    virtual void operator()(osg::StateAttribute* sa, osg::NodeVisitor* nv)
    {
        CUdeviceptr deviceFrame = NULL;
        uint8_t* video = NULL, * frame = NULL;
        int videoBytes = 0, frameReturned = 0, matrixData = 0, pitch = _width * 4;
        if (!_demuxer || !_decoder || !_vendorStatus) { setState(INVALID); return; }

        _demuxer->Demux(&video, &videoBytes);
        frameReturned = _decoder->Decode(video, videoBytes);
        if (!_numFrames && frameReturned)
            OSG_NOTICE << "[CuvidResourceReader] " << _decoder->GetVideoInfo() << std::endl;

        for (int i = 0; i < frameReturned; i++)
        {
            frame = _decoder->GetFrame();
            if (!getDeviceFrameBuffer(&deviceFrame, &pitch)) continue;
            matrixData = _decoder->GetVideoFormatInfo().video_signal_description.matrix_coefficients;

            cudaVideoSurfaceFormat format = _decoder->GetOutputFormat();
            if (_decoder->GetBitDepth() == 8)
            {
                if (format == cudaVideoSurfaceFormat_YUV444)
                    YUV444ToColor32<BGRA32>(frame, _decoder->GetWidth(), (uint8_t*)deviceFrame, pitch,
                                            _decoder->GetWidth(), _decoder->GetHeight(), matrixData);
                else if (format == cudaVideoSurfaceFormat_NV12)
                    Nv12ToColor32<BGRA32>(frame, _decoder->GetWidth(), (uint8_t*)deviceFrame, pitch,
                                          _decoder->GetWidth(), _decoder->GetHeight(), matrixData);
            }
            else
            {
                if (format == cudaVideoSurfaceFormat_YUV444 || format == cudaVideoSurfaceFormat_YUV444_16Bit)
                    YUV444P16ToColor32<BGRA32>(frame, 2 * _decoder->GetWidth(), (uint8_t*)deviceFrame, pitch,
                                               _decoder->GetWidth(), _decoder->GetHeight(), matrixData);
                else if (format == cudaVideoSurfaceFormat_P016)
                    P016ToColor32<BGRA32>(frame, 2 * _decoder->GetWidth(), (uint8_t*)deviceFrame, pitch,
                                          _decoder->GetWidth(), _decoder->GetHeight(), matrixData);
            }
        }

        if (videoBytes == 0) setState(STOPPED); else setState(PLAYING);
        _numFrames += frameReturned;
    }

protected:
    FFmpegDemuxer* _demuxer;
    NvDecoder* _decoder;
    int _numFrames;
};

/** How to use the codec plugin:
    - int numGpu = 0, idGpu = 0; cuInit(0)); cuDeviceGetCount(&numGpu);
      if (idGpu < 0 || idGpu >= numGpu) return 1;
      CUcontext cuContext = NULL; createCudaContext(&cuContext, idGpu, CU_CTX_SCHED_BLOCKING_SYNC);

    - osgDB::Options* opt = new osgDB::Options; opt->setPluginData("Context", cuContext);
      osg::ref_ptr<osgVerse::CudaTexture2D> tex = new osgVerse::CudaTexture2D(cuContext);
      tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
      tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);

    - osgVerse::CudaResourceReaderContainer* container =
          dynamic_cast<osgVerse::CudaResourceReaderContainer*>(osgDB::readObject("movie.codec_nv", opt));
      tex->setResourceReader(container->getResourceReader());
      ......

    - tex->releaseCudaData(); cuCtxDestroy(cuContext);
*/
class ReaderWriterCodecNV : public osgDB::ReaderWriter
{
public:
    ReaderWriterCodecNV()
    {
        supportsExtension("codec_nv", "Pseudo file extension, used to select the plugin.");
        supportsOption("Context", "Cuda context to use, must set it to correctly processing data");
    }

    virtual ~ReaderWriterCodecNV()
    {
    }

    virtual const char* className() const
    { return "[osgVerse] Video codec plugin depending on NVIDIA Video Codec SDK"; }

    virtual ReadResult readObject(const std::string& path, const osgDB::Options* options = NULL) const
    {
        std::string fileName(path);
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

        bool usePseudo = (ext == "codec_nv");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }

        const void* context = (options ? options->getPluginData("Context") : NULL);
        if (context != NULL)
        {
            osg::ref_ptr<CuvidResourceReader> reader = new CuvidResourceReader((CUcontext)context);
            if (reader->openResource(fileName))
            {
                osgVerse::CudaResourceReaderContainer* container =
                    new osgVerse::CudaResourceReaderContainer;
                container->setResourceReader(reader.get()); return container;
            }
        }
        return ReadResult::FILE_NOT_FOUND;
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(codec_nv, ReaderWriterCodecNV)

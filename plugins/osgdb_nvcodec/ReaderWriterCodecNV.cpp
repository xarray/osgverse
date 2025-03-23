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
#include "Utils/NvCodecUtils.h"
#if defined(NV_DECODER)
#   include "NvDecoder/NvDecoder.h"
#   include "Common/AppDecUtils.h"
#endif
#if defined(NV_ENCODER)
#   include "NvEncoder/NvEncoder.h"
#   include "NvEncoder/NvEncoderCuda.h"
#endif
simplelogger::Logger* logger = simplelogger::LoggerFactory::CreateConsoleLogger();

#if defined(NV_DECODER)
class CuvidResourceReader : public osgVerse::CudaResourceReaderBase
{
public:
    CuvidResourceReader(CUcontext cu)
        : osgVerse::CudaResourceReaderBase(cu), _numFrames(0)
    { _decoder = NULL; }

    virtual ~CuvidResourceReader()
    { if (_decoder != NULL) delete _decoder; }

    virtual bool openResource(Demuxer* demuxer)
    {
        if (_decoder != NULL) delete _decoder; _decoder = NULL;
        _demuxer = demuxer; if (!demuxer) return false;
        if (_demuxer->getVideoCodec() != osgVerse::CODEC_INVALID)
        {
            _decoder = new NvDecoder(_cuContext, true, (cudaVideoCodec)_demuxer->getVideoCodec());
            _width = (_demuxer->getWidth() + 1) & ~1; _height = _demuxer->getHeight();
        }
        _numFrames = 0; return true;
    }

    virtual void releaseCuda()
    {
        _mutex.lock();
        ck(cuMemFree(_deviceFrame)); _pbo = 0; _demuxer = NULL;
        if (_decoder != NULL) delete _decoder; _decoder = NULL;
        _mutex.unlock();
    }

    virtual void operator()(osg::StateAttribute* sa, osg::NodeVisitor* nv)
    {
        if (_demuxer && !_decoder)
        {
            if (_demuxer->getVideoCodec() == osgVerse::CODEC_INVALID) return;
            _decoder = new NvDecoder(_cuContext, true, (cudaVideoCodec)_demuxer->getVideoCodec());
            _width = (_demuxer->getWidth() + 1) & ~1; _height = _demuxer->getHeight();
        }

        CUdeviceptr deviceFrame = NULL;
        uint8_t* video = NULL, * frame = NULL;
        int videoBytes = 0, frameReturned = 0, matrixData = 0, pitch = _width * 4;
        if (!_demuxer || !_decoder || !_vendorStatus) { setState(INVALID); return; }
        if (!_demuxer->demux(&video, &videoBytes, NULL)) { setState(PENDING); return; }

        frameReturned = _decoder->Decode(video, videoBytes);
        if (!_numFrames && frameReturned)
            OSG_NOTICE << "[CuvidResourceReader] " << _decoder->GetVideoInfo() << std::endl;
        if (videoBytes > 0 && !frameReturned)
        {
            OSG_INFO << "[CuvidResourceReader] Failed to decode frame (" << _width << "x"
                     << _height << "), bytes = " << videoBytes << std::endl;
        }

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
    NvDecoder* _decoder;
    int _numFrames;
};
#endif

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

        const void* context = (options ? options->getPluginData("Context") : NULL);
        if (context != NULL)
        {
            osg::ref_ptr<osgVerse::CudaResourceReaderWriterContainer> container =
                new osgVerse::CudaResourceReaderWriterContainer;
            std::transform(fileName.begin(), fileName.end(), fileName.begin(), tolower);
            if (fileName.find("encode") != std::string::npos)
            {
#if defined(NV_ENCODER)
                // TODO: container->setReader(new CuvidResourceWriter((CUcontext)context));
#else
                OSG_FATAL << "[ReaderWriterCodecNV] NvEncoder dependency not found" << std::endl;
                return ReadResult::ERROR_IN_READING_FILE;
#endif
            }
            else
            {
#if defined(NV_DECODER)
                container->setReader(new CuvidResourceReader((CUcontext)context));
#else
                OSG_FATAL << "[ReaderWriterCodecNV] NvDecoder dependency not found" << std::endl;
                return ReadResult::ERROR_IN_READING_FILE;
#endif
            }
            return container.get();
        }
        return ReadResult::FILE_NOT_FOUND;
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(codec_nv, ReaderWriterCodecNV)

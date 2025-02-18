#ifndef MANA_PP_CUDATEXTURE2D_HPP
#define MANA_PP_CUDATEXTURE2D_HPP

#include <osg/State>
#include <osg/Texture2D>
#include <mutex>

#if defined(_WIN64) || defined(__LP64__)
typedef unsigned long long CUdeviceptr_v2;
#else
typedef unsigned int CUdeviceptr_v2;
#endif
typedef CUdeviceptr_v2 CUdeviceptr;
typedef struct CUctx_st* CUcontext;

namespace osgVerse
{
    class CudaResourceReaderBase;

    enum VideoCodecType
    {
        CODEC_INVALID = -1,
        CODEC_MPEG1 = 0,                                       /**<  MPEG1             */
        CODEC_MPEG2,                                           /**<  MPEG2             */
        CODEC_MPEG4,                                           /**<  MPEG4             */
        CODEC_VC1,                                             /**<  VC1               */
        CODEC_H264,                                            /**<  H264              */
        CODEC_JPEG,                                            /**<  JPEG              */
        CODEC_H264_SVC,                                        /**<  H264-SVC          */
        CODEC_H264_MVC,                                        /**<  H264-MVC          */
        CODEC_HEVC,                                            /**<  HEVC              */
        CODEC_VP8,                                             /**<  VP8               */
        CODEC_VP9,                                             /**<  VP9               */
        CODEC_AV1,                                             /**<  AV1               */
        CODEC_MAXIMUM,                                         /**<  Max codecs        */
        CODEC_YUV420 = (('I' << 24) | ('Y' << 16) | ('U' << 8) | ('V')),   /**< Y,U,V (4:2:0)      */
        CODEC_YV12 = (('Y' << 24) | ('V' << 16) | ('1' << 8) | ('2')),     /**< Y,V,U (4:2:0)      */
        CODEC_NV12 = (('N' << 24) | ('V' << 16) | ('1' << 8) | ('2')),     /**< Y,UV  (4:2:0)      */
        CODEC_YUYV = (('Y' << 24) | ('U' << 16) | ('Y' << 8) | ('V')),     /**< YUYV/YUY2 (4:2:2)  */
        CODEC_UYVY = (('U' << 24) | ('Y' << 16) | ('V' << 8) | ('Y'))      /**< UYVY (4:2:2)       */
    };

    // The Cuda-based texture
    class CudaTexture2D : public osg::Texture2D
    {
    public:
        CudaTexture2D(void* cuContext);
        CudaTexture2D(const CudaTexture2D& copy, osg::CopyOp op = osg::CopyOp::SHALLOW_COPY);

        void setResourceReader(CudaResourceReaderBase* reader);
        const CudaResourceReaderBase* getResourceReader() const;

        virtual void releaseGLObjects(osg::State* state = NULL) const;
        void releaseCudaData();

    protected:
        void* _cuContext;
    };

    // Cuda resource reader: load H264 frames from Demuxer and decode to texture
    class CudaResourceReaderBase : public osg::Texture2D::SubloadCallback
    {
    public:
        class Demuxer : public osg::Referenced
        {
        public:
            virtual VideoCodecType getVideoCodec() { return CODEC_INVALID; }
            virtual int getWidth() { return 0; }
            virtual int getHeight() { return 0; }

            virtual bool demux(unsigned char** videoData, int* videoBytes, long long* pts)
            { return false; }
        };

        CudaResourceReaderBase(CUcontext cu);
        virtual void operator()(osg::StateAttribute* sa, osg::NodeVisitor* nv) {}

        virtual bool openResource(Demuxer* demuxer) = 0;
        virtual void closeResource() { _demuxer = NULL; }

        virtual void releaseGLObjects(osg::State* state = NULL) const;
        virtual void releaseCuda();

        virtual osg::ref_ptr<osg::Texture::TextureObject> generateTextureObject(
            const osg::Texture2D& texture, osg::State& state) const;
        virtual void load(const osg::Texture2D& texture, osg::State& state) const;
        virtual void subload(const osg::Texture2D& texture, osg::State& state) const;

        enum ResourceState { INVALID = 0, PLAYING, STOPPED, PENDING };
        void setState(ResourceState s) { _state = s; }
        ResourceState getState() const { return _state; }

    protected:
        virtual ~CudaResourceReaderBase() {}
        bool getDeviceFrameBuffer(CUdeviceptr* devFrameOut, int* pitchOut);

        osg::ref_ptr<Demuxer> _demuxer;
        CUcontext _cuContext;
        mutable CUdeviceptr _deviceFrame;
        ResourceState _state;
        int _width, _height;
        mutable GLuint _pbo, _textureID;
        mutable bool _vendorStatus;
        mutable std::mutex _mutex;
    };

    // The Cuda demuxer/muxer container
    class CudaResourceDemuxerMuxerContainer : public osg::Object
    {
    public:
        CudaResourceDemuxerMuxerContainer() {}
        CudaResourceDemuxerMuxerContainer(const CudaResourceDemuxerMuxerContainer& copy,
                                          const osg::CopyOp& op = osg::CopyOp::SHALLOW_COPY)
        : osg::Object(copy, op), _demuxer(copy._demuxer) {}
        META_Object(osgVerse, CudaResourceDemuxerMuxerContainer)

        void setDemuxer(CudaResourceReaderBase::Demuxer* r) { _demuxer = r; }
        CudaResourceReaderBase::Demuxer* getDemuxer() { return _demuxer.get(); }
        const CudaResourceReaderBase::Demuxer* getDemuxer() const { return _demuxer.get(); }

    protected:
        osg::ref_ptr<CudaResourceReaderBase::Demuxer> _demuxer;
    };

    // The Cuda reader/writer container
    class CudaResourceReaderWriterContainer : public osg::Object
    {
    public:
        CudaResourceReaderWriterContainer() {}
        CudaResourceReaderWriterContainer(const CudaResourceReaderWriterContainer& copy,
                                          const osg::CopyOp& op = osg::CopyOp::SHALLOW_COPY)
        : osg::Object(copy, op), _reader(copy._reader) {}
        META_Object(osgVerse, CudaResourceReaderWriterContainer)

        void setReader(CudaResourceReaderBase* r) { _reader = r; }
        CudaResourceReaderBase* getReader() { return _reader.get(); }
        const CudaResourceReaderBase* getReader() const { return _reader.get(); }

    protected:
        osg::ref_ptr<CudaResourceReaderBase> _reader;
    };
}

#endif

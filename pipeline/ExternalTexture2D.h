#ifndef MANA_PP_ExternalTexture2D_HPP
#define MANA_PP_ExternalTexture2D_HPP

#include <osg/Version>
#include <osg/State>
#include <osg/Texture2D>
#include <osg/Camera>
#include <mutex>

#if defined(_WIN64) || defined(__LP64__)
typedef unsigned long long CUdeviceptr_v2;
#else
typedef unsigned int CUdeviceptr_v2;
#endif
typedef CUdeviceptr_v2 CUdeviceptr;

#ifdef VERSE_ENABLE_MTT
typedef struct MUctx_st* CUcontext;
typedef struct MUgraphicsResource_st* CUgraphicsResource;
#else
typedef struct CUctx_st* CUcontext;
typedef struct CUgraphicsResource_st* CUgraphicsResource;
#endif

namespace osgVerse
{
    class GpuResourceReaderBase;
    class GpuResourceDemuxerMuxerContainer;

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

    // 2D texture constructed from external GPU data
    class ExternalTexture2D : public osg::Texture2D
    {
    public:
        ExternalTexture2D(void* cuContext);
        ExternalTexture2D(const ExternalTexture2D& copy, const osg::CopyOp& op = osg::CopyOp::SHALLOW_COPY);

        void setResourceReader(GpuResourceReaderBase* reader);
        const GpuResourceReaderBase* getResourceReader() const;

        virtual void releaseGLObjects(osg::State* state = NULL) const;
        void releaseGpuData();

    protected:
        virtual ~ExternalTexture2D();
        void* _cuContext;
    };

    // Gpu resource reader: load H264 frames from Demuxer and decode to texture
    class GpuResourceReaderBase : public osg::Texture2D::SubloadCallback
    {
    public:
        class Demuxer : public osg::Referenced
        {
        public:
            virtual VideoCodecType getVideoCodec() { return CODEC_INVALID; }
            virtual int getWidth() const { return 0; }
            virtual int getHeight() const { return 0; }
            virtual double getFrameRate() const { return 25; }

            virtual bool demux(unsigned char** videoData, int* videoBytes, long long* pts)
            { return false; }

            void setAudioContainer(osg::Referenced* a) { _audioContainer = a; }
            osg::Referenced* getAudioContainer() { return _audioContainer.get(); }
            const osg::Referenced* getAudioContainer() const { return _audioContainer.get(); }

        protected:
            osg::ref_ptr<osg::Referenced> _audioContainer;
        };

        GpuResourceReaderBase(CUcontext cu);
        virtual void operator()(osg::StateAttribute* sa, osg::NodeVisitor* nv) {}

        bool openResource(GpuResourceDemuxerMuxerContainer* c);
        void setAudioContainer(osg::Referenced* a) { _demuxer->setAudioContainer(a); }

        Demuxer* getDemuxer() { return _demuxer.get(); }
        const Demuxer* getDemuxer() const { return _demuxer.get(); }

        virtual bool openResource(Demuxer* demuxer) = 0;
        virtual void closeResource() { _demuxer = NULL; }

        virtual void releaseGLObjects(osg::State* state = NULL) const;
        virtual void releaseGpu();

#if OSG_VERSION_GREATER_THAN(3, 4, 0)
        virtual osg::ref_ptr<osg::Texture::TextureObject> generateTextureObject(
            const osg::Texture2D& texture, osg::State& state) const;
#else
        virtual osg::Texture::TextureObject* generateTextureObject(
            const osg::Texture2D& texture, osg::State& state) const;
#endif
        virtual void load(const osg::Texture2D& texture, osg::State& state) const;
        virtual void subload(const osg::Texture2D& texture, osg::State& state) const;

        enum ResourceState { INVALID = 0, PLAYING, STOPPED, PENDING };
        void setState(ResourceState s) { _state = s; }
        ResourceState getState() const { return _state; }

    protected:
        virtual ~GpuResourceReaderBase() {}
        bool getDeviceFrameBuffer(CUdeviceptr* devFrameOut, int* pitchOut);

        osg::ref_ptr<Demuxer> _demuxer;
        CUcontext _cuContext;
        mutable CUgraphicsResource _cuResource;
        mutable CUdeviceptr _deviceFrame;
        ResourceState _state;
        int _width, _height;
        mutable GLuint _pbo, _textureID;
        mutable bool _vendorStatus;
        mutable std::mutex _mutex;
    };

    // Gpu resource writer: load texture and encode to H264 frames, then send to Muxer
    class GpuResourceWriterBase : public osg::Camera::DrawCallback
    {
    public:
        class Muxer : public osg::Referenced
        {
        public:
            virtual VideoCodecType getVideoCodec() { return CODEC_INVALID; }
            virtual int getWidth() { return 0; }
            virtual int getHeight() { return 0; }
        };

        GpuResourceWriterBase(CUcontext cu);
        virtual void operator()(osg::RenderInfo& renderInfo) const {}

        bool openResource(GpuResourceDemuxerMuxerContainer* c);
        Muxer* getMuxer() { return _muxer.get(); }
        const Muxer* getMuxer() const { return _muxer.get(); }

        virtual bool openResource(Muxer* muxer) = 0;
        virtual void closeResource() { _muxer = NULL; }

        enum ResourceState { INVALID = 0, RECORDING, STOPPED, PAUSED };
        //void setState(ResourceState s) { _state = s; }
        //ResourceState getState() const { return _state; }

    protected:
        virtual ~GpuResourceWriterBase() {}

        osg::ref_ptr<Muxer> _muxer;
        GpuResourceDemuxerMuxerContainer* _muxerParent;
        CUcontext _cuContext;
    };

    // The Gpu demuxer/muxer container
    class GpuResourceDemuxerMuxerContainer : public osg::Object
    {
    public:
        GpuResourceDemuxerMuxerContainer() {}
        GpuResourceDemuxerMuxerContainer(const GpuResourceDemuxerMuxerContainer& copy,
                                          const osg::CopyOp& op = osg::CopyOp::SHALLOW_COPY)
        : osg::Object(copy, op), _demuxer(copy._demuxer), _muxer(copy._muxer) {}
        META_Object(osgVerse, GpuResourceDemuxerMuxerContainer)

        void setDemuxer(GpuResourceReaderBase::Demuxer* r) { _demuxer = r; }
        GpuResourceReaderBase::Demuxer* getDemuxer() { return _demuxer.get(); }
        const GpuResourceReaderBase::Demuxer* getDemuxer() const { return _demuxer.get(); }

        void setMuxer(GpuResourceWriterBase::Muxer* r) { _muxer = r; }
        GpuResourceWriterBase::Muxer* getMuxer() { return _muxer.get(); }
        const GpuResourceWriterBase::Muxer* getMuxer() const { return _muxer.get(); }

    protected:
        osg::ref_ptr<GpuResourceReaderBase::Demuxer> _demuxer;
        osg::ref_ptr<GpuResourceWriterBase::Muxer> _muxer;
    };

    // The Gpu reader/writer container
    class GpuResourceReaderWriterContainer : public osg::Object
    {
    public:
        GpuResourceReaderWriterContainer() {}
        GpuResourceReaderWriterContainer(const GpuResourceReaderWriterContainer& copy,
                                          const osg::CopyOp& op = osg::CopyOp::SHALLOW_COPY)
        : osg::Object(copy, op), _reader(copy._reader), _writer(copy._writer) {}
        META_Object(osgVerse, GpuResourceReaderWriterContainer)

        void setReader(GpuResourceReaderBase* r) { _reader = r; }
        GpuResourceReaderBase* getReader() { return _reader.get(); }
        const GpuResourceReaderBase* getReader() const { return _reader.get(); }

        void setWriter(GpuResourceWriterBase* w) { _writer = w; }
        GpuResourceWriterBase* getWriter() { return _writer.get(); }
        const GpuResourceWriterBase* getWriter() const { return _writer.get(); }

    protected:
        osg::ref_ptr<GpuResourceReaderBase> _reader;
        osg::ref_ptr<GpuResourceWriterBase> _writer;
    };
}

#endif

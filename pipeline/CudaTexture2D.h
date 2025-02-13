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

    class CudaResourceReaderBase : public osg::Texture2D::SubloadCallback
    {
    public:
        CudaResourceReaderBase(CUcontext cu);
        virtual void operator()(osg::StateAttribute* sa, osg::NodeVisitor* nv) {}

        virtual bool openResource(const std::string& name) = 0;
        virtual void closeResource(const std::string& name) {}

        virtual void releaseGLObjects(osg::State* state = NULL) const;
        virtual void releaseCuda();

        virtual osg::ref_ptr<osg::Texture::TextureObject> generateTextureObject(
            const osg::Texture2D& texture, osg::State& state) const;
        virtual void load(const osg::Texture2D& texture, osg::State& state) const;
        virtual void subload(const osg::Texture2D& texture, osg::State& state) const;

        enum ResourceState { INVALID = 0, PLAYING, STOPPED };
        void setState(ResourceState s) { _state = s; }
        ResourceState getState() const { return _state; }

    protected:
        virtual ~CudaResourceReaderBase() {}
        bool getDeviceFrameBuffer(CUdeviceptr* devFrameOut, int* pitchOut);

        CUcontext _cuContext;
        mutable CUdeviceptr _deviceFrame;
        ResourceState _state;
        int _width, _height;
        mutable GLuint _pbo, _textureID;
        mutable bool _vendorStatus;
        mutable std::mutex _mutex;
    };

    class CudaResourceReaderContainer : public osg::Object
    {
    public:
        CudaResourceReaderContainer() {}
        CudaResourceReaderContainer(const CudaResourceReaderContainer& copy,
                                    const osg::CopyOp& op = osg::CopyOp::SHALLOW_COPY)
        : osg::Object(copy, op), _reader(copy._reader) {}
        META_Object(osgVerse, CudaResourceReaderContainer)

        void setResourceReader(CudaResourceReaderBase* r) { _reader = r; }
        CudaResourceReaderBase* getResourceReader() { return _reader.get(); }
        const CudaResourceReaderBase* getResourceReader() const { return _reader.get(); }

    protected:
        osg::ref_ptr<CudaResourceReaderBase> _reader;
    };
}

#endif

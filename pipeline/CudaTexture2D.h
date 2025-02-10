#ifndef MANA_PP_CUDATEXTURE2D_HPP
#define MANA_PP_CUDATEXTURE2D_HPP

#include <osg/State>
#include <osg/Texture2D>
#include <cuda.h>
#include <mutex>

namespace osgVerse
{
    class CudaTexture2D : public osg::Texture2D
    {
    public:
        CudaTexture2D(void* cuContext);
        CudaTexture2D(const CudaTexture2D& copy, osg::CopyOp op = osg::CopyOp::SHALLOW_COPY);

        virtual void releaseGLObjects(osg::State* state = NULL) const;
        void releaseCudaData();

    protected:
        void* _cuContext;
    };

    class CuvidManageCallback : public osg::Texture2D::SubloadCallback
    {
    public:
        CuvidManageCallback(CudaTexture2D* tex, CUcontext cu);

        virtual void releaseGLObjects(osg::State* state = NULL) const;
        virtual void releaseCuda();

        virtual osg::ref_ptr<osg::Texture::TextureObject> generateTextureObject(
            const osg::Texture2D& texture, osg::State& state) const;
        virtual void load(const osg::Texture2D& texture, osg::State& state) const;
        virtual void subload(const osg::Texture2D& texture, osg::State& state) const;

    protected:
        virtual ~CuvidManageCallback() {}
        bool getDeviceFrameBuffer(CUdeviceptr* devFrameOut, int* pitchOut);

        osg::observer_ptr<CudaTexture2D> _owner;
        CUcontext _cuContext;
        mutable CUdeviceptr _deviceFrame;
        int _width, _height;
        mutable GLuint _pbo, _textureID;
        mutable bool _vendorStatus;
        mutable std::mutex _mutex;
    };
}

#endif

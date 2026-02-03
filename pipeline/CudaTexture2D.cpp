#include <osg/Version>
#include <osg/io_utils>
#include <osg/GLExtensions>
#include <osg/FrameBufferObject>
#include "CudaTexture2D.h"

#if defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE) || defined(OSG_GL3_AVAILABLE)
#   define __GL_H__  // don't include GL/gl.h
#endif

#ifdef VERSE_ENABLE_MTT
#   include <musa.h>
#   include <musaGL.h>
#else
#   include <cuda.h>
#   include <cudaGL.h>
#endif
using namespace osgVerse;

inline bool check(int e, int iLine, const char* szFile)
{
    if (e < 0)
    {
        OSG_WARN << "General error " << e << " at line " << iLine << " in file " << szFile;
        return false;
    }
    return true;
}
#define ck(call) check(call, __LINE__, __FILE__)

CudaResourceWriterBase::CudaResourceWriterBase(CUcontext cu)
:   osg::Camera::DrawCallback(), _muxerParent(NULL)
{ _cuContext = (CUcontext)cu; }

bool CudaResourceWriterBase::openResource(CudaResourceDemuxerMuxerContainer* c)
{
    _muxerParent = NULL; if (!c) return false;
    if (c->getMuxer()) return openResource(c->getMuxer());
    else { _muxerParent = c; return true; }
}

CudaResourceReaderBase::CudaResourceReaderBase(CUcontext cu)
:   osg::Texture2D::SubloadCallback(), _cuResource(NULL), _deviceFrame(NULL), _state(INVALID),
    _width(0), _height(0), _pbo(0), _textureID(0), _vendorStatus(false)
{ _cuContext = (CUcontext)cu; }

bool CudaResourceReaderBase::openResource(CudaResourceDemuxerMuxerContainer* c)
{ return (c && c->getDemuxer()) ? openResource(c->getDemuxer()) : false; }

void CudaResourceReaderBase::releaseCuda()
{
    _mutex.lock();
#ifdef VERSE_ENABLE_MTT
    ck(muMemFree(_deviceFrame));
#else
    ck(cuMemFree(_deviceFrame));
#endif
    _pbo = 0; _demuxer = NULL;
    _mutex.unlock();
}

void CudaResourceReaderBase::releaseGLObjects(osg::State* state) const
{
    if (!state) { _pbo = 0; return; }
#if OSG_VERSION_GREATER_THAN(3, 3, 2)
    osg::GLExtensions* ext = state->get<osg::GLExtensions>();
#else
    osg::GLBufferObject::Extensions* ext = osg::GLBufferObject::getExtensions(state->getContextID(), true);
#endif

    if (_cuResource != NULL)
    {
#ifdef VERSE_ENABLE_MTT
        ck(muGraphicsUnregisterResource(_cuResource));
#else
        ck(cuGraphicsUnregisterResource(_cuResource));
#endif
    }
    if (ext) ext->glDeleteBuffers(1, &_pbo);
    _cuResource = 0; _pbo = 0;
}

#if OSG_VERSION_GREATER_THAN(3, 4, 0)
osg::ref_ptr<osg::Texture::TextureObject> CudaResourceReaderBase::generateTextureObject(
            const osg::Texture2D& texture, osg::State& state) const
#else
osg::Texture::TextureObject* CudaResourceReaderBase::generateTextureObject(
            const osg::Texture2D& texture, osg::State& state) const
#endif
{
    osg::ref_ptr<osg::Texture::TextureObject> obj =
        osg::Texture::generateTextureObject(&texture, state.getContextID(), GL_TEXTURE_2D);
    _textureID = obj->id(); return obj.get();
}

void CudaResourceReaderBase::load(const osg::Texture2D& texture, osg::State& state) const
{
    char* vendor = (char*)glGetString(GL_VENDOR);
    if (std::string(vendor).find("NVIDIA") != std::string::npos) _vendorStatus = true;
    else { _vendorStatus = false; return; }

#if OSG_VERSION_GREATER_THAN(3, 3, 2)
    osg::GLExtensions* ext = state.get<osg::GLExtensions>();
#else
    osg::GLBufferObject::Extensions* ext = osg::GLBufferObject::getExtensions(state.getContextID(), true);
#endif
    if (_width == 0 || _height == 0 || !ext) return;
    if (_pbo != 0) ext->glDeleteBuffers(1, &_pbo);

    ext->glGenBuffers(1, &_pbo);
    ext->glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, _pbo);
    ext->glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, _width * _height * 4, NULL, GL_STREAM_DRAW_ARB);
    ext->glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

    glBindTexture(GL_TEXTURE_2D, _textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

#ifdef VERSE_ENABLE_MTT
    ck(muCtxSetCurrent(_cuContext));
    ck(muMemAlloc(&_deviceFrame, _width * _height * 4));
    ck(muMemsetD8(_deviceFrame, 0, _width * _height * 4));
#else
    ck(cuCtxSetCurrent(_cuContext));
    ck(cuMemAlloc(&_deviceFrame, _width * _height * 4));
    ck(cuMemsetD8(_deviceFrame, 0, _width * _height * 4));
#endif
}

void CudaResourceReaderBase::subload(const osg::Texture2D& texture, osg::State& state) const
{
    if (_width == 0 || _height == 0) return;
    if (_pbo == 0) { load(texture, state); if (_pbo == 0) return; }
    _mutex.lock();

    CUdeviceptr devBackBuffer; size_t size = 0;
#ifdef VERSE_ENABLE_MTT
    if (!_cuResource) ck(muGraphicsGLRegisterBuffer(&_cuResource, _pbo, MU_GRAPHICS_REGISTER_FLAGS_WRITE_DISCARD));
    ck(muGraphicsMapResources(1, &_cuResource, 0));
    ck(muGraphicsResourceGetMappedPointer(&devBackBuffer, &size, _cuResource));
    ck(muMemcpyAsync(devBackBuffer, _deviceFrame, size, 0));
    ck(muGraphicsUnmapResources(1, &_cuResource, 0));
#else
    if (!_cuResource) ck(cuGraphicsGLRegisterBuffer(&_cuResource, _pbo, CU_GRAPHICS_REGISTER_FLAGS_WRITE_DISCARD));
    ck(cuGraphicsMapResources(1, &_cuResource, 0));
    ck(cuGraphicsResourceGetMappedPointer(&devBackBuffer, &size, _cuResource));
    ck(cuMemcpyAsync(devBackBuffer, _deviceFrame, size, 0));
    ck(cuGraphicsUnmapResources(1, &_cuResource, 0));
#endif
    _mutex.unlock();

#if OSG_VERSION_GREATER_THAN(3, 3, 2)
    osg::GLExtensions* ext = state.get<osg::GLExtensions>();
#else
    osg::GLBufferObject::Extensions* ext = osg::GLBufferObject::getExtensions(state.getContextID(), true);
#endif
    if (ext) ext->glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, _pbo);
    glBindTexture(GL_TEXTURE_2D, _textureID);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _width, _height, GL_BGRA, GL_UNSIGNED_BYTE, 0);
    if (ext) ext->glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
}

bool CudaResourceReaderBase::getDeviceFrameBuffer(CUdeviceptr* devFrameOut, int* pitchOut)
{
    if (!_deviceFrame) return false;
    *devFrameOut = (CUdeviceptr)_deviceFrame;
    *pitchOut = _width * 4;
    return true;
}

class CudaResourceUpdateCallback : public osg::StateAttribute::Callback
{
public:
    CudaResourceUpdateCallback(CudaResourceReaderBase* cb) : _manager(cb) {}

    virtual void operator()(osg::StateAttribute* sa, osg::NodeVisitor* nv)
    { if (_manager.valid()) (*_manager)(sa, nv); }

protected:
    osg::observer_ptr<CudaResourceReaderBase> _manager;
};

CudaTexture2D::CudaTexture2D(void* cu) : osg::Texture2D(), _cuContext(cu)
{}

CudaTexture2D::CudaTexture2D(const CudaTexture2D& copy, const osg::CopyOp& op)
:   osg::Texture2D(copy, op), _cuContext(copy._cuContext)
{}

void CudaTexture2D::setResourceReader(CudaResourceReaderBase* reader)
{
    CudaResourceUpdateCallback* cb = NULL;
    if (reader) cb = new CudaResourceUpdateCallback(reader);
    setSubloadCallback(reader); setUpdateCallback(cb);
}

const CudaResourceReaderBase* CudaTexture2D::getResourceReader() const
{ return dynamic_cast<const CudaResourceReaderBase*>(getSubloadCallback()); }

void CudaTexture2D::releaseGLObjects(osg::State* state) const
{
    const CudaResourceReaderBase* callback = static_cast<const CudaResourceReaderBase*>(getSubloadCallback());
    if (callback) callback->releaseGLObjects(state);
    osg::Texture2D::releaseGLObjects(state);
}

void CudaTexture2D::releaseCudaData()
{
    CudaResourceReaderBase* callback = static_cast<CudaResourceReaderBase*>(getSubloadCallback());
    if (callback) callback->releaseCuda();
}

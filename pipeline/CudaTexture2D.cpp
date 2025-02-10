#include <osg/Version>
#include <osg/io_utils>
#include <osg/GLExtensions>

#include "CudaTexture2D.h"
#include <cudaGL.h>
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

CuvidManageCallback::CuvidManageCallback(CudaTexture2D* tex, CUcontext cu)
:   osg::Texture2D::SubloadCallback(), _owner(tex), _deviceFrame(NULL),
    _width(0), _height(0), _pbo(0), _textureID(0), _vendorStatus(false)
{ _cuContext = (CUcontext)cu; }

void CuvidManageCallback::releaseCuda()
{
    _mutex.lock();
    ck(cuMemFree(_deviceFrame)); _pbo = 0;
    _mutex.unlock();
}

void CuvidManageCallback::releaseGLObjects(osg::State* state) const
{
    if (!state) { _pbo = 0; return; }
#if OSG_VERSION_GREATER_THAN(3, 3, 2)
    osg::GLExtensions* ext = state->get<osg::GLExtensions>();
#else
    osg::FBOExtensions* ext = osg::FBOExtensions::instance(state->getContextID(), true);
#endif
    if (ext) ext->glDeleteBuffers(1, &_pbo); _pbo = 0;
}

osg::ref_ptr<osg::Texture::TextureObject> CuvidManageCallback::generateTextureObject(
            const osg::Texture2D& texture, osg::State& state) const
{
    osg::ref_ptr<osg::Texture::TextureObject> obj =
        osg::Texture::generateTextureObject(&texture, state.getContextID(), GL_TEXTURE_2D);
    _textureID = obj->id(); return obj;
}

void CuvidManageCallback::load(const osg::Texture2D& texture, osg::State& state) const
{
    char* vendor = (char*)glGetString(GL_VENDOR);
    if (std::string(vendor).find("NVIDIA") != std::string::npos) _vendorStatus = true;
    else { _vendorStatus = false; return; }

#if OSG_VERSION_GREATER_THAN(3, 3, 2)
    osg::GLExtensions* ext = state.get<osg::GLExtensions>();
#else
    osg::FBOExtensions* ext = osg::FBOExtensions::instance(state.getContextID(), true);
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

    ck(cuCtxSetCurrent(_cuContext));
    ck(cuMemAlloc(&_deviceFrame, _width * _height * 4));
    ck(cuMemsetD8(_deviceFrame, 0, _width * _height * 4));
}

void CuvidManageCallback::subload(const osg::Texture2D& texture, osg::State& state) const
{
    if (_width == 0 || _height == 0 || _pbo == 0) return;
    _mutex.lock();

    CUgraphicsResource cuResource;
    ck(cuGraphicsGLRegisterBuffer(&cuResource, _pbo, CU_GRAPHICS_REGISTER_FLAGS_WRITE_DISCARD));
    ck(cuGraphicsMapResources(1, &cuResource, 0));

    CUdeviceptr devBackBuffer; size_t size = 0;
    ck(cuGraphicsResourceGetMappedPointer(&devBackBuffer, &size, cuResource));

    CUDA_MEMCPY2D m = { 0 };
    m.srcMemoryType = CU_MEMORYTYPE_DEVICE;
    m.srcDevice = _deviceFrame; m.srcPitch = _width * 4;
    m.dstMemoryType = CU_MEMORYTYPE_DEVICE;
    m.dstDevice = devBackBuffer; m.dstPitch = size / _height;
    m.WidthInBytes = _width * 4; m.Height = _height;
    ck(cuMemcpy2DAsync(&m, 0));

    ck(cuGraphicsUnmapResources(1, &cuResource, 0));
    ck(cuGraphicsUnregisterResource(cuResource));
    _mutex.unlock();

#if OSG_VERSION_GREATER_THAN(3, 3, 2)
    osg::GLExtensions* ext = state.get<osg::GLExtensions>();
#else
    osg::FBOExtensions* ext = osg::FBOExtensions::instance(state.getContextID(), true);
#endif
    if (ext) ext->glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, _pbo);
    glBindTexture(GL_TEXTURE_2D, _textureID);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _width, _height, GL_BGRA, GL_UNSIGNED_BYTE, 0);
    if (ext) ext->glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
}

bool CuvidManageCallback::getDeviceFrameBuffer(CUdeviceptr* devFrameOut, int* pitchOut)
{
    if (!_deviceFrame) return false;
    *devFrameOut = (CUdeviceptr)_deviceFrame;
    *pitchOut = _width * 4;
    return true;
}

CudaTexture2D::CudaTexture2D(void* cu)
:   osg::Texture2D(), _cuContext(cu)
{
    CuvidManageCallback* callback = new CuvidManageCallback(this, (CUcontext)_cuContext);
    setSubloadCallback(callback);
}

CudaTexture2D::CudaTexture2D(const CudaTexture2D& copy, osg::CopyOp op)
:   osg::Texture2D(copy, op), _cuContext(copy._cuContext)
{}

void CudaTexture2D::releaseGLObjects(osg::State* state) const
{
    const CuvidManageCallback* callback = static_cast<const CuvidManageCallback*>(getSubloadCallback());
    if (callback) callback->releaseGLObjects(state);
    osg::Texture2D::releaseGLObjects(state);
}

void CudaTexture2D::releaseCudaData()
{
    CuvidManageCallback* callback = static_cast<CuvidManageCallback*>(getSubloadCallback());
    if (callback) callback->releaseCuda();
}

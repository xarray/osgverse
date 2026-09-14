#ifdef VERSE_WITH_GBM
#   include <drm/drm.h>
#   include <drm/drm_fourcc.h>
#   include <sys/mman.h>
#   include <sys/ioctl.h>
#   include <linux/dma-buf.h>
#   include <linux/dma-heap.h>
#   include <fcntl.h>
#   include <unistd.h>

#   define EGL_EGLEXT_PROTOTYPES
#   include <EGL/egl.h>
#endif

#include <iostream>
#include <osg/Version>
#include <osg/io_utils>
#include <osg/GLExtensions>
#include <osg/FrameBufferObject>
#include "ExternalTexture2D.h"

#ifndef EGL_VERSION_1_0
#   define EGL_WIDTH                         0x3057
#   define EGL_HEIGHT                        0x3056
#   define EGL_NONE                          0x3038
#endif

#ifndef EGL_KHR_image_base
#   define EGL_IMAGE_PRESERVED_KHR           0x30D2
#endif

#ifndef EGL_EXT_image_dma_buf_import
#   define EGL_LINUX_DMA_BUF_EXT             0x3270
#   define EGL_LINUX_DRM_FOURCC_EXT          0x3271
#   define EGL_DMA_BUF_PLANE0_FD_EXT         0x3272
#   define EGL_DMA_BUF_PLANE0_OFFSET_EXT     0x3273
#   define EGL_DMA_BUF_PLANE0_PITCH_EXT      0x3274
#   define EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT 0x3443
#   define EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT 0x3444
#endif

#if defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE) || defined(OSG_GL3_AVAILABLE)
#   define __GL_H__  // don't include GL/gl.h
#endif

#ifdef VERSE_WITH_CUDA
#  ifdef VERSE_ENABLE_MTT
#   include <musa.h>
#   include <musaGL.h>
#  else
#   include <cuda.h>
#   include <cudaGL.h>
#  endif
#endif

using namespace osgVerse;
typedef void (GL_APIENTRY* PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)(GLenum target, void* image);
#define TEST_GBM_EGL_CLIENT 0

namespace
{
    inline bool check(int e, int iLine, const char* szFile)
    {
        if (e < 0)
        {
            OSG_WARN << "General error " << e << " at line " << iLine << " in file " << szFile;
            return false;
        }
        return true;
    }

    static std::string fourccToString(uint32_t fourcc)
    {
        std::string result; result.resize(4);
        result[0] = static_cast<char>(fourcc & 0xFF);
        result[1] = static_cast<char>((fourcc >> 8) & 0xFF);
        result[2] = static_cast<char>((fourcc >> 16) & 0xFF);
        result[3] = static_cast<char>((fourcc >> 24) & 0xFF);
        return result;
    }
}
#define ck(call) check(call, __LINE__, __FILE__)

#if defined(VERSE_WITH_GBM) && TEST_GBM_EGL_CLIENT
// Copied from https://gitlab.com/blaztinn/dma-buf-texture-sharing
// See article https://blaztinn.gitlab.io/post/dmabuf-texture-sharing for details
#  include <sys/socket.h>
#  include <sys/un.h>
struct texture_storage_metadata_t
{
    int fourcc;
    uint64_t modifiers;
    EGLint stride;
    EGLint offset;
};

int create_socket(const char *path)
{
    int sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, path);
    unlink(path);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) exit(-1);
    return sock;
}

void read_fd(int sock, int *fd, void *data, size_t data_len)
{
    struct msghdr msg = {0};
    struct iovec io = {.iov_base = data, .iov_len = data_len};
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;

    char c_buffer[256];
    msg.msg_control = c_buffer;
    msg.msg_controllen = sizeof(c_buffer);
    if (recvmsg(sock, &msg, 0) < 0) exit(-1);

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    memmove(fd, CMSG_DATA(cmsg), sizeof(fd));
}

static void createTestImageFromSocket(GpuResourceReaderBase::EglResourceHandle* H)
{
    // Copied from https://gitlab.com/blaztinn/dma-buf-texture-sharing
    // See article https://blaztinn.gitlab.io/post/dmabuf-texture-sharing for details
    int texture_dmabuf_fd = 0;
    struct texture_storage_metadata_t metadata;
    const char* CLIENT_FILE = "/tmp/test_client";

    int sock = create_socket(CLIENT_FILE);
    read_fd(sock, &texture_dmabuf_fd, &metadata, sizeof(texture_storage_metadata)); close(sock);

    unsigned int modL = (uint32_t)(metadata.modifiers & ((((uint64_t)1) << 33) - 1));
    unsigned int modH = (uint32_t)((metadata.modifiers >> 32) & ((((uint64_t)1) << 33) - 1));
    H->createImage(texture_dmabuf_fd, 256, 256, DRM_FORMAT_ARGB8888, metadata.offset,
                   metadata.stride, modL, modH); close(texture_dmabuf_fd);
}
#endif

void GpuResourceReaderBase::EglResourceHandle::createImage(int dmabuf_fd, int w, int h, int fourcc, int offset,
                                                           int stride, unsigned int modifiersL, unsigned int modifiersH)
{
#if defined(VERSE_WITH_GBM)
    EGLAttrib const attr[] = {
        EGL_WIDTH, w, EGL_HEIGHT, h,
        EGL_LINUX_DRM_FOURCC_EXT, fourcc,
        EGL_DMA_BUF_PLANE0_FD_EXT, dmabuf_fd,
        EGL_DMA_BUF_PLANE0_OFFSET_EXT, offset,
        EGL_DMA_BUF_PLANE0_PITCH_EXT, stride,
        EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, modifiersL,
        EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, modifiersH, EGL_NONE };
    if (image != NULL && display != NULL) eglDestroyImage(display, image);
    image = eglCreateImage(display, NULL, EGL_LINUX_DMA_BUF_EXT, NULL, attr); dirty = true;
    if (!image)
    {
        OSG_WARN << "[GpuResourceReaderBase] Failed to create EGL image for DRM buffer, video-fourcc = "
                 << std::hex << fourcc << std::dec << " (" << fourccToString(fourcc) << ")" << std::endl;
        
        EGLAttrib const attr2[] = {
            EGL_WIDTH, w, EGL_HEIGHT, h,
            EGL_LINUX_DRM_FOURCC_EXT, DRM_FORMAT_R8,
            EGL_DMA_BUF_PLANE0_FD_EXT, dmabuf_fd,
            EGL_DMA_BUF_PLANE0_OFFSET_EXT, offset,
            EGL_DMA_BUF_PLANE0_PITCH_EXT, stride,
            EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, modifiersL,
            EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, modifiersH, EGL_NONE };
        image = eglCreateImage(display, NULL, EGL_LINUX_DMA_BUF_EXT, NULL, attr2);  // try again?
        if (image != NULL) { OSG_WARN << "[GpuResourceReaderBase] R8 is used as a temporary result..." << std::endl; }
    }
#endif
}

GpuResourceWriterBase::GpuResourceWriterBase(CUcontext cu)
:   osg::Camera::DrawCallback(), _muxerParent(NULL)
{ _cuContext = (CUcontext)cu; }

bool GpuResourceWriterBase::openResource(GpuResourceDemuxerMuxerContainer* c)
{
    _muxerParent = NULL; if (!c) return false;
    if (c->getMuxer()) return openResource(c->getMuxer());
    else { _muxerParent = c; return true; }
}

GpuResourceReaderBase::GpuResourceReaderBase(CUcontext cu)
:   osg::Texture2D::SubloadCallback(), _textureID(0), _state(INVALID), _width(0), _height(0), _vendorStatus(false)
{ _handle = new CudaResourceHandle(cu); _resourceType = RES_CUDA; _pixelFormat = GL_BGRA; }

GpuResourceReaderBase::GpuResourceReaderBase(void* eglDisplay)
:   osg::Texture2D::SubloadCallback(), _textureID(0), _state(INVALID), _width(0), _height(0), _vendorStatus(false)
{ _handle = new EglResourceHandle(eglDisplay, NULL); _resourceType = RES_EGL; _pixelFormat = GL_BGRA; }

bool GpuResourceReaderBase::openResource(GpuResourceDemuxerMuxerContainer* c)
{ return (c && c->getDemuxer()) ? openResource(c->getDemuxer()) : false; }

void GpuResourceReaderBase::releaseGpu()
{
    if (_resourceType == RES_CUDA)
    {
        CudaResourceHandle* H = static_cast<CudaResourceHandle*>(_handle.get());
#ifdef VERSE_WITH_CUDA
        if (H->cuResource != NULL)
        {
#  ifdef VERSE_ENABLE_MTT
            ck(muGraphicsUnregisterResource(H->cuResource));
#  else
            ck(cuGraphicsUnregisterResource(H->cuResource));
#  endif
        }
#  ifdef VERSE_ENABLE_MTT
        _mutex.lock(); ck(muMemFree(H->deviceFrame)); _mutex.unlock();
#  else
        _mutex.lock(); ck(cuMemFree(H->deviceFrame)); _mutex.unlock();
#  endif
#endif
        H->cuResource = NULL;
    }
    else if (_resourceType == RES_EGL)
    {
        EglResourceHandle* H = static_cast<EglResourceHandle*>(_handle.get());
#ifdef VERSE_WITH_GBM
        if (H->image != NULL && H->display != NULL) eglDestroyImage(H->display, H->image);
#endif
        H->image = NULL;
    }
    _demuxer = NULL;
}

void GpuResourceReaderBase::releaseGLObjects(osg::State* state) const
{
#if OSG_VERSION_GREATER_THAN(3, 3, 2)
    osg::GLExtensions* ext = state->get<osg::GLExtensions>();
#else
    osg::GLBufferObject::Extensions* ext = osg::GLBufferObject::getExtensions(state->getContextID(), true);
#endif
    if (_resourceType == RES_CUDA)
    {
        CudaResourceHandle* H = static_cast<CudaResourceHandle*>(_handle.get());
        if (H && H->pbo != 0) { ext->glDeleteBuffers(1, &(H->pbo)); H->pbo = 0; }
    }
    else if (_resourceType == RES_EGL)
    {
        // nothing to do
    }
}

#if OSG_VERSION_GREATER_THAN(3, 4, 0)
osg::ref_ptr<osg::Texture::TextureObject> GpuResourceReaderBase::generateTextureObject(
            const osg::Texture2D& texture, osg::State& state) const
#else
osg::Texture::TextureObject* GpuResourceReaderBase::generateTextureObject(
            const osg::Texture2D& texture, osg::State& state) const
#endif
{
    osg::ref_ptr<osg::Texture::TextureObject> obj =
        osg::Texture::generateTextureObject(&texture, state.getContextID(), GL_TEXTURE_2D);
    _textureID = obj->id(); return obj.get();
}

void GpuResourceReaderBase::setDefaultTestImage(osg::Image* image)
{
    if (!image) return;
    if (image->getPixelFormat() == GL_RGBA || image->getPixelFormat() == GL_BGRA)
    {
        if (!image->valid()) return;
        if (image->getDataType() == GL_UNSIGNED_BYTE)
        {
            _testImage = image; _pixelFormat = image->getPixelFormat();
            _width = image->s(); _height = image->t();
        }
    }
}

void GpuResourceReaderBase::load(const osg::Texture2D& texture, osg::State& state) const
{
    char* vendor = (char*)glGetString(GL_VENDOR);
    if (std::string(vendor).find("NVIDIA") != std::string::npos) _vendorStatus = true;
    //else { _vendorStatus = false; return; }

#if OSG_VERSION_GREATER_THAN(3, 3, 2)
    osg::GLExtensions* ext = state.get<osg::GLExtensions>();
#else
    osg::GLBufferObject::Extensions* ext = osg::GLBufferObject::getExtensions(state.getContextID(), true);
#endif
    if (_width == 0 || _height == 0 || !ext)
    { OSG_WARN << "[GpuResourceReaderBase] No valid dimension for input texture" << std::endl; return; }

    unsigned int imageSize = _width * _height * 4;
    if (_resourceType == RES_CUDA)
    {
        CudaResourceHandle* H = static_cast<CudaResourceHandle*>(_handle.get());
        if (H->pbo != 0) ext->glDeleteBuffers(1, &(H->pbo));

        ext->glGenBuffers(1, &(H->pbo));
        ext->glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, H->pbo);
        ext->glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, imageSize, NULL, GL_STREAM_DRAW_ARB);
        ext->glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

#ifdef VERSE_WITH_CUDA
#  ifdef VERSE_ENABLE_MTT
        ck(muCtxSetCurrent(H->cuContext));
        ck(muMemAlloc(&(H->deviceFrame), imageSize));
        if (_testImage.valid() && _testImage->getTotalSizeInBytes() == imageSize)
            ck(muMemcpyHtoD(H->deviceFrame, _testImage->data(), imageSize));
        else
            ck(muMemsetD8(H->deviceFrame, 0, imageSize));
#  else
        ck(cuCtxSetCurrent(H->cuContext));
        ck(cuMemAlloc(&(H->deviceFrame), imageSize));
        if (_testImage.valid() && _testImage->getTotalSizeInBytes() == imageSize)
            ck(cuMemcpyHtoD(H->deviceFrame, _testImage->data(), imageSize));
        else
            ck(cuMemsetD8(H->deviceFrame, 0, imageSize));
#  endif
#else
        OSG_FATAL << "[GpuResourceReaderBase] No CUDA/MUSA dependencies for use" << std::endl;
#endif
    }
    else if (_resourceType == RES_EGL)
    {
        EglResourceHandle* H = static_cast<EglResourceHandle*>(_handle.get());
#ifdef VERSE_WITH_GBM
#  if TEST_GBM_EGL_CLIENT
        if (_testImage.valid()) createTestImageFromSocket(H);  // tested with external server, not given image
#  endif
        H->glEGLImageTargetTexture2DOES = (void*)eglGetProcAddress("glEGLImageTargetTexture2DOES");
#else
        OSG_FATAL << "[GpuResourceReaderBase] No GBM/DRM dependencies for use" << std::endl;
#endif
    }

    if (_resourceType != RES_EGL)
    {
        glBindTexture(GL_TEXTURE_2D, _textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void GpuResourceReaderBase::subload(const osg::Texture2D& texture, osg::State& state) const
{
#if OSG_VERSION_GREATER_THAN(3, 3, 2)
    osg::GLExtensions* ext = state.get<osg::GLExtensions>();
#else
    osg::GLBufferObject::Extensions* ext = osg::GLBufferObject::getExtensions(state.getContextID(), true);
#endif
    if (_resourceType == RES_CUDA)
    {
        CudaResourceHandle* H = static_cast<CudaResourceHandle*>(_handle.get());
        if (H->pbo == 0) { load(texture, state); if (H->pbo == 0) return; }
        if (_width == 0 || _height == 0) return;
        _mutex.lock();

#ifdef VERSE_WITH_CUDA
        CUdeviceptr devBackBuffer; size_t size = 0;
#  ifdef VERSE_ENABLE_MTT
        if (!H->cuResource) ck(muGraphicsGLRegisterBuffer(&(H->cuResource), H->pbo, MU_GRAPHICS_REGISTER_FLAGS_WRITE_DISCARD));
        ck(muGraphicsMapResources(1, &(H->cuResource), 0));
        ck(muGraphicsResourceGetMappedPointer(&devBackBuffer, &size, H->cuResource));
        ck(muMemcpyAsync(devBackBuffer, H->deviceFrame, size, 0));
        ck(muGraphicsUnmapResources(1, &(H->cuResource), 0));
#  else
        if (!H->cuResource) ck(cuGraphicsGLRegisterBuffer(&(H->cuResource), H->pbo, CU_GRAPHICS_REGISTER_FLAGS_WRITE_DISCARD));
        ck(cuGraphicsMapResources(1, &(H->cuResource), 0));
        ck(cuGraphicsResourceGetMappedPointer(&devBackBuffer, &size, H->cuResource));
        ck(cuMemcpyAsync(devBackBuffer, H->deviceFrame, size, 0));
        ck(cuGraphicsUnmapResources(1, &(H->cuResource), 0));
#  endif
#endif
        _mutex.unlock();

        if (ext) ext->glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, H->pbo);
        glBindTexture(GL_TEXTURE_2D, _textureID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _width, _height, _pixelFormat, GL_UNSIGNED_BYTE, 0);
        if (ext) ext->glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    }
    else if (_resourceType == RES_EGL)
    {
        EglResourceHandle* H = static_cast<EglResourceHandle*>(_handle.get());
        if (H->dirty && H->image)
        {
            if (!H->glEGLImageTargetTexture2DOES)
                { OSG_WARN << "[GpuResourceReaderBase] glEGLImageTargetTexture2DOES not found\n"; }
            else
            {
                PFNGLEGLIMAGETARGETTEXTURE2DOESPROC func = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)H->glEGLImageTargetTexture2DOES;
                glBindTexture(GL_TEXTURE_2D, _textureID); func(GL_TEXTURE_2D, H->image);
            }
            H->dirty = false;
        }
    }
}

bool GpuResourceReaderBase::getDeviceFrameBuffer(CUdeviceptr* devFrameOut, int* pitchOut)
{
    if (_resourceType == RES_CUDA)
    {
        CudaResourceHandle* H = static_cast<CudaResourceHandle*>(_handle.get());
        if (H->deviceFrame != 0)
        {
            // FIXME: consider use a queue because reader may return multiple data in one frame
            *devFrameOut = (CUdeviceptr)H->deviceFrame;
            *pitchOut = _width * 4; return true;
        }
    }
    return false;
}

bool GpuResourceReaderBase::getDeviceDescriptor(const std::vector<int>& desc, int layers)
{
    if (_resourceType == RES_EGL && desc.size() > 5)
    {
        // Get DRM buf attributes and create EGL image
        EglResourceHandle* H = static_cast<EglResourceHandle*>(_handle.get());
#ifdef VERSE_WITH_GBM
        if (H->image != NULL && H->display != NULL) eglDestroyImage(H->display, H->image);
#endif
        H->createImage(desc[0], _width, _height, desc[1], desc[2],
                       desc[3], desc[4], desc[5]); return true;
    }
    return false;
}

namespace
{
    class ResourceUpdateCallback : public osg::StateAttribute::Callback
    {
    public:
        ResourceUpdateCallback(GpuResourceReaderBase* cb)
            : _manager(cb), _lastTick(0) {}

        virtual void operator()(osg::StateAttribute* sa, osg::NodeVisitor* nv)
        {
            osg::Timer_t now = osg::Timer::instance()->tick();
            if (_manager.valid())
            {
                double fps = _manager->getDemuxer() ? _manager->getDemuxer()->getFrameRate() : -1.0;
                if (fps >= 0.0)
                {
                    double step = fps > 1.0 ? (1000.0 / fps) : 50.0;  // target msecs between two frames
                    double sec = osg::Timer::instance()->delta_m(_lastTick, now);
                    if (step <= sec) { (*_manager)(sa, nv); _lastTick = now; }
                }
                else  // stream mode: fps = -1
                    (*_manager)(sa, nv);
            }
        }

    protected:
        osg::observer_ptr<GpuResourceReaderBase> _manager;
        osg::Timer_t _lastTick;
    };
}

ExternalTexture2D::ExternalTexture2D() : osg::Texture2D()
{}

ExternalTexture2D::ExternalTexture2D(const ExternalTexture2D& copy, const osg::CopyOp& op)
:   osg::Texture2D(copy, op)
{}

ExternalTexture2D::~ExternalTexture2D()
{
    GpuResourceReaderBase* callback = static_cast<GpuResourceReaderBase*>(getSubloadCallback());
    if (callback) callback->releaseGpu();
}

void ExternalTexture2D::setResourceReader(GpuResourceReaderBase* reader)
{
    ResourceUpdateCallback* cb = NULL;
    if (reader) cb = new ResourceUpdateCallback(reader);
    setSubloadCallback(reader); setUpdateCallback(cb);
}

const GpuResourceReaderBase* ExternalTexture2D::getResourceReader() const
{ return dynamic_cast<const GpuResourceReaderBase*>(getSubloadCallback()); }

void ExternalTexture2D::releaseGLObjects(osg::State* state) const
{
    const GpuResourceReaderBase* callback = static_cast<const GpuResourceReaderBase*>(getSubloadCallback());
    if (callback) callback->releaseGLObjects(state);
    osg::Texture2D::releaseGLObjects(state);
}

void ExternalTexture2D::releaseGpuData()
{
    GpuResourceReaderBase* callback = static_cast<GpuResourceReaderBase*>(getSubloadCallback());
    if (callback) callback->releaseGpu();
}

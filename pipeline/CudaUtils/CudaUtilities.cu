#if !defined(VERSE_EMBEDDED_GLES2) && !defined(VERSE_EMBEDDED_GLES3)
#   include "3rdparty/GL/glew.h"
#endif

#include <osg/Version>
#include <osg/io_utils>
#include <osg/GL>
#include <iostream>

#include <thrust/sort.h>
#include <thrust/device_vector.h>
#include <thrust/execution_policy.h>
#include <cuda.h>
#include <cudaGL.h>

#define ONLY_CUDA_DEFINITIONS
#include "../Utilities.h"
using namespace osgVerse;

CudaAlgorithm::TextureResource::TextureResource(osg::Texture* tex, int contextID, bool copyFromTexture)
:   texture(tex), resource(NULL), pbo(0)
{
    int w = tex->getTextureWidth(), h = tex->getTextureHeight(); GLenum dtype = GL_NONE;
    if (tex->getImage(0)) { w = tex->getImage(0)->s(); h = tex->getImage(0)->t(); }
    pixelFormat = tex->getImage(0) ? tex->getImage(0)->getPixelFormat() : tex->getSourceFormat();
    dataType = tex->getImage(0) ? tex->getImage(0)->getDataType() : tex->getSourceType();

    int bits = osg::Image::computePixelSizeInBits(pixelFormat, dataType);
    dataSize = w * h * bits / 8; width = w; height = h;
    if (w < 1 || h < 1)
        { OSG_NOTICE << "[TextureResource] Texture size must be greater than 0\n"; }
    else
    {
        glGenBuffers(1, &pbo);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, dataSize, nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        cuGraphicsGLRegisterBuffer(&resource, pbo, cudaGraphicsMapFlagsNone);

        osg::Texture::TextureObject* texObj = texture->getTextureObject(contextID);
        if (texObj && copyFromTexture)
        {
#if !defined(VERSE_EMBEDDED_GLES2) && !defined(VERSE_EMBEDDED_GLES3)
            glBindTexture(GL_TEXTURE_2D, texObj->id());
            glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
            glGetTexImage(GL_TEXTURE_2D, 0, pixelFormat, dataType, 0);
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
#else
            OSG_NOTICE << "[TextureResource] Texture copying not implemented\n";
#endif
        }
    }
}

CudaAlgorithm::TextureResource::~TextureResource()
{
    if (resource != nullptr) cuGraphicsUnregisterResource(resource);
    if (pbo) glDeleteBuffers(1, &pbo);
}

CUdeviceptr CudaAlgorithm::TextureResource::map(size_t& size, int contextID, bool copyFromTexture)
{
    osg::Texture::TextureObject* texObj = texture->getTextureObject(contextID);
    if (resource == 0) return NULL;
    if (copyFromTexture && texObj)
    {
#if !defined(VERSE_EMBEDDED_GLES2) && !defined(VERSE_EMBEDDED_GLES3)
        glBindTexture(GL_TEXTURE_2D, texObj->id());
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
        glGetTexImage(GL_TEXTURE_2D, 0, pixelFormat, dataType, 0);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
#else
        OSG_NOTICE << "[TextureResource] Texture copying not implemented\n";
#endif
    }

    CUdeviceptr devicePtr = NULL;
    cuGraphicsMapResources(1, &resource, 0);
    cuGraphicsResourceGetMappedPointer(&devicePtr, &size, resource);
    return devicePtr;
}

void CudaAlgorithm::TextureResource::unmap(int contextID, bool copyToTexture)
{
    if (resource == 0) return;
    cuGraphicsUnmapResources(1, &resource, 0);

    osg::Texture::TextureObject* texObj = texture->getTextureObject(contextID);
    if (copyToTexture && texObj)
    {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
        glBindTexture(GL_TEXTURE_2D, texObj->id());
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture->getTextureWidth(), texture->getTextureHeight(),
                        pixelFormat, dataType, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }
}

namespace osgVerse
{
    CUcontext CudaAlgorithm::initializeContext(int gpuID)
    {
        CUcontext cuContext = NULL;
        CUdevice cuDevice = 0; char deviceName[80];
        int numGpu = 0; cuInit(0); cuDeviceGetCount(&numGpu);
        if (gpuID < 0 || gpuID >= numGpu) return NULL;

        cuDeviceGet(&cuDevice, gpuID);
        cuDeviceGetName(deviceName, sizeof(deviceName), cuDevice);
    #if CUDA_VERSION >= 13000
        CUctxCreateParams params = {};
        cuCtxCreate(&cuContext, &params, CU_CTX_SCHED_BLOCKING_SYNC, cuDevice);
    #else
        cuCtxCreate(&cuContext, CU_CTX_SCHED_BLOCKING_SYNC, cuDevice);
    #endif
        OSG_NOTICE << "[CudaAlgorithm] GPU in use: " << deviceName << std::endl;
        return cuContext;
    }

    void CudaAlgorithm::deinitializeContext(CUcontext context)
    {
        cuCtxDestroy(context);
    }

    bool CudaAlgorithm::radixSort(const std::vector<unsigned int>& inValues, const std::vector<unsigned int>& inIDs,
                                  std::vector<unsigned int>& outIDs)
    {
        size_t numElements = inValues.size();
        if (numElements > inIDs.size()) return false;
        if (numElements != outIDs.size()) outIDs.resize(numElements);

        try
        {
            thrust::device_vector<unsigned int> d_values(inValues);
            thrust::device_vector<unsigned int> d_ids(inIDs);
            thrust::sort_by_key(thrust::device, d_values.begin(), d_values.end(), d_ids.begin());
            thrust::copy(d_ids.begin(), d_ids.end(), outIDs.begin());
        }
        catch (std::exception& ex)
            { OSG_NOTICE << "[CudaAlgorithm] Failed to sort: " << ex.what() << std::endl; return false; }
        return true;
    }
}

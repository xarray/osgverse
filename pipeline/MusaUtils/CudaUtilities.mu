#include "3rdparty/GL/glew.h"
#include <osg/io_utils>
#include <osg/Version>
#include <osg/GL>
#include <iostream>

#include <thrust/sort.h>
#include <thrust/device_vector.h>
#include <thrust/execution_policy.h>
#include <musa.h>
#include <musaGL.h>

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

    if (w < 1 || h < 1)
        { OSG_NOTICE << "[TextureResource] Texture size must be greater than 0\n"; }
    else
    {
        int bits = osg::Image::computePixelSizeInBits(pixelFormat, dataType);
        size_t dataSize = w * h * bits / 8;
        glGenBuffers(1, &pbo);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, dataSize, nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        muGraphicsGLRegisterBuffer(&resource, pbo, mudaGraphicsMapFlagsNone);

        osg::Texture::TextureObject* texObj = texture->getTextureObject(contextID);
        if (texObj && copyFromTexture)
        {
            glBindTexture(GL_TEXTURE_2D, texObj->id());
            glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
            glGetTexImage(GL_TEXTURE_2D, 0, pixelFormat, dataType, 0);
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
}

CudaAlgorithm::TextureResource::~TextureResource()
{
    if (resource != nullptr) muGraphicsUnregisterResource(resource);
    if (pbo) glDeleteBuffers(1, &pbo);
}

CUdeviceptr CudaAlgorithm::TextureResource::map(size_t& size, int contextID, bool copyFromTexture)
{
    osg::Texture::TextureObject* texObj = texture->getTextureObject(contextID);
    if (resource == 0) return NULL;
    if (copyFromTexture && texObj)
    {
        glBindTexture(GL_TEXTURE_2D, texObj->id());
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
        glGetTexImage(GL_TEXTURE_2D, 0, pixelFormat, dataType, 0);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    CUdeviceptr devicePtr = NULL;
    muGraphicsMapResources(1, &resource, 0);
    muGraphicsResourceGetMappedPointer(&devicePtr, &size, resource);
    return devicePtr;
}

void CudaAlgorithm::TextureResource::unmap(int contextID, bool copyToTexture)
{
    if (resource == 0) return;
    muGraphicsUnmapResources(1, &resource, 0);

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
        MUdevice cuDevice = 0; char deviceName[80];
        int numGpu = 0; muInit(0); muDeviceGetCount(&numGpu);
        if (gpuID < 0 || gpuID >= numGpu) return NULL;

        muDeviceGet(&cuDevice, gpuID);
        muDeviceGetName(deviceName, sizeof(deviceName), cuDevice);
        muCtxCreate(&cuContext, MU_CTX_SCHED_BLOCKING_SYNC, cuDevice);
        OSG_NOTICE << "[CudaAlgorithm] GPU in use: " << deviceName << std::endl;
        return cuContext;
    }

    void CudaAlgorithm::deinitializeContext(CUcontext context)
    {
        muCtxDestroy(context);
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

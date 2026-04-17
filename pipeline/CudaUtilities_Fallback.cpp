#include <osg/io_utils>
#include <osg/Version>
#include <iostream>

#include "Utilities.h"
using namespace osgVerse;

CudaAlgorithm::TextureResource::TextureResource(osg::Texture*, int, bool) : resource(0), pbo(0) {}
CudaAlgorithm::TextureResource::~TextureResource() {}
void CudaAlgorithm::TextureResource::unmap(int contextID, bool copyToTexture) {}
CUdeviceptr CudaAlgorithm::TextureResource::map(size_t& size, int contextID, bool copyFromTexture)
{ OSG_WARN << "[TextureResource] CUDA/MUSA not compiled\n"; return NULL; }

namespace osgVerse
{
    CUcontext CudaAlgorithm::initializeContext(int gpuID) { return NULL; }
    void CudaAlgorithm::deinitializeContext(CUcontext context) {}

    bool CudaAlgorithm::radixSort(const std::vector<unsigned int>& inValues, const std::vector<unsigned int>& inIDs,
                                  std::vector<unsigned int>& outIDs)
    { OSG_WARN << "[CudaAlgorithm] CUDA/MUSA not compiled\n"; return false; }
}

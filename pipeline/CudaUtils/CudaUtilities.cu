#include <osg/io_utils>
#include <osg/Version>
#include <osg/GL>
#include <iostream>

#include <cuda.h>
#include <thrust/sort.h>
#include <thrust/device_vector.h>
#include <thrust/execution_policy.h>
#include "../Utilities.h"
using namespace osgVerse;

CudaAlgorithm::CUcontext CudaAlgorithm::initializeContext(int gpuID)
{
    CudaAlgorithm::CUcontext cuContext = NULL;
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

void CudaAlgorithm::deinitializeContext(CudaAlgorithm::CUcontext context)
{
    cuCtxDestroy(context);
}

bool CudaAlgorithm::radixSort(const std::vector<unsigned int>& inValues, const std::vector<unsigned int>& inIDs,
                              std::vector<unsigned int>& outIDs)
{
    size_t numElements = inValues.size();
    if (numElements > inIDs.size()) return false;
    if (numElements != outIDs.size()) outIDs.resize(numElements);

    thrust::device_vector<unsigned int> d_values(inValues);
    thrust::device_vector<unsigned int> d_ids(inIDs);
    thrust::sort_by_key(thrust::device, d_values.begin(), d_values.end(), d_ids.begin());
    thrust::copy(d_ids.begin(), d_ids.end(), outIDs.begin());
    return true;
}

#include <osg/io_utils>
#include <osg/Version>
#include <osg/GL>
#include <iostream>

#ifdef VERSE_ENABLE_MTT
#   include <musa.h>
#   include <musaGL.h>
#else
#   include <cuda.h>
#   include <cudaGL.h>
#endif
#include "CudaUtils/RadixSort.h"
#include "Utilities.h"
using namespace osgVerse;

CUcontext CudaAlgorithm::initializeContext(int gpuID)
{
    CUcontext cuContext = NULL;
    CUdevice cuDevice = 0; char deviceName[80];
    int numGpu = 0; cuInit(0); cuDeviceGetCount(&numGpu);
    if (gpuID < 0 || gpuID >= numGpu) return NULL;

#ifdef VERSE_ENABLE_MTT
    muDeviceGet(&cuDevice, gpuID);
    muDeviceGetName(deviceName, sizeof(deviceName), cuDevice);
    muCtxCreate(&cuContext, MU_CTX_SCHED_BLOCKING_SYNC, cuDevice);
#else
    cuDeviceGet(&cuDevice, gpuID);
    cuDeviceGetName(deviceName, sizeof(deviceName), cuDevice);
    cuCtxCreate(&cuContext, CU_CTX_SCHED_BLOCKING_SYNC, cuDevice);
#endif
    OSG_NOTICE << "[CudaAlgorithm] GPU in use: " << deviceName << std::endl;
    return cuContext;
}

void CudaAlgorithm::deinitializeContext(CUcontext context)
{
#ifdef VERSE_ENABLE_MTT
    muCtxDestroy(context);
#else
    cuCtxDestroy(context);
#endif
}

bool CudaAlgorithm::radixSort(const std::vector<unsigned int>& inValues, const std::vector<unsigned int>& inIDs,
                              std::vector<unsigned int>& outIDs)
{
    size_t numElements = inValues.size();
    if (numElements > inIDs.size()) return false;
    if (numElements != outIDs.size()) outIDs.resize(numElements);

    unsigned int *d_in, *v_in, *d_out;
    checkCudaErrors(cudaMalloc(&d_in, sizeof(unsigned int) * numElements));
    checkCudaErrors(cudaMalloc(&v_in, sizeof(unsigned int) * numElements));
    checkCudaErrors(cudaMalloc(&d_out, sizeof(unsigned int) * numElements));
    checkCudaErrors(cudaMemcpy(d_in, inValues.data(), sizeof(unsigned int) * numElements, cudaMemcpyHostToDevice));
    checkCudaErrors(cudaMemcpy(v_in, inIDs.data(), sizeof(unsigned int) * numElements, cudaMemcpyHostToDevice));

    radix_sort(d_out, d_in, v_in, numElements);
    checkCudaErrors(cudaMemcpy(outIDs.data(), d_out, sizeof(unsigned int) * numElements, cudaMemcpyDeviceToHost));
    checkCudaErrors(cudaFree(d_in)); checkCudaErrors(cudaFree(v_in));
    checkCudaErrors(cudaFree(d_out));
    return true;
}

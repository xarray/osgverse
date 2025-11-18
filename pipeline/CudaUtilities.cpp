#include <osg/io_utils>
#include <osg/Version>
#include <osg/GL>
#include <iostream>

#ifdef VERSE_ENABLE_MTT
#   include <musa.h>
#   include <musaGL.h>
#   include "MusaUtils/RadixSort.h"
#else
#   include <cuda.h>
#   include <cudaGL.h>
#   include "CudaUtils/RadixSort.h"
#endif
#include "Utilities.h"
using namespace osgVerse;

CudaAlgorithm::CUcontext CudaAlgorithm::initializeContext(int gpuID)
{
    CudaAlgorithm::CUcontext cuContext = NULL;
#ifdef VERSE_ENABLE_MTT
    MUdevice cuDevice = 0; char deviceName[80];
    int numGpu = 0; muInit(0); muDeviceGetCount(&numGpu);
    if (gpuID < 0 || gpuID >= numGpu) return NULL;

    muDeviceGet(&cuDevice, gpuID);
    muDeviceGetName(deviceName, sizeof(deviceName), cuDevice);
    muCtxCreate(&cuContext, MU_CTX_SCHED_BLOCKING_SYNC, cuDevice);
#else
    CUdevice cuDevice = 0; char deviceName[80];
    int numGpu = 0; cuInit(0); cuDeviceGetCount(&numGpu);
    if (gpuID < 0 || gpuID >= numGpu) return NULL;

    cuDeviceGet(&cuDevice, gpuID);
    cuDeviceGetName(deviceName, sizeof(deviceName), cuDevice);
    cuCtxCreate(&cuContext, CU_CTX_SCHED_BLOCKING_SYNC, cuDevice);
#endif
    OSG_NOTICE << "[CudaAlgorithm] GPU in use: " << deviceName << std::endl;
    return cuContext;
}

void CudaAlgorithm::deinitializeContext(CudaAlgorithm::CUcontext context)
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
#ifdef VERSE_ENABLE_MTT
    checkCudaErrors(musaMalloc(&d_in, sizeof(unsigned int) * numElements));
    checkCudaErrors(musaMalloc(&v_in, sizeof(unsigned int) * numElements));
    checkCudaErrors(musaMalloc(&d_out, sizeof(unsigned int) * numElements));
    checkCudaErrors(musaMemcpy(d_in, inValues.data(), sizeof(unsigned int) * numElements, musaMemcpyHostToDevice));
    checkCudaErrors(musaMemcpy(v_in, inIDs.data(), sizeof(unsigned int) * numElements, musaMemcpyHostToDevice));

    radix_sort(d_out, d_in, v_in, numElements);
    checkCudaErrors(musaMemcpy(outIDs.data(), d_out, sizeof(unsigned int) * numElements, musaMemcpyDeviceToHost));
    checkCudaErrors(musaFree(d_in)); checkCudaErrors(musaFree(v_in));
    checkCudaErrors(musaFree(d_out));
#else
    checkCudaErrors(cudaMalloc(&d_in, sizeof(unsigned int) * numElements));
    checkCudaErrors(cudaMalloc(&v_in, sizeof(unsigned int) * numElements));
    checkCudaErrors(cudaMalloc(&d_out, sizeof(unsigned int) * numElements));
    checkCudaErrors(cudaMemcpy(d_in, inValues.data(), sizeof(unsigned int) * numElements, cudaMemcpyHostToDevice));
    checkCudaErrors(cudaMemcpy(v_in, inIDs.data(), sizeof(unsigned int) * numElements, cudaMemcpyHostToDevice));

    radix_sort(d_out, d_in, v_in, numElements);
    checkCudaErrors(cudaMemcpy(outIDs.data(), d_out, sizeof(unsigned int) * numElements, cudaMemcpyDeviceToHost));
    checkCudaErrors(cudaFree(d_in)); checkCudaErrors(cudaFree(v_in));
    checkCudaErrors(cudaFree(d_out));
#endif
    return true;
}

#include <osg/io_utils>
#include <osg/Version>
#include <osg/GL>
#include <iostream>

#include <musa.h>
#include <thrust/sort.h>
#include <thrust/device_vector.h>
#include <thrust/execution_policy.h>
#include "../Utilities.h"
using namespace osgVerse;

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

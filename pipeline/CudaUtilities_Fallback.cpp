#include <osg/io_utils>
#include <osg/Version>
#include <iostream>

#include "Utilities.h"
using namespace osgVerse;

CudaAlgorithm::CUcontext CudaAlgorithm::initializeContext(int gpuID) { return NULL; }
void CudaAlgorithm::deinitializeContext(CudaAlgorithm::CUcontext context) {}

bool CudaAlgorithm::radixSort(const std::vector<unsigned int>& inValues, const std::vector<unsigned int>& inIDs,
                              std::vector<unsigned int>& outIDs) { return false; }

// Kernel launcher interface: implemented by fusion_kernel.cu (compiled by nvcc)
// for CUDA, and by fusion_kernel.mu (compiled by mcc) for MooreThreads MUSA;
// either one can also be compiled by g++ in host-emulation mode. The
// orchestration layer (cuda_integrator.cpp) does not include any GPU runtime
// header directly and only interacts through this interface, so the backends can
// be swapped seamlessly.

#ifndef CHISEL_CUDA_KERNEL_LAUNCHER_H_
#define CHISEL_CUDA_KERNEL_LAUNCHER_H_

#include <cstddef>

namespace chisel_cuda
{

    // ---- Device memory management (degrades to malloc/free/memcpy in emulation mode) ----
    void* DeviceAlloc(std::size_t bytes);
    void  DeviceFree(void* ptr);
    void  DeviceUpload(void* dst, const void* src, std::size_t bytes);   // H2D
    void  DeviceDownload(void* dst, const void* src, std::size_t bytes); // D2H
    void  DeviceSynchronize();

    // ---- TSDF fusion kernel ----
    // depth:      imgW*imgH depth image (meters, NaN for invalid values), row-major
    // Rwc/twc:    camera pose (camera -> world); Rwc is a 3x3 row-major matrix
    // voxels:     numChunks * voxelsPerChunk * 2, (sdf, weight) interleaved, float2 layout
    // origins:    numChunks * 3, origin of each chunk (world space, meters)
    // chunkDim:   number of voxels per chunk side (must be equal for all chunks)
    // updatedFlags: numChunks; the kernel writes 1 to mark a chunk with updated voxels
    void LaunchFuseKernel(const float* depth, int imgW, int imgH,
                          float fx, float fy, float cx, float cy,
                          const float* Rwc, const float* twc,
                          float* voxels, const float* origins,
                          int numChunks, int voxelsPerChunk,
                          int chunkDim, float voxelRes,
                          float truncationDist, float weight,
                          float carvingDist, bool enableCarving,
                          float maxWeight,
                          int* updatedFlags);

    // "cuda" / "musa", or "host-emulation"
    const char* KernelBackend();

} // namespace chisel_cuda

#endif // CHISEL_CUDA_KERNEL_LAUNCHER_H_

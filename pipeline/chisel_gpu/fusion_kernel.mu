// TSDF fusion kernel: thread-per-voxel projective fusion.
//
// The structure follows the kernel_fusion pattern of
// mesh-fusion/libfusiongpu/fusion.cu (OctNet authors, BSD license); the fusion
// math is identical to OpenChisel's (MIT) ProjectionIntegrator (incremental
// weighted average + optional carving), keeping it voxel-by-voxel aligned with
// the CPU reference implementation.
//
// Note: this is the MooreThreads MUSA twin of fusion_kernel.cu. It shares
// kernel_launcher.h and cuda_integrator.* with the CUDA build; only the GPU
// runtime and its includes differ.
//
// Compilation modes:
//   - Normal: mcc (-DCHISEL_CUDA_ENABLED)
//   - Host emulation: g++ (-DCHISEL_CUDA_ENABLED -DCHISEL_CUDA_HOST_EMULATION),
//     where the kernel degrades to a single-threaded loop, used to verify
//     algorithm correctness in environments without a GPU.

#include "kernel_launcher.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

#ifdef CHISEL_CUDA_HOST_EMULATION
    #define CHISEL_HD inline
    #define CHISEL_ISNAN(x) std::isnan(x)
#else
    #include <musa_runtime.h>
    #define CHISEL_HD __device__ inline
    #define CHISEL_ISNAN(x) isnan(x)
#endif

namespace chisel_cuda
{

// ------------------------------------------------------------------
// Core fusion logic: process a single voxel. Shared by CUDA / host emulation.
// It corresponds line by line to OpenChisel ProjectionIntegrator::Integrate
// (non-color path).
// ------------------------------------------------------------------
CHISEL_HD void FuseOneVoxel(int globalIdx,
                            const float* depth, int imgW, int imgH,
                            float fx, float fy, float cx, float cy,
                            const float* Rwc, const float* twc,
                            float* voxels, const float* origins,
                            int voxelsPerChunk, int chunkDim, float voxelRes,
                            float truncationDist, float weight,
                            float carvingDist, bool enableCarving,
                            float maxWeight,
                            int* updatedFlags)
{
    const int chunkIdx = globalIdx / voxelsPerChunk;
    const int local    = globalIdx % voxelsPerChunk;

    // In-chunk voxel coordinates (z-y-x row-major, consistent with Chunk::GetVoxelID)
    const int vx = local % chunkDim;
    const int vy = (local / chunkDim) % chunkDim;
    const int vz = local / (chunkDim * chunkDim);

    // Voxel center in world space = chunk origin + (x+0.5)*res
    // (aligned with ChunkManager's centroid convention)
    const float ox = origins[chunkIdx * 3 + 0];
    const float oy = origins[chunkIdx * 3 + 1];
    const float oz = origins[chunkIdx * 3 + 2];
    const float wx = ox + (vx + 0.5f) * voxelRes;
    const float wy = oy + (vy + 0.5f) * voxelRes;
    const float wz = oz + (vz + 0.5f) * voxelRes;

    // world -> camera: pc = R^T * (pw - twc).
    // Note: what is passed in is Eigen::Matrix3f's column-major storage s
    // (s[0]=R(0,0), s[1]=R(1,0), ...), so
    // (R^T v)_x = R(0,0)vx + R(1,0)vy + R(2,0)vz, i.e. a dot product of 3
    // consecutive elements.
    const float dx = wx - twc[0];
    const float dy = wy - twc[1];
    const float dz = wz - twc[2];
    const float px = Rwc[0] * dx + Rwc[1] * dy + Rwc[2] * dz;
    const float py = Rwc[3] * dx + Rwc[4] * dy + Rwc[5] * dz;
    const float pz = Rwc[6] * dx + Rwc[7] * dy + Rwc[8] * dz;

    if (pz <= 0.0f) return;

    // Pinhole projection (aligned with PinholeCamera::ProjectPoint / IsPointOnImage)
    const float u = fx * px / pz + cx;
    const float v = fy * py / pz + cy;
    if (u < 0.0f || v < 0.0f || u >= (float)imgW || v >= (float)imgH) return;

    const float d = depth[(int)v * imgW + (int)u];
    if (CHISEL_ISNAN(d)) return;

    const float surfaceDist  = d - pz;
    const float truncation   = truncationDist;          // ConstantTruncator
    const float diag         = 2.0f * sqrtf(3.0f) * voxelRes;

    const int base = (chunkIdx * voxelsPerChunk + local) * 2;
    float sdf = voxels[base];
    float w   = voxels[base + 1];

    if (fabsf(surfaceDist) < truncation + diag)
    {
        // Incremental weighted average of DistVoxel::Integrate(surfaceDist, weight)
        const float newW = w + weight;
        voxels[base]     = (w * sdf + weight * surfaceDist) / newW;
        voxels[base + 1] = fminf(newW, maxWeight);
        updatedFlags[chunkIdx] = 1;   // Benign race write; value is always 1, no atomics needed
    }
    else if (enableCarving && surfaceDist > truncation + carvingDist)
    {
        // Aligned with OpenChisel's carving branch + DistVoxel::Carve() == Integrate(0, 1.5)
        if (w > 0.0f && sdf < 1e-5f)
        {
            const float cw  = 1.5f;
            const float nw  = w + cw;
            voxels[base]     = (w * sdf) / nw;
            voxels[base + 1] = fminf(nw, maxWeight);
            updatedFlags[chunkIdx] = 1;
        }
    }
}

// ------------------------------------------------------------------
// CUDA backend
// ------------------------------------------------------------------
#ifndef CHISEL_CUDA_HOST_EMULATION

__global__ void FuseKernel(const float* depth, int imgW, int imgH,
                           float fx, float fy, float cx, float cy,
                           const float* Rwc, const float* twc,
                           float* voxels, const float* origins,
                           int totalVoxels, int voxelsPerChunk,
                           int chunkDim, float voxelRes,
                           float truncationDist, float weight,
                           float carvingDist, bool enableCarving,
                           float maxWeight, int* updatedFlags)
{
    // grid-stride loop (cf. CUDA_KERNEL_LOOP in fusion.cu)
    for (int idx = blockIdx.x * blockDim.x + threadIdx.x;
         idx < totalVoxels;
         idx += gridDim.x * blockDim.x)
    {
        FuseOneVoxel(idx, depth, imgW, imgH, fx, fy, cx, cy, Rwc, twc,
                     voxels, origins, voxelsPerChunk, chunkDim, voxelRes,
                     truncationDist, weight, carvingDist, enableCarving,
                     maxWeight, updatedFlags);
    }
}

void* DeviceAlloc(std::size_t bytes)
{
    void* p = nullptr;
    musaMalloc(&p, bytes);
    return p;
}
void DeviceFree(void* ptr) { if (ptr) musaFree(ptr); }
void DeviceUpload(void* dst, const void* src, std::size_t bytes)
{
    musaMemcpy(dst, src, bytes, musaMemcpyHostToDevice);
}
void DeviceDownload(void* dst, const void* src, std::size_t bytes)
{
    musaMemcpy(dst, src, bytes, musaMemcpyDeviceToHost);
}
void DeviceSynchronize() { musaDeviceSynchronize(); }

void LaunchFuseKernel(const float* depth, int imgW, int imgH,
                      float fx, float fy, float cx, float cy,
                      const float* Rwc, const float* twc,
                      float* voxels, const float* origins,
                      int numChunks, int voxelsPerChunk,
                      int chunkDim, float voxelRes,
                      float truncationDist, float weight,
                      float carvingDist, bool enableCarving,
                      float maxWeight, int* updatedFlags)
{
    const int totalVoxels = numChunks * voxelsPerChunk;
    const int block = 256;
    const int grid  = (totalVoxels + block - 1) / block;
    FuseKernel<<<grid, block>>>(depth, imgW, imgH, fx, fy, cx, cy,
                                Rwc, twc, voxels, origins,
                                totalVoxels, voxelsPerChunk, chunkDim,
                                voxelRes, truncationDist, weight,
                                carvingDist, enableCarving, maxWeight,
                                updatedFlags);
    musaDeviceSynchronize();
}

const char* KernelBackend() { return "musa"; }

// ------------------------------------------------------------------
// Host emulation backend: the same code, run as a per-voxel loop
// ------------------------------------------------------------------
#else

void* DeviceAlloc(std::size_t bytes) { return std::malloc(bytes); }
void  DeviceFree(void* ptr) { std::free(ptr); }
void  DeviceUpload(void* dst, const void* src, std::size_t bytes)
{
    std::memcpy(dst, src, bytes);
}
void  DeviceDownload(void* dst, const void* src, std::size_t bytes)
{
    std::memcpy(dst, src, bytes);
}
void  DeviceSynchronize() {}

void LaunchFuseKernel(const float* depth, int imgW, int imgH,
                      float fx, float fy, float cx, float cy,
                      const float* Rwc, const float* twc,
                      float* voxels, const float* origins,
                      int numChunks, int voxelsPerChunk,
                      int chunkDim, float voxelRes,
                      float truncationDist, float weight,
                      float carvingDist, bool enableCarving,
                      float maxWeight, int* updatedFlags)
{
    const int totalVoxels = numChunks * voxelsPerChunk;
    for (int idx = 0; idx < totalVoxels; ++idx)
    {
        FuseOneVoxel(idx, depth, imgW, imgH, fx, fy, cx, cy, Rwc, twc,
                     voxels, origins, voxelsPerChunk, chunkDim, voxelRes,
                     truncationDist, weight, carvingDist, enableCarving,
                     maxWeight, updatedFlags);
    }
}

const char* KernelBackend() { return "host-emulation"; }

#endif // CHISEL_CUDA_HOST_EMULATION

} // namespace chisel_cuda

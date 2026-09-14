// CUDA-accelerated TSDF integrator for depth images.
//
// Architecture: OpenChisel's Chunk/ChunkManager (spatial-hash chunk structure)
// stays unchanged. On every frame, the voxel data of chunks inside the frustum
// is packed and uploaded to the device, where a thread-per-voxel kernel
// (modeled after mesh-fusion/OctNet's fusion.cu) performs projection +
// truncation + weighted update, and the result is downloaded back to the CPU
// side chunks.
//
// Notes for this initial version:
//  - Only equal-sized chunks are supported (OpenChisel's default);
//  - Only the distance field (sdf/weight) is fused; use the original CPU path
//    for color fusion;
//  - Each frame incurs H2D/D2H transfer cost, a "conservatively correct"
//    initial version; see README for further optimization (keep voxels
//    resident in device memory).
//
// License: the fusion math matches OpenChisel (MIT); the kernel structure is
// modeled after mesh-fusion/libfusiongpu/fusion.cu (OctNet authors, BSD
// license header).

#ifndef CHISEL_CUDA_INTEGRATOR_H_
#define CHISEL_CUDA_INTEGRATOR_H_

#include <memory>
#include <vector>

#include <open_chisel/ChunkManager.h>
#include <open_chisel/camera/PinholeCamera.h>
#include <open_chisel/camera/DepthImage.h>
#include <open_chisel/geometry/Frustum.h>

namespace chisel_cuda
{

    struct FusionParams
    {
        float truncationDist;    // Truncation distance (meters), corresponds to ConstantTruncator
        float weight;            // Weight per observation, 1.0f for OpenChisel non-color path
        float carvingDist;       // Distance that triggers voxel carving
        bool  enableCarving;     // Whether to enable dynamic carving
        float maxWeight;         // Weight upper bound (prevents unbounded growth over long runs)

        FusionParams()
            : truncationDist(0.05f), weight(1.0f),
              carvingDist(0.05f), enableCarving(false), maxWeight(1000.0f) {}
    };

    class CudaDepthIntegrator
    {
        public:
            explicit CudaDepthIntegrator(const FusionParams& params);
            ~CudaDepthIntegrator();

            // Entry point equivalent to chisel::Chisel::IntegrateDepthScan:
            // frustum culling, on-demand chunk allocation, fusion, and returning
            // the list of updated chunks. depthImage must be an organized depth
            // image in meters, with NaN for invalid values. Returns false if no
            // voxel was updated this time.
            bool IntegrateDepthScan(chisel::ChunkManager& chunkManager,
                                    const chisel::DepthImage<float>& depthImage,
                                    const chisel::Transform& cameraPose,
                                    const chisel::PinholeCamera& camera,
                                    chisel::ChunkIDList* updatedChunks);

            // Current device type (cuda / host-emulation), useful for logging and tests.
            static const char* BackendName();

        private:
            FusionParams params_;

            // Device-side buffers (grow on demand, reused across frames)
            float* d_depth        = nullptr;   // Depth image
            float* d_voxels       = nullptr;   // Chunk voxels, (sdf, weight) interleaved
            float* d_origins      = nullptr;   // Chunk origins (x, y, z)
            int*   d_updatedFlags = nullptr;   // Whether each chunk was updated
            size_t depthCapacity  = 0;
            size_t chunkCapacity  = 0;         // Counted in chunks

            std::vector<float> h_staging;      // Host-side packing buffer, reused across frames
    };

} // namespace chisel_cuda

#endif // CHISEL_CUDA_INTEGRATOR_H_

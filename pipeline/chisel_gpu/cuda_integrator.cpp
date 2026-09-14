// CudaDepthIntegrator orchestration layer:
//   1) Reuse OpenChisel's frustum culling and chunk allocation on the CPU side;
//   2) Pack the voxel data of affected chunks (after devirtualization
//      DistVoxel == 8 bytes, copied as a whole with memcpy);
//   3) Call the kernel launcher (CUDA or host emulation) to perform fusion;
//   4) Only copy back updated chunks and clean up newly created "empty" chunks
//      (aligned with Chisel's behavior).
//
// This file includes no CUDA header; all device interaction goes through the
// kernel_launcher interface.

#include "cuda_integrator.h"
#include "kernel_launcher.h"

#include <cassert>
#include <cmath>
#include <cstring>
#include <unordered_set>

namespace chisel_cuda
{

// Layout guarantee after devirtualization: DistVoxel == {float sdf, float weight}, 8 bytes
static_assert(sizeof(chisel::DistVoxel) == 2 * sizeof(float),
              "DistVoxel layout must be {float sdf, float weight}");

CudaDepthIntegrator::CudaDepthIntegrator(const FusionParams& params)
    : params_(params) {}

CudaDepthIntegrator::~CudaDepthIntegrator()
{
    DeviceFree(d_depth);
    DeviceFree(d_voxels);
    DeviceFree(d_origins);
    DeviceFree(d_updatedFlags);
}

const char* CudaDepthIntegrator::BackendName() { return KernelBackend(); }

bool CudaDepthIntegrator::IntegrateDepthScan(chisel::ChunkManager& chunkManager,
                                             const chisel::DepthImage<float>& depthImage,
                                             const chisel::Transform& cameraPose,
                                             const chisel::PinholeCamera& camera,
                                             chisel::ChunkIDList* updatedChunks)
{
    using namespace chisel;

    // ---- 1. Frustum culling (same as Chisel::IntegrateDepthScan) ----
    float minimum, maximum, mean;
    depthImage.GetStats(minimum, maximum, mean);

    Frustum frustum;
    PinholeCamera cameraCopy = camera;
    cameraCopy.SetNearPlane(minimum);
    cameraCopy.SetFarPlane(maximum);
    cameraCopy.SetupFrustum(cameraPose, &frustum);

    ChunkIDList chunkIDs;
    chunkManager.GetChunkIDsIntersecting(frustum, &chunkIDs);
    if (chunkIDs.empty()) return false;

    // ---- 2. Allocate chunks on demand and collect their pointers ----
    const int numChunks = static_cast<int>(chunkIDs.size());
    std::vector<ChunkPtr> chunks(numChunks);
    std::vector<char>     isNew(numChunks, 0);

    for (int i = 0; i < numChunks; ++i)
    {
        if (!chunkManager.HasChunk(chunkIDs[i]))
        {
            chunkManager.CreateChunk(chunkIDs[i]);
            isNew[i] = 1;
        }
        chunks[i] = chunkManager.GetChunk(chunkIDs[i]);
        assert(chunks[i] != nullptr);
        if (!chunks[i]->HasVoxels()) chunks[i]->AllocateDistVoxels();
    }

    // This initial version requires all chunks to be equally sized (OpenChisel's default)
    const Eigen::Vector3i numVoxels3 = chunks[0]->GetNumVoxels();
    const int chunkDim = numVoxels3(0);
    assert(numVoxels3(1) == chunkDim && numVoxels3(2) == chunkDim &&
           "Initial chunk size must be a cube");
    const int voxelsPerChunk = chunkDim * chunkDim * chunkDim;
    const float voxelRes = chunks[0]->GetVoxelResolutionMeters();
    for (const auto& c : chunks)
        assert(c->GetNumVoxels() == numVoxels3 && "chunk size must be consistent");

    // ---- 3. Host-side packing: voxel data + chunk origins ----
    const size_t voxelFloats = static_cast<size_t>(numChunks) * voxelsPerChunk * 2;
    h_staging.resize(voxelFloats + numChunks * 3);
    float* h_voxels  = h_staging.data();
    float* h_origins = h_staging.data() + voxelFloats;

    for (int i = 0; i < numChunks; ++i)
    {
        // DistVoxel has a contiguous {sdf, weight} layout, copied as a whole
        std::memcpy(h_voxels + static_cast<size_t>(i) * voxelsPerChunk * 2,
                    chunks[i]->GetVoxels().data(),
                    static_cast<size_t>(voxelsPerChunk) * 2 * sizeof(float));
        const Vec3& o = chunks[i]->GetOrigin();
        h_origins[i * 3 + 0] = o.x();
        h_origins[i * 3 + 1] = o.y();
        h_origins[i * 3 + 2] = o.z();
    }

    // ---- 4. Device buffers (reused across frames, grown on demand) ----
    const size_t depthFloats = static_cast<size_t>(depthImage.GetWidth()) *
                               depthImage.GetHeight();
    if (depthFloats > depthCapacity)
    {
        DeviceFree(d_depth);
        d_depth = static_cast<float*>(DeviceAlloc(depthFloats * sizeof(float)));
        depthCapacity = depthFloats;
    }
    if (static_cast<size_t>(numChunks) > chunkCapacity)
    {
        DeviceFree(d_voxels); DeviceFree(d_origins); DeviceFree(d_updatedFlags);
        d_voxels       = static_cast<float*>(DeviceAlloc(
                             static_cast<size_t>(numChunks) * voxelsPerChunk * 2 * sizeof(float)));
        d_origins      = static_cast<float*>(DeviceAlloc(
                             static_cast<size_t>(numChunks) * 3 * sizeof(float)));
        d_updatedFlags = static_cast<int*>(DeviceAlloc(
                             static_cast<size_t>(numChunks) * sizeof(int)));
        chunkCapacity  = numChunks;
    }
    std::vector<int> h_flags(numChunks, 0);

    // ---- 5. Upload -> fuse -> download ----
    DeviceUpload(d_depth, depthImage.GetData(), depthFloats * sizeof(float));
    DeviceUpload(d_voxels, h_voxels, voxelFloats * sizeof(float));
    DeviceUpload(d_origins, h_origins, numChunks * 3 * sizeof(float));
    DeviceUpload(d_updatedFlags, h_flags.data(), numChunks * sizeof(int));

    const chisel::Mat3x3 R = cameraPose.linear();
    const chisel::Vec3   t = cameraPose.translation();
    const Intrinsics& intr = camera.GetIntrinsics();

    LaunchFuseKernel(d_depth, depthImage.GetWidth(), depthImage.GetHeight(),
                     intr.GetFx(), intr.GetFy(), intr.GetCx(), intr.GetCy(),
                     R.data(), t.data(),   // Eigen 3x3 column-major storage;
                                           // the kernel implements R^T*v via
                                           // consecutive triples
                     d_voxels, d_origins,
                     numChunks, voxelsPerChunk, chunkDim, voxelRes,
                     params_.truncationDist, params_.weight,
                     params_.carvingDist, params_.enableCarving,
                     params_.maxWeight,
                     d_updatedFlags);

    DeviceDownload(h_flags.data(), d_updatedFlags, numChunks * sizeof(int));

    // ---- 6. Read back only updated chunks from the device; collect the
    //         newly created but unupdated chunks ----
    bool anyUpdated = false;
    ChunkIDList garbage;
    for (int i = 0; i < numChunks; ++i)
    {
        if (h_flags[i])
        {
            anyUpdated = true;
            DeviceDownload(chunks[i]->GetVoxelsMutable().data(),
                           d_voxels + static_cast<size_t>(i) * voxelsPerChunk * 2,
                           static_cast<size_t>(voxelsPerChunk) * 2 * sizeof(float));
            if (updatedChunks) updatedChunks->push_back(chunkIDs[i]);
        }
        else if (isNew[i])
        {
            garbage.push_back(chunkIDs[i]);
        }
    }

    // Clean up "newly created but unupdated" empty chunks
    // (aligned with Chisel::GarbageCollect's behavior)
    for (const ChunkID& id : garbage)
    {
        chunkManager.RemoveChunk(id);
    }

    return anyUpdated;
}

} // namespace chisel_cuda

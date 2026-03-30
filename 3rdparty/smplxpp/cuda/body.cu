#include <iostream>

#include "smplx/smplx.hpp"
#include "smplx/util.hpp"
#include "smplx/internal/cuda_util.cuh"

namespace smplx {
namespace {
using cuda_util::device::BLOCK_SIZE;
using cuda_util::from_host_eigen_sparse_matrix;
using cuda_util::from_host_eigen_matrix;
using cuda_util::to_host_eigen_matrix;

namespace device {
/** Rodrigues formula: d_pose_full (#joints,3) -> out(#joints,9);
 * also copies out to upper-left 3x3 part of out_joint_local_transform
 * (#joints,12)
 * Note this is no longer used; however I have not deleted it since
 * it may be useful in the future */
/* __global__ void rodrigues(float* RESTRICT d_pose_full, float* RESTRICT out,
                          float* RESTRICT out_joint_local_transform) {
    const int in_idx = threadIdx.x * 3;
    const int out_idx = threadIdx.x * 9;
    const int out_transform_idx = threadIdx.x * 12;
    float theta = norm3df(d_pose_full[in_idx], d_pose_full[in_idx + 1],
                          d_pose_full[in_idx + 2]);
    if (fabsf(theta) < 1e-5f) {
        for (int i = out_idx; i < out_idx + 9; ++i) {
            out[i] = 0.f;
        }
        out_joint_local_transform[out_transform_idx + 1] =
            out_joint_local_transform[out_transform_idx + 2] =
                out_joint_local_transform[out_transform_idx + 4] =
                    out_joint_local_transform[out_transform_idx + 6] =
                        out_joint_local_transform[out_transform_idx + 8] =
                            out_joint_local_transform[out_transform_idx + 9] =
                                0.f;
        out_joint_local_transform[out_transform_idx] =
            out_joint_local_transform[out_transform_idx + 5] =
                out_joint_local_transform[out_transform_idx + 10] = 1.f;
    } else {
        float cm1 = cos(theta) - 1.f;
        float s = sin(theta);

        const float a = d_pose_full[in_idx] /= theta;
        const float b = d_pose_full[in_idx + 1] /= theta;
        const float c = d_pose_full[in_idx + 2] /= theta;

        out[out_idx] = cm1;
        out[out_idx + 1] = -s * c;
        out[out_idx + 2] = s * b;
        out[out_idx + 3] = s * c;
        out[out_idx + 4] = cm1;
        out[out_idx + 5] = -s * a;
        out[out_idx + 6] = -s * b;
        out[out_idx + 7] = s * a;
        out[out_idx + 8] = cm1;

        for (int j = 0; j < 3; ++j) {
            for (int k = 0; k < 3; ++k) {
                out_joint_local_transform[out_transform_idx + j * 4 + k] =
                    (out[out_idx + j * 3 + k] -=
                     cm1 * d_pose_full[in_idx + j] * d_pose_full[in_idx + k]);
            }
            // Un-subtract identity
            out_joint_local_transform[out_transform_idx + j * 4 + j] += 1.f;
        }
    }
} */

/** Joint regressor: multiples sparse matrix in CSR represented by
 *  (model_jr_values(nnz), ..inner(nnz), ..outer(#joints+1)) to
 *  d_verts_shaped(#verts,3) row-major
 *  -> outputs to out(#joints, 3) row-major
 *  TODO: Optimize. The matrix is very wide and this is not efficient */
__global__ void joint_regressor(float* RESTRICT d_verts_shaped, float* RESTRICT model_jr_values,
                                int* RESTRICT model_jr_inner, int* RESTRICT model_jr_outer,
                                float* RESTRICT out_joints) {
    const int joint = threadIdx.y, idx = threadIdx.x;
    out_joints[joint * 3 + idx] = 0.f;
    for (int i = model_jr_outer[joint]; i < model_jr_outer[joint + 1]; ++i) {
        out_joints[joint * 3 + idx] +=
            model_jr_values[i] * d_verts_shaped[model_jr_inner[i] * 3 + idx];
    }
}

/** Linear blend skinning kernel.
  * d_joint_global_transform (#joints, 12) row-major;
  *   global-space homogeneous transforms (bottom row dropped)
  *   at each joint from local_to_global
  * d_points_shaped (#points, 3) row-major; vertices after blendshapes applied
  * (model_weights_values(nnz), ..inner(nnz), ..outer(#joints+1)) sparse LBS weights in CSR
  * -> out_verts(#points, 3) resulting vertices after deformation */
__global__ void lbs(float* RESTRICT d_joint_global_transform, float* RESTRICT d_verts_shaped,
                    float* RESTRICT model_weights_values, int* RESTRICT model_weights_inner,
                    int* RESTRICT model_weights_outer,
                    float* RESTRICT out_verts,  // transformed joint pos
                    const int n_joints, const int n_verts) {
    const int vert = blockDim.x * blockIdx.x + threadIdx.x;  // Vert idx
    if (vert < n_verts) {
        for (int i = 0; i < 3; ++i) {
            out_verts[vert * 3 + i] = 0.f;
            for (int joint_it = model_weights_outer[vert];
                 joint_it < model_weights_outer[vert + 1]; ++joint_it) {
                const int joint_row_idx =
                    model_weights_inner[joint_it] * 12 + i * 4;
                for (int j = 0; j < 3; ++j) {
                    out_verts[vert * 3 + i] +=
                        model_weights_values[joint_it] *
                        d_joint_global_transform[joint_row_idx + j] *
                        d_verts_shaped[vert * 3 + j];
                }
                out_verts[vert * 3 + i] +=
                    model_weights_values[joint_it] *
                    d_joint_global_transform[joint_row_idx + 3];
            }
        }
    }
}

}  // namespace device
}  // namespace

/*
struct {
   float* params = nullptr;
   float* verts = nullptr;
   float* blendshape_params = nullptr;
   float* joint_transforms = nullptr;
} device; */
template<class ModelConfig>
__host__ void Body<ModelConfig>::_cuda_load() {
    cudaCheck(cudaMalloc((void**)&device.verts, model.n_verts() * 3 * sizeof(float)));
    cudaCheck(cudaMalloc((void**)&device.blendshape_params,
               model.n_blend_shapes() * sizeof(float)));
    cudaCheck(cudaMalloc((void**)&device.joint_transforms,
               model.n_joints() * 12 * sizeof(float)));
    cudaCheck(cudaMalloc((void**)&device.verts_shaped,
                         model.n_verts() * 3 * sizeof(float)));
    cudaCheck(cudaMalloc((void**)&device.joints_shaped,
                         model.n_joints() * 3 * sizeof(float)));
}
template<class ModelConfig>
__host__ void Body<ModelConfig>::_cuda_free() {
    if (device.verts) cudaFree(device.verts);
    if (device.blendshape_params) cudaFree(device.blendshape_params);
    if (device.joint_transforms) cudaFree(device.joint_transforms);
    if (device.verts_shaped) cudaFree(device.verts_shaped);
    if (device.joints_shaped) cudaFree(device.joints_shaped);
}
template<class ModelConfig>
__host__ void Body<ModelConfig>::_cuda_maybe_retrieve_verts() const {
    if (!_verts_retrieved) {
        _verts.resize(model.n_verts(), 3);
        cudaMemcpy(_verts.data(), device.verts, _verts.size() * sizeof(float),
                   cudaMemcpyDeviceToHost);
        _verts_retrieved = true;
    }
}

template<class ModelConfig>
__host__ void Body<ModelConfig>::_cuda_maybe_retrieve_verts_shaped() const {
    if (!_verts_shaped_retrieved) {
        _verts_shaped.resize(model.n_verts(), 3);
        cudaMemcpy(_verts_shaped.data(), device.verts_shaped,
                    _verts_shaped.size() * sizeof(float),
                   cudaMemcpyDeviceToHost);
        _verts_shaped_retrieved = true;
    }
}


template<class ModelConfig>
SMPLX_HOST void Body<ModelConfig>::_cuda_update(
        float* h_blendshape_params,
        float* h_joint_transforms,
        bool enable_pose_blendshapes) {
    // Verts will be updated
    _verts_retrieved = false;
    _verts_shaped_retrieved = false;

    // Copy parameters to GPU
    cudaCheck(cudaMemcpyAsync(device.blendshape_params, h_blendshape_params,
                ModelConfig::n_blend_shapes() * sizeof(float),
               cudaMemcpyHostToDevice));
    // Shape blendshapes
    cudaCheck(cudaMemcpyAsync(device.verts_shaped, model.device.verts,
               model.n_verts() * 3 * sizeof(float), cudaMemcpyDeviceToDevice));
    cuda_util::mmv_block<float, true>(model.device.blend_shapes,
            device.blendshape_params, device.verts_shaped, ModelConfig::n_verts() * 3,
            ModelConfig::n_shape_blends());

    // Joint regressor
    // TODO: optimize sparse matrix multiplication, maybe use ELL format
    dim3 jr_blocks(3, model.n_joints());
    device::joint_regressor<<<1, jr_blocks>>>(
        device.verts_shaped, model.device.joint_reg.values, model.device.joint_reg.inner,
        model.device.joint_reg.outer, device.joints_shaped);

    if (enable_pose_blendshapes) {
        // Pose blendshapes.
        // Note: this is the most expensive operation.
        cuda_util::mmv_block<float, true>(model.device.blend_shapes + ModelConfig::n_shape_blends() * 3 * ModelConfig::n_verts(),
               device.blendshape_params + ModelConfig::n_shape_blends(), device.verts_shaped, ModelConfig::n_verts() * 3,
               ModelConfig::n_pose_blends());
    }

    // Compute global joint transforms, this part can't be parallized and
    // is horribly slow on GPU; we do it on CPU instead
    // Actually, this is pretty bad too, TODO try implementing on GPU again
    cudaCheck(cudaMemcpyAsync(_joints_shaped.data(), device.joints_shaped, model.n_joints() * 3 * sizeof(float),
               cudaMemcpyDeviceToHost));
    _local_to_global();
    cudaCheck(cudaMemcpyAsync(device.joint_transforms, _joint_transforms.data(),
            _joint_transforms.size() * sizeof(float), cudaMemcpyHostToDevice));

    // weights: (#verts, #joints)
    device::lbs<<<(model.verts.size() - 1) / BLOCK_SIZE + 1, BLOCK_SIZE>>>(
        device.joint_transforms, device.verts_shaped, model.device.weights.values,
        model.device.weights.inner, model.device.weights.outer,
        device.verts,
        model.n_joints(), model.n_verts());
}

// Instantiation
template class Body<model_config::SMPL>;
template class Body<model_config::SMPL_v1>;
template class Body<model_config::SMPLH>;
template class Body<model_config::SMPLX>;
template class Body<model_config::SMPLXpca>;
template class Body<model_config::SMPLX_v1>;
template class Body<model_config::SMPLXpca_v1>;

}  // namespace smplx

#include <iostream>

#include "smplx/smplx.hpp"
#include "smplx/internal/cuda_util.cuh"

namespace smplx {

namespace {
using cuda_util::from_host_eigen_sparse_matrix;
using cuda_util::from_host_eigen_matrix;
}  // namespace

/*
struct {
   float* verts = nullptr;
   float* shape_blend = nullptr;
   GPUSparseMatrix joint_reg;
   GPUSparseMatrix weights;
   float* hand_comps_l = nullptr, * hand_comps_r = nullptr;
   float* hand_mean_l = nullptr, * hand_mean_r = nullptr;
} device; */
template<class ModelConfig>
__host__ void Model<ModelConfig>::_cuda_load() {
    from_host_eigen_matrix(device.verts, verts);
    from_host_eigen_matrix(device.blend_shapes, blend_shapes);
    /* { */
    /*     // To dense */
    /*     MatrixColMajor tmp_jreg = joint_reg;  // Change to CSR */
    /*     from_host_eigen_matrix(device.joint_reg_dense, tmp_jreg); */
    /* } */
    from_host_eigen_sparse_matrix(device.joint_reg, joint_reg);
    {
        SparseMatrix tmp_weights = weights;  // Change to CSR
        from_host_eigen_sparse_matrix(device.weights, tmp_weights);
    }

    if (n_hand_pca) {
        from_host_eigen_matrix(device.hand_comps_l, hand_comps_l);
        from_host_eigen_matrix(device.hand_comps_r, hand_comps_r);
        from_host_eigen_matrix(device.hand_mean_l, hand_mean_l);
        from_host_eigen_matrix(device.hand_mean_r, hand_mean_r);
    }
}
template<class ModelConfig>
__host__ void Model<ModelConfig>::_cuda_copy_template() {
    const size_t dsize = verts.size() * sizeof(verts.data()[0]);
    cudaMemcpy(device.verts, verts.data(), dsize, cudaMemcpyHostToDevice);
}
template<class ModelConfig>
__host__ void Model<ModelConfig>::_cuda_free() {
    if (device.verts) cudaFree(device.verts);
    if (device.blend_shapes) cudaFree(device.blend_shapes);
    if (device.joint_reg_dense) cudaFree(device.joint_reg_dense);
    if (device.joint_reg.values) cudaFree(device.joint_reg.values);
    if (device.joint_reg.inner) cudaFree(device.joint_reg.inner);
    if (device.joint_reg.outer) cudaFree(device.joint_reg.outer);
    if (device.weights.values) cudaFree(device.weights.values);
    if (device.weights.inner) cudaFree(device.weights.inner);
    if (device.weights.outer) cudaFree(device.weights.outer);
    if (device.hand_comps_l) cudaFree(device.hand_comps_l);
    if (device.hand_comps_r) cudaFree(device.hand_comps_r);
    if (device.hand_mean_l) cudaFree(device.hand_mean_l);
    if (device.hand_mean_r) cudaFree(device.hand_mean_r);
}

// Instantiation
template class Model<model_config::SMPL>;
template class Model<model_config::SMPL_v1>;
template class Model<model_config::SMPLH>;
template class Model<model_config::SMPLX>;
template class Model<model_config::SMPLXpca>;
template class Model<model_config::SMPLX_v1>;
template class Model<model_config::SMPLXpca_v1>;

}  // namespace smplx

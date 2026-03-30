#pragma once
#ifndef SMPLX_INTERNAL_SEQUENCE_MODEL_SPEC_1275AC79_D796_4D22_B1EF_6A3D1F920235
#define SMPLX_INTERNAL_SEQUENCE_MODEL_SPEC_1275AC79_D796_4D22_B1EF_6A3D1F920235

#include "smplx/smplx.hpp"

namespace smplx {

template <class SequenceConfig>
class Sequence;
namespace internal {

// Sadly, C++ does not allow specialization of templated members of template
// classes, so we have to implement in a separate struct; Also, C++14 doesn't
// allow specializing member structs, so we have to put it here (we are mixing
// C++14 (nvcc) with C++17
template <class SequenceConfig, class ModelConfig>
struct SequenceModelSpec {
    // Set shape
    static void set_shape(const Sequence<SequenceConfig>& seq,
                          Body<ModelConfig>& body) {
        throw std::invalid_argument(
            std::string("smplx::Sequence does not currently support model: ") +
            ModelConfig::model_name);
    }
    // Set pose and root transform
    static void set_pose(const Sequence<SequenceConfig>& seq,
                         Body<ModelConfig>& body, size_t frame) {
        throw std::invalid_argument(
            std::string("smplx::Sequence does not currently support model: ") +
            ModelConfig::model_name);
    }
};

// ** Per-model specializations **
template <class SequenceConfig>
struct SequenceModelSpec<SequenceConfig, model_config::SMPL> {
    static void set_shape(const Sequence<SequenceConfig>& seq,
                          Body<model_config::SMPL>& body) {
        body.shape().noalias() =
            seq.shape.template head<model_config::SMPL::n_shape_blends()>();
    }
    static void set_pose(const Sequence<SequenceConfig>& seq,
                         Body<model_config::SMPL>& body, size_t frame) {
        constexpr size_t n_common = SequenceConfig::n_body_joints() * 3;
        body.trans().noalias() = seq.trans.row(frame).transpose();
        body.pose().template head<n_common>().noalias() =
            seq.pose.row(frame).template head<n_common>().transpose();
        // Remaining joints assumed to already be set to 0
    }
};

template <class SequenceConfig>
struct SequenceModelSpec<SequenceConfig, model_config::SMPLH> {
    static void set_shape(const Sequence<SequenceConfig>& seq,
                          Body<model_config::SMPLH>& body) {
        body.shape().noalias() = seq.shape;
    }
    static void set_pose(const Sequence<SequenceConfig>& seq,
                         Body<model_config::SMPLH>& body, size_t frame) {
        body.trans().noalias() = seq.trans.row(frame).transpose();
        body.pose().noalias() = seq.pose.row(frame).transpose();
    }
};

// SMPLX specialization
// WARNING: set shape is not supported
template <class SequenceConfig>
struct SequenceModelSpec<SequenceConfig, model_config::SMPLX> {
    static void set_shape(const Sequence<SequenceConfig>& seq,
                          Body<model_config::SMPLX>& body) {
        // Shape space is not compatible, so we do nothing
    }
    static void set_pose(const Sequence<SequenceConfig>& seq,
                         Body<model_config::SMPLX>& body, size_t frame) {
        constexpr size_t n_body_common = SequenceConfig::n_body_joints() * 3;
        constexpr size_t n_hand_common = SequenceConfig::n_hand_joints() * 6;
        body.trans().noalias() = seq.trans.row(frame).transpose();
        body.pose().template head<n_body_common>().noalias() =
            seq.pose.row(frame).template head<n_body_common>().transpose();
        body.pose().template tail<n_hand_common>().noalias() =
            seq.pose.row(frame).template tail<n_hand_common>().transpose();
        // 3 remaining middle joints are face joints and are assumed to be set
        // to 0
    }
};

template <class SequenceConfig>
struct SequenceModelSpec<SequenceConfig, model_config::SMPLX_v1> {
    static void set_shape(const Sequence<SequenceConfig>& seq,
                          Body<model_config::SMPLX_v1>& body) {
        // Shape space is not compatible, so we do nothing
    }
    static void set_pose(const Sequence<SequenceConfig>& seq,
                         Body<model_config::SMPLX_v1>& body, size_t frame) {
        constexpr size_t n_body_common = SequenceConfig::n_body_joints() * 3;
        constexpr size_t n_hand_common = SequenceConfig::n_hand_joints() * 6;
        body.trans().noalias() = seq.trans.row(frame).transpose();
        body.pose().template head<n_body_common>().noalias() =
            seq.pose.row(frame).template head<n_body_common>().transpose();
        body.pose().template tail<n_hand_common>().noalias() =
            seq.pose.row(frame).template tail<n_hand_common>().transpose();
        // 3 remaining middle joints are face joints and are assumed to be set
        // to 0
    }
};

// NOTE: SMPLX with hand pca (SMPLXpca) is not supported
}  // namespace internal
}  // namespace smplx
#endif  // ifndef

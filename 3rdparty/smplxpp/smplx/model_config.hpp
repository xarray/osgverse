#pragma once
#ifndef SMPLX_MODEL_CONFIG_93EE8559_02EF_474A_B63C_C148E6F1F61A
#define SMPLX_MODEL_CONFIG_93EE8559_02EF_474A_B63C_C148E6F1F61A

#include <cstddef>

namespace smplx {
namespace model_config {

namespace internal {
template <class Derived>
struct ModelConfigBase {
    static constexpr size_t n_joints() {
        return Derived::n_explicit_joints() + Derived::n_hand_pca_joints() * 2;
    }
    static constexpr size_t n_params() {
        return 3 + Derived::n_explicit_joints() * 3 +
               Derived::n_hand_pca() * 2 + Derived::n_shape_blends();
    }
    static constexpr size_t n_pose_blends() { return 9 * (n_joints() - 1); }
    static constexpr size_t n_blend_shapes() {
        return Derived::n_shape_blends() + n_pose_blends();
    }
    static constexpr size_t n_hand_pca_joints() { return 0; }
    static constexpr size_t n_hand_pca() { return 0; }
};

template <class Derived>
struct SMPLXBase : public internal::ModelConfigBase<Derived> {
    static constexpr size_t n_verts() { return 10475; }
    static constexpr size_t n_faces() { return 20908; }
    static constexpr size_t parent[] = {
        0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  9,
        9,  12, 13, 14, 16, 17, 18, 19, 15, 15, 15, 20, 25, 26,
        20, 28, 29, 20, 31, 32, 20, 34, 35, 20, 37, 38, 21, 40,
        41, 21, 43, 44, 21, 46, 47, 21, 49, 50, 21, 52, 53};
    static constexpr const char* joint_name[] = {"pelvis",
                                                 "left_hip",
                                                 "right_hip",
                                                 "spine1",
                                                 "left_knee",
                                                 "right_knee",
                                                 "spine2",
                                                 "left_ankle",
                                                 "right_ankle",
                                                 "spine3",
                                                 "left_foot",
                                                 "right_foot",
                                                 "neck",
                                                 "left_collar",
                                                 "right_collar",
                                                 "head",
                                                 "left_shoulder",
                                                 "right_shoulder",
                                                 "left_elbow",
                                                 "right_elbow",
                                                 "left_wrist",
                                                 "right_wrist",
                                                 "jaw",
                                                 "left_eye_smplhf",
                                                 "right_eye_smplhf",
                                                 "left_index1",
                                                 "left_index2",
                                                 "left_index3",
                                                 "left_middle1",
                                                 "left_middle2",
                                                 "left_middle3",
                                                 "left_pinky1",
                                                 "left_pinky2",
                                                 "left_pinky3",
                                                 "left_ring1",
                                                 "left_ring2",
                                                 "left_ring3",
                                                 "left_thumb1",
                                                 "left_thumb2",
                                                 "left_thumb3",
                                                 "right_index1",
                                                 "right_index2",
                                                 "right_index3",
                                                 "right_middle1",
                                                 "right_middle2",
                                                 "right_middle3",
                                                 "right_pinky1",
                                                 "right_pinky2",
                                                 "right_pinky3",
                                                 "right_ring1",
                                                 "right_ring2",
                                                 "right_ring3",
                                                 "right_thumb1",
                                                 "right_thumb2",
                                                 "right_thumb3"};
    static constexpr const char* default_path_prefix = "models/smplx/SMPLX_";
    static constexpr const char* default_uv_path = "models/smplx/uv.txt";
};
}  // namespace internal

// SMPL-X V 1.1, has way more expression blendshapes
// SMPL-X with 6 PCA components/hand
struct SMPLXpca : public internal::SMPLXBase<SMPLXpca> {
    static constexpr size_t n_explicit_joints() { return 25; }
    static constexpr size_t n_hand_pca_joints() { return 15; }
    static constexpr size_t n_shape_blends() { return 400; }
    static constexpr size_t n_hand_pca() { return 6; }
    static constexpr const char* model_name = "SMPL-X v1.1 (with hand PCA)";
};

// SMPL-X with all hand joints explicitly parameterized (rather than via PCA)
struct SMPLX : public internal::SMPLXBase<SMPLX> {
    static constexpr size_t n_explicit_joints() { return SMPLXpca::n_joints(); }
    static constexpr size_t n_shape_blends() {
        return SMPLXpca::n_shape_blends();
    }
    static constexpr const char* model_name = "SMPL-X v1.1";
};

// -- OLD SMPLX v1.0 --
// SMPL-X with 6 PCA components/hand
struct SMPLXpca_v1 : public internal::SMPLXBase<SMPLXpca_v1> {
    static constexpr size_t n_explicit_joints() {
        return 25;
    }  // 3 facial joints
    static constexpr size_t n_hand_pca_joints() { return 15; }
    static constexpr size_t n_shape_blends() { return 20; }
    static constexpr size_t n_hand_pca() { return 6; }
    static constexpr const char* model_name = "SMPL-X v1.0 (with hand PCA)";
};

// SMPL-X with all hand joints explicitly parameterized (rather than via PCA)
struct SMPLX_v1 : public internal::SMPLXBase<SMPLX_v1> {
    static constexpr size_t n_explicit_joints() {
        return SMPLXpca_v1::n_joints();
    }
    static constexpr size_t n_shape_blends() {
        return SMPLXpca_v1::n_shape_blends();
    }
    static constexpr const char* model_name = "SMPL-X v1.0";
};

// SMPL+H with 16-param shape space and hand joints (rather than hand PCA), as
// used by AMASS
struct SMPLH : public internal::ModelConfigBase<SMPLH> {
    static constexpr size_t n_verts() { return 6890; }
    static constexpr size_t n_faces() { return 13776; }
    static constexpr size_t n_explicit_joints() {
        return 52;
    }  // No hands in body, no face
    static constexpr size_t n_shape_blends() { return 16; }
    static constexpr size_t parent[] = {
        0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  9,  9,  12, 13, 14,
        16, 17, 18, 19, 20, 22, 23, 20, 25, 26, 20, 28, 29, 20, 31, 32, 20, 34,
        35, 21, 37, 38, 21, 40, 41, 21, 43, 44, 21, 46, 47, 21, 49, 50};
    static constexpr const char* joint_name[] = {
        "pelvis",        "left_hip",       "right_hip",     "spine1",
        "left_knee",     "right_knee",     "spine2",        "left_ankle",
        "right_ankle",   "spine3",         "left_foot",     "right_foot",
        "neck",          "left_collar",    "right_collar",  "head",
        "left_shoulder", "right_shoulder", "left_elbow",    "right_elbow",
        "left_wrist",    "right_wrist",    "left_index1",   "left_index2",
        "left_index3",   "left_middle1",   "left_middle2",  "left_middle3",
        "left_pinky1",   "left_pinky2",    "left_pinky3",   "left_ring1",
        "left_ring2",    "left_ring3",     "left_thumb1",   "left_thumb2",
        "left_thumb3",   "right_index1",   "right_index2",  "right_index3",
        "right_middle1", "right_middle2",  "right_middle3", "right_pinky1",
        "right_pinky2",  "right_pinky3",   "right_ring1",   "right_ring2",
        "right_ring3",   "right_thumb1",   "right_thumb2",  "right_thumb3"};
    static constexpr const char* model_name = "SMPL+H";
    static constexpr const char* default_path_prefix = "models/smplh/SMPLH_";
    static constexpr const char* default_uv_path =
        "models/smplh/uv.txt";  // Not available
};

// Classic SMPL model
struct SMPL_v1 : public internal::ModelConfigBase<SMPL_v1> {
    static constexpr size_t n_verts() { return 6890; }
    static constexpr size_t n_faces() { return 13776; }
    static constexpr size_t n_explicit_joints() { return 24; }
    static constexpr size_t n_shape_blends() { return 10; }
    static constexpr size_t parent[] = {0,  0,  0,  0,  1,  2,  3,  4,
                                        5,  6,  7,  8,  9,  9,  9,  12,
                                        13, 14, 16, 17, 18, 19, 20, 21};
    static constexpr const char* joint_name[] = {
        "pelvis",        "left_hip",       "right_hip",    "spine1",
        "left_knee",     "right_knee",     "spine2",       "left_ankle",
        "right_ankle",   "spine3",         "left_foot",    "right_foot",
        "neck",          "left_collar",    "right_collar", "head",
        "left_shoulder", "right_shoulder", "left_elbow",   "right_elbow",
        "left_wrist",    "right_wrist",    "left_hand",    "right_hand"};
    static constexpr const char* model_name = "SMPL";
    static constexpr const char* default_path_prefix = "models/smpl/SMPL_";
    static constexpr const char* default_uv_path = "models/smpl/uv.txt";
};

// SMPL 1.1
struct SMPL : public internal::ModelConfigBase<SMPL> {
    static constexpr size_t n_verts() { return 6890; }
    static constexpr size_t n_faces() { return 13776; }
    static constexpr size_t n_explicit_joints() { return 24; }
    static constexpr size_t n_shape_blends() { return 300; }
    static constexpr size_t parent[] = {0,  0,  0,  0,  1,  2,  3,  4,
                                        5,  6,  7,  8,  9,  9,  9,  12,
                                        13, 14, 16, 17, 18, 19, 20, 21};
    static constexpr const char* joint_name[] = {
        "pelvis",        "left_hip",       "right_hip",    "spine1",
        "left_knee",     "right_knee",     "spine2",       "left_ankle",
        "right_ankle",   "spine3",         "left_foot",    "right_foot",
        "neck",          "left_collar",    "right_collar", "head",
        "left_shoulder", "right_shoulder", "left_elbow",   "right_elbow",
        "left_wrist",    "right_wrist",    "left_hand",    "right_hand"};
    static constexpr const char* model_name = "SMPL";
    static constexpr const char* default_path_prefix = "models/smpl/SMPL_";
    static constexpr const char* default_uv_path = "models/smpl/uv.txt";
};

}  // namespace model_config
}  // namespace smplx

#endif  // ifndef SMPLX_MODEL_CONFIG_93EE8559_02EF_474A_B63C_C148E6F1F61A

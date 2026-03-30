#pragma once
#ifndef SMPLX_SMPLX_3F77A808_CB46_4AF6_A5FD_70CF554F8871
#define SMPLX_SMPLX_3F77A808_CB46_4AF6_A5FD_70CF554F8871

#include "smplx/defs.hpp"
#include "smplx/model_config.hpp"

#include <string>
#include <vector>

#define __SMPLX_MEMBER_ACCESSOR(name, body) \
    inline auto name() { return body; }     \
    inline auto name() const { return body; }

namespace smplx {
#ifdef SMPLX_CUDA_ENABLED
namespace internal {
// Basic CSR sparse matrix repr
struct GPUSparseMatrix {
    float* values = nullptr;
    int* inner = nullptr;
    int* outer = nullptr;
    int rows, cols, nnz;
};
}  // namespace internal
#endif

/** Represents a generic SMPL-like human model.
 *  This contains the base shape/mesh/LBS weights of a SMPL-type
 *  model and handles loading from a standard SMPL-X npz file.
 *
 *  The loaded model can be passed to the Body<ModelConfig> class constructor.
 *  The Body class  can then generate a skinned human mesh from parameters using
 *  the model's data.
 *
 *  template arg ModelConfig is the static 'model configuration', which you
 *  should pick from smplx::model_config::SMPL/SMPLH/SMPLX */
template <class ModelConfig>
class Model {
   public:
    // Construct from .npz at default path for given gender, in
    // data/models/modelname/MODELNAME_GENDER.npz
    explicit Model(Gender gender = Gender::neutral);

    // Construct from .npz at path (standard SMPL-X npz format)
    // path: .npz model path, e.g. data/models/smplx/*.npz
    // uv_path: UV map information path, see data/models/smplx/uv.txt for an
    // example gender: records gender of model. For informational purposes only.
    explicit Model(const std::string& path, const std::string& uv_path = "",
                   Gender gender = Gender::unknown);
    // Destructor
    ~Model();

    // Disable copy/move assignment
    Model& operator=(const Model& other) = delete;
    Model& operator=(Model&& other) = delete;

    /*** MODEL NPZ LOADING ***/
    // Load from .npz at default path for given gender
    // useful for dynamically switching genders
    void load(Gender gender = Gender::neutral);
    // Load from .npz at path (standard SMPL-X npz format)
    // path: .npz model path, in data/models/smplx/*.npz
    // uv_path: UV map information path, see data/models/smplx/uv.txt for an
    // example gender: records gender of model. For informational purposes only.
    void load(const std::string& path, const std::string& uv_path = "",
              Gender new_gender = Gender::unknown);

    /*** MODEL MANIPULATION ***/
    // Set model deformations: verts := verts_load + d
    void set_deformations(const Eigen::Ref<const Points>& d);

    // Set model template: verts := t
    void set_template(const Eigen::Ref<const Points>& t);

    using Config = ModelConfig;

    /*** STATIC DATA SHAPE INFO SHORTHANDS,
     *   mostly forwarding to ModelConfig ***/

    // Number of vertices in model
    static constexpr size_t n_verts() { return Config::n_verts(); }
    // Number of faces in model
    static constexpr size_t n_faces() { return Config::n_faces(); }

    // Total number of joints = n_explicit_joints + n_hand_pca_joints * 2
    static constexpr size_t n_joints() { return Config::n_joints(); }
    // Number of explicit joint parameters stored as angle-axis
    static constexpr size_t n_explicit_joints() {
        return Config::n_explicit_joints();
    }
    // Number of joints per hand implicit computed from PCA
    static constexpr size_t n_hand_pca_joints() {
        return Config::n_hand_pca_joints();
    }

    // Total number of blend shapes = n_shape_blends + n_pose_blends
    static constexpr size_t n_blend_shapes() {
        return Config::n_blend_shapes();
    }
    // Number of shape-dep blend shapes, including body and face
    static constexpr size_t n_shape_blends() {
        return Config::n_shape_blends();
    }
    // Number of pose-dep blend shapes = 9 * (n_joints - 1)
    static constexpr size_t n_pose_blends() { return Config::n_pose_blends(); }

    // Number of PCA components for each hand
    static constexpr size_t n_hand_pca() { return Config::n_hand_pca(); }

    // Total number of params = 3 + 3 * n_body_joints + 2 * n_hand_pca +
    static constexpr size_t n_params() { return Config::n_params(); }

    // Number UV vertices (may be more than n_verts due to seams)
    // 0 if UV not available
    // NOTE: not static or a constexpr
    inline size_t n_uv_verts() const { return _n_uv_verts; }

    /*** ADDITIONAL MODEL INFORMATION ***/
    // Model name
    static constexpr const char* name() { return Config::model_name; }
    // Joint names name
    static constexpr const char* joint_name(size_t joint) {
        return Config::joint_name[joint];
    }
    // Parent joint
    static constexpr size_t parent(size_t joint) {
        return Config::parent[joint];
    }

    // Model gender, may be unknown.
    Gender gender;

    // Returns true if has UV map.
    // Note: not static, since we allow UV map variation among model instances.
    inline bool has_uv_map() const { return _n_uv_verts > 0; }

    /*** MODEL DATA ***/
    // Kinematic tree: joint children
    std::vector<std::vector<size_t>> children;

    // Vertices in the unskinned mesh, (#verts, 3).
    // This is verts_load with deformations (set with set_deformations).
    Points verts;

    // Vertices in the initial loaded mesh, (#verts, 3)
    Points verts_load;

    // Triangles in the mesh, (#faces, 3)
    Triangles faces;

    // Initial joint positions
    Points joints;

    // Shape- and pose-dependent blend shapes,
    // (3*#verts, #shape blends + #pose blends)
    // each col represents a point cloud (#verts, 3) in row-major order
    Eigen::Matrix<Scalar, Eigen::Dynamic, Model::n_blend_shapes()> blend_shapes;

    // Joint regressor: verts -> joints, (#joints, #verts)
    SparseMatrix joint_reg;

    // LBS weights, (#verts, #joints).
    // NOTE: this is ColMajor because I notice a speedup while profiling
    SparseMatrixColMajor weights;

    /*** Hand PCA data ***/
    // Hand PCA comps: pca -> joint pos delta
    // 3*#hand joints (=45) * #hand pca
    // columns are PC's
    Matrix hand_comps_l, hand_comps_r;
    // Hand PCA means: mean pos of 3x15 hand joints
    Vector hand_mean_l, hand_mean_r;

    /*** UV Data , available if has_uv_map() ***/
    // UV coordinates, size (n_uv_verts, 2)
    Points2D uv;
    // UV triangles (indices in uv), size (n_faces, 3)
    Triangles uv_faces;

#ifdef SMPLX_CUDA_ENABLED
    /*** ADVANCED: GPU data pointers ***/
    struct {
        float* verts = nullptr;
        float* blend_shapes = nullptr;
        internal::GPUSparseMatrix joint_reg;
        float* joint_reg_dense;
        internal::GPUSparseMatrix weights;
        float *hand_comps_l = nullptr, *hand_comps_r = nullptr;
        float *hand_mean_l = nullptr, *hand_mean_r = nullptr;
    } device;

   private:
    void _cuda_load();
    void _cuda_copy_template();
    void _cuda_free();
#else
   private:
#endif
    // Number UV vertices (may be more than n_verts due to seams)
    // 0 if UV not available
    size_t _n_uv_verts;
};
// SMPL Model
using ModelS = Model<model_config::SMPL>;
// SMPL+H Model
using ModelH = Model<model_config::SMPLH>;
// SMPL-X Model with hand joint rotations
using ModelX = Model<model_config::SMPLX>;
// SMPL-X Model with hand PCA
using ModelXpca = Model<model_config::SMPLXpca>;

/** A particular SMPL instance constructed from a Model<ModelConfig>,
 *  storing pose/shape/hand parameters and a skinned point cloud generated
 *  from the parameters (via calling the update method).
 *  Implements linear blend skinning with GPU and CPU (Eigen) support. */
template <class ModelConfig>
class Body {
   public:
    // Construct body from model
    // set_zero: set to false to leave parameter array uninitialized
    explicit Body(const Model<ModelConfig>& model, bool set_zero = true);
    ~Body();

    // Perform LBS and output verts
    // enable_pose_blendshapes: if false, disables pose blendshapes;
    //                          this provides a significant speedup at the cost
    //                          of worse accuracy
    void update(bool force_cpu = false, bool enable_pose_blendshapes = true);

    // Save as obj file
    void save_obj(const std::string& path) const;

    using Config = ModelConfig;

    // Parameter accessors (maps to parts of params)
    // Base position (translation)
    __SMPLX_MEMBER_ACCESSOR(trans, params.template head<3>());
    // Pose (angle-axis)
    __SMPLX_MEMBER_ACCESSOR(
        pose, params.template segment<ModelConfig::n_explicit_joints() * 3>(3));
    // Hand principal component weights
    __SMPLX_MEMBER_ACCESSOR(hand_pca,
                            params.segment<ModelConfig::n_hand_pca() * 2>(
                                3 + 3 * model.n_explicit_joints()));
    __SMPLX_MEMBER_ACCESSOR(hand_pca_l,
                            params.template segment<ModelConfig::n_hand_pca()>(
                                3 + 3 * model.n_explicit_joints()));
    __SMPLX_MEMBER_ACCESSOR(hand_pca_r,
                            params.template segment<ModelConfig::n_hand_pca()>(
                                3 + 3 * model.n_explicit_joints() +
                                model.n_hand_pca()));
    // Shape params
    __SMPLX_MEMBER_ACCESSOR(
        shape, params.template tail<ModelConfig::n_shape_blends()>());

    // * OUTPUTS accessors
    // Get shaped + posed body vertices, in same order as model.verts;
    // must call update() before this is available
    const Points& verts() const;

    // Get shaped (but not posed) body vertices, in same order as model.verts;
    // must call update() before this is available
    const Points& verts_shaped() const;

    // Get deformed body joints, in same order as model.joints;
    // must call update() before this is available
    const Points& joints() const;

    // Get homogeneous transforms at each joint. (n_joints, 12).
    // Each row is a row-major (3, 4) rigid body transform matrix,
    // canonical -> posed space.
    const Eigen::Matrix<Scalar, Eigen::Dynamic, 12, Eigen::RowMajor>&
    joint_transforms() const;

    // Get homogeneous transforms at each vertex. (n_verts, 12).
    // Each row is a row-major (3, 4) rigid body transform matrix,
    // canonical -> posed space.
    const Eigen::Matrix<Scalar, Eigen::Dynamic, 12, Eigen::RowMajor>&
    vert_transforms() const;

    // Set parameters to zero
    inline void set_zero() { params.setZero(); }

    // Set parameters uar in [-0.25, 0.25]
    inline void set_random() { params.setRandom() * 0.25; }

    // The SMPL model used
    const Model<ModelConfig>& model;

    // * INPUTS
    // Parameters vector
    Vector params;

   private:
    // * OUTPUTS generated by update
    // Deformed vertices (only shape applied); not available in case of GPU
    // (only device.verts_shaped)
    mutable Points _verts_shaped;

    // Deformed vertices (shape and pose applied)
    mutable Points _verts;

    // Deformed joints (only shape applied)
    Points _joints_shaped;

    // Homogeneous transforms at each joint (bottom row omitted)
    Eigen::Matrix<Scalar, Eigen::Dynamic, 12, Eigen::RowMajor>
        _joint_transforms;

    // Homogeneous transforms at each vertex (bottom row omitted)
    mutable Eigen::Matrix<Scalar, Eigen::Dynamic, 12, Eigen::RowMajor>
        _vert_transforms;

    // Deformed joints (shape and pose applied)
    mutable Points _joints;

    // Transform local to global coordinates
    // Inputs: trans(), _joints_shaped
    // Outputs: _joints
    // Input/output: _joint_transforms
    void _local_to_global();

#ifdef SMPLX_CUDA_ENABLED
   public:
    struct {
        float* verts = nullptr;
        float* verts_shaped = nullptr;
        float* joints_shaped = nullptr;
        // Internal temp
        float* verts_tmp = nullptr;
        // Internal (#total blend shapes) rm
        float* blendshape_params = nullptr;
        // Internal (#joints, 12) rm
        float* joint_transforms = nullptr;
    } device;

   private:
    // True if latest posed vertices constructed by update()
    // have been retrieved to main memory
    mutable bool _verts_retrieved;
    // True if latest shaped, unposed vertices constructed by update()
    // have been retrieved to main memory
    mutable bool _verts_shaped_retrieved;
    // True if last update made use of the GPU
    bool _last_update_used_gpu;
    // Cuda helpers
    void _cuda_load();
    void _cuda_free();
    void _cuda_maybe_retrieve_verts() const;
    void _cuda_maybe_retrieve_verts_shaped() const;
    void _cuda_update(float* h_blendshape_params, float* h_joint_transforms,
                      bool enable_pose_blendshapes = true);
#endif
};
// SMPL Body
using BodyS = Body<model_config::SMPL>;
// SMPL-H Body
using BodyH = Body<model_config::SMPLH>;
// SMPL-X Body with hand joint rotations
using BodyX = Body<model_config::SMPLX>;
// SMPL-X Body with hand PCA
using BodyXpca = Body<model_config::SMPLXpca>;

}  // namespace smplx

#endif  // ifndef SMPLX_SMPLX_3F77A808_CB46_4AF6_A5FD_70CF554F8871

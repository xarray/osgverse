#include "smplx/smplx.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <cnpy.h>

#include "smplx/util.hpp"
#include "smplx/util_cnpy.hpp"
#include "smplx/version.hpp"

namespace smplx {
namespace {
using util::assert_shape;
}  // namespace

template <class ModelConfig>
Model<ModelConfig>::Model(Gender gender) {
    load(gender);
}

template <class ModelConfig>
Model<ModelConfig>::Model(const std::string& path, const std::string& uv_path,
                          Gender gender) {
    load(path, uv_path, gender);
}

template <class ModelConfig>
Model<ModelConfig>::~Model() {
#ifdef SMPLX_CUDA_ENABLED
    _cuda_free();
#endif
}

template <class ModelConfig>
void Model<ModelConfig>::load(Gender gender) {
    load(util::find_data_file(std::string(ModelConfig::default_path_prefix) +
                              util::gender_to_str(gender) + ".npz"),
         util::find_data_file(ModelConfig::default_uv_path), gender);
}

template <class ModelConfig>
void Model<ModelConfig>::load(const std::string& path,
                              const std::string& uv_path, Gender new_gender) {
    if (!std::ifstream(path)) {
        std::cerr << "ERROR: Model '" << path
                  << "' does not exist, "
                     "did you download the model following instructions in "
                     "https://github.com/sxyu/smplxpp/tree/master/data/models/"
                     "README.md?\n";
        return;
    }
    gender = new_gender;
    cnpy::npz_t npz = cnpy::npz_load(path);

    // Load kintree
    children.resize(n_joints());
    for (size_t i = 1; i < n_joints(); ++i) {
        children[ModelConfig::parent[i]].push_back(i);
    }

    // Load base template
    const auto& verts_raw = npz.at("v_template");
    assert_shape(verts_raw, {n_verts(), 3});
    verts.noalias() = util::load_float_matrix(verts_raw, n_verts(), 3);
    verts_load.noalias() = verts;

    // Load triangle mesh
    const auto& faces_raw = npz.at("f");
    assert_shape(faces_raw, {n_faces(), 3});
    faces = util::load_uint_matrix(faces_raw, n_faces(), 3);

    // Load joint regressor
    const auto& jreg_raw = npz.at("J_regressor");
    assert_shape(jreg_raw, {n_joints(), n_verts()});
    joint_reg.resize(n_joints(), n_verts());
    joint_reg =
        util::load_float_matrix(jreg_raw, n_joints(), n_verts()).sparseView();
    joints = joint_reg * verts;
    joint_reg.makeCompressed();

    // Load LBS weights
    const auto& wt_raw = npz.at("weights");
    assert_shape(wt_raw, {n_verts(), n_joints()});
    weights.resize(n_verts(), n_joints());
    weights =
        util::load_float_matrix(wt_raw, n_verts(), n_joints()).sparseView();
    weights.makeCompressed();

    blend_shapes.resize(3 * n_verts(), n_blend_shapes());
    // Load shape-dep blend shapes
    const auto& sb_raw = npz.at("shapedirs");
    assert_shape(sb_raw, {n_verts(), 3, n_shape_blends()});
    blend_shapes.template leftCols<n_shape_blends()>().noalias() =
        util::load_float_matrix(sb_raw, 3 * n_verts(), n_shape_blends());

    // Load pose-dep blend shapes
    const auto& pb_raw = npz.at("posedirs");
    assert_shape(pb_raw, {n_verts(), 3, n_pose_blends()});
    blend_shapes.template rightCols<n_pose_blends()>().noalias() =
        util::load_float_matrix(pb_raw, 3 * n_verts(), n_pose_blends());

    if (n_hand_pca() && npz.count("hands_meanl") && npz.count("hands_meanr")) {
        // Model has hand PCA (e.g. SMPLXpca), load hand PCA
        const auto& hml_raw = npz.at("hands_meanl");
        const auto& hmr_raw = npz.at("hands_meanr");
        const auto& hcl_raw = npz.at("hands_componentsl");
        const auto& hcr_raw = npz.at("hands_componentsr");

        assert_shape(hml_raw, {util::ANY_SHAPE});
        assert_shape(hmr_raw, {hml_raw.shape[0]});

        size_t n_hand_params = hml_raw.shape[0];
        _SMPLX_ASSERT_EQ(n_hand_params, n_hand_pca_joints() * 3);

        assert_shape(hcl_raw, {n_hand_params, n_hand_params});
        assert_shape(hcr_raw, {n_hand_params, n_hand_params});

        hand_mean_l = util::load_float_matrix(hml_raw, n_hand_params, 1);
        hand_mean_r = util::load_float_matrix(hmr_raw, n_hand_params, 1);

        hand_comps_l =
            util::load_float_matrix(hcl_raw, n_hand_params, n_hand_params)
                .topRows(n_hand_pca())
                .transpose();
        hand_comps_r =
            util::load_float_matrix(hcr_raw, n_hand_params, n_hand_params)
                .topRows(n_hand_pca())
                .transpose();
    }

    // Maybe load UV (UV mapping WIP)
    if (uv_path.size()) {
        std::ifstream ifs(uv_path);
        ifs >> _n_uv_verts;
        if (_n_uv_verts) {
            if (ifs) {
                // _SMPLX_ASSERT_LE(n_verts(), _n_uv_verts);
                // Load the uv data
                uv.resize(_n_uv_verts, 2);
                for (size_t i = 0; i < _n_uv_verts; ++i)
                    ifs >> uv(i, 0) >> uv(i, 1);
                _SMPLX_ASSERT(ifs);
                uv_faces.resize(n_faces(), 3);
                for (size_t i = 0; i < n_faces(); ++i) {
                    _SMPLX_ASSERT(ifs);
                    for (size_t j = 0; j < 3; ++j) {
                        ifs >> uv_faces(i, j);
                        // Make indices 0-based
                        --uv_faces(i, j);
                        _SMPLX_ASSERT_LT(uv_faces(i, j), _n_uv_verts);
                    }
                }
            }
        }
    }
#ifdef SMPLX_CUDA_ENABLED
    _cuda_load();
#endif
}

template <class ModelConfig>
void Model<ModelConfig>::set_deformations(const Eigen::Ref<const Points>& d) {
    verts.noalias() = verts_load + d;
#ifdef SMPLX_CUDA_ENABLED
    _cuda_copy_template();
#endif
}

template <class ModelConfig>
void Model<ModelConfig>::set_template(const Eigen::Ref<const Points>& t) {
    verts.noalias() = t;
#ifdef SMPLX_CUDA_ENABLED
    _cuda_copy_template();
#endif
}

// Instantiations
template class Model<model_config::SMPL>;
template class Model<model_config::SMPL_v1>;
template class Model<model_config::SMPLH>;
template class Model<model_config::SMPLX>;
template class Model<model_config::SMPLXpca>;
template class Model<model_config::SMPLX_v1>;
template class Model<model_config::SMPLXpca_v1>;

}  // namespace smplx

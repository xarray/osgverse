#include "smplx/sequence.hpp"

#include <fstream>
#include <iostream>
#include <cnpy.h>
#include "smplx/util.hpp"
#include "smplx/util_cnpy.hpp"

namespace smplx {

namespace {
using util::assert_shape;
}  // namespace

// AMASS npz structure
// 'trans':           (#frames, 3)
// 'gender':          str
// 'mocap_framerate': float
// 'betas':           (16)              first 10 are from the usual SMPL
// 'dmpls':           (#frames, 8)      soft tissue
// 'poses':           (#frames, 156)    first 66 are SMPL joint parameters
// excluding hand.
//                                      last 90 are MANO joint parameters, which
//                                          correspond to last 90 joints in
//                                          SMPL-X (NOT hand PCA)
//
template <class SequenceConfig>
Sequence<SequenceConfig>::Sequence(const std::string& path) {
    if (path.size()) {
        load(path);
    } else {
        n_frames = 0;
        gender = Gender::neutral;
    }
}

template <class SequenceConfig>
bool Sequence<SequenceConfig>::load(const std::string& path) {
    if (!std::ifstream(path)) {
        n_frames = 0;
        gender = Gender::unknown;
        std::cerr << "WARNING: Sequence '" << path
                  << "' does not exist, loaded empty sequence\n";
        return false;
    }
    // ** READ NPZ **
    cnpy::npz_t npz = cnpy::npz_load(path);

    if (npz.count("trans") != 1 || npz.count("poses") != 1 ||
        npz.count("betas") != 1) {
        n_frames = 0;
        gender = Gender::unknown;
        std::cerr << "WARNING: Sequence '" << path
                  << "' is invalid, loaded empty sequence\n";
        return false;
    }
    auto& trans_raw = npz["trans"];
    assert_shape(trans_raw, {util::ANY_SHAPE, 3});
    n_frames = trans_raw.shape[0];
    trans = util::load_float_matrix(trans_raw, n_frames, 3);

    auto& poses_raw = npz["poses"];
    assert_shape(poses_raw, {n_frames, SequenceConfig::n_pose_params()});
    pose = util::load_float_matrix(poses_raw, n_frames,
                                   SequenceConfig::n_pose_params());

    auto& shape_raw = npz["betas"];
    assert_shape(shape_raw, {SequenceConfig::n_shape_params()});
    shape =
        util::load_float_matrix(poses_raw, SequenceConfig::n_shape_params(), 1);

    if (SequenceConfig::n_dmpls() && npz.count("dmpls") == 1) {
        auto& dmpls_raw = npz["dmpls"];
        assert_shape(dmpls_raw, {n_frames, SequenceConfig::n_dmpls()});
        dmpls = util::load_float_matrix(poses_raw, n_frames,
                                        SequenceConfig::n_dmpls());
    }

    if (npz.count("gender")) {
        char gender_spec = npz["gender"].data_holder[0];
        gender =
            gender_spec == 'f'
                ? Gender::female
                : gender_spec == 'm'
                      ? Gender::male
                      : gender_spec == 'n' ? Gender::neutral : Gender::unknown;
    } else {
        // Default to neutral
        std::cerr << "WARNING: gender not present in '" << path
                  << "', using neutral\n";
        gender = Gender::neutral;
    }

    if (npz.count("mocap_framerate")) {
        auto& mocap_frate_raw = npz["mocap_framerate"];
        if (mocap_frate_raw.word_size == 8)
            frame_rate = *mocap_frate_raw.data<double>();
        else if (mocap_frate_raw.word_size == 4)
            frame_rate = *mocap_frate_raw.data<float>();
    } else {
        // Reasonable default
        std::cerr << "WARNING: mocap_framerate not present in '" << path
                  << "', assuming 120 FPS\n";
        frame_rate = 120.f;
    }
    return true;
}

// Instantiation
template class Sequence<sequence_config::AMASS>;

}  // namespace smplx

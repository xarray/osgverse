#pragma once
#ifndef SMPLX_SEQUENCE_CONFIG_22F8D090_05E0_4DF5_A962_042D5BAD0E17
#define SMPLX_SEQUENCE_CONFIG_22F8D090_05E0_4DF5_A962_042D5BAD0E17

#include <cstddef>

namespace smplx {
namespace sequence_config {

namespace internal {
template<class Derived>
struct SequenceConfigBase {
    static constexpr size_t n_pose_params() {
        return (Derived::n_body_joints() + Derived::n_hand_joints() * 2) * 3;
    }
    // 0 to disable
    static constexpr size_t n_dmpls() { return 0; }
};
}  // namespace internal

struct AMASS : public internal::SequenceConfigBase<AMASS> {
    static constexpr size_t n_shape_params() { return 16; }
    static constexpr size_t n_body_joints() { return 22; }
    static constexpr size_t n_hand_joints() { return 15; }
    static constexpr size_t n_dmpls() { return 8; }
};

}  // namespace sequence_config
}  // namespace smplx

#endif  // ifndef SMPLX_SEQUENCE_CONFIG_22F8D090_05E0_4DF5_A962_042D5BAD0E17

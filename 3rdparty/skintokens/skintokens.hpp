#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <tl/expected.hpp>
#include <span.hpp>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#if defined(SKINTOKENS_SHARED)
#  if defined(_WIN32) && !defined(__MINGW32__)
#    if defined(SKINTOKENS_BUILD)
#      define SKINTOKENS_API __declspec(dllexport)
#    else
#      define SKINTOKENS_API __declspec(dllimport)
#    endif
#  else
#    define SKINTOKENS_API __attribute__((visibility("default")))
#  endif
#else
#  define SKINTOKENS_API
#endif

namespace skintokens {

inline constexpr std::string_view version = "0.1.0";

enum class error_code : std::uint8_t {
    invalid_argument,
    invalid_format,
    limit_exceeded,
    io,
    incompatible_model,
    backend_unavailable,
    allocation,
    compute,
};

struct error {
    error_code code = error_code::invalid_argument;
    std::string message;
};

template<class T> using result = tl::expected<T, error>;

struct vec3 {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
};

struct quat {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
    float w = 1.0F;
};

struct color4 {
    float r = 1.0F;
    float g = 1.0F;
    float b = 1.0F;
    float a = 1.0F;
};

using triangle = std::array<std::uint32_t, 3>;

struct mesh {
    std::vector<vec3> vertices;
    std::vector<vec3> normals;
    // Linear RGBA vertex colours. Trellis' standard portable GLB export uses
    // COLOR_0, so retaining this stream keeps its generated appearance.
    std::vector<color4> colors;
    std::vector<triangle> faces;
    float metallic_factor = 0.0F;
    float roughness_factor = 0.72F;
    bool double_sided = false;
};

struct skeleton {
    std::vector<std::string> names;
    std::vector<std::int32_t> parents;
    std::vector<vec3> rest_positions;
};

struct motion {
    std::size_t frames = 0;
    float frames_per_second = 30.0F;
    skeleton rig;
    std::vector<vec3> root_translations;
    std::vector<quat> local_rotations; // [frame, joint]
};

enum class rig_kind : std::uint8_t { unknown, soma30, mixamo52 };

// How a separately supplied skeleton is placed into a mesh coordinate system.
// `global_similarity` preserves every relative joint position. `articulated`
// may instead adopt the clip's own first frame as the rest pose, and reposes
// recognized limb chains that still miss the mesh. Both preserve bone lengths.
enum class skeleton_fit : std::uint8_t { none, global_similarity, articulated };

struct glb_info {
    bool has_mesh = false;
    bool has_skin = false;
    bool has_skeleton = false;
    bool has_animation = false;
    std::size_t joint_count = 0;
    std::size_t frame_count = 0;
    float frames_per_second = 0.0F;
    rig_kind rig = rig_kind::unknown;
};

struct retarget_report {
    std::size_t isolated_cases = 0;
    float isolated_mean_position_error = 0.0F;
    float isolated_max_position_error = 0.0F;
    float motion_mean_position_error = 0.0F;
    float motion_max_position_error = 0.0F;
    float foot_velocity_relative_error = 0.0F;
};

enum class finger_transfer : std::uint8_t { neutral, map_soma_endpoints };

enum class semantic_role : std::uint8_t {
    unmapped,
    hips,
    spine1,
    spine2,
    chest,
    neck,
    head,
    left_shoulder,
    left_upper_arm,
    left_forearm,
    left_hand,
    right_shoulder,
    right_upper_arm,
    right_forearm,
    right_hand,
    left_upper_leg,
    left_shin,
    left_foot,
    left_toe,
    right_upper_leg,
    right_shin,
    right_foot,
    right_toe,
};

struct humanoid_match_options {
    float minimum_confidence = 0.80F;
    bool allow_flexible_hands = true;
};

struct retarget_options {
    float minimum_confidence = 0.80F;
    finger_transfer fingers = finger_transfer::neutral;
    bool scale_root_motion = true;
};

struct humanoid_match_report {
    float confidence = 0.0F;
    float core_score = 0.0F;
    float alternative_margin = 0.0F;
    std::size_t mapped_core_joints = 0;
    std::size_t ignored_terminal_joints = 0;
};

struct motion_divergence_report {
    float mean_normalized_displacement_error = 0.0F;
    float maximum_normalized_displacement_error = 0.0F;
    std::size_t worst_frame = 0;
    std::uint32_t worst_soma30_joint = 0;
    std::array<float, 30> per_joint_maximum{};
};

struct humanoid_mapping {
    // Fixed SOMA30 order. A negative value means that the source joint has no
    // generated-rig counterpart and its track is intentionally ignored.
    std::array<std::int32_t, 30> soma_to_generated{};
    std::vector<semantic_role> generated_roles;
    humanoid_match_report report;
};

struct skin {
    skeleton rig;
    // Four normalized influences per source vertex, ready for glTF JOINTS_0
    // and WEIGHTS_0. TokenRig's dense matrix is intentionally not retained.
    std::vector<std::array<std::uint16_t, 4>> joints;
    std::vector<std::array<float, 4>> weights;
    // False denotes the deterministic geometric integration baseline. It is
    // never presented as learned TokenRig output.
    bool learned = false;
};

struct skinned_asset {
    mesh geometry;
    skin binding;
    motion animation;
};

enum class device_kind : std::uint8_t { automatic, cpu, vulkan };

struct runtime_options {
    device_kind device = device_kind::automatic;
    std::uint32_t threads = 0;
    std::filesystem::path backend_directory;
};

struct generation_options {
    std::uint64_t seed = 0;
    std::uint32_t top_k = 5;
    float top_p = 0.95F;
    float temperature = 1.0F;
    float repetition_penalty = 2.0F;
    std::uint32_t beams = 10;
    std::size_t max_tokens = 2048;
    // Diagnostic integration fallback; learned TokenRig/SkinVAE is the default.
    bool geometric_only = false;
    // Optional upstream voxel_skin heuristic. The released demo leaves this
    // disabled by default; raw learned weights are its normal export path.
    bool surface_postprocess = false;
};

class SKINTOKENS_API model final {
public:
    model(model &&) noexcept;
    model & operator=(model &&) noexcept;
    model(const model &) = delete;
    model & operator=(const model &) = delete;
    ~model();

    [[nodiscard]] static result<model> load(
        const std::filesystem::path & bundle,
        const runtime_options & options = {});

    [[nodiscard]] result<skin> rig(
        const mesh & source,
        const generation_options & options = {}) const;

    // Preferred Kimodo integration: the supplied skeleton is emitted as a
    // constrained TokenRig prefix, so inference generates skin weights only.
    [[nodiscard]] result<skin> bind(
        const mesh & source,
        const skeleton & target,
        const generation_options & options = {}) const;

    [[nodiscard]] std::string_view backend_name() const noexcept;

private:
    struct impl;
    explicit model(std::unique_ptr<impl>) noexcept;
    std::unique_ptr<impl> impl_;
};

[[nodiscard]] SKINTOKENS_API result<mesh> load_glb_file(
    const std::filesystem::path & path);

// Reads trellis2cpp's versioned persisted mesh format directly. Coordinates
// and PBR vertex colours are converted exactly as its portable GLB exporter.
[[nodiscard]] SKINTOKENS_API result<mesh> load_trellis_mesh_file(
    const std::filesystem::path & path);

[[nodiscard]] SKINTOKENS_API result<motion> load_kimodo_glb_file(
    const std::filesystem::path & path);

// Loads the joint hierarchy and rest pose from a glTF skin. Animation is
// optional; static armatures are represented by one identity rest frame.
[[nodiscard]] SKINTOKENS_API result<motion> load_skeleton_glb_file(
    const std::filesystem::path & path);

// Loads mesh geometry, JOINTS_0/WEIGHTS_0, skeleton, and optional animation
// from one bounded GLB. This enables model-independent motion retargeting of a
// previously generated SkinTokens asset.
[[nodiscard]] SKINTOKENS_API result<skinned_asset> load_skinned_glb_file(
    const std::filesystem::path & path);

// Performs bounded structural inspection without loading model weights.
[[nodiscard]] SKINTOKENS_API result<glb_info> inspect_glb_file(
    const std::filesystem::path & path);

[[nodiscard]] SKINTOKENS_API rig_kind identify_rig(const skeleton & value);

// Uniformly fits a motion rig to the mesh's vertical extent and centre while
// preserving every bone length ratio. This is the conservative default for a
// motion-only Kimodo skeleton and a separately generated character.
[[nodiscard]] SKINTOKENS_API result<motion> fit_motion_to_mesh(
    const mesh & geometry, const motion & animation);

// Explicit fit selection. The articulated mode first chooses between the
// supplied rest pose and the clip's own first frame by which one actually runs
// inside the mesh's arms, then applies a bounded analytic two-bone pose fit to
// any recognized arm chain that still misses. Adopting the first frame makes
// the bind pose and frame zero identical, so playback starts without warping
// the mesh. Every mode preserves the globally scaled length of every bone.
[[nodiscard]] SKINTOKENS_API result<motion> fit_motion_to_mesh(
    const mesh & geometry, const motion & animation, skeleton_fit fit);

// Transfers a motion onto a mesh-specific SkinTokens rig by matching its
// generated rest-pose topology and normalized joint positions. Root travel is
// scaled to the target rig while local rotations remain frame-exact.
[[nodiscard]] SKINTOKENS_API result<motion> retarget_motion_to_rig(
    const motion & animation, const skeleton & target);

// Recognizes the mesh-fitted humanoid core produced by unconstrained
// SkinTokens generation. Short terminal subtrees below the hands are ignored,
// allowing the generated rig to contain different numbers of finger branches.
[[nodiscard]] SKINTOKENS_API result<humanoid_mapping> match_generated_humanoid(
    const skeleton & generated,
    const humanoid_match_options & options = {});

// Transfers SOMA30 motion onto the generated hierarchy. The target hierarchy,
// bind pose, and learned skin weights are not modified.
[[nodiscard]] SKINTOKENS_API result<motion> retarget_soma30_motion_to_generated(
    const motion & soma30,
    const skeleton & generated,
    const humanoid_mapping & mapping,
    const retarget_options & options = {});

// Compares motion rather than raw proportions: each corresponding joint is
// root-relative, height-normalized, and measured against its own rest pose.
[[nodiscard]] SKINTOKENS_API result<motion_divergence_report>
compare_soma30_to_generated(
    const motion & soma30,
    const motion & generated,
    const humanoid_mapping & mapping);

[[nodiscard]] SKINTOKENS_API result<humanoid_match_report> retarget_soma30_glb_file(
    const std::filesystem::path & generated_rigged_glb,
    const std::filesystem::path & soma30_motion_glb,
    const std::filesystem::path & output_glb,
    const humanoid_match_options & match_options = {},
    const retarget_options & options = {});

// Builds SkinTokens' published Mixamo52 topology from an already fitted
// SOMA30 rest rig. Body joints are mapped anatomically; SOMA's hand endpoints
// anchor deterministic neutral finger chains.
[[nodiscard]] SKINTOKENS_API result<skeleton> make_mixamo52_rig(
    const skeleton & fitted_soma30);

// Explicit semantic SOMA30 -> Mixamo52 transfer. Unlike the generic fallback
// above, this never selects joints by spatial proximity.
[[nodiscard]] SKINTOKENS_API result<motion> retarget_soma30_to_mixamo52(
    const motion & animation, const skeleton & mixamo52);

// Runs deterministic +/- joint-isolation probes and compares a complete clip
// in root-relative normalized model space. Lower errors indicate that the
// manual mapping preserves source joint trajectories.
[[nodiscard]] SKINTOKENS_API result<retarget_report> validate_soma30_to_mixamo52(
    const motion & animation, const skeleton & mixamo52);

// Compares two already-exported/re-imported clips. This closes the validation
// loop around GLB serialization instead of checking only in-memory transfer.
[[nodiscard]] SKINTOKENS_API result<retarget_report> compare_soma30_to_mixamo52(
    const motion & soma30_animation, const motion & mixamo52_animation);

// Evaluates the same glTF linear-blend skinning transform emitted by
// save_skinned_animation_glb_file at an exact animation frame. This is useful
// for render-independent parity tests and offline vertex export.
[[nodiscard]] SKINTOKENS_API result<std::vector<vec3>> deform_vertices(
    const mesh & geometry,
    const skin & binding,
    const motion & animation,
    std::size_t frame);

[[nodiscard]] SKINTOKENS_API result<void> save_skinned_animation_glb_file(
    const std::filesystem::path & path,
    const mesh & geometry,
    const skin & binding,
    const motion & animation);

} // namespace skintokens

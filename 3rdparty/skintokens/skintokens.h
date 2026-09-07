#ifndef SKINTOKENS_H
#define SKINTOKENS_H

#include <stddef.h>
#include <stdint.h>

#define ST_ABI_VERSION 1U

#if defined(_WIN32) && defined(SKINTOKENS_SHARED)
#  if defined(SKINTOKENS_BUILD)
#    define ST_API __declspec(dllexport)
#  else
#    define ST_API __declspec(dllimport)
#  endif
#elif defined(__GNUC__) && defined(SKINTOKENS_SHARED)
#  define ST_API __attribute__((visibility("default")))
#else
#  define ST_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct st_model st_model;
typedef struct st_retarget_options st_retarget_options;
typedef struct st_humanoid_match st_humanoid_match;

typedef uint32_t st_finger_transfer;
#define ST_FINGER_TRANSFER_NEUTRAL UINT32_C(0)
#define ST_FINGER_TRANSFER_MAP_SOMA_ENDPOINTS UINT32_C(1)

typedef uint32_t st_semantic_role;
#define ST_ROLE_UNMAPPED UINT32_C(0)
#define ST_ROLE_HIPS UINT32_C(1)
#define ST_ROLE_SPINE1 UINT32_C(2)
#define ST_ROLE_SPINE2 UINT32_C(3)
#define ST_ROLE_CHEST UINT32_C(4)
#define ST_ROLE_NECK UINT32_C(5)
#define ST_ROLE_HEAD UINT32_C(6)
#define ST_ROLE_LEFT_SHOULDER UINT32_C(7)
#define ST_ROLE_LEFT_UPPER_ARM UINT32_C(8)
#define ST_ROLE_LEFT_FOREARM UINT32_C(9)
#define ST_ROLE_LEFT_HAND UINT32_C(10)
#define ST_ROLE_RIGHT_SHOULDER UINT32_C(11)
#define ST_ROLE_RIGHT_UPPER_ARM UINT32_C(12)
#define ST_ROLE_RIGHT_FOREARM UINT32_C(13)
#define ST_ROLE_RIGHT_HAND UINT32_C(14)
#define ST_ROLE_LEFT_UPPER_LEG UINT32_C(15)
#define ST_ROLE_LEFT_SHIN UINT32_C(16)
#define ST_ROLE_LEFT_FOOT UINT32_C(17)
#define ST_ROLE_LEFT_TOE UINT32_C(18)
#define ST_ROLE_RIGHT_UPPER_LEG UINT32_C(19)
#define ST_ROLE_RIGHT_SHIN UINT32_C(20)
#define ST_ROLE_RIGHT_FOOT UINT32_C(21)
#define ST_ROLE_RIGHT_TOE UINT32_C(22)

typedef enum st_status {
    ST_OK = 0,
    ST_INVALID_ARGUMENT,
    ST_INVALID_FORMAT,
    ST_LIMIT_EXCEEDED,
    ST_IO_ERROR,
    ST_INCOMPATIBLE_MODEL,
    ST_BACKEND_UNAVAILABLE,
    ST_ALLOCATION_FAILED,
    ST_COMPUTE_FAILED
} st_status;

typedef enum st_device { ST_DEVICE_AUTO = 0, ST_DEVICE_CPU = 1, ST_DEVICE_VULKAN = 2 } st_device;

typedef enum st_target_rig {
    ST_TARGET_GENERATED = 0,
    ST_TARGET_SOMA30 = 1,
    ST_TARGET_MIXAMO52 = 2
} st_target_rig;

typedef enum st_rig_kind {
    ST_RIG_UNKNOWN = 0,
    ST_RIG_SOMA30 = 1,
    ST_RIG_MIXAMO52 = 2
} st_rig_kind;

typedef enum st_skeleton_fit {
    ST_FIT_NONE = 0,
    ST_FIT_GLOBAL_SIMILARITY = 1,
    ST_FIT_ARTICULATED = 2
} st_skeleton_fit;

typedef struct st_runtime_options {
    st_device device;
    uint32_t threads;
    const char * backend_directory;
} st_runtime_options;

typedef struct st_generation_options {
    uint64_t seed;
    uint32_t top_k;
    float top_p;
    float temperature;
    float repetition_penalty;
    uint32_t beams;
    size_t max_tokens;
    int geometric_only;
    st_target_rig target_rig;
    int surface_postprocess;
} st_generation_options;

typedef struct st_mesh_info {
    size_t vertex_count;
    size_t triangle_count;
} st_mesh_info;

typedef struct st_motion_info {
    size_t frame_count;
    size_t joint_count;
    float frames_per_second;
} st_motion_info;

typedef struct st_glb_info {
    int has_mesh;
    int has_skin;
    int has_skeleton;
    int has_animation;
    size_t joint_count;
    size_t frame_count;
    float frames_per_second;
    st_rig_kind rig_kind;
} st_glb_info;

/* Return versioned library defaults. Prefer these over zero-initialising an
 * options structure so future defaults remain source-compatible. */
ST_API uint32_t st_abi_version(void);
ST_API st_runtime_options st_default_runtime_options(void);
ST_API st_generation_options st_default_generation_options(void);

ST_API st_status st_model_load(const char * bundle, const st_runtime_options * options,
                               st_model ** output, char * error, size_t error_capacity);
ST_API void st_model_free(st_model * value);
ST_API const char * st_model_backend_name(const st_model * value);
ST_API const char * st_model_last_error(const st_model * value);

/* Parse and validate an input without loading model weights. These functions
 * are useful for upload validation and are covered through the public C ABI by
 * the sanitizer/libFuzzer target. */
ST_API st_status st_inspect_mesh_file(const char * path, st_mesh_info * output,
                                      char * error, size_t error_capacity);
ST_API st_status st_inspect_motion_glb_file(const char * path, st_motion_info * output,
                                            char * error, size_t error_capacity);
ST_API st_status st_inspect_glb_file(const char * path, st_glb_info * output,
                                     char * error, size_t error_capacity);

/* Generate both a skeleton and learned skin weights for an unrigged mesh. */
ST_API st_status st_rig_file(st_model * value, const char * mesh_path,
                             const char * output_path,
                             const st_generation_options * options, int * learned,
                             char * error, size_t error_capacity);

/* Generate learned skin weights for a supplied armature. skeleton_path may
 * equal mesh_path when both are stored in one GLB. Static and animated
 * armatures are accepted. target_rig=SOMA30 preserves the supplied hierarchy;
 * target_rig=MIXAMO52 is accepted only for a detected SOMA30 hierarchy.
 * fit_skeleton_to_mesh accepts st_skeleton_fit values; the historical value
 * 1 now selects the conservative global-similarity fit. */
ST_API st_status st_skin_files(st_model * value, const char * mesh_path,
                               const char * skeleton_path, const char * output_path,
                               int fit_skeleton_to_mesh,
                               const st_generation_options * options, int * learned,
                               char * error, size_t error_capacity);

ST_API st_status st_bind_glb_files(st_model * value, const char * mesh_path,
                                   const char * kimodo_motion_path, const char * output_path,
                                   const st_generation_options * options, int * learned,
                                   char * error, size_t error_capacity);

/* Preferred file API. Mesh input may be GLB or trellis2cpp T2MESH; motion and
 * output are GLB. Defaults bind the fitted SOMA30 hierarchy supplied by the
 * Kimodo motion. target_rig may instead request Mixamo52 or an unconstrained
 * generated rig; geometric_only bypasses learned skin-weight generation. */
ST_API st_status st_bind_files(st_model * value, const char * mesh_path,
                               const char * kimodo_motion_path, const char * output_path,
                               const st_generation_options * options, int * learned,
                               char * error, size_t error_capacity);

/* PureGo-friendly retarget configuration. These are opaque heap objects:
 * callers do not need to duplicate any compiler-dependent structure layout.
 * All scalar access uses fixed-width values and checked getters/setters. */
ST_API st_status st_retarget_options_create(st_retarget_options ** output,
                                            char * error, size_t error_capacity);
ST_API void st_retarget_options_free(st_retarget_options * value);
ST_API st_status st_retarget_options_set_minimum_confidence(
    st_retarget_options * value, float confidence, char * error, size_t error_capacity);
ST_API st_status st_retarget_options_get_minimum_confidence(
    const st_retarget_options * value, float * output, char * error, size_t error_capacity);
ST_API st_status st_retarget_options_set_allow_flexible_hands(
    st_retarget_options * value, int enabled, char * error, size_t error_capacity);
ST_API st_status st_retarget_options_get_allow_flexible_hands(
    const st_retarget_options * value, int * output, char * error, size_t error_capacity);
ST_API st_status st_retarget_options_set_finger_transfer(
    st_retarget_options * value, st_finger_transfer mode, char * error, size_t error_capacity);
ST_API st_status st_retarget_options_get_finger_transfer(
    const st_retarget_options * value, st_finger_transfer * output,
    char * error, size_t error_capacity);
ST_API st_status st_retarget_options_set_scale_root_motion(
    st_retarget_options * value, int enabled, char * error, size_t error_capacity);
ST_API st_status st_retarget_options_get_scale_root_motion(
    const st_retarget_options * value, int * output, char * error, size_t error_capacity);

/* Analyze a generated SkinTokens GLB once, then optionally reuse the match
 * for retargeting. Hand terminal subtrees may differ; the recognized core is
 * exposed only through stable scalar getters. */
ST_API st_status st_humanoid_match_glb_file(
    const char * generated_rigged_glb, const st_retarget_options * options,
    st_humanoid_match ** output, char * error, size_t error_capacity);
ST_API void st_humanoid_match_free(st_humanoid_match * value);
ST_API st_status st_humanoid_match_get_confidence(
    const st_humanoid_match * value, float * output, char * error, size_t error_capacity);
ST_API st_status st_humanoid_match_get_core_score(
    const st_humanoid_match * value, float * output, char * error, size_t error_capacity);
ST_API st_status st_humanoid_match_get_alternative_margin(
    const st_humanoid_match * value, float * output, char * error, size_t error_capacity);
ST_API st_status st_humanoid_match_get_mapped_core_joint_count(
    const st_humanoid_match * value, uint64_t * output, char * error, size_t error_capacity);
ST_API st_status st_humanoid_match_get_ignored_terminal_joint_count(
    const st_humanoid_match * value, uint64_t * output, char * error, size_t error_capacity);
ST_API st_status st_humanoid_match_get_generated_joint_count(
    const st_humanoid_match * value, uint64_t * output, char * error, size_t error_capacity);
ST_API st_status st_humanoid_match_get_generated_joint_role(
    const st_humanoid_match * value, uint64_t generated_joint,
    st_semantic_role * output, char * error, size_t error_capacity);
ST_API st_status st_humanoid_match_get_soma30_target_joint(
    const st_humanoid_match * value, uint32_t soma30_joint,
    int64_t * output, char * error, size_t error_capacity);

/* Retargets SOMA30 motion onto an already-generated SkinTokens rig while
 * preserving its hierarchy, bind pose, mesh, and learned skin weights.
 * match may be NULL to analyze the generated GLB in the same call. */
ST_API st_status st_retarget_soma30_glb_files(
    const char * generated_rigged_glb, const char * soma30_motion_glb,
    const char * output_glb, const st_humanoid_match * match,
    const st_retarget_options * options, char * error, size_t error_capacity);

#ifdef __cplusplus
}
#endif
#endif

#include <skintokens/skintokens.h>
#include <skintokens/skintokens.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>

struct st_model {
    skintokens::model value;
    std::string backend;
    std::string last_error;
    explicit st_model(skintokens::model && model) : value(std::move(model)), backend(value.backend_name()) {}
};

struct st_retarget_options {
    skintokens::humanoid_match_options match;
    skintokens::retarget_options transfer;
};

struct st_humanoid_match {
    skintokens::skeleton generated;
    skintokens::humanoid_mapping mapping;
};

namespace {
st_status status(skintokens::error_code code) {
    return static_cast<st_status>(static_cast<unsigned>(code) + 1U);
}
void copy_error(std::string_view message, char * output, std::size_t capacity) {
    if (output == nullptr || capacity == 0U) return;
    const auto count = std::min(capacity - 1U, message.size());
    if (count != 0U) std::memcpy(output, message.data(), count);
    output[count] = '\0';
}
st_status fail(st_model * model, st_status code, std::string message, char * output, std::size_t capacity) {
    if (model != nullptr) model->last_error = message;
    copy_error(message, output, capacity); return code;
}
skintokens::runtime_options runtime(const st_runtime_options * input) {
    skintokens::runtime_options output;
    if (input == nullptr) return output;
    output.device = input->device == ST_DEVICE_CPU ? skintokens::device_kind::cpu :
        input->device == ST_DEVICE_VULKAN ? skintokens::device_kind::vulkan : skintokens::device_kind::automatic;
    output.threads = input->threads;
    if (input->backend_directory != nullptr) output.backend_directory = input->backend_directory;
    return output;
}
skintokens::generation_options generation(const st_generation_options * input) {
    skintokens::generation_options output;
    if (input == nullptr) return output;
    output.seed = input->seed; output.top_k = input->top_k; output.top_p = input->top_p;
    output.temperature = input->temperature; output.repetition_penalty = input->repetition_penalty;
    output.beams = input->beams; output.max_tokens = input->max_tokens;
    output.geometric_only = input->geometric_only != 0;
    output.surface_postprocess = input->surface_postprocess != 0;
    return output;
}

bool valid_generation(const st_generation_options * value) {
    if (value == nullptr) return true;
    return value->top_k <= 32769U && value->top_p >= 0.0F && value->top_p <= 1.0F &&
           std::isfinite(value->top_p) && value->temperature >= 0.0F &&
           std::isfinite(value->temperature) && value->repetition_penalty > 0.0F &&
           std::isfinite(value->repetition_penalty) && value->beams >= 1U &&
           value->beams <= 32U && value->max_tokens >= 8U && value->max_tokens <= 65536U &&
           value->target_rig >= ST_TARGET_GENERATED && value->target_rig <= ST_TARGET_MIXAMO52;
}

bool same_skeleton(const skintokens::skeleton & a, const skintokens::skeleton & b) {
    if (a.names != b.names || a.parents != b.parents || a.rest_positions.size() != b.rest_positions.size())
        return false;
    for (std::size_t index = 0; index < a.rest_positions.size(); ++index) {
        const auto & x = a.rest_positions[index];
        const auto & y = b.rest_positions[index];
        if (std::abs(x.x-y.x)>1.0e-5F || std::abs(x.y-y.y)>1.0e-5F || std::abs(x.z-y.z)>1.0e-5F)
            return false;
    }
    return true;
}

skintokens::humanoid_match_options match_options(const st_retarget_options * input) {
    return input == nullptr ? skintokens::humanoid_match_options{} : input->match;
}

skintokens::retarget_options transfer_options(const st_retarget_options * input) {
    return input == nullptr ? skintokens::retarget_options{} : input->transfer;
}
}

extern "C" std::uint32_t st_abi_version(void) { return ST_ABI_VERSION; }

extern "C" st_runtime_options st_default_runtime_options(void) {
    return {ST_DEVICE_AUTO, 0U, nullptr};
}

extern "C" st_generation_options st_default_generation_options(void) {
    const skintokens::generation_options value;
    return {value.seed, value.top_k, value.top_p, value.temperature,
            value.repetition_penalty, value.beams, value.max_tokens, value.geometric_only ? 1 : 0,
            ST_TARGET_SOMA30, value.surface_postprocess ? 1 : 0};
}

extern "C" st_status st_model_load(const char * bundle, const st_runtime_options * options,
                                    st_model ** output, char * error, size_t error_capacity) try {
    if (output == nullptr)
        return fail(nullptr, ST_INVALID_ARGUMENT, "bundle and output are required", error, error_capacity);
    *output = nullptr;
    if (bundle == nullptr || bundle[0] == '\0' ||
        (options != nullptr && (options->device < ST_DEVICE_AUTO || options->device > ST_DEVICE_VULKAN)))
        return fail(nullptr, ST_INVALID_ARGUMENT, "bundle and valid runtime options are required", error, error_capacity);
    auto loaded = skintokens::model::load(bundle, runtime(options));
    if (!loaded) return fail(nullptr, status(loaded.error().code), loaded.error().message, error, error_capacity);
    auto handle = std::make_unique<st_model>(std::move(*loaded));
    *output = handle.release(); copy_error({}, error, error_capacity); return ST_OK;
} catch (const std::bad_alloc &) {
    return fail(nullptr, ST_ALLOCATION_FAILED, "allocation failed", error, error_capacity);
} catch (const std::exception & exception) {
    return fail(nullptr, ST_COMPUTE_FAILED, exception.what(), error, error_capacity);
} catch (...) {
    return fail(nullptr, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity);
}

extern "C" void st_model_free(st_model * value) { delete value; }
extern "C" const char * st_model_backend_name(const st_model * value) {
    return value == nullptr ? "" : value->backend.c_str();
}
extern "C" const char * st_model_last_error(const st_model * value) {
    return value == nullptr ? "invalid model handle" : value->last_error.c_str();
}

extern "C" st_status st_inspect_mesh_file(const char * path, st_mesh_info * output,
                                            char * error, size_t error_capacity) try {
    if (output != nullptr) *output = {};
    if (path == nullptr || path[0] == '\0' || output == nullptr)
        return fail(nullptr, ST_INVALID_ARGUMENT, "mesh path and output are required", error, error_capacity);
    const std::filesystem::path source = path;
    auto mesh = source.extension() == ".t2mesh" ?
        skintokens::load_trellis_mesh_file(source) : skintokens::load_glb_file(source);
    if (!mesh) return fail(nullptr, status(mesh.error().code), mesh.error().message, error, error_capacity);
    output->vertex_count = mesh->vertices.size();
    output->triangle_count = mesh->faces.size();
    copy_error({}, error, error_capacity);
    return ST_OK;
} catch (const std::bad_alloc &) {
    return fail(nullptr, ST_ALLOCATION_FAILED, "allocation failed", error, error_capacity);
} catch (const std::exception & exception) {
    return fail(nullptr, ST_COMPUTE_FAILED, exception.what(), error, error_capacity);
} catch (...) {
    return fail(nullptr, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity);
}

extern "C" st_status st_inspect_motion_glb_file(const char * path, st_motion_info * output,
                                                  char * error, size_t error_capacity) try {
    if (output != nullptr) *output = {};
    if (path == nullptr || path[0] == '\0' || output == nullptr)
        return fail(nullptr, ST_INVALID_ARGUMENT, "motion path and output are required", error, error_capacity);
    auto motion = skintokens::load_kimodo_glb_file(path);
    if (!motion) return fail(nullptr, status(motion.error().code), motion.error().message, error, error_capacity);
    output->frame_count = motion->frames;
    output->joint_count = motion->rig.names.size();
    output->frames_per_second = motion->frames_per_second;
    copy_error({}, error, error_capacity);
    return ST_OK;
} catch (const std::bad_alloc &) {
    return fail(nullptr, ST_ALLOCATION_FAILED, "allocation failed", error, error_capacity);
} catch (const std::exception & exception) {
    return fail(nullptr, ST_COMPUTE_FAILED, exception.what(), error, error_capacity);
} catch (...) {
    return fail(nullptr, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity);
}

extern "C" st_status st_inspect_glb_file(const char * path, st_glb_info * output,
                                           char * error, size_t error_capacity) try {
    if (output != nullptr) *output = {};
    if (path == nullptr || path[0] == '\0' || output == nullptr)
        return fail(nullptr, ST_INVALID_ARGUMENT, "GLB path and output are required", error, error_capacity);
    auto info = skintokens::inspect_glb_file(path);
    if (!info) return fail(nullptr, status(info.error().code), info.error().message, error, error_capacity);
    output->has_mesh = info->has_mesh ? 1 : 0;
    output->has_skin = info->has_skin ? 1 : 0;
    output->has_skeleton = info->has_skeleton ? 1 : 0;
    output->has_animation = info->has_animation ? 1 : 0;
    output->joint_count = info->joint_count;
    output->frame_count = info->frame_count;
    output->frames_per_second = info->frames_per_second;
    output->rig_kind = info->rig == skintokens::rig_kind::soma30 ? ST_RIG_SOMA30 :
        info->rig == skintokens::rig_kind::mixamo52 ? ST_RIG_MIXAMO52 : ST_RIG_UNKNOWN;
    copy_error({}, error, error_capacity);
    return ST_OK;
} catch (const std::bad_alloc &) {
    return fail(nullptr, ST_ALLOCATION_FAILED, "allocation failed", error, error_capacity);
} catch (const std::exception & exception) {
    return fail(nullptr, ST_COMPUTE_FAILED, exception.what(), error, error_capacity);
} catch (...) {
    return fail(nullptr, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity);
}

extern "C" st_status st_rig_file(st_model * value, const char * mesh_path,
                                   const char * output_path,
                                   const st_generation_options * options, int * learned,
                                   char * error, size_t error_capacity) try {
    if (value == nullptr || mesh_path == nullptr || output_path == nullptr ||
        mesh_path[0] == '\0' || output_path[0] == '\0' || !valid_generation(options))
        return fail(value, ST_INVALID_ARGUMENT, "model, mesh, output, and valid options are required", error, error_capacity);
    if (learned != nullptr) *learned = 0;
    const std::filesystem::path source_path = mesh_path;
    auto mesh = source_path.extension() == ".t2mesh" ?
        skintokens::load_trellis_mesh_file(source_path) : skintokens::load_glb_file(source_path);
    if (!mesh) return fail(value, status(mesh.error().code), mesh.error().message, error, error_capacity);
    auto binding = value->value.rig(*mesh, generation(options));
    if (!binding) return fail(value, status(binding.error().code), binding.error().message, error, error_capacity);
    skintokens::motion rest;
    rest.frames = 1U;
    rest.frames_per_second = 30.0F;
    rest.rig = binding->rig;
    rest.root_translations.assign(1U, rest.rig.rest_positions.front());
    rest.local_rotations.assign(rest.rig.names.size(), {});
    auto saved = skintokens::save_skinned_animation_glb_file(output_path, *mesh, *binding, rest);
    if (!saved) return fail(value, status(saved.error().code), saved.error().message, error, error_capacity);
    if (learned != nullptr) *learned = binding->learned ? 1 : 0;
    value->last_error.clear(); copy_error({}, error, error_capacity); return ST_OK;
} catch (const std::bad_alloc &) {
    return fail(value, ST_ALLOCATION_FAILED, "allocation failed", error, error_capacity);
} catch (const std::exception & exception) {
    return fail(value, ST_COMPUTE_FAILED, exception.what(), error, error_capacity);
} catch (...) {
    return fail(value, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity);
}

extern "C" st_status st_skin_files(st_model * value, const char * mesh_path,
                                     const char * skeleton_path, const char * output_path,
                                     int fit_skeleton_to_mesh,
                                     const st_generation_options * options, int * learned,
                                     char * error, size_t error_capacity) try {
    if (value == nullptr || mesh_path == nullptr || skeleton_path == nullptr || output_path == nullptr ||
        mesh_path[0] == '\0' || skeleton_path[0] == '\0' || output_path[0] == '\0' ||
        fit_skeleton_to_mesh < ST_FIT_NONE || fit_skeleton_to_mesh > ST_FIT_ARTICULATED ||
        !valid_generation(options))
        return fail(value, ST_INVALID_ARGUMENT, "model, mesh, skeleton, output, and valid options are required", error, error_capacity);
    const auto target = options == nullptr ? ST_TARGET_SOMA30 : options->target_rig;
    if (target == ST_TARGET_GENERATED)
        return fail(value, ST_INVALID_ARGUMENT, "skin-only generation requires a supplied skeleton target", error, error_capacity);
    if (learned != nullptr) *learned = 0;
    const std::filesystem::path source_path = mesh_path;
    auto mesh = source_path.extension() == ".t2mesh" ?
        skintokens::load_trellis_mesh_file(source_path) : skintokens::load_glb_file(source_path);
    if (!mesh) return fail(value, status(mesh.error().code), mesh.error().message, error, error_capacity);
    auto motion = skintokens::load_skeleton_glb_file(skeleton_path);
    if (!motion) return fail(value, status(motion.error().code), motion.error().message, error, error_capacity);
    if (fit_skeleton_to_mesh != ST_FIT_NONE) {
        const auto fit = fit_skeleton_to_mesh == ST_FIT_ARTICULATED ?
            skintokens::skeleton_fit::articulated : skintokens::skeleton_fit::global_similarity;
        auto fitted = skintokens::fit_motion_to_mesh(*mesh, *motion, fit);
        if (!fitted) return fail(value, status(fitted.error().code), fitted.error().message, error, error_capacity);
        motion = std::move(fitted);
    }
    if (target == ST_TARGET_MIXAMO52) {
        if (skintokens::identify_rig(motion->rig) != skintokens::rig_kind::soma30)
            return fail(value, ST_INVALID_ARGUMENT, "Mixamo52 retargeting requires a detected SOMA30 skeleton", error, error_capacity);
        auto mixamo = skintokens::make_mixamo52_rig(motion->rig);
        if (!mixamo) return fail(value, status(mixamo.error().code), mixamo.error().message, error, error_capacity);
        auto retargeted = skintokens::retarget_soma30_to_mixamo52(*motion, *mixamo);
        if (!retargeted) return fail(value, status(retargeted.error().code), retargeted.error().message, error, error_capacity);
        motion = std::move(retargeted);
    }
    auto binding = value->value.bind(*mesh, motion->rig, generation(options));
    if (!binding) return fail(value, status(binding.error().code), binding.error().message, error, error_capacity);
    auto saved = skintokens::save_skinned_animation_glb_file(output_path, *mesh, *binding, *motion);
    if (!saved) return fail(value, status(saved.error().code), saved.error().message, error, error_capacity);
    if (learned != nullptr) *learned = binding->learned ? 1 : 0;
    value->last_error.clear(); copy_error({}, error, error_capacity); return ST_OK;
} catch (const std::bad_alloc &) {
    return fail(value, ST_ALLOCATION_FAILED, "allocation failed", error, error_capacity);
} catch (const std::exception & exception) {
    return fail(value, ST_COMPUTE_FAILED, exception.what(), error, error_capacity);
} catch (...) {
    return fail(value, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity);
}

extern "C" st_status st_bind_glb_files(st_model * value, const char * mesh_path,
                                        const char * kimodo_motion_path, const char * output_path,
                                        const st_generation_options * options, int * learned,
                                        char * error, size_t error_capacity) {
    return st_bind_files(value, mesh_path, kimodo_motion_path, output_path,
                         options, learned, error, error_capacity);
}

extern "C" st_status st_bind_files(st_model * value, const char * mesh_path,
                                    const char * kimodo_motion_path, const char * output_path,
                                    const st_generation_options * options, int * learned,
                                    char * error, size_t error_capacity) try {
    if (value == nullptr || mesh_path == nullptr || kimodo_motion_path == nullptr || output_path == nullptr)
        return fail(value, ST_INVALID_ARGUMENT, "model, mesh, motion, and output paths are required", error, error_capacity);
    if (mesh_path[0] == '\0' || kimodo_motion_path[0] == '\0' || output_path[0] == '\0' ||
        !valid_generation(options))
        return fail(value, ST_INVALID_ARGUMENT, "paths and generation options must be valid", error, error_capacity);
    if (learned != nullptr) *learned = 0;
    const std::filesystem::path source_path = mesh_path;
    auto mesh = source_path.extension() == ".t2mesh" ?
        skintokens::load_trellis_mesh_file(source_path) : skintokens::load_glb_file(source_path);
    if (!mesh) return fail(value, status(mesh.error().code), mesh.error().message, error, error_capacity);
    auto motion = skintokens::load_kimodo_glb_file(kimodo_motion_path);
    if (!motion) return fail(value, status(motion.error().code), motion.error().message, error, error_capacity);
    const auto settings = generation(options);
    skintokens::result<skintokens::skin> binding;
    const auto target = options == nullptr ? ST_TARGET_SOMA30 : options->target_rig;
    if (settings.geometric_only || target != ST_TARGET_GENERATED) {
        auto fitted = skintokens::fit_motion_to_mesh(*mesh, *motion);
        if (!fitted) return fail(value, status(fitted.error().code), fitted.error().message, error, error_capacity);
        motion = std::move(fitted);
        if (target == ST_TARGET_MIXAMO52) {
            auto mixamo = skintokens::make_mixamo52_rig(motion->rig);
            if (!mixamo) return fail(value, status(mixamo.error().code), mixamo.error().message, error, error_capacity);
            auto retargeted = skintokens::retarget_soma30_to_mixamo52(*motion, *mixamo);
            if (!retargeted)
                return fail(value, status(retargeted.error().code), retargeted.error().message, error, error_capacity);
            motion = std::move(retargeted);
        }
        binding = value->value.bind(*mesh, motion->rig, settings);
    } else {
        binding = value->value.rig(*mesh, settings);
        if (binding) {
            auto matched = skintokens::match_generated_humanoid(binding->rig);
            if (!matched) return fail(value, status(matched.error().code), matched.error().message, error, error_capacity);
            auto retargeted = skintokens::retarget_soma30_motion_to_generated(
                *motion, binding->rig, *matched);
            if (!retargeted) return fail(value, status(retargeted.error().code), retargeted.error().message, error, error_capacity);
            motion = std::move(retargeted);
        }
    }
    if (!binding) return fail(value, status(binding.error().code), binding.error().message, error, error_capacity);
    auto saved = skintokens::save_skinned_animation_glb_file(output_path, *mesh, *binding, *motion);
    if (!saved) return fail(value, status(saved.error().code), saved.error().message, error, error_capacity);
    if (learned != nullptr) *learned = binding->learned ? 1 : 0;
    value->last_error.clear(); copy_error({}, error, error_capacity); return ST_OK;
} catch (const std::bad_alloc &) {
    return fail(value, ST_ALLOCATION_FAILED, "allocation failed", error, error_capacity);
} catch (const std::exception & exception) {
    return fail(value, ST_COMPUTE_FAILED, exception.what(), error, error_capacity);
} catch (...) {
    return fail(value, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity);
}

extern "C" st_status st_retarget_options_create(st_retarget_options ** output,
                                                   char * error, size_t error_capacity) try {
    if (output == nullptr)
        return fail(nullptr, ST_INVALID_ARGUMENT, "retarget options output is required", error, error_capacity);
    *output = new st_retarget_options{};
    copy_error({}, error, error_capacity);
    return ST_OK;
} catch (const std::bad_alloc &) {
    return fail(nullptr, ST_ALLOCATION_FAILED, "allocation failed", error, error_capacity);
} catch (...) {
    return fail(nullptr, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity);
}

extern "C" void st_retarget_options_free(st_retarget_options * value) { delete value; }

extern "C" st_status st_retarget_options_set_minimum_confidence(
    st_retarget_options * value, float confidence, char * error, size_t error_capacity) try {
    if (value == nullptr || !std::isfinite(confidence) || confidence < 0.0F || confidence > 1.0F)
        return fail(nullptr, ST_INVALID_ARGUMENT, "confidence must be between zero and one", error, error_capacity);
    value->match.minimum_confidence = confidence;
    value->transfer.minimum_confidence = confidence;
    copy_error({}, error, error_capacity); return ST_OK;
} catch (...) { return fail(nullptr, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity); }

extern "C" st_status st_retarget_options_get_minimum_confidence(
    const st_retarget_options * value, float * output, char * error, size_t error_capacity) try {
    if (value == nullptr || output == nullptr)
        return fail(nullptr, ST_INVALID_ARGUMENT, "retarget options and output are required", error, error_capacity);
    *output = value->match.minimum_confidence;
    copy_error({}, error, error_capacity); return ST_OK;
} catch (...) { return fail(nullptr, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity); }

extern "C" st_status st_retarget_options_set_allow_flexible_hands(
    st_retarget_options * value, int enabled, char * error, size_t error_capacity) try {
    if (value == nullptr || (enabled != 0 && enabled != 1))
        return fail(nullptr, ST_INVALID_ARGUMENT, "retarget options and a boolean value are required", error, error_capacity);
    value->match.allow_flexible_hands = enabled != 0;
    copy_error({}, error, error_capacity); return ST_OK;
} catch (...) { return fail(nullptr, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity); }

extern "C" st_status st_retarget_options_get_allow_flexible_hands(
    const st_retarget_options * value, int * output, char * error, size_t error_capacity) try {
    if (value == nullptr || output == nullptr)
        return fail(nullptr, ST_INVALID_ARGUMENT, "retarget options and output are required", error, error_capacity);
    *output = value->match.allow_flexible_hands ? 1 : 0;
    copy_error({}, error, error_capacity); return ST_OK;
} catch (...) { return fail(nullptr, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity); }

extern "C" st_status st_retarget_options_set_finger_transfer(
    st_retarget_options * value, st_finger_transfer mode, char * error, size_t error_capacity) try {
    if (value == nullptr || mode > ST_FINGER_TRANSFER_MAP_SOMA_ENDPOINTS)
        return fail(nullptr, ST_INVALID_ARGUMENT, "retarget options and a valid finger mode are required", error, error_capacity);
    value->transfer.fingers = mode == ST_FINGER_TRANSFER_MAP_SOMA_ENDPOINTS ?
        skintokens::finger_transfer::map_soma_endpoints : skintokens::finger_transfer::neutral;
    copy_error({}, error, error_capacity); return ST_OK;
} catch (...) { return fail(nullptr, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity); }

extern "C" st_status st_retarget_options_get_finger_transfer(
    const st_retarget_options * value, st_finger_transfer * output,
    char * error, size_t error_capacity) try {
    if (value == nullptr || output == nullptr)
        return fail(nullptr, ST_INVALID_ARGUMENT, "retarget options and output are required", error, error_capacity);
    *output = value->transfer.fingers == skintokens::finger_transfer::map_soma_endpoints ?
        ST_FINGER_TRANSFER_MAP_SOMA_ENDPOINTS : ST_FINGER_TRANSFER_NEUTRAL;
    copy_error({}, error, error_capacity); return ST_OK;
} catch (...) { return fail(nullptr, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity); }

extern "C" st_status st_retarget_options_set_scale_root_motion(
    st_retarget_options * value, int enabled, char * error, size_t error_capacity) try {
    if (value == nullptr || (enabled != 0 && enabled != 1))
        return fail(nullptr, ST_INVALID_ARGUMENT, "retarget options and a boolean value are required", error, error_capacity);
    value->transfer.scale_root_motion = enabled != 0;
    copy_error({}, error, error_capacity); return ST_OK;
} catch (...) { return fail(nullptr, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity); }

extern "C" st_status st_retarget_options_get_scale_root_motion(
    const st_retarget_options * value, int * output, char * error, size_t error_capacity) try {
    if (value == nullptr || output == nullptr)
        return fail(nullptr, ST_INVALID_ARGUMENT, "retarget options and output are required", error, error_capacity);
    *output = value->transfer.scale_root_motion ? 1 : 0;
    copy_error({}, error, error_capacity); return ST_OK;
} catch (...) { return fail(nullptr, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity); }

extern "C" st_status st_humanoid_match_glb_file(
    const char * generated_rigged_glb, const st_retarget_options * options,
    st_humanoid_match ** output, char * error, size_t error_capacity) try {
    if (output != nullptr) *output = nullptr;
    if (generated_rigged_glb == nullptr || generated_rigged_glb[0] == '\0' || output == nullptr)
        return fail(nullptr, ST_INVALID_ARGUMENT, "generated GLB and match output are required", error, error_capacity);
    auto asset = skintokens::load_skinned_glb_file(generated_rigged_glb);
    if (!asset) return fail(nullptr, status(asset.error().code), asset.error().message, error, error_capacity);
    auto mapping = skintokens::match_generated_humanoid(asset->binding.rig, match_options(options));
    if (!mapping) return fail(nullptr, status(mapping.error().code), mapping.error().message, error, error_capacity);
    auto handle = std::make_unique<st_humanoid_match>();
    handle->generated = asset->binding.rig;
    handle->mapping = std::move(*mapping);
    *output = handle.release();
    copy_error({}, error, error_capacity); return ST_OK;
} catch (const std::bad_alloc &) {
    return fail(nullptr, ST_ALLOCATION_FAILED, "allocation failed", error, error_capacity);
} catch (const std::exception & exception) {
    return fail(nullptr, ST_COMPUTE_FAILED, exception.what(), error, error_capacity);
} catch (...) { return fail(nullptr, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity); }

extern "C" void st_humanoid_match_free(st_humanoid_match * value) { delete value; }

extern "C" st_status st_humanoid_match_get_confidence(
    const st_humanoid_match * value, float * output, char * error, size_t error_capacity) try {
    if (value == nullptr || output == nullptr)
        return fail(nullptr, ST_INVALID_ARGUMENT, "humanoid match and output are required", error, error_capacity);
    *output = value->mapping.report.confidence; copy_error({}, error, error_capacity); return ST_OK;
} catch (...) { return fail(nullptr, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity); }

extern "C" st_status st_humanoid_match_get_core_score(
    const st_humanoid_match * value, float * output, char * error, size_t error_capacity) try {
    if (value == nullptr || output == nullptr)
        return fail(nullptr, ST_INVALID_ARGUMENT, "humanoid match and output are required", error, error_capacity);
    *output = value->mapping.report.core_score; copy_error({}, error, error_capacity); return ST_OK;
} catch (...) { return fail(nullptr, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity); }

extern "C" st_status st_humanoid_match_get_alternative_margin(
    const st_humanoid_match * value, float * output, char * error, size_t error_capacity) try {
    if (value == nullptr || output == nullptr)
        return fail(nullptr, ST_INVALID_ARGUMENT, "humanoid match and output are required", error, error_capacity);
    *output = value->mapping.report.alternative_margin; copy_error({}, error, error_capacity); return ST_OK;
} catch (...) { return fail(nullptr, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity); }

extern "C" st_status st_humanoid_match_get_mapped_core_joint_count(
    const st_humanoid_match * value, uint64_t * output, char * error, size_t error_capacity) try {
    if (value == nullptr || output == nullptr)
        return fail(nullptr, ST_INVALID_ARGUMENT, "humanoid match and output are required", error, error_capacity);
    *output = static_cast<uint64_t>(value->mapping.report.mapped_core_joints);
    copy_error({}, error, error_capacity); return ST_OK;
} catch (...) { return fail(nullptr, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity); }

extern "C" st_status st_humanoid_match_get_ignored_terminal_joint_count(
    const st_humanoid_match * value, uint64_t * output, char * error, size_t error_capacity) try {
    if (value == nullptr || output == nullptr)
        return fail(nullptr, ST_INVALID_ARGUMENT, "humanoid match and output are required", error, error_capacity);
    *output = static_cast<uint64_t>(value->mapping.report.ignored_terminal_joints);
    copy_error({}, error, error_capacity); return ST_OK;
} catch (...) { return fail(nullptr, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity); }

extern "C" st_status st_humanoid_match_get_generated_joint_count(
    const st_humanoid_match * value, uint64_t * output, char * error, size_t error_capacity) try {
    if (value == nullptr || output == nullptr)
        return fail(nullptr, ST_INVALID_ARGUMENT, "humanoid match and output are required", error, error_capacity);
    *output = static_cast<uint64_t>(value->generated.names.size());
    copy_error({}, error, error_capacity); return ST_OK;
} catch (...) { return fail(nullptr, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity); }

extern "C" st_status st_humanoid_match_get_generated_joint_role(
    const st_humanoid_match * value, uint64_t generated_joint,
    st_semantic_role * output, char * error, size_t error_capacity) try {
    if (value == nullptr || output == nullptr ||
        generated_joint >= value->mapping.generated_roles.size())
        return fail(nullptr, ST_INVALID_ARGUMENT, "humanoid match, bounded joint index, and output are required", error, error_capacity);
    *output = static_cast<st_semantic_role>(
        value->mapping.generated_roles[static_cast<std::size_t>(generated_joint)]);
    copy_error({}, error, error_capacity); return ST_OK;
} catch (...) { return fail(nullptr, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity); }

extern "C" st_status st_humanoid_match_get_soma30_target_joint(
    const st_humanoid_match * value, uint32_t soma30_joint,
    int64_t * output, char * error, size_t error_capacity) try {
    if (value == nullptr || output == nullptr || soma30_joint >= value->mapping.soma_to_generated.size())
        return fail(nullptr, ST_INVALID_ARGUMENT, "humanoid match, SOMA30 joint index, and output are required", error, error_capacity);
    *output = static_cast<int64_t>(value->mapping.soma_to_generated[soma30_joint]);
    copy_error({}, error, error_capacity); return ST_OK;
} catch (...) { return fail(nullptr, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity); }

extern "C" st_status st_retarget_soma30_glb_files(
    const char * generated_rigged_glb, const char * soma30_motion_glb,
    const char * output_glb, const st_humanoid_match * match,
    const st_retarget_options * options, char * error, size_t error_capacity) try {
    if (generated_rigged_glb == nullptr || soma30_motion_glb == nullptr || output_glb == nullptr ||
        generated_rigged_glb[0] == '\0' || soma30_motion_glb[0] == '\0' || output_glb[0] == '\0')
        return fail(nullptr, ST_INVALID_ARGUMENT, "generated, SOMA30 motion, and output GLBs are required", error, error_capacity);
    auto asset = skintokens::load_skinned_glb_file(generated_rigged_glb);
    if (!asset) return fail(nullptr, status(asset.error().code), asset.error().message, error, error_capacity);
    skintokens::humanoid_mapping mapping;
    if (match != nullptr) {
        if (!same_skeleton(match->generated, asset->binding.rig))
            return fail(nullptr, ST_INVALID_ARGUMENT, "humanoid match belongs to a different generated skeleton", error, error_capacity);
        mapping = match->mapping;
    } else {
        auto detected = skintokens::match_generated_humanoid(asset->binding.rig, match_options(options));
        if (!detected) return fail(nullptr, status(detected.error().code), detected.error().message, error, error_capacity);
        mapping = std::move(*detected);
    }
    auto source = skintokens::load_kimodo_glb_file(soma30_motion_glb);
    if (!source) return fail(nullptr, status(source.error().code), source.error().message, error, error_capacity);
    if (skintokens::identify_rig(source->rig) != skintokens::rig_kind::soma30)
        return fail(nullptr, ST_INVALID_ARGUMENT, "motion GLB is not a recognized SOMA30 animation", error, error_capacity);
    auto animation = skintokens::retarget_soma30_motion_to_generated(
        *source, asset->binding.rig, mapping, transfer_options(options));
    if (!animation) return fail(nullptr, status(animation.error().code), animation.error().message, error, error_capacity);
    auto saved = skintokens::save_skinned_animation_glb_file(
        output_glb, asset->geometry, asset->binding, *animation);
    if (!saved) return fail(nullptr, status(saved.error().code), saved.error().message, error, error_capacity);
    copy_error({}, error, error_capacity); return ST_OK;
} catch (const std::bad_alloc &) {
    return fail(nullptr, ST_ALLOCATION_FAILED, "allocation failed", error, error_capacity);
} catch (const std::exception & exception) {
    return fail(nullptr, ST_COMPUTE_FAILED, exception.what(), error, error_capacity);
} catch (...) { return fail(nullptr, ST_COMPUTE_FAILED, "unknown C++ exception", error, error_capacity); }

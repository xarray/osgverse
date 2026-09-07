#pragma once

#include <skintokens/skintokens.hpp>

#if defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wshadow"
#endif
#include <ggml.h>
#include <ggml-alloc.h>
#include <ggml-backend.h>
#include <ggml-cpu.h>
#include <gguf.h>
#if defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif

#include <cstdint>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace skintokens::detail {

[[nodiscard]] inline bool profiling_enabled() noexcept {
    static const bool enabled = [] {
        const char * value = std::getenv("SKINTOKENS_PROFILE");
        return value != nullptr && *value != '\0' && std::string_view{value} != "0";
    }();
    return enabled;
}

inline void profile_record(std::string_view label, double milliseconds) noexcept {
    if (!profiling_enabled()) return;
    std::fprintf(stderr, "[skintokens-profile] %.*s ms=%.3f\n",
                 static_cast<int>(label.size()), label.data(), milliseconds);
}

class profile_scope {
public:
    explicit profile_scope(std::string_view label) noexcept
        : label_(label), start_(std::chrono::steady_clock::now()) {}
    profile_scope(const profile_scope &) = delete;
    profile_scope & operator=(const profile_scope &) = delete;
    ~profile_scope() {
        const auto elapsed = std::chrono::steady_clock::now() - start_;
        profile_record(label_, std::chrono::duration<double, std::milli>(elapsed).count());
    }
private:
    std::string_view label_;
    std::chrono::steady_clock::time_point start_;
};

template<class Function>
auto profiled(std::string_view label, Function && function) {
    const auto start = std::chrono::steady_clock::now();
    auto output = std::forward<Function>(function)();
    const auto elapsed = std::chrono::steady_clock::now() - start;
    profile_record(label, std::chrono::duration<double, std::milli>(elapsed).count());
    return output;
}

[[nodiscard]] inline error fail(error_code code, std::string message) {
    return error{code, std::move(message)};
}

[[nodiscard]] inline bool checked_mul(std::size_t a, std::size_t b, std::size_t & result) {
    if (a != 0 && b > std::numeric_limits<std::size_t>::max() / a) return false;
    result = a * b;
    return true;
}

struct backend_handle {
    ggml_backend_t value = nullptr;
    std::string name;
    backend_handle() = default;
    backend_handle(const backend_handle &) = delete;
    backend_handle & operator=(const backend_handle &) = delete;
    ~backend_handle();
};

[[nodiscard]] result<std::unique_ptr<backend_handle>> make_backend(
    const runtime_options & options);

struct bundle_metadata {
    std::string architecture;
    std::string component;
    std::string upstream_revision;
    std::string tokenrig_sha256;
    std::string skin_vae_sha256;
    std::uint64_t format_version = 0;
    std::uint64_t tensor_count = 0;
};

struct weight_component {
    ggml_context * context = nullptr;
    gguf_context * file = nullptr;
    ggml_backend_buffer_t buffer = nullptr;
    std::filesystem::path path;
    weight_component() = default;
    weight_component(const weight_component &) = delete;
    weight_component & operator=(const weight_component &) = delete;
    ~weight_component();
    [[nodiscard]] ggml_tensor * tensor(std::string_view name) const;
};

[[nodiscard]] result<std::unique_ptr<weight_component>> load_component(
    const std::filesystem::path & path, ggml_backend_t backend);

[[nodiscard]] result<bundle_metadata> inspect_gguf(
    const std::filesystem::path & path);

using tensor_snapshot = std::unordered_map<std::string, std::vector<float>>;

// Qwen3 graph layout follows llama.cpp's Qwen3 implementation at the revision
// recorded in NOTICE. It is expressed here against GGML directly; no llama.cpp
// source or runtime is linked into this project.
[[nodiscard]] result<tensor_snapshot> run_qwen_layer0(
    const weight_component & weights, ggml_backend_t backend,
    nonstd::span<const float> input, std::size_t sequence);

[[nodiscard]] result<std::vector<float>> run_qwen_layer(
    const weight_component & weights, ggml_backend_t backend,
    nonstd::span<const float> input, std::size_t sequence, std::size_t layer);

[[nodiscard]] result<std::vector<float>> run_qwen_head(
    const weight_component & weights, ggml_backend_t backend,
    nonstd::span<const float> input, std::size_t sequence);

[[nodiscard]] result<std::vector<std::int32_t>> generate_skin_codes(
    const weight_component & weights, ggml_backend_t backend,
    nonstd::span<const float> mesh_embeddings,
    nonstd::span<const std::int32_t> skeleton_tokens,
    std::size_t joint_count, const generation_options & options);

struct generated_rig_tokens {
    std::vector<std::int32_t> skeleton_tokens;
    std::vector<std::int32_t> skin_codes;
    std::size_t joint_count = 0;
};

// Generate TokenizerPart skeleton tokens followed by four FSQ skin codes per
// generated joint. This is the checkpoint's normal, unconstrained TokenRig
// route; supplied-skeleton generation above is a separate diagnostic lane.
[[nodiscard]] result<generated_rig_tokens> generate_rig_tokens(
    const weight_component & weights, ggml_backend_t backend,
    nonstd::span<const float> mesh_embeddings,
    const generation_options & options);

[[nodiscard]] result<std::vector<float>> run_qwen_logits(
    const weight_component & weights, ggml_backend_t backend,
    nonstd::span<const float> mesh_embeddings,
    nonstd::span<const std::int32_t> tokens);

[[nodiscard]] result<std::vector<float>> run_qwen_logits_batch(
    const weight_component & weights, ggml_backend_t backend,
    nonstd::span<const float> mesh_embeddings,
    nonstd::span<const std::int32_t> tokens,
    std::size_t token_count, std::size_t batch_size);

[[nodiscard]] result<std::vector<float>> run_qwen_kv_trajectory(
    const weight_component & weights, ggml_backend_t backend,
    nonstd::span<const float> mesh_embeddings,
    nonstd::span<const std::int32_t> prefix,
    nonstd::span<const std::int32_t> continuation);

[[nodiscard]] result<std::vector<float>> run_qwen_kv_branches(
    const weight_component & weights, ggml_backend_t backend,
    nonstd::span<const float> mesh_embeddings,
    nonstd::span<const std::int32_t> prefix,
    nonstd::span<const std::int32_t> branch_tokens);

struct skeleton_prefix {
    std::vector<std::int32_t> tokens;
    std::vector<vec3> normalized_vertices;
    std::vector<vec3> normalized_joints;
    std::vector<std::array<double, 3>> precise_normalized_vertices;
};

[[nodiscard]] result<skeleton_prefix> tokenize_skeleton_prefix(
    const mesh & source, const skeleton & target);

[[nodiscard]] result<skeleton> detokenize_generated_skeleton(
    nonstd::span<const std::int32_t> tokens, vec3 center, float scale);

[[nodiscard]] result<tensor_snapshot> run_mesh_encoder_fixture(
    const weight_component & weights, ggml_backend_t backend,
    nonstd::span<const float> points, nonstd::span<const float> normals);

[[nodiscard]] result<std::vector<float>> encode_mesh(
    const weight_component & weights, ggml_backend_t backend,
    nonstd::span<const vec3> points, nonstd::span<const vec3> normals,
    nonstd::span<const std::int32_t> query_indices);

[[nodiscard]] result<tensor_snapshot> run_skin_vae_fixture(
    const weight_component & weights, ggml_backend_t backend,
    nonstd::span<const float> points, nonstd::span<const float> normals);

[[nodiscard]] result<std::vector<float>> encode_skin_condition(
    const weight_component & weights, ggml_backend_t backend,
    nonstd::span<const vec3> points, nonstd::span<const vec3> normals,
    nonstd::span<const std::int32_t> query_indices);

[[nodiscard]] result<std::vector<float>> decode_skin_joint(
    const weight_component & weights, ggml_backend_t backend,
    nonstd::span<const std::int32_t> codes, nonstd::span<const float> condition_latents,
    nonstd::span<const vec3> points, nonstd::span<const vec3> normals);

} // namespace skintokens::detail

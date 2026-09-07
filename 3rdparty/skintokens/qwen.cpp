#include "internal.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <memory>
#include <random>

namespace skintokens::detail {
namespace {

constexpr std::int64_t hidden = 896;
constexpr std::int64_t heads = 16;
constexpr std::int64_t kv_heads = 8;
constexpr std::int64_t head_dim = 128;

using profile_clock = std::chrono::steady_clock;

struct qwen_profile_stats {
    std::string_view label;
    profile_clock::time_point started = profile_clock::now();
    double setup_ms = 0.0;
    double host_to_device_ms = 0.0;
    double compute_sync_ms = 0.0;
    double device_to_host_ms = 0.0;
    std::uint64_t host_to_device_bytes = 0U;
    std::uint64_t device_to_host_bytes = 0U;
    std::size_t logits_calls = 0U;
    std::size_t layers = 0U;
    std::size_t graph_computes = 0U;
    double cache_copy_ms = 0.0;
    std::uint64_t cache_copy_bytes = 0U;
};

thread_local qwen_profile_stats * active_qwen_profile = nullptr;

double elapsed_ms(profile_clock::time_point start) {
    return std::chrono::duration<double, std::milli>(profile_clock::now() - start).count();
}

class qwen_profile_session {
public:
    explicit qwen_profile_session(std::string_view label) : stats_{label}, previous_(active_qwen_profile) {
        if (profiling_enabled()) active_qwen_profile = &stats_;
    }
    qwen_profile_session(const qwen_profile_session &) = delete;
    qwen_profile_session & operator=(const qwen_profile_session &) = delete;
    ~qwen_profile_session() {
        if (!profiling_enabled()) return;
        active_qwen_profile = previous_;
        const double total_ms = elapsed_ms(stats_.started);
        const double accounted = stats_.setup_ms + stats_.host_to_device_ms +
                                 stats_.compute_sync_ms + stats_.device_to_host_ms +
                                 stats_.cache_copy_ms;
        constexpr double gib = 1024.0 * 1024.0 * 1024.0;
        std::fprintf(stderr,
            "[skintokens-profile] %.*s ms=%.3f logits_calls=%zu layers=%zu graph_computes=%zu "
            "setup_ms=%.3f h2d_ms=%.3f h2d_gib=%.3f compute_sync_ms=%.3f "
            "d2h_wait_ms=%.3f d2h_gib=%.3f cache_copy_ms=%.3f cache_copy_gib=%.3f "
            "cpu_other_ms=%.3f\n",
            static_cast<int>(stats_.label.size()), stats_.label.data(), total_ms,
            stats_.logits_calls, stats_.layers, stats_.graph_computes, stats_.setup_ms,
            stats_.host_to_device_ms, static_cast<double>(stats_.host_to_device_bytes) / gib,
            stats_.compute_sync_ms, stats_.device_to_host_ms,
            static_cast<double>(stats_.device_to_host_bytes) / gib,
            stats_.cache_copy_ms, static_cast<double>(stats_.cache_copy_bytes) / gib,
            std::max(0.0, total_ms - accounted));
    }
private:
    qwen_profile_stats stats_;
    qwen_profile_stats * previous_;
};

ggml_tensor * required(const weight_component & weights, std::string_view name) {
    auto * value = weights.tensor(name);
    if (value == nullptr) throw std::runtime_error("missing TokenRig tensor: " + std::string{name});
    return value;
}

ggml_tensor * linear(ggml_context * context, ggml_tensor * input, ggml_tensor * weight) {
    auto * output = ggml_mul_mat(context, weight, input);
    ggml_mul_mat_set_prec(output, GGML_PREC_F32);
    return output;
}

ggml_tensor * rms(ggml_context * context, ggml_tensor * input, ggml_tensor * weight, float epsilon) {
    auto * normalized = ggml_rms_norm(context, input, epsilon);
    return ggml_mul(context, normalized, ggml_repeat(context, weight, normalized));
}

ggml_tensor * repeat_kv(ggml_context * context, ggml_tensor * value, std::int64_t sequence) {
    auto * grouped = ggml_reshape_4d(context, value, head_dim, kv_heads, 1, sequence);
    auto * shape = ggml_new_tensor_4d(context, value->type, head_dim, kv_heads, heads / kv_heads, sequence);
    auto * repeated = ggml_repeat(context, grouped, shape);
    repeated = ggml_cont(context, ggml_permute(context, repeated, 0, 2, 1, 3));
    return ggml_reshape_3d(context, repeated, head_dim, heads, sequence);
}

ggml_tensor * repeat_kv_batched(ggml_context * context, ggml_tensor * value,
                                std::int64_t sequence, std::int64_t batch) {
    value = ggml_reshape_3d(context, value, head_dim, kv_heads, sequence * batch);
    value = repeat_kv(context, value, sequence * batch);
    return ggml_reshape_4d(context, value, head_dim, heads, sequence, batch);
}

ggml_tensor * qwen_layer_graph(ggml_context * context, ggml_tensor * input,
                               ggml_tensor * positions, const weight_component & weights,
                               std::size_t layer, std::int64_t sequence) {
    const std::string prefix = "llm.l." + std::to_string(layer) + ".";
    const auto get = [&](std::string_view suffix) {
        return required(weights, prefix + std::string{suffix});
    };
    const auto batch = input->ne[2];
    auto * normalized = rms(context, input, get("an.w"), 1e-6F);
    auto * q = ggml_reshape_4d(context, linear(context, normalized, get("attn.q.w")),
                              head_dim, heads, sequence, batch);
    auto * k = ggml_reshape_4d(context, linear(context, normalized, get("attn.k.w")),
                              head_dim, kv_heads, sequence, batch);
    auto * v = ggml_reshape_4d(context, linear(context, normalized, get("attn.v.w")),
                              head_dim, kv_heads, sequence, batch);
    q = rms(context, q, get("attn.q_norm.w"), 1e-6F);
    k = rms(context, k, get("attn.k_norm.w"), 1e-6F);
    q = ggml_rope_ext(context, q, positions, nullptr, head_dim, GGML_ROPE_TYPE_NEOX,
                      3192, 1'000'000.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F);
    k = ggml_rope_ext(context, k, positions, nullptr, head_dim, GGML_ROPE_TYPE_NEOX,
                      3192, 1'000'000.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F);
    k = repeat_kv_batched(context, k, sequence, batch);
    v = repeat_kv_batched(context, v, sequence, batch);
    q = ggml_permute(context, q, 0, 2, 1, 3);
    k = ggml_permute(context, k, 0, 2, 1, 3);
    v = ggml_permute(context, v, 0, 2, 1, 3);
    auto * scores = ggml_mul_mat(context, k, q);
    ggml_mul_mat_set_prec(scores, GGML_PREC_F32);
    scores = ggml_diag_mask_inf(context,
        ggml_scale(context, scores, 1.0F / std::sqrt(static_cast<float>(head_dim))), 0);
    auto * attended = ggml_mul_mat(context, ggml_cont(context, ggml_transpose(context, v)),
                                   ggml_soft_max(context, scores));
    ggml_mul_mat_set_prec(attended, GGML_PREC_F32);
    attended = ggml_cont(context, ggml_permute(context, attended, 0, 2, 1, 3));
    auto * state = ggml_add(context, input, linear(context,
        ggml_reshape_3d(context, attended, heads * head_dim, sequence, batch), get("attn.o.w")));
    normalized = rms(context, state, get("fn.w"), 1e-6F);
    auto * gate = ggml_silu(context, linear(context, normalized, get("mlp.g.w")));
    auto * up = linear(context, normalized, get("mlp.u.w"));
    return ggml_add(context, state,
        linear(context, ggml_mul(context, gate, up), get("mlp.d.w")));
}

std::vector<float> read_f32(ggml_tensor * tensor) {
    const auto count = ggml_nelements(tensor);
    if (count < 0) throw std::runtime_error("negative GGML tensor size");
    std::vector<float> output(static_cast<std::size_t>(count));
    if (tensor->type == GGML_TYPE_F32) {
        ggml_backend_tensor_get(tensor, output.data(), 0, output.size() * sizeof(float));
        return output;
    }
    throw std::runtime_error("parity snapshot unexpectedly is not F32");
}

std::int32_t sample_code(nonstd::span<const float> logits, nonstd::span<const std::int32_t> previous,
                         const generation_options & options, std::mt19937_64 & random) {
    constexpr std::int32_t first = 267;
    constexpr std::int32_t count = 32768;
    std::vector<std::pair<float, std::int32_t>> candidates;
    candidates.reserve(static_cast<std::size_t>(count));
    for (std::int32_t code = 0; code < count; ++code) {
        const auto token = first + code;
        float value = logits[static_cast<std::size_t>(token)];
        if (options.repetition_penalty > 0.0F && options.repetition_penalty != 1.0F &&
            std::find(previous.begin(), previous.end(), token) != previous.end())
            value = value < 0.0F ? value * options.repetition_penalty : value / options.repetition_penalty;
        candidates.emplace_back(value, token);
    }
    const std::size_t keep = options.top_k == 0U ? candidates.size() :
        std::min<std::size_t>(options.top_k, candidates.size());
    std::partial_sort(candidates.begin(), candidates.begin() + static_cast<std::ptrdiff_t>(keep), candidates.end(),
        [](const auto & a, const auto & b) { return a.first > b.first; });
    candidates.resize(keep);
    if (options.temperature <= 0.0F || keep == 1U) return candidates.front().second;
    const float maximum = candidates.front().first;
    std::vector<double> probabilities(keep);
    double total = 0.0;
    for (std::size_t index = 0; index < keep; ++index) {
        probabilities[index] = std::exp(static_cast<double>((candidates[index].first - maximum) / options.temperature));
        total += probabilities[index];
    }
    if (options.top_p > 0.0F && options.top_p < 1.0F) {
        double cumulative = 0.0;
        std::size_t nucleus = 0;
        for (; nucleus < probabilities.size(); ++nucleus) {
            cumulative += probabilities[nucleus] / total;
            if (cumulative >= options.top_p) { ++nucleus; break; }
        }
        nucleus = std::max<std::size_t>(1U, nucleus);
        candidates.resize(nucleus); probabilities.resize(nucleus);
    }
    std::discrete_distribution<std::size_t> distribution(probabilities.begin(), probabilities.end());
    return candidates[distribution(random)].second;
}

std::vector<std::pair<double, std::int32_t>> beam_candidates(
    nonstd::span<const float> logits, nonstd::span<const std::int32_t> previous,
    const generation_options & options) {
    constexpr std::int32_t first = 267;
    constexpr std::int32_t count = 32768;
    const float maximum = *std::max_element(logits.begin(), logits.end());
    double sum = 0.0;
    for (const float value : logits) sum += std::exp(static_cast<double>(value - maximum));
    const double normalization = static_cast<double>(maximum) + std::log(sum);
    std::vector<std::pair<double, std::int32_t>> candidates;
    candidates.reserve(static_cast<std::size_t>(count));
    for (std::int32_t code = 0; code < count; ++code) {
        const auto token = first + code;
        double score = static_cast<double>(logits[static_cast<std::size_t>(token)]) - normalization;
        if (options.repetition_penalty > 0.0F && options.repetition_penalty != 1.0F &&
            std::find(previous.begin(), previous.end(), token) != previous.end())
            score = score < 0.0 ? score * options.repetition_penalty : score / options.repetition_penalty;
        if (options.temperature > 0.0F) score /= options.temperature;
        candidates.emplace_back(score, token);
    }
    // Transformers 4.57 keeps two candidates per beam after top-k/top-p
    // warping (one EOS plus one continuation), not 2*num_beams candidates.
    const std::size_t minimum = options.beams > 1U ? 2U : 1U;
    const std::size_t keep = options.top_k == 0U ? candidates.size() :
        std::min(candidates.size(), std::max<std::size_t>(options.top_k, minimum));
    std::partial_sort(candidates.begin(), candidates.begin() + static_cast<std::ptrdiff_t>(keep), candidates.end(),
        [](const auto & left, const auto & right) { return left.first > right.first; });
    candidates.resize(keep);
    if (options.top_p > 0.0F && options.top_p < 1.0F) {
        const double peak = candidates.front().first;
        double total = 0.0;
        for (const auto & candidate : candidates) total += std::exp(candidate.first - peak);
        double cumulative = 0.0;
        std::size_t nucleus = 0U;
        for (; nucleus < candidates.size(); ++nucleus) {
            cumulative += std::exp(candidates[nucleus].first - peak) / total;
            if (cumulative >= options.top_p && nucleus + 1U >= minimum) { ++nucleus; break; }
        }
        candidates.resize(std::max<std::size_t>(1U, nucleus));
    }
    return candidates;
}

enum class skeleton_state {
    expect_bos,
    expect_cls_or_part_or_joint,
    expect_part_or_joint,
    expect_joint_2,
    expect_joint_3,
    expect_branch_or_part_or_joint,
    expect_joint,
};

struct skeleton_parse {
    skeleton_state state = skeleton_state::expect_bos;
    std::size_t joints = 0;
    bool ended = false;
};

skeleton_parse parse_skeleton(nonstd::span<const std::int32_t> ids) {
    skeleton_parse output;
    bool branch_half = false;
    for (const auto id : ids) {
        if (output.ended) throw std::runtime_error("tokens follow the skeleton terminator");
        switch (output.state) {
        case skeleton_state::expect_bos:
            if (id != 257) throw std::runtime_error("skeleton token sequence does not start with BOS");
            output.state = skeleton_state::expect_cls_or_part_or_joint; break;
        case skeleton_state::expect_cls_or_part_or_joint:
            if (id < 256) output.state = skeleton_state::expect_joint_2;
            else if (id >= 263 && id <= 266) output.state = skeleton_state::expect_part_or_joint;
            else if (id >= 260 && id <= 262) output.state = skeleton_state::expect_joint;
            else throw std::runtime_error("invalid token after skeleton BOS");
            break;
        case skeleton_state::expect_part_or_joint:
            if (id == 258) output.ended = true;
            else if (id < 256) output.state = skeleton_state::expect_joint_2;
            else if (id >= 260 && id <= 262) output.state = skeleton_state::expect_part_or_joint;
            else throw std::runtime_error("invalid skeleton part/joint token");
            break;
        case skeleton_state::expect_joint_2:
            if (id >= 256) throw std::runtime_error("invalid second coordinate token");
            output.state = skeleton_state::expect_joint_3; break;
        case skeleton_state::expect_joint_3:
            if (id >= 256) throw std::runtime_error("invalid third coordinate token");
            if (branch_half) branch_half = false;
            else ++output.joints;
            output.state = skeleton_state::expect_branch_or_part_or_joint; break;
        case skeleton_state::expect_branch_or_part_or_joint:
            if (id == 258) output.ended = true;
            else if (id == 256) { output.state = skeleton_state::expect_joint; branch_half = true; }
            else if (id < 256) output.state = skeleton_state::expect_joint_2;
            else if (id >= 260 && id <= 262) output.state = skeleton_state::expect_joint;
            else throw std::runtime_error("invalid token between skeleton joints");
            break;
        case skeleton_state::expect_joint:
            if (id >= 256) throw std::runtime_error("expected skeleton coordinate token");
            output.state = skeleton_state::expect_joint_2; break;
        }
    }
    return output;
}

std::vector<std::int32_t> allowed_skeleton_tokens(const skeleton_parse & parsed) {
    std::vector<std::int32_t> output;
    const auto coordinates = [&] { for (std::int32_t id = 0; id < 256; ++id) output.push_back(id); };
    const auto parts = [&] { output.insert(output.end(), {260, 261, 262}); };
    switch (parsed.state) {
    case skeleton_state::expect_bos: output.push_back(257); break;
    case skeleton_state::expect_cls_or_part_or_joint:
        output.insert(output.end(), {263, 264, 265, 266}); parts(); coordinates(); break;
    case skeleton_state::expect_part_or_joint:
        parts(); coordinates(); if (parsed.joints != 0U) output.push_back(258); break;
    case skeleton_state::expect_joint_2:
    case skeleton_state::expect_joint_3:
    case skeleton_state::expect_joint: coordinates(); break;
    case skeleton_state::expect_branch_or_part_or_joint:
        coordinates(); parts(); output.push_back(256); if (parsed.joints != 0U) output.push_back(258); break;
    }
    return output;
}

std::vector<std::pair<double, std::int32_t>> masked_candidates(
    nonstd::span<const float> logits, nonstd::span<const std::int32_t> allowed,
    nonstd::span<const std::int32_t> previous, const generation_options & options) {
    if (allowed.empty()) return {};
    const float maximum = *std::max_element(logits.begin(), logits.end());
    double full_sum = 0.0;
    for (const float value : logits) full_sum += std::exp(static_cast<double>(value - maximum));
    const double full_normalization = static_cast<double>(maximum) + std::log(full_sum);
    std::vector<std::pair<double, std::int32_t>> candidates;
    candidates.reserve(allowed.size());
    for (const auto token : allowed) {
        // Transformers beam sampling applies log_softmax over the complete
        // vocabulary before all logits processors/warpers, and does not
        // renormalize each beam after the grammar mask.
        double value = static_cast<double>(logits[static_cast<std::size_t>(token)]) - full_normalization;
        if (options.repetition_penalty > 0.0F && options.repetition_penalty != 1.0F &&
            std::find(previous.begin(), previous.end(), token) != previous.end())
            value = value < 0.0 ? value * options.repetition_penalty : value / options.repetition_penalty;
        if (options.temperature > 0.0F) value /= options.temperature;
        candidates.emplace_back(value, token);
    }
    const std::size_t minimum = std::min(candidates.size(),
        static_cast<std::size_t>(options.beams > 1U ? 2U : 1U));
    const std::size_t keep = options.top_k == 0U ? candidates.size() :
        std::min(candidates.size(), std::max<std::size_t>(options.top_k, minimum));
    std::partial_sort(candidates.begin(), candidates.begin() + static_cast<std::ptrdiff_t>(keep), candidates.end(),
        [](const auto & a, const auto & b) { return a.first > b.first; });
    candidates.resize(keep);
    if (options.top_p > 0.0F && options.top_p < 1.0F && candidates.size() > minimum) {
        const double local_peak = candidates.front().first;
        double local_sum = 0.0;
        for (const auto & candidate : candidates) local_sum += std::exp(candidate.first - local_peak);
        double cumulative = 0.0; std::size_t nucleus = 0;
        for (; nucleus < candidates.size(); ++nucleus) {
            cumulative += std::exp(candidates[nucleus].first - local_peak) / local_sum;
            if (cumulative >= options.top_p && nucleus + 1U >= minimum) { ++nucleus; break; }
        }
        candidates.resize(std::max<std::size_t>(1U, nucleus));
    }
    return candidates;
}

class qwen_graph_evaluator {
    using context_ptr = std::unique_ptr<ggml_context, decltype(&ggml_free)>;
    using allocator_ptr = std::unique_ptr<ggml_gallocr, decltype(&ggml_gallocr_free)>;
public:
    qwen_graph_evaluator(const weight_component & weights, ggml_backend_t backend,
                         std::size_t mesh_sequence, std::size_t token_count,
                         std::size_t batch_size)
        : backend_(backend), mesh_sequence_(mesh_sequence), token_count_(token_count),
          batch_size_(batch_size),
          context_(nullptr, ggml_free), allocator_(nullptr, ggml_gallocr_free) {
        const auto profile_start = profile_clock::now();
        const std::size_t sequence = mesh_sequence_ + token_count_;
        if (backend_ == nullptr || mesh_sequence_ == 0U || token_count_ == 0U ||
            batch_size_ == 0U || batch_size_ > 32U || sequence > 3192U)
            throw std::invalid_argument("invalid full Qwen graph shape");
        context_.reset(ggml_init({128ULL << 20U, nullptr, true}));
        if (!context_) throw std::runtime_error("cannot create full Qwen graph context");
        mesh_input_ = ggml_new_tensor_2d(context_.get(), GGML_TYPE_F32, hidden,
                                         static_cast<std::int64_t>(mesh_sequence_));
        positions_ = ggml_new_tensor_1d(context_.get(), GGML_TYPE_I32,
                                        static_cast<std::int64_t>(sequence));
        ggml_set_input(mesh_input_);
        ggml_set_input(positions_);
        ggml_tensor * token_state = nullptr;
        for (std::size_t beam = 0; beam < batch_size_; ++beam) {
            auto * ids = ggml_new_tensor_1d(context_.get(), GGML_TYPE_I32,
                                            static_cast<std::int64_t>(token_count_));
            ggml_set_input(ids);
            token_ids_.push_back(ids);
            auto * embedded = ggml_get_rows(context_.get(), required(weights, "llm.tok.w"), ids);
            token_state = token_state == nullptr ? embedded :
                ggml_concat(context_.get(), token_state, embedded, 2);
        }
        auto * mesh_shape = ggml_new_tensor_3d(context_.get(), GGML_TYPE_F32, hidden,
                                               static_cast<std::int64_t>(mesh_sequence_),
                                               static_cast<std::int64_t>(batch_size_));
        auto * state = ggml_concat(context_.get(),
            ggml_repeat(context_.get(), mesh_input_, mesh_shape), token_state, 1);
        for (std::size_t layer = 0; layer < 28U; ++layer) {
            state = qwen_layer_graph(context_.get(), state, positions_, weights, layer,
                                     static_cast<std::int64_t>(sequence));
        }
        auto * final_norm = rms(context_.get(), state, required(weights, "llm.norm.w"), 1e-6F);
        ggml_tensor * last = nullptr;
        for (std::size_t beam = 0; beam < batch_size_; ++beam) {
            auto * beam_last = ggml_view_2d(context_.get(), final_norm, hidden, 1,
                final_norm->nb[1], beam * final_norm->nb[2] +
                (sequence - 1U) * final_norm->nb[1]);
            last = last == nullptr ? beam_last : ggml_concat(context_.get(), last, beam_last, 1);
        }
        logits_ = linear(context_.get(), last, required(weights, "llm.out.w"));
        graph_ = ggml_new_graph_custom(context_.get(), 8192, false);
        ggml_build_forward_expand(graph_, logits_);
        allocator_.reset(ggml_gallocr_new(ggml_backend_get_default_buffer_type(backend_)));
        if (!allocator_ || !ggml_gallocr_reserve(allocator_.get(), graph_) ||
            !ggml_gallocr_alloc_graph(allocator_.get(), graph_))
            throw std::runtime_error("cannot allocate full Qwen graph");
        position_values_.resize(sequence);
        for (std::size_t index = 0; index < sequence; ++index)
            position_values_[index] = static_cast<std::int32_t>(index);
        if (active_qwen_profile != nullptr) {
            active_qwen_profile->layers += 28U;
            active_qwen_profile->setup_ms += elapsed_ms(profile_start);
        }
    }

    [[nodiscard]] std::size_t token_count() const noexcept { return token_count_; }
    [[nodiscard]] std::size_t batch_size() const noexcept { return batch_size_; }

    result<std::vector<float>> evaluate(nonstd::span<const float> mesh_embeddings,
                                        nonstd::span<const std::int32_t> tokens) try {
        if (mesh_embeddings.size() != mesh_sequence_ * static_cast<std::size_t>(hidden) ||
            tokens.size() != token_count_ * batch_size_)
            return tl::unexpected(fail(error_code::invalid_argument, "Qwen evaluator input shape changed"));
        if (active_qwen_profile != nullptr) ++active_qwen_profile->logits_calls;
        auto phase_start = profile_clock::now();
        ggml_backend_tensor_set(mesh_input_, mesh_embeddings.data(), 0, mesh_embeddings.size_bytes());
        for (std::size_t beam = 0; beam < batch_size_; ++beam)
            ggml_backend_tensor_set(token_ids_[beam], tokens.data() + beam * token_count_, 0,
                                    token_count_ * sizeof(std::int32_t));
        ggml_backend_tensor_set(positions_, position_values_.data(), 0,
                                position_values_.size() * sizeof(std::int32_t));
        if (active_qwen_profile != nullptr) {
            active_qwen_profile->host_to_device_ms += elapsed_ms(phase_start);
            active_qwen_profile->host_to_device_bytes += mesh_embeddings.size_bytes() +
                tokens.size_bytes() + position_values_.size() * sizeof(std::int32_t);
        }
        phase_start = profile_clock::now();
        if (ggml_backend_graph_compute(backend_, graph_) != GGML_STATUS_SUCCESS)
            return tl::unexpected(fail(error_code::compute, "full Qwen graph execution failed"));
        if (active_qwen_profile != nullptr) {
            active_qwen_profile->compute_sync_ms += elapsed_ms(phase_start);
            ++active_qwen_profile->graph_computes;
        }
        phase_start = profile_clock::now();
        auto result = read_f32(logits_);
        if (active_qwen_profile != nullptr) {
            active_qwen_profile->device_to_host_ms += elapsed_ms(phase_start);
            active_qwen_profile->device_to_host_bytes += result.size() * sizeof(float);
        }
        return result;
    } catch (const std::exception & exception) {
        return tl::unexpected(fail(error_code::compute, exception.what()));
    }

private:
    ggml_backend_t backend_ = nullptr;
    std::size_t mesh_sequence_ = 0U;
    std::size_t token_count_ = 0U;
    std::size_t batch_size_ = 0U;
    context_ptr context_;
    allocator_ptr allocator_;
    ggml_tensor * mesh_input_ = nullptr;
    std::vector<ggml_tensor *> token_ids_;
    ggml_tensor * positions_ = nullptr;
    ggml_tensor * logits_ = nullptr;
    ggml_cgraph * graph_ = nullptr;
    std::vector<std::int32_t> position_values_;
};

class qwen_graph_cache {
public:
    qwen_graph_cache(const weight_component & weights, ggml_backend_t backend,
                     std::size_t mesh_sequence)
        : weights_(weights), backend_(backend), mesh_sequence_(mesh_sequence) {}

    result<std::vector<float>> evaluate(nonstd::span<const float> mesh_embeddings,
                                        nonstd::span<const std::int32_t> tokens,
                                        std::size_t token_count, std::size_t batch_size = 1U) try {
        if (tokens.size() != token_count * batch_size)
            return tl::unexpected(fail(error_code::invalid_argument, "invalid batched Qwen token input"));
        if (!evaluator_ || evaluator_->token_count() != token_count ||
            evaluator_->batch_size() != batch_size)
            evaluator_ = std::make_unique<qwen_graph_evaluator>(
                weights_, backend_, mesh_sequence_, token_count, batch_size);
        return evaluator_->evaluate(mesh_embeddings, tokens);
    } catch (const std::exception & exception) {
        return tl::unexpected(fail(error_code::compute, exception.what()));
    }
private:
    const weight_component & weights_;
    ggml_backend_t backend_;
    std::size_t mesh_sequence_;
    std::unique_ptr<qwen_graph_evaluator> evaluator_;
};

class qwen_kv_generator {
    using context_ptr = std::unique_ptr<ggml_context, decltype(&ggml_free)>;
    using buffer_ptr = std::unique_ptr<ggml_backend_buffer, decltype(&ggml_backend_buffer_free)>;
    using allocator_ptr = std::unique_ptr<ggml_gallocr, decltype(&ggml_gallocr_free)>;

public:
    qwen_kv_generator(const weight_component & weights, ggml_backend_t backend,
                      std::size_t mesh_sequence, std::size_t capacity,
                      std::size_t maximum_beams)
        : weights_(weights), backend_(backend), mesh_sequence_(mesh_sequence),
          capacity_(capacity), maximum_beams_(maximum_beams),
          cache_context_(nullptr, ggml_free), cache_buffer_(nullptr, ggml_backend_buffer_free) {
        const auto setup_start = profile_clock::now();
        if (backend_ == nullptr || mesh_sequence_ == 0U || capacity_ == 0U ||
            capacity_ > 3192U || maximum_beams_ == 0U || maximum_beams_ > 32U)
            throw std::invalid_argument("invalid TokenRig KV cache shape");
        cache_context_.reset(ggml_init({8ULL << 20U, nullptr, true}));
        if (!cache_context_) throw std::runtime_error("cannot create TokenRig KV cache context");
        keys_.reserve(28U);
        values_.reserve(28U);
        for (std::size_t layer = 0; layer < 28U; ++layer) {
            keys_.push_back(ggml_new_tensor_4d(cache_context_.get(), GGML_TYPE_F32,
                head_dim, kv_heads, static_cast<std::int64_t>(capacity_),
                static_cast<std::int64_t>(maximum_beams_)));
            values_.push_back(ggml_new_tensor_4d(cache_context_.get(), GGML_TYPE_F32,
                head_dim, kv_heads, static_cast<std::int64_t>(capacity_),
                static_cast<std::int64_t>(maximum_beams_)));
        }
        cache_buffer_.reset(ggml_backend_alloc_ctx_tensors(cache_context_.get(), backend_));
        if (!cache_buffer_) throw std::runtime_error("cannot allocate TokenRig KV cache");
        if (active_qwen_profile != nullptr)
            active_qwen_profile->setup_ms += elapsed_ms(setup_start);
    }

    [[nodiscard]] result<std::vector<float>> prefill(
        nonstd::span<const float> mesh_embeddings,
        nonstd::span<const std::int32_t> tokens) try {
        const auto profile_start = profile_clock::now();
        const std::size_t sequence = mesh_sequence_ + tokens.size();
        if (mesh_embeddings.size() != mesh_sequence_ * static_cast<std::size_t>(hidden) ||
            tokens.empty() || sequence > capacity_)
            return tl::unexpected(fail(error_code::invalid_argument, "invalid TokenRig KV prefill input"));
        auto context = context_ptr{ggml_init({128ULL << 20U, nullptr, true}), ggml_free};
        if (!context) return tl::unexpected(fail(error_code::allocation, "cannot create KV prefill graph"));
        auto * mesh_input = ggml_new_tensor_2d(context.get(), GGML_TYPE_F32, hidden,
                                               static_cast<std::int64_t>(mesh_sequence_));
        auto * ids = ggml_new_tensor_1d(context.get(), GGML_TYPE_I32,
                                        static_cast<std::int64_t>(tokens.size()));
        auto * positions = ggml_new_tensor_1d(context.get(), GGML_TYPE_I32,
                                              static_cast<std::int64_t>(sequence));
        ggml_set_input(mesh_input);
        ggml_set_input(ids);
        ggml_set_input(positions);
        auto * state = ggml_concat(context.get(), mesh_input,
            ggml_get_rows(context.get(), required(weights_, "llm.tok.w"), ids), 1);
        for (std::size_t layer = 0; layer < 28U; ++layer)
            state = prefill_layer(context.get(), state, positions, layer,
                                  static_cast<std::int64_t>(sequence));
        auto * normalized = rms(context.get(), state, required(weights_, "llm.norm.w"), 1e-6F);
        auto * last = ggml_view_2d(context.get(), normalized, hidden, 1, normalized->nb[1],
                                  (sequence - 1U) * normalized->nb[1]);
        auto * logits = linear(context.get(), last, required(weights_, "llm.out.w"));
        auto * graph = ggml_new_graph_custom(context.get(), 8192, false);
        ggml_build_forward_expand(graph, logits);
        auto allocator = allocator_ptr{
            ggml_gallocr_new(ggml_backend_get_default_buffer_type(backend_)), ggml_gallocr_free};
        if (!allocator || !ggml_gallocr_reserve(allocator.get(), graph) ||
            !ggml_gallocr_alloc_graph(allocator.get(), graph))
            return tl::unexpected(fail(error_code::allocation, "cannot allocate KV prefill graph"));
        std::vector<std::int32_t> position_values(sequence);
        for (std::size_t index = 0; index < sequence; ++index)
            position_values[index] = static_cast<std::int32_t>(index);
        if (active_qwen_profile != nullptr) {
            active_qwen_profile->setup_ms += elapsed_ms(profile_start);
            active_qwen_profile->layers += 28U;
            ++active_qwen_profile->logits_calls;
        }
        auto phase_start = profile_clock::now();
        ggml_backend_tensor_set(mesh_input, mesh_embeddings.data(), 0, mesh_embeddings.size_bytes());
        ggml_backend_tensor_set(ids, tokens.data(), 0, tokens.size_bytes());
        ggml_backend_tensor_set(positions, position_values.data(), 0, position_values.size() * sizeof(std::int32_t));
        if (active_qwen_profile != nullptr) {
            active_qwen_profile->host_to_device_ms += elapsed_ms(phase_start);
            active_qwen_profile->host_to_device_bytes += mesh_embeddings.size_bytes() +
                tokens.size_bytes() + position_values.size() * sizeof(std::int32_t);
        }
        phase_start = profile_clock::now();
        if (ggml_backend_graph_compute(backend_, graph) != GGML_STATUS_SUCCESS)
            return tl::unexpected(fail(error_code::compute, "TokenRig KV prefill failed"));
        if (active_qwen_profile != nullptr) {
            active_qwen_profile->compute_sync_ms += elapsed_ms(phase_start);
            ++active_qwen_profile->graph_computes;
        }
        phase_start = profile_clock::now();
        auto output = read_f32(logits);
        if (active_qwen_profile != nullptr) {
            active_qwen_profile->device_to_host_ms += elapsed_ms(phase_start);
            active_qwen_profile->device_to_host_bytes += output.size() * sizeof(float);
        }
        return output;
    } catch (const std::exception & exception) {
        return tl::unexpected(fail(error_code::compute, exception.what()));
    }

    [[nodiscard]] result<std::vector<float>> decode(
        nonstd::span<const std::int32_t> tokens, nonstd::span<const std::size_t> slots,
        std::size_t past) try {
        const auto profile_start = profile_clock::now();
        const std::size_t batch = tokens.size();
        if (batch == 0U || slots.size() != batch || batch > maximum_beams_ ||
            past == 0U || past >= capacity_ ||
            std::any_of(slots.begin(), slots.end(), [&](std::size_t slot) { return slot >= maximum_beams_; }))
            return tl::unexpected(fail(error_code::invalid_argument, "invalid TokenRig KV decode input"));
        auto context = context_ptr{ggml_init({96ULL << 20U, nullptr, true}), ggml_free};
        if (!context) return tl::unexpected(fail(error_code::allocation, "cannot create KV decode graph"));
        auto * ids = ggml_new_tensor_1d(context.get(), GGML_TYPE_I32, static_cast<std::int64_t>(batch));
        auto * position = ggml_new_tensor_1d(context.get(), GGML_TYPE_I32, 1);
        ggml_set_input(ids);
        ggml_set_input(position);
        auto * state = ggml_reshape_3d(context.get(),
            ggml_get_rows(context.get(), required(weights_, "llm.tok.w"), ids),
            hidden, 1, static_cast<std::int64_t>(batch));
        for (std::size_t layer = 0; layer < 28U; ++layer)
            state = decode_layer(context.get(), state, position, slots, layer, past);
        auto * normalized = rms(context.get(), state, required(weights_, "llm.norm.w"), 1e-6F);
        auto * last = ggml_reshape_2d(context.get(), normalized, hidden,
                                     static_cast<std::int64_t>(batch));
        auto * logits = linear(context.get(), last, required(weights_, "llm.out.w"));
        auto * graph = ggml_new_graph_custom(context.get(), 8192, false);
        ggml_build_forward_expand(graph, logits);
        auto allocator = allocator_ptr{
            ggml_gallocr_new(ggml_backend_get_default_buffer_type(backend_)), ggml_gallocr_free};
        if (!allocator || !ggml_gallocr_reserve(allocator.get(), graph) ||
            !ggml_gallocr_alloc_graph(allocator.get(), graph))
            return tl::unexpected(fail(error_code::allocation, "cannot allocate KV decode graph"));
        if (active_qwen_profile != nullptr) {
            active_qwen_profile->setup_ms += elapsed_ms(profile_start);
            active_qwen_profile->layers += 28U;
            ++active_qwen_profile->logits_calls;
        }
        auto phase_start = profile_clock::now();
        const auto position_value = static_cast<std::int32_t>(past);
        ggml_backend_tensor_set(ids, tokens.data(), 0, tokens.size_bytes());
        ggml_backend_tensor_set(position, &position_value, 0, sizeof(position_value));
        if (active_qwen_profile != nullptr) {
            active_qwen_profile->host_to_device_ms += elapsed_ms(phase_start);
            active_qwen_profile->host_to_device_bytes += tokens.size_bytes() + sizeof(position_value);
        }
        phase_start = profile_clock::now();
        if (ggml_backend_graph_compute(backend_, graph) != GGML_STATUS_SUCCESS)
            return tl::unexpected(fail(error_code::compute, "TokenRig KV decode failed"));
        if (active_qwen_profile != nullptr) {
            active_qwen_profile->compute_sync_ms += elapsed_ms(phase_start);
            ++active_qwen_profile->graph_computes;
        }
        phase_start = profile_clock::now();
        auto output = read_f32(logits);
        if (active_qwen_profile != nullptr) {
            active_qwen_profile->device_to_host_ms += elapsed_ms(phase_start);
            active_qwen_profile->device_to_host_bytes += output.size() * sizeof(float);
        }
        return output;
    } catch (const std::exception & exception) {
        return tl::unexpected(fail(error_code::compute, exception.what()));
    }

    [[nodiscard]] result<std::vector<std::size_t>> remap(
        nonstd::span<const std::size_t> old_slots,
        nonstd::span<const std::size_t> parents,
        std::size_t length) try {
        if (parents.empty()) return std::vector<std::size_t>{};
        if (length == 0U || length > capacity_ || parents.size() > maximum_beams_ ||
            old_slots.size() > maximum_beams_ ||
            std::any_of(old_slots.begin(), old_slots.end(),
                [&](std::size_t slot) { return slot >= maximum_beams_; }) ||
            std::any_of(parents.begin(), parents.end(), [&](std::size_t parent) { return parent >= old_slots.size(); }))
            return tl::unexpected(fail(error_code::invalid_argument, "invalid TokenRig beam remap"));
        auto unique_slots = std::vector<std::size_t>{old_slots.begin(), old_slots.end()};
        std::sort(unique_slots.begin(), unique_slots.end());
        if (std::adjacent_find(unique_slots.begin(), unique_slots.end()) != unique_slots.end())
            return tl::unexpected(fail(error_code::invalid_argument, "TokenRig beam cache slots overlap"));
        constexpr std::size_t unassigned = std::numeric_limits<std::size_t>::max();
        std::vector<std::size_t> output(parents.size(), unassigned);
        std::vector<bool> retained(maximum_beams_, false);
        std::vector<bool> parent_seen(old_slots.size(), false);
        for (std::size_t child = 0; child < parents.size(); ++child) {
            const auto parent = parents[child];
            if (!parent_seen[parent]) {
                output[child] = old_slots[parent];
                retained[output[child]] = true;
                parent_seen[parent] = true;
            }
        }
        std::vector<std::size_t> free_slots;
        for (std::size_t slot = 0; slot < maximum_beams_; ++slot)
            if (!retained[slot]) free_slots.push_back(slot);
        std::size_t free_index = 0U;
        const auto copy_start = profile_clock::now();
        std::uint64_t copied = 0U;
        for (std::size_t child = 0; child < parents.size(); ++child) {
            const auto parent = parents[child];
            if (output[child] != unassigned) continue;
            if (free_index >= free_slots.size())
                return tl::unexpected(fail(error_code::allocation, "no free TokenRig beam cache slot"));
            const auto destination = free_slots[free_index++];
            output[child] = destination;
            clone_slot(old_slots[parent], destination, length);
            copied += static_cast<std::uint64_t>(length) * head_dim * kv_heads *
                      sizeof(float) * 2U * 28U;
        }
        if (copied != 0U) {
            ggml_backend_synchronize(backend_);
            if (active_qwen_profile != nullptr) {
                active_qwen_profile->cache_copy_ms += elapsed_ms(copy_start);
                active_qwen_profile->cache_copy_bytes += copied;
            }
        }
        return output;
    } catch (const std::exception & exception) {
        return tl::unexpected(fail(error_code::compute, exception.what()));
    }

private:
    ggml_tensor * cache_view(ggml_context * context, ggml_tensor * cache,
                             std::size_t slot, std::size_t length,
                             std::size_t position = 0U) const {
        return ggml_view_4d(context, cache, head_dim, kv_heads,
            static_cast<std::int64_t>(length), 1, cache->nb[1], cache->nb[2], cache->nb[3],
            slot * cache->nb[3] + position * cache->nb[2]);
    }

    ggml_tensor * prefill_layer(ggml_context * context, ggml_tensor * input,
                                ggml_tensor * positions, std::size_t layer,
                                std::int64_t sequence) {
        const std::string prefix = "llm.l." + std::to_string(layer) + ".";
        const auto get = [&](std::string_view suffix) {
            return required(weights_, prefix + std::string{suffix});
        };
        auto * normalized = rms(context, input, get("an.w"), 1e-6F);
        auto * q = ggml_reshape_4d(context, linear(context, normalized, get("attn.q.w")),
                                  head_dim, heads, sequence, 1);
        auto * k = ggml_reshape_4d(context, linear(context, normalized, get("attn.k.w")),
                                  head_dim, kv_heads, sequence, 1);
        auto * v = ggml_reshape_4d(context, linear(context, normalized, get("attn.v.w")),
                                  head_dim, kv_heads, sequence, 1);
        q = rms(context, q, get("attn.q_norm.w"), 1e-6F);
        k = rms(context, k, get("attn.k_norm.w"), 1e-6F);
        q = ggml_rope_ext(context, q, positions, nullptr, head_dim, GGML_ROPE_TYPE_NEOX,
                          3192, 1'000'000.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F);
        k = ggml_rope_ext(context, k, positions, nullptr, head_dim, GGML_ROPE_TYPE_NEOX,
                          3192, 1'000'000.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F);
        k = ggml_cpy(context, k, cache_view(context, keys_[layer], 0U,
                                            static_cast<std::size_t>(sequence)));
        v = ggml_cpy(context, v, cache_view(context, values_[layer], 0U,
                                            static_cast<std::size_t>(sequence)));
        k = repeat_kv_batched(context, k, sequence, 1);
        v = repeat_kv_batched(context, v, sequence, 1);
        q = ggml_permute(context, q, 0, 2, 1, 3);
        k = ggml_permute(context, k, 0, 2, 1, 3);
        v = ggml_permute(context, v, 0, 2, 1, 3);
        auto * scores = ggml_mul_mat(context, k, q);
        ggml_mul_mat_set_prec(scores, GGML_PREC_F32);
        scores = ggml_diag_mask_inf(context,
            ggml_scale(context, scores, 1.0F / std::sqrt(static_cast<float>(head_dim))), 0);
        auto * attended = ggml_mul_mat(context, ggml_cont(context, ggml_transpose(context, v)),
                                       ggml_soft_max(context, scores));
        ggml_mul_mat_set_prec(attended, GGML_PREC_F32);
        attended = ggml_cont(context, ggml_permute(context, attended, 0, 2, 1, 3));
        auto * state = ggml_add(context, input, linear(context,
            ggml_reshape_2d(context, attended, heads * head_dim, sequence), get("attn.o.w")));
        normalized = rms(context, state, get("fn.w"), 1e-6F);
        auto * gate = ggml_silu(context, linear(context, normalized, get("mlp.g.w")));
        auto * up = linear(context, normalized, get("mlp.u.w"));
        return ggml_add(context, state,
            linear(context, ggml_mul(context, gate, up), get("mlp.d.w")));
    }

    ggml_tensor * decode_layer(ggml_context * context, ggml_tensor * input,
                               ggml_tensor * position, nonstd::span<const std::size_t> slots,
                               std::size_t layer, std::size_t past) {
        const std::string prefix = "llm.l." + std::to_string(layer) + ".";
        const auto get = [&](std::string_view suffix) {
            return required(weights_, prefix + std::string{suffix});
        };
        const auto batch = static_cast<std::int64_t>(slots.size());
        auto * normalized = rms(context, input, get("an.w"), 1e-6F);
        auto * q = ggml_reshape_4d(context, linear(context, normalized, get("attn.q.w")),
                                  head_dim, heads, 1, batch);
        auto * k_new = ggml_reshape_4d(context, linear(context, normalized, get("attn.k.w")),
                                      head_dim, kv_heads, 1, batch);
        auto * v_new = ggml_reshape_4d(context, linear(context, normalized, get("attn.v.w")),
                                      head_dim, kv_heads, 1, batch);
        q = rms(context, q, get("attn.q_norm.w"), 1e-6F);
        k_new = rms(context, k_new, get("attn.k_norm.w"), 1e-6F);
        q = ggml_rope_ext(context, q, position, nullptr, head_dim, GGML_ROPE_TYPE_NEOX,
                          3192, 1'000'000.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F);
        k_new = ggml_rope_ext(context, k_new, position, nullptr, head_dim, GGML_ROPE_TYPE_NEOX,
                              3192, 1'000'000.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F);
        ggml_tensor * all_k = nullptr;
        ggml_tensor * all_v = nullptr;
        for (std::size_t beam = 0; beam < slots.size(); ++beam) {
            auto * beam_k = ggml_view_4d(context, k_new, head_dim, kv_heads, 1, 1,
                                         k_new->nb[1], k_new->nb[2], k_new->nb[3], beam * k_new->nb[3]);
            auto * beam_v = ggml_view_4d(context, v_new, head_dim, kv_heads, 1, 1,
                                         v_new->nb[1], v_new->nb[2], v_new->nb[3], beam * v_new->nb[3]);
            beam_k = ggml_cpy(context, beam_k, cache_view(context, keys_[layer], slots[beam], 1U, past));
            beam_v = ggml_cpy(context, beam_v, cache_view(context, values_[layer], slots[beam], 1U, past));
            auto * slot_k = ggml_concat(context, cache_view(context, keys_[layer], slots[beam], past), beam_k, 2);
            auto * slot_v = ggml_concat(context, cache_view(context, values_[layer], slots[beam], past), beam_v, 2);
            all_k = all_k == nullptr ? slot_k : ggml_concat(context, all_k, slot_k, 3);
            all_v = all_v == nullptr ? slot_v : ggml_concat(context, all_v, slot_v, 3);
        }
        const auto sequence = static_cast<std::int64_t>(past + 1U);
        all_k = repeat_kv_batched(context, all_k, sequence, batch);
        all_v = repeat_kv_batched(context, all_v, sequence, batch);
        q = ggml_permute(context, q, 0, 2, 1, 3);
        all_k = ggml_permute(context, all_k, 0, 2, 1, 3);
        all_v = ggml_permute(context, all_v, 0, 2, 1, 3);
        auto * scores = ggml_mul_mat(context, all_k, q);
        ggml_mul_mat_set_prec(scores, GGML_PREC_F32);
        scores = ggml_scale(context, scores, 1.0F / std::sqrt(static_cast<float>(head_dim)));
        auto * attended = ggml_mul_mat(context, ggml_cont(context, ggml_transpose(context, all_v)),
                                       ggml_soft_max(context, scores));
        ggml_mul_mat_set_prec(attended, GGML_PREC_F32);
        attended = ggml_cont(context, ggml_permute(context, attended, 0, 2, 1, 3));
        auto * state = ggml_add(context, input, linear(context,
            ggml_reshape_3d(context, attended, heads * head_dim, 1, batch), get("attn.o.w")));
        normalized = rms(context, state, get("fn.w"), 1e-6F);
        auto * gate = ggml_silu(context, linear(context, normalized, get("mlp.g.w")));
        auto * up = linear(context, normalized, get("mlp.u.w"));
        return ggml_add(context, state,
            linear(context, ggml_mul(context, gate, up), get("mlp.d.w")));
    }

    void clone_slot(std::size_t source, std::size_t destination, std::size_t length) {
        auto context = context_ptr{ggml_init({4ULL << 20U, nullptr, true}), ggml_free};
        if (!context) throw std::runtime_error("cannot create TokenRig cache-copy context");
        for (std::size_t layer = 0; layer < 28U; ++layer) {
            for (auto * cache : {keys_[layer], values_[layer]}) {
                auto * src = cache_view(context.get(), cache, source, length);
                auto * dst = cache_view(context.get(), cache, destination, length);
                if (ggml_backend_view_init(src) != GGML_STATUS_SUCCESS ||
                    ggml_backend_view_init(dst) != GGML_STATUS_SUCCESS)
                    throw std::runtime_error("cannot initialize TokenRig cache-copy view");
                ggml_backend_tensor_copy_async(backend_, backend_, src, dst);
            }
        }
    }

    const weight_component & weights_;
    ggml_backend_t backend_ = nullptr;
    std::size_t mesh_sequence_ = 0U;
    std::size_t capacity_ = 0U;
    std::size_t maximum_beams_ = 0U;
    context_ptr cache_context_;
    buffer_ptr cache_buffer_;
    std::vector<ggml_tensor *> keys_;
    std::vector<ggml_tensor *> values_;
};

} // namespace

result<tensor_snapshot> run_qwen_layer0(const weight_component & weights, ggml_backend_t backend,
                                        nonstd::span<const float> input, std::size_t sequence) try {
    if (backend == nullptr || sequence == 0U || sequence > 3192U ||
        input.size() != sequence * static_cast<std::size_t>(hidden))
        return tl::unexpected(fail(error_code::invalid_argument, "invalid Qwen layer parity input"));
    auto * context = ggml_init({96ULL << 20U, nullptr, true});
    if (context == nullptr)
        return tl::unexpected(fail(error_code::allocation, "cannot create Qwen graph context"));
    auto cleanup = std::unique_ptr<ggml_context, decltype(&ggml_free)>(context, ggml_free);
    const auto seq = static_cast<std::int64_t>(sequence);
    auto * x = ggml_new_tensor_2d(context, GGML_TYPE_F32, hidden, seq);
    auto * positions = ggml_new_tensor_1d(context, GGML_TYPE_I32, seq);
    ggml_set_input(x);
    ggml_set_input(positions);

    auto * attn_norm = rms(context, x, required(weights, "llm.l.0.an.w"), 1e-6F);
    auto * q_linear = linear(context, attn_norm, required(weights, "llm.l.0.attn.q.w"));
    auto * k_linear = linear(context, attn_norm, required(weights, "llm.l.0.attn.k.w"));
    auto * v_linear = linear(context, attn_norm, required(weights, "llm.l.0.attn.v.w"));
    auto * q = ggml_reshape_3d(context, q_linear, head_dim, heads, seq);
    auto * k = ggml_reshape_3d(context, k_linear, head_dim, kv_heads, seq);
    auto * v = ggml_reshape_3d(context, v_linear, head_dim, kv_heads, seq);
    auto * q_norm = rms(context, q, required(weights, "llm.l.0.attn.q_norm.w"), 1e-6F);
    auto * k_norm = rms(context, k, required(weights, "llm.l.0.attn.k_norm.w"), 1e-6F);
    q = ggml_rope_ext(context, q_norm, positions, nullptr, head_dim, GGML_ROPE_TYPE_NEOX,
                      3192, 1'000'000.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F);
    k = ggml_rope_ext(context, k_norm, positions, nullptr, head_dim, GGML_ROPE_TYPE_NEOX,
                      3192, 1'000'000.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F);
    auto * q_rope = q;
    auto * k_rope = k;
    k = repeat_kv(context, k, seq);
    v = repeat_kv(context, v, seq);
    q = ggml_permute(context, q, 0, 2, 1, 3);
    k = ggml_permute(context, k, 0, 2, 1, 3);
    v = ggml_permute(context, v, 0, 2, 1, 3);
    auto * scores = ggml_mul_mat(context, k, q);
    ggml_mul_mat_set_prec(scores, GGML_PREC_F32);
    scores = ggml_scale(context, scores, 1.0F / std::sqrt(static_cast<float>(head_dim)));
    auto * raw_scores = scores;
    scores = ggml_diag_mask_inf(context, scores, 0);
    auto * probabilities = ggml_soft_max(context, scores);
    v = ggml_cont(context, ggml_transpose(context, v));
    auto * attention = ggml_mul_mat(context, v, probabilities);
    ggml_mul_mat_set_prec(attention, GGML_PREC_F32);
    attention = ggml_cont(context, ggml_permute(context, attention, 0, 2, 1, 3));
    auto * o_linear = linear(context, ggml_reshape_2d(context, attention, heads * head_dim, seq),
                             required(weights, "llm.l.0.attn.o.w"));
    auto * state = ggml_add(context, x, o_linear);
    auto * ffn_norm = rms(context, state, required(weights, "llm.l.0.fn.w"), 1e-6F);
    auto * gate_linear = linear(context, ffn_norm, required(weights, "llm.l.0.mlp.g.w"));
    auto * up_linear = linear(context, ffn_norm, required(weights, "llm.l.0.mlp.u.w"));
    auto * activated = ggml_mul(context, ggml_silu(context, gate_linear), up_linear);
    auto * down_linear = linear(context, activated, required(weights, "llm.l.0.mlp.d.w"));
    auto * layer_output = ggml_add(context, state, down_linear);

    auto * attention_flat = ggml_reshape_2d(context, attention, heads * head_dim, seq);
    const std::pair<const char *, ggml_tensor *> snapshots[] = {
        {"attn_norm", attn_norm}, {"q_linear", q_linear}, {"k_linear", k_linear},
        {"v_linear", v_linear}, {"q_norm", q_norm}, {"k_norm", k_norm},
        {"q_rope", q_rope}, {"k_rope", k_rope}, {"scores", raw_scores},
        {"probabilities", probabilities}, {"attention", attention_flat},
        {"o_linear", o_linear}, {"ffn_norm", ffn_norm}, {"gate_linear", gate_linear},
        {"up_linear", up_linear}, {"down_linear", down_linear}, {"layer_output", layer_output},
    };
    for (const auto & [name, tensor] : snapshots) {
        (void) name;
        ggml_set_output(tensor);
    }
    auto * graph = ggml_new_graph_custom(context, 4096, false);
    for (const auto & [name, tensor] : snapshots) {
        (void) name;
        ggml_build_forward_expand(graph, tensor);
    }
    auto * allocator = ggml_gallocr_new(ggml_backend_get_default_buffer_type(backend));
    if (allocator == nullptr || !ggml_gallocr_reserve(allocator, graph) || !ggml_gallocr_alloc_graph(allocator, graph)) {
        if (allocator != nullptr) ggml_gallocr_free(allocator);
        return tl::unexpected(fail(error_code::allocation, "cannot allocate Qwen layer graph"));
    }
    auto allocator_cleanup = std::unique_ptr<ggml_gallocr, decltype(&ggml_gallocr_free)>(allocator, ggml_gallocr_free);
    std::vector<std::int32_t> position_values(sequence);
    for (std::size_t index = 0; index < sequence; ++index)
        position_values[index] = static_cast<std::int32_t>(index);
    ggml_backend_tensor_set(x, input.data(), 0, input.size_bytes());
    ggml_backend_tensor_set(positions, position_values.data(), 0, position_values.size() * sizeof(std::int32_t));
    if (ggml_backend_graph_compute(backend, graph) != GGML_STATUS_SUCCESS)
        return tl::unexpected(fail(error_code::compute, "Qwen layer graph execution failed"));
    tensor_snapshot output;
    for (const auto & [name, tensor] : snapshots) output.emplace(name, read_f32(tensor));
    return output;
} catch (const std::exception & exception) {
    return tl::unexpected(fail(error_code::compute, exception.what()));
}

result<std::vector<float>> run_qwen_layer(const weight_component & weights, ggml_backend_t backend,
                                          nonstd::span<const float> input, std::size_t sequence,
                                          std::size_t layer) try {
    const auto profile_start = profile_clock::now();
    if (backend == nullptr || sequence == 0U || sequence > 3192U || layer >= 28U ||
        input.size() != sequence * static_cast<std::size_t>(hidden))
        return tl::unexpected(fail(error_code::invalid_argument, "invalid Qwen layer input"));
    auto * context = ggml_init({64ULL << 20U, nullptr, true});
    if (context == nullptr) return tl::unexpected(fail(error_code::allocation, "cannot create Qwen graph context"));
    auto cleanup = std::unique_ptr<ggml_context, decltype(&ggml_free)>(context, ggml_free);
    const auto seq = static_cast<std::int64_t>(sequence);
    const std::string prefix = "llm.l." + std::to_string(layer) + ".";
    auto get = [&](std::string_view suffix) { return required(weights, prefix + std::string{suffix}); };
    auto * x = ggml_new_tensor_2d(context, GGML_TYPE_F32, hidden, seq);
    auto * positions = ggml_new_tensor_1d(context, GGML_TYPE_I32, seq);
    ggml_set_input(x); ggml_set_input(positions);
    auto * normalized = rms(context, x, get("an.w"), 1e-6F);
    auto * q = ggml_reshape_3d(context, linear(context, normalized, get("attn.q.w")), head_dim, heads, seq);
    auto * k = ggml_reshape_3d(context, linear(context, normalized, get("attn.k.w")), head_dim, kv_heads, seq);
    auto * v = ggml_reshape_3d(context, linear(context, normalized, get("attn.v.w")), head_dim, kv_heads, seq);
    q = rms(context, q, get("attn.q_norm.w"), 1e-6F);
    k = rms(context, k, get("attn.k_norm.w"), 1e-6F);
    q = ggml_rope_ext(context, q, positions, nullptr, head_dim, GGML_ROPE_TYPE_NEOX,
                      3192, 1'000'000.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F);
    k = ggml_rope_ext(context, k, positions, nullptr, head_dim, GGML_ROPE_TYPE_NEOX,
                      3192, 1'000'000.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F);
    k = repeat_kv(context, k, seq); v = repeat_kv(context, v, seq);
    q = ggml_permute(context, q, 0, 2, 1, 3);
    k = ggml_permute(context, k, 0, 2, 1, 3);
    v = ggml_permute(context, v, 0, 2, 1, 3);
    auto * scores = ggml_mul_mat(context, k, q);
    ggml_mul_mat_set_prec(scores, GGML_PREC_F32);
    scores = ggml_diag_mask_inf(context,
        ggml_scale(context, scores, 1.0F / std::sqrt(static_cast<float>(head_dim))), 0);
    auto * attended = ggml_mul_mat(context, ggml_cont(context, ggml_transpose(context, v)),
                                   ggml_soft_max(context, scores));
    ggml_mul_mat_set_prec(attended, GGML_PREC_F32);
    attended = ggml_cont(context, ggml_permute(context, attended, 0, 2, 1, 3));
    auto * state = ggml_add(context, x, linear(context,
        ggml_reshape_2d(context, attended, heads * head_dim, seq), get("attn.o.w")));
    normalized = rms(context, state, get("fn.w"), 1e-6F);
    auto * gate = ggml_silu(context, linear(context, normalized, get("mlp.g.w")));
    auto * up = linear(context, normalized, get("mlp.u.w"));
    auto * output = ggml_add(context, state,
        linear(context, ggml_mul(context, gate, up), get("mlp.d.w")));
    auto * graph = ggml_new_graph_custom(context, 4096, false);
    ggml_build_forward_expand(graph, output);
    auto * allocator = ggml_gallocr_new(ggml_backend_get_default_buffer_type(backend));
    if (allocator == nullptr || !ggml_gallocr_reserve(allocator, graph) || !ggml_gallocr_alloc_graph(allocator, graph)) {
        if (allocator != nullptr) ggml_gallocr_free(allocator);
        return tl::unexpected(fail(error_code::allocation, "cannot allocate Qwen layer graph"));
    }
    auto allocator_cleanup = std::unique_ptr<ggml_gallocr, decltype(&ggml_gallocr_free)>(allocator, ggml_gallocr_free);
    std::vector<std::int32_t> position_values(sequence);
    for (std::size_t index = 0; index < sequence; ++index) position_values[index] = static_cast<std::int32_t>(index);
    if (active_qwen_profile != nullptr) {
        active_qwen_profile->setup_ms += elapsed_ms(profile_start);
        ++active_qwen_profile->layers;
    }
    auto phase_start = profile_clock::now();
    ggml_backend_tensor_set(x, input.data(), 0, input.size_bytes());
    ggml_backend_tensor_set(positions, position_values.data(), 0, position_values.size() * sizeof(std::int32_t));
    if (active_qwen_profile != nullptr) {
        active_qwen_profile->host_to_device_ms += elapsed_ms(phase_start);
        active_qwen_profile->host_to_device_bytes += input.size_bytes() + position_values.size() * sizeof(std::int32_t);
    }
    phase_start = profile_clock::now();
    if (ggml_backend_graph_compute(backend, graph) != GGML_STATUS_SUCCESS)
        return tl::unexpected(fail(error_code::compute, "Qwen layer graph execution failed"));
    if (active_qwen_profile != nullptr) {
        active_qwen_profile->compute_sync_ms += elapsed_ms(phase_start);
        ++active_qwen_profile->graph_computes;
    }
    phase_start = profile_clock::now();
    auto result = read_f32(output);
    if (active_qwen_profile != nullptr) {
        active_qwen_profile->device_to_host_ms += elapsed_ms(phase_start);
        active_qwen_profile->device_to_host_bytes += result.size() * sizeof(float);
    }
    return result;
} catch (const std::exception & exception) {
    return tl::unexpected(fail(error_code::compute, exception.what()));
}

result<std::vector<float>> run_qwen_head(const weight_component & weights, ggml_backend_t backend,
                                         nonstd::span<const float> input, std::size_t sequence) try {
    const auto profile_start = profile_clock::now();
    if (backend == nullptr || sequence == 0U || input.size() != sequence * static_cast<std::size_t>(hidden))
        return tl::unexpected(fail(error_code::invalid_argument, "invalid Qwen head input"));
    auto * context = ggml_init({16ULL << 20U, nullptr, true});
    if (context == nullptr) return tl::unexpected(fail(error_code::allocation, "cannot create Qwen head context"));
    auto cleanup = std::unique_ptr<ggml_context, decltype(&ggml_free)>(context, ggml_free);
    auto * x = ggml_new_tensor_2d(context, GGML_TYPE_F32, hidden, static_cast<std::int64_t>(sequence));
    ggml_set_input(x);
    auto * final_norm = rms(context, x, required(weights, "llm.norm.w"), 1e-6F);
    auto * last = ggml_view_2d(context, final_norm, hidden, 1, final_norm->nb[1],
                              (sequence - 1U) * final_norm->nb[1]);
    auto * logits = linear(context, last, required(weights, "llm.out.w"));
    auto * graph = ggml_new_graph_custom(context, 1024, false);
    ggml_build_forward_expand(graph, logits);
    auto * allocator = ggml_gallocr_new(ggml_backend_get_default_buffer_type(backend));
    if (allocator == nullptr || !ggml_gallocr_reserve(allocator, graph) || !ggml_gallocr_alloc_graph(allocator, graph)) {
        if (allocator != nullptr) ggml_gallocr_free(allocator);
        return tl::unexpected(fail(error_code::allocation, "cannot allocate Qwen head graph"));
    }
    auto allocator_cleanup = std::unique_ptr<ggml_gallocr, decltype(&ggml_gallocr_free)>(allocator, ggml_gallocr_free);
    if (active_qwen_profile != nullptr) active_qwen_profile->setup_ms += elapsed_ms(profile_start);
    auto phase_start = profile_clock::now();
    ggml_backend_tensor_set(x, input.data(), 0, input.size_bytes());
    if (active_qwen_profile != nullptr) {
        active_qwen_profile->host_to_device_ms += elapsed_ms(phase_start);
        active_qwen_profile->host_to_device_bytes += input.size_bytes();
    }
    phase_start = profile_clock::now();
    if (ggml_backend_graph_compute(backend, graph) != GGML_STATUS_SUCCESS)
        return tl::unexpected(fail(error_code::compute, "Qwen head graph execution failed"));
    if (active_qwen_profile != nullptr) {
        active_qwen_profile->compute_sync_ms += elapsed_ms(phase_start);
        ++active_qwen_profile->graph_computes;
    }
    phase_start = profile_clock::now();
    auto result = read_f32(logits);
    if (active_qwen_profile != nullptr) {
        active_qwen_profile->device_to_host_ms += elapsed_ms(phase_start);
        active_qwen_profile->device_to_host_bytes += result.size() * sizeof(float);
    }
    return result;
} catch (const std::exception & exception) {
    return tl::unexpected(fail(error_code::compute, exception.what()));
}

result<std::vector<float>> run_qwen_logits(const weight_component & weights, ggml_backend_t backend,
                                            nonstd::span<const float> mesh_embeddings,
                                            nonstd::span<const std::int32_t> tokens) try {
    if (backend == nullptr || mesh_embeddings.empty() ||
        mesh_embeddings.size() % static_cast<std::size_t>(hidden) != 0U || tokens.empty())
        return tl::unexpected(fail(error_code::invalid_argument, "invalid Qwen logits input"));
    const std::size_t mesh_sequence = mesh_embeddings.size() / static_cast<std::size_t>(hidden);
    qwen_graph_cache graph{weights, backend, mesh_sequence};
    return graph.evaluate(mesh_embeddings, tokens, tokens.size());
} catch (const std::exception & exception) {
    return tl::unexpected(fail(error_code::compute, exception.what()));
}

result<std::vector<float>> run_qwen_logits_batch(
    const weight_component & weights, ggml_backend_t backend,
    nonstd::span<const float> mesh_embeddings, nonstd::span<const std::int32_t> tokens,
    std::size_t token_count, std::size_t batch_size) try {
    if (backend == nullptr || mesh_embeddings.empty() ||
        mesh_embeddings.size() % static_cast<std::size_t>(hidden) != 0U ||
        tokens.size() != token_count * batch_size)
        return tl::unexpected(fail(error_code::invalid_argument, "invalid batched Qwen logits input"));
    qwen_graph_cache graph{weights, backend,
        mesh_embeddings.size() / static_cast<std::size_t>(hidden)};
    return graph.evaluate(mesh_embeddings, tokens, token_count, batch_size);
} catch (const std::exception & exception) {
    return tl::unexpected(fail(error_code::compute, exception.what()));
}

result<std::vector<float>> run_qwen_kv_trajectory(
    const weight_component & weights, ggml_backend_t backend,
    nonstd::span<const float> mesh_embeddings, nonstd::span<const std::int32_t> prefix,
    nonstd::span<const std::int32_t> continuation) try {
    if (backend == nullptr || mesh_embeddings.empty() || prefix.empty() || continuation.empty() ||
        mesh_embeddings.size() % static_cast<std::size_t>(hidden) != 0U)
        return tl::unexpected(fail(error_code::invalid_argument, "invalid Qwen KV trajectory input"));
    const std::size_t mesh_sequence = mesh_embeddings.size() / static_cast<std::size_t>(hidden);
    const std::size_t capacity = mesh_sequence + prefix.size() + continuation.size();
    qwen_kv_generator generator{weights, backend, mesh_sequence, capacity, 1U};
    auto logits = generator.prefill(mesh_embeddings, prefix);
    if (!logits) return tl::unexpected(logits.error());
    std::vector<float> output;
    output.reserve(logits->size() * continuation.size());
    std::size_t past = mesh_sequence + prefix.size();
    constexpr std::size_t slot = 0U;
    for (std::size_t step = 0; step < continuation.size(); ++step) {
        output.insert(output.end(), logits->begin(), logits->end());
        if (step + 1U == continuation.size()) break;
        logits = generator.decode(
            nonstd::span{continuation.data() + step, 1U}, nonstd::span{&slot, 1U}, past);
        if (!logits) return tl::unexpected(logits.error());
        ++past;
    }
    return output;
} catch (const std::exception & exception) {
    return tl::unexpected(fail(error_code::compute, exception.what()));
}

result<std::vector<float>> run_qwen_kv_branches(
    const weight_component & weights, ggml_backend_t backend,
    nonstd::span<const float> mesh_embeddings, nonstd::span<const std::int32_t> prefix,
    nonstd::span<const std::int32_t> branch_tokens) try {
    if (backend == nullptr || mesh_embeddings.empty() || prefix.empty() || branch_tokens.empty() ||
        branch_tokens.size() > 32U ||
        mesh_embeddings.size() % static_cast<std::size_t>(hidden) != 0U)
        return tl::unexpected(fail(error_code::invalid_argument, "invalid Qwen KV branch input"));
    const std::size_t mesh_sequence = mesh_embeddings.size() / static_cast<std::size_t>(hidden);
    const std::size_t past = mesh_sequence + prefix.size();
    qwen_kv_generator generator{weights, backend, mesh_sequence, past + 1U, branch_tokens.size()};
    auto prefill = generator.prefill(mesh_embeddings, prefix);
    if (!prefill) return tl::unexpected(prefill.error());
    constexpr std::size_t initial_slot = 0U;
    std::vector<std::size_t> parents(branch_tokens.size(), 0U);
    auto slots = generator.remap(nonstd::span{&initial_slot, 1U}, parents, past);
    if (!slots) return tl::unexpected(slots.error());
    return generator.decode(branch_tokens, *slots, past);
} catch (const std::exception & exception) {
    return tl::unexpected(fail(error_code::compute, exception.what()));
}

result<std::vector<std::int32_t>> generate_skin_codes(
    const weight_component & weights, ggml_backend_t backend,
    nonstd::span<const float> mesh_embeddings, nonstd::span<const std::int32_t> skeleton_tokens,
    std::size_t joint_count, const generation_options & options) try {
    if (backend == nullptr || mesh_embeddings.empty() || mesh_embeddings.size() % static_cast<std::size_t>(hidden) != 0U ||
        skeleton_tokens.empty() || joint_count == 0U || joint_count > 256U || options.top_p < 0.0F ||
        options.top_p > 1.0F || options.beams == 0U || options.beams > 32U ||
        !std::isfinite(options.temperature) || !std::isfinite(options.repetition_penalty))
        return tl::unexpected(fail(error_code::invalid_argument, "invalid TokenRig generation input"));
    qwen_profile_session profile{"qwen.generate_skin_codes"};
    const std::size_t wanted = joint_count * 4U;
    if (wanted > options.max_tokens)
        return tl::unexpected(fail(error_code::limit_exceeded, "generation token limit is smaller than four codes per joint"));
    const std::size_t mesh_sequence = mesh_embeddings.size() / static_cast<std::size_t>(hidden);
    const std::size_t capacity = mesh_sequence + skeleton_tokens.size() + wanted;
    if (capacity > 3192U)
        return tl::unexpected(fail(error_code::limit_exceeded, "TokenRig sequence exceeds Qwen context"));
    qwen_kv_generator generator{weights, backend, mesh_sequence, capacity, options.beams};
    auto current_logits = generator.prefill(mesh_embeddings, skeleton_tokens);
    if (!current_logits) return tl::unexpected(current_logits.error());
    std::size_t cached_length = mesh_sequence + skeleton_tokens.size();
    std::mt19937_64 random{options.seed};
    std::vector<std::int32_t> generated;
    if (options.beams == 1U) {
        generated.reserve(wanted);
        while (generated.size() < wanted) {
            generated.push_back(sample_code(*current_logits, generated, options, random));
            if (generated.size() == wanted) break;
            constexpr std::size_t slot = 0U;
            current_logits = generator.decode(
                nonstd::span{&generated.back(), 1U}, nonstd::span{&slot, 1U}, cached_length);
            if (!current_logits) return tl::unexpected(current_logits.error());
            ++cached_length;
        }
    } else {
        struct beam_state {
            std::vector<std::int32_t> tokens;
            double score = 0.0;
            std::size_t slot = 0U;
        };
        struct continuation { std::size_t source; std::int32_t token; double score; };
        std::vector<beam_state> beams(1U);
        while (beams.front().tokens.size() < wanted) {
            std::vector<continuation> choices;
            constexpr std::size_t vocabulary = 33036U;
            if (current_logits->size() != vocabulary * beams.size())
                return tl::unexpected(fail(error_code::compute, "cached Qwen logits shape mismatch"));
            for (std::size_t beam = 0; beam < beams.size(); ++beam) {
                if (beams[beam].tokens.size() != beams.front().tokens.size())
                    return tl::unexpected(fail(error_code::compute, "beam token lengths diverged"));
                const nonstd::span<const float> beam_logits{
                    current_logits->data() + beam * vocabulary, vocabulary};
                for (const auto & [score, token] : beam_candidates(beam_logits, beams[beam].tokens, options))
                    choices.push_back({beam, token, beams[beam].score + score});
            }
            const std::size_t wanted_choices = std::min<std::size_t>(static_cast<std::size_t>(options.beams) * 2U, choices.size());
            std::vector<continuation> selected;
            selected.reserve(wanted_choices);
            if (options.temperature > 0.0F) {
                while (selected.size() < wanted_choices) {
                    const double peak = std::max_element(choices.begin(), choices.end(),
                        [](const auto & left, const auto & right) { return left.score < right.score; })->score;
                    std::vector<double> probabilities(choices.size());
                    for (std::size_t i = 0; i < choices.size(); ++i)
                        probabilities[i] = std::exp(choices[i].score - peak);
                    std::discrete_distribution<std::size_t> distribution(probabilities.begin(), probabilities.end());
                    const std::size_t index = distribution(random);
                    selected.push_back(choices[index]);
                    choices.erase(choices.begin() + static_cast<std::ptrdiff_t>(index));
                }
            } else {
                std::partial_sort(choices.begin(), choices.begin() + static_cast<std::ptrdiff_t>(wanted_choices), choices.end(),
                    [](const auto & left, const auto & right) { return left.score > right.score; });
                selected.assign(choices.begin(), choices.begin() + static_cast<std::ptrdiff_t>(wanted_choices));
            }
            std::sort(selected.begin(), selected.end(),
                [](const auto & left, const auto & right) { return left.score > right.score; });
            const std::size_t keep = std::min<std::size_t>(options.beams, selected.size());
            std::vector<std::size_t> old_slots;
            old_slots.reserve(beams.size());
            for (const auto & beam : beams) old_slots.push_back(beam.slot);
            std::vector<std::size_t> parents;
            parents.reserve(keep);
            for (std::size_t index = 0; index < keep; ++index)
                parents.push_back(selected[index].source);
            auto slots = generator.remap(old_slots, parents, cached_length);
            if (!slots) return tl::unexpected(slots.error());
            std::vector<beam_state> next;
            next.reserve(keep);
            for (std::size_t i = 0; i < keep; ++i) {
                beam_state value = beams[selected[i].source];
                value.tokens.push_back(selected[i].token);
                value.score = selected[i].score;
                value.slot = (*slots)[i];
                next.push_back(std::move(value));
            }
            beams = std::move(next);
            if (beams.front().tokens.size() == wanted) break;
            std::vector<std::int32_t> next_tokens;
            std::vector<std::size_t> next_slots;
            next_tokens.reserve(beams.size());
            next_slots.reserve(beams.size());
            for (const auto & beam : beams) {
                next_tokens.push_back(beam.tokens.back());
                next_slots.push_back(beam.slot);
            }
            current_logits = generator.decode(next_tokens, next_slots, cached_length);
            if (!current_logits) return tl::unexpected(current_logits.error());
            ++cached_length;
        }
        generated = std::move(beams.front().tokens);
    }
    for (auto & token : generated) token -= 267;
    return generated;
} catch (const std::exception & exception) {
    return tl::unexpected(fail(error_code::compute, exception.what()));
}

result<generated_rig_tokens> generate_rig_tokens(
    const weight_component & weights, ggml_backend_t backend,
    nonstd::span<const float> mesh_embeddings, const generation_options & options) try {
    if (backend == nullptr || mesh_embeddings.empty() ||
        mesh_embeddings.size() % static_cast<std::size_t>(hidden) != 0U ||
        options.beams == 0U || options.beams > 32U || options.max_tokens < 8U ||
        options.top_p < 0.0F || options.top_p > 1.0F)
        return tl::unexpected(fail(error_code::invalid_argument, "invalid unconstrained TokenRig generation input"));
    qwen_profile_session profile{"qwen.generate_rig_tokens"};
    // `articulation` is the class used by the released inference dataset.
    constexpr std::int32_t start[]{257, 266};
    constexpr std::int32_t first_skin = 267;
    constexpr std::int32_t last_skin = 33034;
    constexpr std::int32_t final_eos = 33035;
    const std::size_t mesh_sequence = mesh_embeddings.size() / static_cast<std::size_t>(hidden);
    const std::size_t capacity = mesh_sequence + std::size(start) + options.max_tokens;
    if (capacity > 3192U)
        return tl::unexpected(fail(error_code::limit_exceeded, "TokenRig sequence exceeds Qwen context"));
    qwen_kv_generator generator{weights, backend, mesh_sequence, capacity, options.beams};
    auto current_logits = generator.prefill(mesh_embeddings, start);
    if (!current_logits) return tl::unexpected(current_logits.error());
    std::size_t cached_length = mesh_sequence + std::size(start);
    struct beam_state {
        std::vector<std::int32_t> tokens;
        double score = 0.0;
        std::size_t slot = 0U;
    };
    struct choice { std::size_t source; std::int32_t token; double score; };
    std::vector<beam_state> active{{std::vector<std::int32_t>{std::begin(start), std::end(start)}, 0.0, 0U}}, finished;
    std::mt19937_64 random{options.seed};
    for (std::size_t step = 0; step < options.max_tokens && !active.empty(); ++step) {
        std::vector<choice> choices;
        std::vector<std::size_t> valid_beams;
        std::vector<std::vector<std::int32_t>> allowed_by_beam;
        const std::size_t token_count = active.front().tokens.size();
        constexpr std::size_t vocabulary = 33036U;
        if (current_logits->size() != vocabulary * active.size())
            return tl::unexpected(fail(error_code::compute, "cached Qwen logits shape mismatch"));
        for (std::size_t beam = 0; beam < active.size(); ++beam) {
            if (active[beam].tokens.size() != token_count)
                return tl::unexpected(fail(error_code::compute, "beam token lengths diverged"));
            const auto skeleton_end = std::find(active[beam].tokens.begin(), active[beam].tokens.end(), 258);
            std::vector<std::int32_t> allowed;
            if (skeleton_end == active[beam].tokens.end()) {
                const auto parsed = parse_skeleton(active[beam].tokens);
                allowed = allowed_skeleton_tokens(parsed);
            } else {
                const auto skeleton_size = static_cast<std::size_t>(skeleton_end - active[beam].tokens.begin()) + 1U;
                const auto parsed = parse_skeleton(nonstd::span{active[beam].tokens.data(), skeleton_size});
                const std::size_t codes = active[beam].tokens.size() - skeleton_size;
                if (parsed.joints == 0U || parsed.joints > 256U) continue;
                // The released processor counts the skeleton switch itself in
                // the J*4 span. Consequently EOS is the fourth FSQ value of
                // the final joint (index 32768 after subtracting vocab_size).
                if (codes + 1U < parsed.joints * 4U) {
                    allowed.reserve(static_cast<std::size_t>(last_skin - 258 + 1));
                    for (std::int32_t token = 258; token <= last_skin; ++token) allowed.push_back(token);
                } else if (codes + 1U == parsed.joints * 4U) allowed.push_back(final_eos);
                else continue;
            }
            if (allowed.empty()) continue;
            valid_beams.push_back(beam);
            allowed_by_beam.push_back(std::move(allowed));
        }
        if (valid_beams.empty()) break;
        for (std::size_t index = 0; index < valid_beams.size(); ++index) {
            const auto beam = valid_beams[index];
            const nonstd::span<const float> beam_logits{
                current_logits->data() + beam * vocabulary, vocabulary};
            for (const auto & [score, token] : masked_candidates(
                     beam_logits, allowed_by_beam[index], active[beam].tokens, options))
                choices.push_back({beam, token, active[beam].score + score});
        }
        if (choices.empty()) break;
        const std::size_t wanted = std::min(choices.size(), static_cast<std::size_t>(options.beams) * 2U);
        std::vector<choice> selected; selected.reserve(wanted);
        if (options.temperature > 0.0F) {
            while (selected.size() < wanted && !choices.empty()) {
                const double peak = std::max_element(choices.begin(), choices.end(),
                    [](const auto & a, const auto & b) { return a.score < b.score; })->score;
                std::vector<double> probabilities(choices.size());
                for (std::size_t i = 0; i < choices.size(); ++i) probabilities[i] = std::exp(choices[i].score - peak);
                std::discrete_distribution<std::size_t> distribution(probabilities.begin(), probabilities.end());
                const std::size_t index = distribution(random);
                selected.push_back(choices[index]);
                choices.erase(choices.begin() + static_cast<std::ptrdiff_t>(index));
            }
        } else {
            std::partial_sort(choices.begin(), choices.begin() + static_cast<std::ptrdiff_t>(wanted), choices.end(),
                [](const auto & a, const auto & b) { return a.score > b.score; });
            selected.assign(choices.begin(), choices.begin() + static_cast<std::ptrdiff_t>(wanted));
        }
        std::sort(selected.begin(), selected.end(), [](const auto & a, const auto & b) { return a.score > b.score; });
        std::vector<beam_state> next;
        std::vector<std::size_t> next_parents;
        for (const auto & selected_choice : selected) {
            auto value = active[selected_choice.source]; value.tokens.push_back(selected_choice.token);
            value.score = selected_choice.score;
            if (selected_choice.token == final_eos) finished.push_back(std::move(value));
            else if (next.size() < options.beams) {
                next_parents.push_back(selected_choice.source);
                next.push_back(std::move(value));
            }
        }
        if (finished.size() >= options.beams) break;
        if (next.empty()) {
            active.clear();
            break;
        }
        std::vector<std::size_t> old_slots;
        old_slots.reserve(active.size());
        for (const auto & beam : active) old_slots.push_back(beam.slot);
        auto next_slots = generator.remap(old_slots, next_parents, cached_length);
        if (!next_slots) return tl::unexpected(next_slots.error());
        for (std::size_t index = 0; index < next.size(); ++index)
            next[index].slot = (*next_slots)[index];
        active = std::move(next);
        std::vector<std::int32_t> next_tokens;
        std::vector<std::size_t> slots;
        next_tokens.reserve(active.size());
        slots.reserve(active.size());
        for (const auto & beam : active) {
            next_tokens.push_back(beam.tokens.back());
            slots.push_back(beam.slot);
        }
        current_logits = generator.decode(next_tokens, slots, cached_length);
        if (!current_logits) return tl::unexpected(current_logits.error());
        ++cached_length;
    }
    if (finished.empty()) {
        std::size_t switched = 0U, maximum_joints = 0U, maximum_length = 0U;
        for (const auto & beam : active) {
            maximum_length = std::max(maximum_length, beam.tokens.size());
            const auto end = std::find(beam.tokens.begin(), beam.tokens.end(), 258);
            const std::size_t size = end == beam.tokens.end() ? beam.tokens.size() :
                static_cast<std::size_t>(end - beam.tokens.begin()) + 1U;
            const auto parsed = parse_skeleton(nonstd::span{beam.tokens.data(), size});
            maximum_joints = std::max(maximum_joints, parsed.joints);
            if (end != beam.tokens.end()) ++switched;
        }
        return tl::unexpected(fail(error_code::compute,
            "TokenRig did not complete within the token limit (active=" + std::to_string(active.size()) +
            ", switched=" + std::to_string(switched) + ", max_joints=" + std::to_string(maximum_joints) +
            ", max_sequence=" + std::to_string(maximum_length) + ")"));
    }
    const auto best = std::max_element(finished.begin(), finished.end(), [](const auto & a, const auto & b) {
        const double as = a.score / static_cast<double>(a.tokens.size() - 2U);
        const double bs = b.score / static_cast<double>(b.tokens.size() - 2U);
        return as < bs;
    });
    const auto skeleton_end = std::find(best->tokens.begin(), best->tokens.end(), 258);
    const std::size_t skeleton_size = static_cast<std::size_t>(skeleton_end - best->tokens.begin()) + 1U;
    const auto parsed = parse_skeleton(nonstd::span{best->tokens.data(), skeleton_size});
    generated_rig_tokens output;
    output.skeleton_tokens.assign(best->tokens.begin(), skeleton_end + 1);
    output.skin_codes.assign(skeleton_end + 1, best->tokens.end());
    for (auto & code : output.skin_codes) code -= first_skin;
    output.joint_count = parsed.joints;
    if (output.skin_codes.size() != output.joint_count * 4U)
        return tl::unexpected(fail(error_code::compute, "TokenRig generated an incomplete skin-code sequence"));
    return output;
} catch (const std::exception & exception) {
    return tl::unexpected(fail(error_code::compute, exception.what()));
}

} // namespace skintokens::detail

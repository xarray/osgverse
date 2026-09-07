#include "internal.hpp"

#include <cmath>
#include <cstdio>
#include <memory>

namespace skintokens::detail {
namespace {

constexpr std::int64_t width = 512;
constexpr std::int64_t heads = 8;
constexpr std::int64_t head_dim = 64;
constexpr std::int64_t queries = 8;
constexpr std::int64_t points_count = 64;

ggml_tensor * need(const weight_component & weights, const std::string & name) {
    auto * value = weights.tensor(name);
    if (value == nullptr) throw std::runtime_error("missing mesh encoder tensor: " + name);
    return value;
}

ggml_tensor * linear(ggml_context * context, ggml_tensor * input, const weight_component & weights,
                     const std::string & stem, bool bias = true) {
    auto * output = ggml_mul_mat(context, need(weights, stem + ".w"), input);
    ggml_mul_mat_set_prec(output, GGML_PREC_F32);
    if (bias) {
        auto * value = need(weights, stem + ".b");
        if (value->type != output->type) value = ggml_cast(context, value, output->type);
        output = ggml_add(context, output, ggml_repeat(context, value, output));
    }
    return output;
}

ggml_tensor * norm(ggml_context * context, ggml_tensor * input, const weight_component & weights,
                   const std::string & stem) {
    auto * value = ggml_norm(context, input, 1e-5F);
    auto * scale = need(weights, stem + ".w");
    auto * bias = need(weights, stem + ".b");
    if (scale->type != value->type) scale = ggml_cast(context, scale, value->type);
    if (bias->type != value->type) bias = ggml_cast(context, bias, value->type);
    value = ggml_mul(context, value, ggml_repeat(context, scale, value));
    return ggml_add(context, value, ggml_repeat(context, bias, value));
}

ggml_tensor * attention(ggml_context * context, ggml_tensor * q, ggml_tensor * k, ggml_tensor * v,
                        std::int64_t query_count) {
    q = ggml_permute(context, q, 0, 2, 1, 3);
    k = ggml_permute(context, k, 0, 2, 1, 3);
    v = ggml_permute(context, v, 0, 2, 1, 3);
    auto * scores = ggml_mul_mat(context, k, q);
    ggml_mul_mat_set_prec(scores, GGML_PREC_F32);
    auto * probability = ggml_soft_max(context,
        ggml_scale(context, scores, 1.0F / std::sqrt(static_cast<float>(head_dim))));
    auto * output = ggml_mul_mat(context, ggml_cont(context, ggml_transpose(context, v)), probability);
    ggml_mul_mat_set_prec(output, GGML_PREC_F32);
    output = ggml_cont(context, ggml_permute(context, output, 0, 2, 1, 3));
    return ggml_reshape_2d(context, output, width, query_count);
}

ggml_tensor * mlp(ggml_context * context, ggml_tensor * input, const weight_component & weights,
                  const std::string & stem) {
    return linear(context, ggml_gelu_erf(context, linear(context, input, weights, stem + ".c_fc")),
                  weights, stem + ".c_proj");
}

std::vector<float> read_output(ggml_tensor * value) {
    const auto count = ggml_nelements(value);
    if (count < 0 || value->type != GGML_TYPE_F32) throw std::runtime_error("invalid mesh snapshot type");
    std::vector<float> output(static_cast<std::size_t>(count));
    ggml_backend_tensor_get(value, output.data(), 0, output.size() * sizeof(float));
    return output;
}

} // namespace

result<tensor_snapshot> run_mesh_encoder_fixture(const weight_component & weights, ggml_backend_t backend,
                                                  nonstd::span<const float> points,
                                                  nonstd::span<const float> normals) try {
    if (backend == nullptr || points.size() != static_cast<std::size_t>(points_count * 3) || normals.size() != points.size())
        return tl::unexpected(fail(error_code::invalid_argument, "mesh fixture must contain 64 point/normal triples"));
    std::vector<float> embedded(static_cast<std::size_t>(points_count * 54));
    for (std::size_t point = 0; point < static_cast<std::size_t>(points_count); ++point) {
        std::size_t cursor = point * 54U;
        for (std::size_t axis = 0; axis < 3U; ++axis) embedded[cursor++] = points[point * 3U + axis];
        for (std::size_t axis = 0; axis < 3U; ++axis)
            for (std::size_t frequency = 0; frequency < 8U; ++frequency)
                embedded[cursor++] = std::sin(points[point * 3U + axis] * static_cast<float>(1U << frequency));
        for (std::size_t axis = 0; axis < 3U; ++axis)
            for (std::size_t frequency = 0; frequency < 8U; ++frequency)
                embedded[cursor++] = std::cos(points[point * 3U + axis] * static_cast<float>(1U << frequency));
        for (std::size_t axis = 0; axis < 3U; ++axis) embedded[cursor++] = normals[point * 3U + axis];
    }
    auto * context = ggml_init({128ULL << 20U, nullptr, true});
    if (context == nullptr) return tl::unexpected(fail(error_code::allocation, "cannot create mesh encoder graph"));
    auto cleanup = std::unique_ptr<ggml_context, decltype(&ggml_free)>(context, ggml_free);
    auto * input = ggml_new_tensor_2d(context, GGML_TYPE_F32, 54, points_count);
    auto * indices = ggml_new_tensor_1d(context, GGML_TYPE_I32, queries);
    ggml_set_input(input); ggml_set_input(indices);
    auto * projected = linear(context, input, weights, "mesh.enc.input_proj");
    auto * state = ggml_get_rows(context, projected, indices);
    const std::string cross = "mesh.enc.xattn";
    auto * q = linear(context, norm(context, state, weights, cross + ".ln_1"), weights, cross + ".attn.c_q", false);
    auto * kv = linear(context, norm(context, projected, weights, cross + ".ln_2"), weights, cross + ".attn.c_kv", false);
    q = ggml_reshape_3d(context, q, head_dim, heads, queries);
    auto * k = ggml_view_3d(context, kv, head_dim, heads, points_count,
        static_cast<std::size_t>(2 * head_dim) * kv->nb[0], kv->nb[1], 0);
    auto * v = ggml_view_3d(context, kv, head_dim, heads, points_count,
        static_cast<std::size_t>(2 * head_dim) * kv->nb[0], kv->nb[1],
        static_cast<std::size_t>(head_dim) * kv->nb[0]);
    state = ggml_add(context, state, linear(context, attention(context, q, k, v, queries),
                                            weights, cross + ".attn.c_proj"));
    state = ggml_add(context, state, mlp(context, norm(context, state, weights, cross + ".ln_3"),
                                         weights, cross + ".mlp"));
    std::vector<std::pair<std::string, ggml_tensor *>> snapshots{{"projected", projected}, {"cross", state}};
    for (std::size_t layer = 0; layer < 8U; ++layer) {
        const std::string stem = "mesh.enc.attn.resblocks." + std::to_string(layer);
        auto * qkv = linear(context, norm(context, state, weights, stem + ".ln_1"),
                            weights, stem + ".attn.c_qkv", false);
        q = ggml_view_3d(context, qkv, head_dim, heads, queries,
            static_cast<std::size_t>(3 * head_dim) * qkv->nb[0], qkv->nb[1], 0);
        k = ggml_view_3d(context, qkv, head_dim, heads, queries,
            static_cast<std::size_t>(3 * head_dim) * qkv->nb[0], qkv->nb[1],
            static_cast<std::size_t>(head_dim) * qkv->nb[0]);
        v = ggml_view_3d(context, qkv, head_dim, heads, queries,
            static_cast<std::size_t>(3 * head_dim) * qkv->nb[0], qkv->nb[1],
            static_cast<std::size_t>(2 * head_dim) * qkv->nb[0]);
        state = ggml_add(context, state, linear(context, attention(context, q, k, v, queries),
                                                weights, stem + ".attn.c_proj"));
        state = ggml_add(context, state, mlp(context, norm(context, state, weights, stem + ".ln_2"),
                                             weights, stem + ".mlp"));
        char name[16]; std::snprintf(name, sizeof(name), "self_%02zu", layer + 1U);
        snapshots.emplace_back(name, state);
    }
    state = norm(context, state, weights, "mesh.enc.ln_post");
    snapshots.emplace_back("ln_post", state);
    state = linear(context, state, weights, "mesh.out.0");
    auto * output_scale = need(weights, "mesh.out.1.w");
    if (output_scale->type != state->type) output_scale = ggml_cast(context, output_scale, state->type);
    state = ggml_mul(context, ggml_rms_norm(context, state, std::numeric_limits<float>::epsilon()),
                     ggml_repeat(context, output_scale, state));
    snapshots.emplace_back("output", state);
    for (const auto & [name, tensor] : snapshots) { (void) name; ggml_set_output(tensor); }
    auto * graph = ggml_new_graph_custom(context, 8192, false);
    for (const auto & [name, tensor] : snapshots) { (void) name; ggml_build_forward_expand(graph, tensor); }
    auto * allocator = ggml_gallocr_new(ggml_backend_get_default_buffer_type(backend));
    if (allocator == nullptr || !ggml_gallocr_reserve(allocator, graph) || !ggml_gallocr_alloc_graph(allocator, graph)) {
        if (allocator != nullptr) ggml_gallocr_free(allocator);
        return tl::unexpected(fail(error_code::allocation, "cannot allocate mesh encoder graph"));
    }
    auto allocator_cleanup = std::unique_ptr<ggml_gallocr, decltype(&ggml_gallocr_free)>(allocator, ggml_gallocr_free);
    constexpr std::int32_t query_indices[]{0, 7, 13, 21, 32, 44, 55, 63};
    ggml_backend_tensor_set(input, embedded.data(), 0, embedded.size() * sizeof(float));
    ggml_backend_tensor_set(indices, query_indices, 0, sizeof(query_indices));
    if (ggml_backend_graph_compute(backend, graph) != GGML_STATUS_SUCCESS)
        return tl::unexpected(fail(error_code::compute, "mesh encoder graph execution failed"));
    tensor_snapshot output{{"embedded", std::move(embedded)}};
    for (const auto & [name, tensor] : snapshots) output.emplace(name, read_output(tensor));
    return output;
} catch (const std::exception & exception) {
    return tl::unexpected(fail(error_code::compute, exception.what()));
}

result<std::vector<float>> encode_mesh(const weight_component & weights, ggml_backend_t backend,
                                       nonstd::span<const vec3> points, nonstd::span<const vec3> normals,
                                       nonstd::span<const std::int32_t> query_indices) try {
    if (backend == nullptr || points.empty() || normals.size() != points.size() || query_indices.empty() ||
        query_indices.size() > points.size())
        return tl::unexpected(fail(error_code::invalid_argument, "invalid mesh encoder input"));
    for (const auto index : query_indices)
        if (index < 0 || static_cast<std::size_t>(index) >= points.size())
            return tl::unexpected(fail(error_code::invalid_argument, "mesh encoder query index is out of range"));
    const auto data_count = static_cast<std::int64_t>(points.size());
    const auto query_count = static_cast<std::int64_t>(query_indices.size());
    std::vector<float> embedded(points.size() * 54U);
    for (std::size_t point = 0; point < points.size(); ++point) {
        const float position[]{points[point].x, points[point].y, points[point].z};
        const float normal_values[]{normals[point].x, normals[point].y, normals[point].z};
        std::size_t cursor = point * 54U;
        for (float value : position) embedded[cursor++] = value;
        for (float value : position) for (std::size_t frequency = 0; frequency < 8U; ++frequency)
            embedded[cursor++] = std::sin(value * static_cast<float>(1U << frequency));
        for (float value : position) for (std::size_t frequency = 0; frequency < 8U; ++frequency)
            embedded[cursor++] = std::cos(value * static_cast<float>(1U << frequency));
        for (float value : normal_values) embedded[cursor++] = value;
    }
    auto * context = ggml_init({256ULL << 20U, nullptr, true});
    if (context == nullptr) return tl::unexpected(fail(error_code::allocation, "cannot create mesh encoder graph"));
    auto cleanup = std::unique_ptr<ggml_context, decltype(&ggml_free)>(context, ggml_free);
    auto * input = ggml_new_tensor_2d(context, GGML_TYPE_F32, 54, data_count);
    auto * indices = ggml_new_tensor_1d(context, GGML_TYPE_I32, query_count);
    ggml_set_input(input); ggml_set_input(indices);
    auto * projected = linear(context, input, weights, "mesh.enc.input_proj");
    auto * state = ggml_get_rows(context, projected, indices);
    const std::string cross = "mesh.enc.xattn";
    auto * q = linear(context, norm(context, state, weights, cross + ".ln_1"), weights, cross + ".attn.c_q", false);
    auto * kv = linear(context, norm(context, projected, weights, cross + ".ln_2"), weights, cross + ".attn.c_kv", false);
    q = ggml_reshape_3d(context, q, head_dim, heads, query_count);
    auto * k = ggml_view_3d(context, kv, head_dim, heads, data_count,
        static_cast<std::size_t>(2 * head_dim) * kv->nb[0], kv->nb[1], 0);
    auto * v = ggml_view_3d(context, kv, head_dim, heads, data_count,
        static_cast<std::size_t>(2 * head_dim) * kv->nb[0], kv->nb[1],
        static_cast<std::size_t>(head_dim) * kv->nb[0]);
    state = ggml_add(context, state, linear(context, attention(context, q, k, v, query_count),
                                            weights, cross + ".attn.c_proj"));
    state = ggml_add(context, state, mlp(context, norm(context, state, weights, cross + ".ln_3"), weights, cross + ".mlp"));
    for (std::size_t layer = 0; layer < 8U; ++layer) {
        const std::string stem = "mesh.enc.attn.resblocks." + std::to_string(layer);
        auto * qkv = linear(context, norm(context, state, weights, stem + ".ln_1"), weights, stem + ".attn.c_qkv", false);
        q = ggml_view_3d(context, qkv, head_dim, heads, query_count,
            static_cast<std::size_t>(3 * head_dim) * qkv->nb[0], qkv->nb[1], 0);
        k = ggml_view_3d(context, qkv, head_dim, heads, query_count,
            static_cast<std::size_t>(3 * head_dim) * qkv->nb[0], qkv->nb[1],
            static_cast<std::size_t>(head_dim) * qkv->nb[0]);
        v = ggml_view_3d(context, qkv, head_dim, heads, query_count,
            static_cast<std::size_t>(3 * head_dim) * qkv->nb[0], qkv->nb[1],
            static_cast<std::size_t>(2 * head_dim) * qkv->nb[0]);
        state = ggml_add(context, state, linear(context, attention(context, q, k, v, query_count),
                                                weights, stem + ".attn.c_proj"));
        state = ggml_add(context, state, mlp(context, norm(context, state, weights, stem + ".ln_2"), weights, stem + ".mlp"));
    }
    state = norm(context, state, weights, "mesh.enc.ln_post");
    state = linear(context, state, weights, "mesh.out.0");
    auto * output_scale = need(weights, "mesh.out.1.w");
    if (output_scale->type != state->type) output_scale = ggml_cast(context, output_scale, state->type);
    state = ggml_mul(context, ggml_rms_norm(context, state, std::numeric_limits<float>::epsilon()),
                     ggml_repeat(context, output_scale, state));
    auto * graph = ggml_new_graph_custom(context, 16384, false);
    ggml_build_forward_expand(graph, state);
    auto * allocator = ggml_gallocr_new(ggml_backend_get_default_buffer_type(backend));
    if (allocator == nullptr || !ggml_gallocr_reserve(allocator, graph) || !ggml_gallocr_alloc_graph(allocator, graph)) {
        if (allocator != nullptr) ggml_gallocr_free(allocator);
        return tl::unexpected(fail(error_code::allocation, "cannot allocate mesh encoder graph"));
    }
    auto allocator_cleanup = std::unique_ptr<ggml_gallocr, decltype(&ggml_gallocr_free)>(allocator, ggml_gallocr_free);
    ggml_backend_tensor_set(input, embedded.data(), 0, embedded.size() * sizeof(float));
    ggml_backend_tensor_set(indices, query_indices.data(), 0, query_indices.size_bytes());
    if (ggml_backend_graph_compute(backend, graph) != GGML_STATUS_SUCCESS)
        return tl::unexpected(fail(error_code::compute, "mesh encoder graph execution failed"));
    return read_output(state);
} catch (const std::exception & exception) {
    return tl::unexpected(fail(error_code::compute, exception.what()));
}

} // namespace skintokens::detail

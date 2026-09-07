#include "internal.hpp"

#include <cmath>
#include <cstdio>
#include <memory>

namespace skintokens::detail {
namespace {

constexpr std::int64_t width = 768;
constexpr std::int64_t heads = 12;
constexpr std::int64_t head_dim = 64;
constexpr std::int64_t points_count = 64;
constexpr std::int64_t condition_queries = 8;
constexpr std::int64_t skin_tokens = 4;

ggml_tensor * need(const weight_component & weights, const std::string & name) {
    auto * value = weights.tensor(name);
    if (value == nullptr) throw std::runtime_error("missing SkinVAE tensor: " + name);
    return value;
}

ggml_tensor * linear(ggml_context * context, ggml_tensor * input,
                     const weight_component & weights, const std::string & stem) {
    auto * output = ggml_mul_mat(context, need(weights, stem + ".w"), input);
    ggml_mul_mat_set_prec(output, GGML_PREC_F32);
    if (auto * bias = weights.tensor(stem + ".b"))
        output = ggml_add(context, output, ggml_repeat(context, bias, output));
    return output;
}

ggml_tensor * norm(ggml_context * context, ggml_tensor * input,
                   const weight_component & weights, const std::string & stem) {
    auto * output = ggml_norm(context, input, 1e-5F);
    output = ggml_mul(context, output, ggml_repeat(context, need(weights, stem + ".w"), output));
    return ggml_add(context, output, ggml_repeat(context, need(weights, stem + ".b"), output));
}

struct qkv_values { ggml_tensor * q; ggml_tensor * k; ggml_tensor * v; };

qkv_values split_self(ggml_context * context, ggml_tensor * q, ggml_tensor * k,
                      ggml_tensor * v, std::int64_t sequence) {
    // Tripo2AttnProcessor2_0 does not use the conventional independent
    // q/k/v head reshape. It concatenates complete q, k and v projections,
    // views that buffer as [head, 3 * head_dim], then splits the innermost
    // triplet. Preserve that released layout exactly.
    auto * joined = ggml_concat(context, ggml_concat(context, q, k, 0), v, 0);
    joined = ggml_reshape_3d(context, ggml_cont(context, joined), head_dim * 3, heads, sequence);
    const auto part = [&](std::size_t index) {
        return ggml_view_3d(context, joined, head_dim, heads, sequence,
            joined->nb[1], joined->nb[2], index * static_cast<std::size_t>(head_dim) * sizeof(float));
    };
    return {part(0U), part(1U), part(2U)};
}

std::pair<ggml_tensor *, ggml_tensor *> split_cross(
    ggml_context * context, ggml_tensor * k, ggml_tensor * v, std::int64_t sequence) {
    // Cross attention applies the analogous concatenate-then-split layout to
    // K/V. See upstream Tripo2AttnProcessor2_0 lines 100-111.
    auto * joined = ggml_concat(context, k, v, 0);
    joined = ggml_reshape_3d(context, ggml_cont(context, joined), head_dim * 2, heads, sequence);
    const auto part = [&](std::size_t index) {
        return ggml_view_3d(context, joined, head_dim, heads, sequence,
            joined->nb[1], joined->nb[2], index * static_cast<std::size_t>(head_dim) * sizeof(float));
    };
    return {part(0U), part(1U)};
}

ggml_tensor * attend(ggml_context * context, ggml_tensor * q, ggml_tensor * k,
                     ggml_tensor * v, std::int64_t query_count, std::int64_t) {
    q = ggml_permute(context, q, 0, 2, 1, 3);
    k = ggml_permute(context, k, 0, 2, 1, 3);
    v = ggml_permute(context, v, 0, 2, 1, 3);
    auto * scores = ggml_mul_mat(context, k, q);
    ggml_mul_mat_set_prec(scores, GGML_PREC_F32);
    auto * probabilities = ggml_soft_max(context,
        ggml_scale(context, scores, 1.0F / std::sqrt(static_cast<float>(head_dim))));
    auto * output = ggml_mul_mat(context, ggml_cont(context, ggml_transpose(context, v)), probabilities);
    ggml_mul_mat_set_prec(output, GGML_PREC_F32);
    output = ggml_cont(context, ggml_permute(context, output, 0, 2, 1, 3));
    return ggml_reshape_2d(context, output, width, query_count);
}

ggml_tensor * self_attention(ggml_context * context, ggml_tensor * input,
                             const weight_component & weights, const std::string & stem,
                             std::int64_t sequence) {
    auto values = split_self(context,
        linear(context, input, weights, stem + ".to_q"),
        linear(context, input, weights, stem + ".to_k"),
        linear(context, input, weights, stem + ".to_v"), sequence);
    return linear(context, attend(context, values.q, values.k, values.v, sequence, sequence),
                  weights, stem + ".to_out.0");
}

ggml_tensor * cross_attention(ggml_context * context, ggml_tensor * input, ggml_tensor * data,
                              const weight_component & weights, const std::string & stem,
                              std::int64_t query_count, std::int64_t data_count) {
    auto * q = linear(context, input, weights, stem + ".to_q");
    auto * normalized_data = norm(context, data, weights, stem + ".norm_cross");
    auto [k, v] = split_cross(context,
        linear(context, normalized_data, weights, stem + ".to_k"),
        linear(context, normalized_data, weights, stem + ".to_v"), data_count);
    q = ggml_reshape_3d(context, q, head_dim, heads, query_count);
    return linear(context, attend(context, q, k, v, query_count, data_count),
                  weights, stem + ".to_out.0");
}

ggml_tensor * feed_forward(ggml_context * context, ggml_tensor * input,
                           const weight_component & weights, const std::string & stem) {
    auto * hidden = linear(context, input, weights, stem + ".net.0.proj");
    return linear(context, ggml_gelu_erf(context, hidden), weights, stem + ".net.2");
}

ggml_tensor * self_block(ggml_context * context, ggml_tensor * state,
                         const weight_component & weights, const std::string & stem,
                         std::int64_t sequence) {
    state = ggml_add(context, state, self_attention(context,
        norm(context, state, weights, stem + ".norm1"), weights, stem + ".attn1", sequence));
    return ggml_add(context, state, feed_forward(context,
        norm(context, state, weights, stem + ".norm3"), weights, stem + ".ff"));
}

ggml_tensor * cross_block(ggml_context * context, ggml_tensor * state, ggml_tensor * data,
                          const weight_component & weights, const std::string & stem,
                          std::int64_t query_count, std::int64_t data_count) {
    state = ggml_add(context, state, cross_attention(context,
        norm(context, state, weights, stem + ".norm2"), data, weights, stem + ".attn2",
        query_count, data_count));
    return ggml_add(context, state, feed_forward(context,
        norm(context, state, weights, stem + ".norm3"), weights, stem + ".ff"));
}

std::vector<float> read_output(ggml_tensor * value) {
    if (value->type != GGML_TYPE_F32) throw std::runtime_error("SkinVAE snapshot is not F32");
    std::vector<float> output(static_cast<std::size_t>(ggml_nelements(value)));
    ggml_backend_tensor_get(value, output.data(), 0, output.size() * sizeof(float));
    return output;
}

std::vector<float> embed_condition(nonstd::span<const float> points, nonstd::span<const float> normals) {
    constexpr float pi = 3.14159265358979323846F;
    std::array<float, 8> frequency{};
    std::array<float, 8> phase{};
    for (std::size_t index = 0; index < 8U; ++index) {
        frequency[index] = pi * static_cast<float>(1U << index);
        const float step = static_cast<float>(index + 1U) / 8.0F;
        phase[index] = (std::pow(8.0F, 1.0F - step) + step) * 2.0F * pi;
    }
    std::vector<float> output(static_cast<std::size_t>(points_count) * 54U);
    for (std::size_t point = 0; point < static_cast<std::size_t>(points_count); ++point) {
        std::size_t cursor = point * 54U;
        for (std::size_t axis = 0; axis < 3U; ++axis) output[cursor++] = points[point * 3U + axis];
        for (std::size_t axis = 0; axis < 3U; ++axis) for (std::size_t f = 0; f < 8U; ++f) {
            const float x = points[point * 3U + axis];
            output[cursor++] = std::sin(x * frequency[f]) + std::sin(x * pi * 0.5F + phase[f]);
        }
        for (std::size_t axis = 0; axis < 3U; ++axis) for (std::size_t f = 0; f < 8U; ++f) {
            const float x = points[point * 3U + axis];
            output[cursor++] = std::cos(x * frequency[f]) + std::cos(x * pi * 0.5F + phase[f]);
        }
        for (std::size_t axis = 0; axis < 3U; ++axis) output[cursor++] = normals[point * 3U + axis];
    }
    return output;
}

std::vector<float> embed_condition(nonstd::span<const vec3> points, nonstd::span<const vec3> normals) {
    std::vector<float> flat_points(points.size() * 3U);
    std::vector<float> flat_normals(normals.size() * 3U);
    for (std::size_t index = 0; index < points.size(); ++index) {
        flat_points[index * 3U] = points[index].x;
        flat_points[index * 3U + 1U] = points[index].y;
        flat_points[index * 3U + 2U] = points[index].z;
        flat_normals[index * 3U] = normals[index].x;
        flat_normals[index * 3U + 1U] = normals[index].y;
        flat_normals[index * 3U + 2U] = normals[index].z;
    }
    constexpr float pi = 3.14159265358979323846F;
    std::array<float, 8> frequency{};
    std::array<float, 8> phase{};
    for (std::size_t index = 0; index < 8U; ++index) {
        frequency[index] = pi * static_cast<float>(1U << index);
        const float step = static_cast<float>(index + 1U) / 8.0F;
        phase[index] = (std::pow(8.0F, 1.0F - step) + step) * 2.0F * pi;
    }
    std::vector<float> output(points.size() * 54U);
    for (std::size_t point = 0; point < points.size(); ++point) {
        std::size_t cursor = point * 54U;
        for (std::size_t axis = 0; axis < 3U; ++axis) output[cursor++] = flat_points[point * 3U + axis];
        for (std::size_t axis = 0; axis < 3U; ++axis) for (std::size_t f = 0; f < 8U; ++f) {
            const float x = flat_points[point * 3U + axis];
            output[cursor++] = std::sin(x * frequency[f]) + std::sin(x * pi * 0.5F + phase[f]);
        }
        for (std::size_t axis = 0; axis < 3U; ++axis) for (std::size_t f = 0; f < 8U; ++f) {
            const float x = flat_points[point * 3U + axis];
            output[cursor++] = std::cos(x * frequency[f]) + std::cos(x * pi * 0.5F + phase[f]);
        }
        for (std::size_t axis = 0; axis < 3U; ++axis) output[cursor++] = flat_normals[point * 3U + axis];
    }
    return output;
}

} // namespace

result<tensor_snapshot> run_skin_vae_fixture(const weight_component & weights, ggml_backend_t backend,
                                              nonstd::span<const float> points,
                                              nonstd::span<const float> normals) try {
    if (backend == nullptr || points.size() != static_cast<std::size_t>(points_count * 3) || normals.size() != points.size())
        return tl::unexpected(fail(error_code::invalid_argument, "SkinVAE fixture must contain 64 point/normal triples"));
    auto embedding = embed_condition(points, normals);
    std::vector<float> codes(static_cast<std::size_t>(skin_tokens * 5));
    constexpr std::int32_t fsq_indices[]{0, 1, 12345, 32767};
    constexpr std::int32_t basis[]{1, 8, 64, 512, 4096};
    for (std::size_t token = 0; token < static_cast<std::size_t>(skin_tokens); ++token)
        for (std::size_t digit = 0; digit < 5U; ++digit)
            codes[token * 5U + digit] = static_cast<float>((fsq_indices[token] / basis[digit]) % 8 - 4) / 4.0F;

    auto * context = ggml_init({384ULL << 20U, nullptr, true});
    if (context == nullptr) return tl::unexpected(fail(error_code::allocation, "cannot create SkinVAE graph"));
    auto cleanup = std::unique_ptr<ggml_context, decltype(&ggml_free)>(context, ggml_free);
    auto * input = ggml_new_tensor_2d(context, GGML_TYPE_F32, 54, points_count);
    auto * indices = ggml_new_tensor_1d(context, GGML_TYPE_I32, condition_queries);
    auto * code_input = ggml_new_tensor_2d(context, GGML_TYPE_F32, 5, skin_tokens);
    ggml_set_input(input); ggml_set_input(indices); ggml_set_input(code_input);

    auto * cond_projected = linear(context, input, weights, "vae.cond_encoder.proj_in");
    auto * cond = ggml_get_rows(context, cond_projected, indices);
    cond = cross_block(context, cond, cond_projected, weights, "vae.cond_encoder.blocks.0",
                       condition_queries, points_count);
    std::vector<std::pair<std::string, ggml_tensor *>> snapshots{{"cond_projected", cond_projected}, {"cond_cross", cond}};
    for (std::size_t layer = 0; layer < 2U; ++layer) {
        cond = self_block(context, cond, weights, "vae.cond_encoder.blocks." + std::to_string(layer + 1U), condition_queries);
        char name[24]; std::snprintf(name, sizeof(name), "cond_self_%02zu", layer + 1U);
        snapshots.emplace_back(name, cond);
    }
    cond = norm(context, cond, weights, "vae.cond_encoder.norm_out");
    snapshots.emplace_back("cond_norm", cond);
    cond = linear(context, cond, weights, "vae.cond_quant");
    snapshots.emplace_back("cond_latents", cond);
    auto * decoded_codes = linear(context, code_input, weights, "vae.FSQ.project_out");
    snapshots.emplace_back("codes", decoded_codes);
    auto * decoder_input = ggml_concat(context, decoded_codes, cond, 1);
    auto * decoder = linear(context, decoder_input, weights, "vae.post_quant");
    snapshots.emplace_back("decoder_input", decoder);
    const auto decoder_sequence = skin_tokens + condition_queries;
    for (std::size_t layer = 0; layer < 10U; ++layer) {
        decoder = self_block(context, decoder, weights, "vae.decoder.blocks." + std::to_string(layer), decoder_sequence);
        char name[28]; std::snprintf(name, sizeof(name), "decoder_self_%02zu", layer + 1U);
        snapshots.emplace_back(name, decoder);
    }
    snapshots.emplace_back("decoder_cache", decoder);
    auto * query = linear(context, input, weights, "vae.decoder.proj_query");
    query = cross_block(context, query, decoder, weights, "vae.decoder.blocks.10", points_count, decoder_sequence);
    snapshots.emplace_back("decoder_cross", query);
    auto * logits = linear(context, norm(context, query, weights, "vae.decoder.norm_out"), weights, "vae.decoder.proj_out");
    auto * output = ggml_sigmoid(context, logits);
    snapshots.emplace_back("output", output);
    for (const auto & [name, tensor] : snapshots) { (void) name; ggml_set_output(tensor); }
    auto * graph = ggml_new_graph_custom(context, 32768, false);
    for (const auto & [name, tensor] : snapshots) { (void) name; ggml_build_forward_expand(graph, tensor); }
    auto * allocator = ggml_gallocr_new(ggml_backend_get_default_buffer_type(backend));
    if (allocator == nullptr || !ggml_gallocr_reserve(allocator, graph) || !ggml_gallocr_alloc_graph(allocator, graph)) {
        if (allocator != nullptr) ggml_gallocr_free(allocator);
        return tl::unexpected(fail(error_code::allocation, "cannot allocate SkinVAE graph"));
    }
    auto allocator_cleanup = std::unique_ptr<ggml_gallocr, decltype(&ggml_gallocr_free)>(allocator, ggml_gallocr_free);
    constexpr std::int32_t query_indices[]{0, 7, 13, 21, 32, 44, 55, 63};
    ggml_backend_tensor_set(input, embedding.data(), 0, embedding.size() * sizeof(float));
    ggml_backend_tensor_set(indices, query_indices, 0, sizeof(query_indices));
    ggml_backend_tensor_set(code_input, codes.data(), 0, codes.size() * sizeof(float));
    if (ggml_backend_graph_compute(backend, graph) != GGML_STATUS_SUCCESS)
        return tl::unexpected(fail(error_code::compute, "SkinVAE graph execution failed"));
    tensor_snapshot result{{"condition_embedding", std::move(embedding)}};
    for (const auto & [name, tensor] : snapshots) result.emplace(name, read_output(tensor));
    return result;
} catch (const std::exception & exception) {
    return tl::unexpected(fail(error_code::compute, exception.what()));
}

result<std::vector<float>> encode_skin_condition(const weight_component & weights, ggml_backend_t backend,
                                                  nonstd::span<const vec3> points,
                                                  nonstd::span<const vec3> normals,
                                                  nonstd::span<const std::int32_t> query_indices) try {
    if (backend == nullptr || points.empty() || normals.size() != points.size() || query_indices.empty())
        return tl::unexpected(fail(error_code::invalid_argument, "invalid SkinVAE condition input"));
    for (const auto index : query_indices)
        if (index < 0 || static_cast<std::size_t>(index) >= points.size())
            return tl::unexpected(fail(error_code::invalid_argument, "SkinVAE condition query index is out of range"));
    const auto data_count = static_cast<std::int64_t>(points.size());
    const auto query_count = static_cast<std::int64_t>(query_indices.size());
    auto embedding = embed_condition(points, normals);
    auto * context = ggml_init({256ULL << 20U, nullptr, true});
    if (context == nullptr) return tl::unexpected(fail(error_code::allocation, "cannot create SkinVAE condition graph"));
    auto cleanup = std::unique_ptr<ggml_context, decltype(&ggml_free)>(context, ggml_free);
    auto * input = ggml_new_tensor_2d(context, GGML_TYPE_F32, 54, data_count);
    auto * indices = ggml_new_tensor_1d(context, GGML_TYPE_I32, query_count);
    ggml_set_input(input); ggml_set_input(indices);
    auto * projected = linear(context, input, weights, "vae.cond_encoder.proj_in");
    auto * state = ggml_get_rows(context, projected, indices);
    state = cross_block(context, state, projected, weights, "vae.cond_encoder.blocks.0", query_count, data_count);
    for (std::size_t layer = 0; layer < 2U; ++layer)
        state = self_block(context, state, weights, "vae.cond_encoder.blocks." + std::to_string(layer + 1U), query_count);
    state = norm(context, state, weights, "vae.cond_encoder.norm_out");
    state = linear(context, state, weights, "vae.cond_quant");
    auto * graph = ggml_new_graph_custom(context, 8192, false); ggml_build_forward_expand(graph, state);
    auto * allocator = ggml_gallocr_new(ggml_backend_get_default_buffer_type(backend));
    if (allocator == nullptr || !ggml_gallocr_reserve(allocator, graph) || !ggml_gallocr_alloc_graph(allocator, graph)) {
        if (allocator != nullptr) ggml_gallocr_free(allocator);
        return tl::unexpected(fail(error_code::allocation, "cannot allocate SkinVAE condition graph"));
    }
    auto allocator_cleanup = std::unique_ptr<ggml_gallocr, decltype(&ggml_gallocr_free)>(allocator, ggml_gallocr_free);
    ggml_backend_tensor_set(input, embedding.data(), 0, embedding.size() * sizeof(float));
    ggml_backend_tensor_set(indices, query_indices.data(), 0, query_indices.size_bytes());
    if (ggml_backend_graph_compute(backend, graph) != GGML_STATUS_SUCCESS)
        return tl::unexpected(fail(error_code::compute, "SkinVAE condition graph execution failed"));
    return read_output(state);
} catch (const std::exception & exception) {
    return tl::unexpected(fail(error_code::compute, exception.what()));
}

result<std::vector<float>> decode_skin_joint(const weight_component & weights, ggml_backend_t backend,
                                              nonstd::span<const std::int32_t> codes,
                                              nonstd::span<const float> condition_latents,
                                              nonstd::span<const vec3> points,
                                              nonstd::span<const vec3> normals) try {
    if (backend == nullptr || codes.size() != 4U || points.empty() || normals.size() != points.size() ||
        condition_latents.empty() || condition_latents.size() % 512U != 0U)
        return tl::unexpected(fail(error_code::invalid_argument, "invalid SkinVAE decoder input"));
    const auto cond_count = static_cast<std::int64_t>(condition_latents.size() / 512U);
    const auto sequence = cond_count + skin_tokens;
    std::vector<float> fsq(20U);
    constexpr std::int32_t basis[]{1, 8, 64, 512, 4096};
    for (std::size_t token = 0; token < codes.size(); ++token) {
        // Upstream's logits processor intentionally/accidentally uses its
        // model EOS (32768 after the vocabulary offset) as the final joint's
        // fourth FSQ code. FSQ's mixed-radix modulo operation defines it.
        if (codes[token] < 0 || codes[token] > 32768)
            return tl::unexpected(fail(error_code::invalid_argument, "SkinVAE code is out of range"));
        for (std::size_t digit = 0; digit < 5U; ++digit)
            fsq[token * 5U + digit] = static_cast<float>((codes[token] / basis[digit]) % 8 - 4) / 4.0F;
    }
    // The ten self-attention blocks depend only on the four codes and the
    // compact condition latent. Compute that cache once per joint, then query
    // it in bounded vertex chunks. A dense Trellis mesh can contain millions
    // of vertices and must not create one correspondingly huge GGML graph.
    std::vector<float> decoder_cache;
    {
        auto * context = ggml_init({256ULL << 20U, nullptr, true});
        if (context == nullptr) return tl::unexpected(fail(error_code::allocation, "cannot create SkinVAE cache graph"));
        auto cleanup = std::unique_ptr<ggml_context, decltype(&ggml_free)>(context, ggml_free);
        auto * code_input = ggml_new_tensor_2d(context, GGML_TYPE_F32, 5, skin_tokens);
        auto * cond_input = ggml_new_tensor_2d(context, GGML_TYPE_F32, 512, cond_count);
        ggml_set_input(code_input); ggml_set_input(cond_input);
        auto * decoded_codes = linear(context, code_input, weights, "vae.FSQ.project_out");
        auto * state = linear(context, ggml_concat(context, decoded_codes, cond_input, 1), weights, "vae.post_quant");
        for (std::size_t layer = 0; layer < 10U; ++layer)
            state = self_block(context, state, weights, "vae.decoder.blocks." + std::to_string(layer), sequence);
        auto * graph = ggml_new_graph_custom(context, 16384, false); ggml_build_forward_expand(graph, state);
        auto * allocator = ggml_gallocr_new(ggml_backend_get_default_buffer_type(backend));
        if (allocator == nullptr || !ggml_gallocr_reserve(allocator, graph) || !ggml_gallocr_alloc_graph(allocator, graph)) {
            if (allocator != nullptr) ggml_gallocr_free(allocator);
            return tl::unexpected(fail(error_code::allocation, "cannot allocate SkinVAE cache graph"));
        }
        auto allocator_cleanup = std::unique_ptr<ggml_gallocr, decltype(&ggml_gallocr_free)>(allocator, ggml_gallocr_free);
        ggml_backend_tensor_set(code_input, fsq.data(), 0, fsq.size() * sizeof(float));
        ggml_backend_tensor_set(cond_input, condition_latents.data(), 0, condition_latents.size_bytes());
        if (ggml_backend_graph_compute(backend, graph) != GGML_STATUS_SUCCESS)
            return tl::unexpected(fail(error_code::compute, "SkinVAE cache graph execution failed"));
        decoder_cache = read_output(state);
    }

    constexpr std::size_t vertex_chunk = 16384U;
    std::vector<float> result(points.size());
    for (std::size_t begin = 0; begin < points.size(); begin += vertex_chunk) {
        const std::size_t count = std::min(vertex_chunk, points.size() - begin);
        auto embedding = embed_condition(points.subspan(begin, count), normals.subspan(begin, count));
        auto * context = ggml_init({256ULL << 20U, nullptr, true});
        if (context == nullptr) return tl::unexpected(fail(error_code::allocation, "cannot create SkinVAE query graph"));
        auto cleanup = std::unique_ptr<ggml_context, decltype(&ggml_free)>(context, ggml_free);
        auto * state_input = ggml_new_tensor_2d(context, GGML_TYPE_F32, width, sequence);
        auto * query_input = ggml_new_tensor_2d(context, GGML_TYPE_F32, 54, static_cast<std::int64_t>(count));
        ggml_set_input(state_input); ggml_set_input(query_input);
        auto * query = linear(context, query_input, weights, "vae.decoder.proj_query");
        query = cross_block(context, query, state_input, weights, "vae.decoder.blocks.10",
                            static_cast<std::int64_t>(count), sequence);
        auto * output = ggml_sigmoid(context, linear(context,
            norm(context, query, weights, "vae.decoder.norm_out"), weights, "vae.decoder.proj_out"));
        auto * graph = ggml_new_graph_custom(context, 8192, false); ggml_build_forward_expand(graph, output);
        auto * allocator = ggml_gallocr_new(ggml_backend_get_default_buffer_type(backend));
        if (allocator == nullptr || !ggml_gallocr_reserve(allocator, graph) || !ggml_gallocr_alloc_graph(allocator, graph)) {
            if (allocator != nullptr) ggml_gallocr_free(allocator);
            return tl::unexpected(fail(error_code::allocation, "cannot allocate SkinVAE query graph"));
        }
        auto allocator_cleanup = std::unique_ptr<ggml_gallocr, decltype(&ggml_gallocr_free)>(allocator, ggml_gallocr_free);
        ggml_backend_tensor_set(state_input, decoder_cache.data(), 0, decoder_cache.size() * sizeof(float));
        ggml_backend_tensor_set(query_input, embedding.data(), 0, embedding.size() * sizeof(float));
        if (ggml_backend_graph_compute(backend, graph) != GGML_STATUS_SUCCESS)
            return tl::unexpected(fail(error_code::compute, "SkinVAE query graph execution failed"));
        auto chunk = read_output(output);
        std::copy(chunk.begin(), chunk.end(), result.begin() + static_cast<std::ptrdiff_t>(begin));
    }
    return result;
} catch (const std::exception & exception) {
    return tl::unexpected(fail(error_code::compute, exception.what()));
}

} // namespace skintokens::detail

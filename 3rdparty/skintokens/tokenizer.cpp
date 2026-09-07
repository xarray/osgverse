#include "internal.hpp"

#include <algorithm>
#include <cfenv>
#include <cmath>
#include <optional.hpp>
#include <stdexcept>

namespace skintokens::detail {
namespace {

std::int32_t discrete(float value) {
    const float mapped = (value + 1.0F) * 128.0F;
    return static_cast<std::int32_t>(std::clamp(std::nearbyint(mapped), 0.0F, 255.0F));
}

} // namespace

result<skeleton_prefix> tokenize_skeleton_prefix(const mesh & source, const skeleton & target) {
    if (source.vertices.empty() || target.rest_positions.empty() ||
        target.parents.size() != target.rest_positions.size())
        return tl::unexpected(fail(error_code::invalid_argument, "cannot tokenize empty mesh/skeleton"));
    std::array<double, 3> low{source.vertices.front().x, source.vertices.front().y, source.vertices.front().z};
    auto high = low;
    const auto include = [&](vec3 value) {
        const std::array<double, 3> precise{value.x, value.y, value.z};
        for (std::size_t axis = 0; axis < 3U; ++axis) {
            low[axis] = std::min(low[axis], precise[axis]);
            high[axis] = std::max(high[axis], precise[axis]);
        }
    };
    for (const auto & value : source.vertices) include(value);
    for (const auto & value : target.rest_positions) include(value);
    const double half_extent = std::max({high[0] - low[0], high[1] - low[1], high[2] - low[2]}) * 0.5;
    if (!std::isfinite(half_extent) || half_extent <= 1e-12)
        return tl::unexpected(fail(error_code::invalid_argument, "mesh/skeleton bounds are degenerate"));
    // AugmentAffine assembles and multiplies float32 translation/scale
    // matrices, then applies the result to Blender's float64 coordinates.
    const float inverse_scale = static_cast<float>(1.0 / half_extent);
    std::array<float, 3> bias{};
    for (std::size_t axis = 0; axis < 3U; ++axis)
        bias[axis] = inverse_scale * static_cast<float>(-(high[axis] + low[axis]) * 0.5);
    const auto precise_normalized = [&](vec3 value) {
        return std::array<double, 3>{
            static_cast<double>(value.x) * inverse_scale + bias[0],
            static_cast<double>(value.y) * inverse_scale + bias[1],
            static_cast<double>(value.z) * inverse_scale + bias[2]};
    };
    const auto normalized = [&](vec3 value) {
        const auto precise = precise_normalized(value);
        return vec3{static_cast<float>(precise[0]), static_cast<float>(precise[1]),
                    static_cast<float>(precise[2])};
    };
    skeleton_prefix output;
    output.normalized_vertices.reserve(source.vertices.size());
    output.precise_normalized_vertices.reserve(source.vertices.size());
    for (const auto & value : source.vertices) {
        output.normalized_vertices.push_back(normalized(value));
        output.precise_normalized_vertices.push_back(precise_normalized(value));
    }
    output.normalized_joints.reserve(target.rest_positions.size());
    for (const auto & value : target.rest_positions) output.normalized_joints.push_back(normalized(value));

    // Match the released executable path, including its configuration quirk:
    // imported demo assets use class articulation=266, but TokenizerPart.parse
    // looks for `order` while the checkpoint stores `order_config`. Its order
    // therefore remains null and no spring/part marker is emitted. The
    // official supplied-skeleton capture starts [257, 266, coordinates...].
    output.tokens = {257, 266};
    std::int32_t previous = -1;
    const auto append = [&](vec3 value, std::vector<std::int32_t> & tokens) {
        tokens.push_back(discrete(value.x)); tokens.push_back(discrete(value.y)); tokens.push_back(discrete(value.z));
    };
    for (std::size_t joint = 0; joint < output.normalized_joints.size(); ++joint) {
        const std::int32_t parent = target.parents[joint];
        const bool branch = joint != 0U && parent != previous;
        if (branch) {
            if (parent < 0 || static_cast<std::size_t>(parent) >= joint)
                return tl::unexpected(fail(error_code::invalid_argument, "skeleton parent order is invalid"));
            output.tokens.push_back(256);
            append(output.normalized_joints[static_cast<std::size_t>(parent)], output.tokens);
        }
        append(output.normalized_joints[joint], output.tokens);
        previous = static_cast<std::int32_t>(joint);
    }
    output.tokens.push_back(258);
    return output;
}

result<skeleton> detokenize_generated_skeleton(
    nonstd::span<const std::int32_t> tokens, vec3 center, float scale) try {
    if (tokens.size() < 6U || tokens.front() != 257 || tokens.back() != 258 ||
        !std::isfinite(scale) || scale <= 0.0F)
        return tl::unexpected(fail(error_code::invalid_argument, "invalid generated skeleton token sequence"));
    const auto coordinate = [](std::int32_t token) {
        if (token < 0 || token >= 256) throw std::runtime_error("skeleton coordinate is outside the tokenizer vocabulary");
        return ((static_cast<float>(token) + 0.5F) / 256.0F) * 2.0F - 1.0F;
    };
    const auto point = [&](std::size_t offset) {
        if (offset + 3U > tokens.size()) throw std::runtime_error("truncated skeleton coordinate");
        return vec3{coordinate(tokens[offset]) * scale + center.x,
                    coordinate(tokens[offset + 1U]) * scale + center.y,
                    coordinate(tokens[offset + 2U]) * scale + center.z};
    };
    skeleton output;
    std::size_t cursor = 1U;
    bool branch = false;
    nonstd::optional<vec3> previous;
    while (cursor + 1U < tokens.size()) {
        const auto token = tokens[cursor];
        if (token == 258) break;
        if (token == 256) { branch = true; previous.reset(); ++cursor; continue; }
        if (token >= 260 && token <= 266) { ++cursor; continue; }
        vec3 parent_point{};
        if (branch) { parent_point = point(cursor); cursor += 3U; }
        const vec3 current = point(cursor); cursor += 3U;
        if (!branch) parent_point = previous.value_or(current);
        std::int32_t parent = -1;
        if (!output.rest_positions.empty()) {
            float best = std::numeric_limits<float>::infinity();
            for (std::size_t index = 0; index < output.rest_positions.size(); ++index) {
                const auto candidate = output.rest_positions[index];
                const float x = candidate.x - parent_point.x, y = candidate.y - parent_point.y,
                            z = candidate.z - parent_point.z;
                const float distance = x*x + y*y + z*z;
                if (distance < best) { best = distance; parent = static_cast<std::int32_t>(index); }
            }
        }
        output.names.push_back("bone_" + std::to_string(output.names.size()));
        output.parents.push_back(parent);
        output.rest_positions.push_back(current);
        previous = current;
        branch = false;
    }
    if (output.names.empty() || output.names.size() > 256U)
        return tl::unexpected(fail(error_code::compute, "TokenRig generated an empty or oversized skeleton"));
    return output;
} catch (const std::exception & exception) {
    return tl::unexpected(fail(error_code::compute, exception.what()));
}

} // namespace skintokens::detail

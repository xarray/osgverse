#include <skintokens/skintokens.hpp>

#include "internal.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace skintokens {
namespace {

using matrix4 = std::array<float, 16>;

[[nodiscard]] bool finite(vec3 value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool finite(quat value) {
    return std::isfinite(value.x) && std::isfinite(value.y) &&
           std::isfinite(value.z) && std::isfinite(value.w);
}

[[nodiscard]] matrix4 local_matrix(vec3 translation, quat rotation) {
    const float length_squared = rotation.x * rotation.x + rotation.y * rotation.y +
        rotation.z * rotation.z + rotation.w * rotation.w;
    const float scale = length_squared > 0.0F ? 2.0F / length_squared : 0.0F;
    const float xx = rotation.x * rotation.x * scale;
    const float yy = rotation.y * rotation.y * scale;
    const float zz = rotation.z * rotation.z * scale;
    const float xy = rotation.x * rotation.y * scale;
    const float xz = rotation.x * rotation.z * scale;
    const float yz = rotation.y * rotation.z * scale;
    const float wx = rotation.w * rotation.x * scale;
    const float wy = rotation.w * rotation.y * scale;
    const float wz = rotation.w * rotation.z * scale;
    return {
        1.0F - yy - zz, xy + wz, xz - wy, 0.0F,
        xy - wz, 1.0F - xx - zz, yz + wx, 0.0F,
        xz + wy, yz - wx, 1.0F - xx - yy, 0.0F,
        translation.x, translation.y, translation.z, 1.0F,
    };
}

[[nodiscard]] matrix4 multiply(const matrix4 & left, const matrix4 & right) {
    matrix4 output{};
    for (std::size_t column = 0; column < 4U; ++column) {
        for (std::size_t row = 0; row < 4U; ++row) {
            double value = 0.0;
            for (std::size_t inner = 0; inner < 4U; ++inner) {
                value += static_cast<double>(left[inner * 4U + row]) *
                    static_cast<double>(right[column * 4U + inner]);
            }
            output[column * 4U + row] = static_cast<float>(value);
        }
    }
    return output;
}

[[nodiscard]] vec3 transform_point(const matrix4 & matrix, vec3 point) {
    return {
        matrix[0] * point.x + matrix[4] * point.y + matrix[8] * point.z + matrix[12],
        matrix[1] * point.x + matrix[5] * point.y + matrix[9] * point.z + matrix[13],
        matrix[2] * point.x + matrix[6] * point.y + matrix[10] * point.z + matrix[14],
    };
}

[[nodiscard]] bool compatible(const skeleton & left, const skeleton & right) {
    if (left.parents != right.parents || left.rest_positions.size() != right.rest_positions.size()) {
        return false;
    }
    for (std::size_t index = 0; index < left.rest_positions.size(); ++index) {
        const auto & a = left.rest_positions[index];
        const auto & b = right.rest_positions[index];
        if (a.x != b.x || a.y != b.y || a.z != b.z) return false;
    }
    return true;
}

} // namespace

result<std::vector<vec3>> deform_vertices(
    const mesh & geometry,
    const skin & binding,
    const motion & animation,
    std::size_t frame) {
    const std::size_t joint_count = binding.rig.parents.size();
    if (geometry.vertices.empty()) {
        return tl::unexpected(detail::fail(error_code::invalid_argument, "mesh has no vertices"));
    }
    if (joint_count == 0U || binding.rig.rest_positions.size() != joint_count ||
        binding.rig.names.size() != joint_count) {
        return tl::unexpected(detail::fail(error_code::invalid_argument, "skin skeleton arrays do not agree"));
    }
    if (binding.joints.size() != geometry.vertices.size() ||
        binding.weights.size() != geometry.vertices.size()) {
        return tl::unexpected(detail::fail(error_code::invalid_argument, "skin influence count does not match mesh vertices"));
    }
    if (animation.frames == 0U || frame >= animation.frames) {
        return tl::unexpected(detail::fail(error_code::invalid_argument, "animation frame is out of range"));
    }
    if (!compatible(binding.rig, animation.rig)) {
        return tl::unexpected(detail::fail(error_code::invalid_argument, "skin and animation skeletons are incompatible"));
    }
    std::size_t rotation_count = 0U;
    if (!detail::checked_mul(animation.frames, joint_count, rotation_count) ||
        animation.local_rotations.size() != rotation_count ||
        animation.root_translations.size() != animation.frames) {
        return tl::unexpected(detail::fail(error_code::invalid_argument, "animation arrays do not agree"));
    }

    std::vector<matrix4> globals(joint_count);
    std::size_t roots = 0U;
    for (std::size_t joint = 0U; joint < joint_count; ++joint) {
        const auto parent = binding.rig.parents[joint];
        if (parent < -1 || (parent >= 0 && static_cast<std::size_t>(parent) >= joint)) {
            return tl::unexpected(detail::fail(error_code::invalid_argument,
                "skeleton parents must precede their children"));
        }
        if (parent < 0) ++roots;
        vec3 translation = animation.root_translations[frame];
        if (parent >= 0) {
            const auto parent_index = static_cast<std::size_t>(parent);
            const auto & child_rest = binding.rig.rest_positions[joint];
            const auto & parent_rest = binding.rig.rest_positions[parent_index];
            translation = {child_rest.x - parent_rest.x, child_rest.y - parent_rest.y,
                           child_rest.z - parent_rest.z};
        }
        const quat rotation = animation.local_rotations[frame * joint_count + joint];
        if (!finite(translation) || !finite(rotation)) {
            return tl::unexpected(detail::fail(error_code::invalid_argument, "animation contains non-finite transforms"));
        }
        const matrix4 local = local_matrix(translation, rotation);
        globals[joint] = parent < 0 ? local : multiply(globals[static_cast<std::size_t>(parent)], local);
    }
    if (roots != 1U || binding.rig.parents.front() != -1) {
        return tl::unexpected(detail::fail(error_code::invalid_argument,
            "skeleton must contain exactly one root at joint zero"));
    }

    std::vector<vec3> output(geometry.vertices.size());
    for (std::size_t vertex = 0U; vertex < geometry.vertices.size(); ++vertex) {
        const auto source = geometry.vertices[vertex];
        if (!finite(source)) {
            return tl::unexpected(detail::fail(error_code::invalid_argument, "mesh contains non-finite vertices"));
        }
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
        double weight_sum = 0.0;
        for (std::size_t influence = 0U; influence < 4U; ++influence) {
            const std::size_t joint = binding.joints[vertex][influence];
            const float weight = binding.weights[vertex][influence];
            if (joint >= joint_count || !std::isfinite(weight) || weight < 0.0F) {
                return tl::unexpected(detail::fail(error_code::invalid_argument, "skin contains an invalid influence"));
            }
            if (weight == 0.0F) continue;
            const auto & bind = binding.rig.rest_positions[joint];
            const vec3 relative{source.x - bind.x, source.y - bind.y, source.z - bind.z};
            const vec3 transformed = transform_point(globals[joint], relative);
            x += static_cast<double>(weight) * transformed.x;
            y += static_cast<double>(weight) * transformed.y;
            z += static_cast<double>(weight) * transformed.z;
            weight_sum += weight;
        }
        if (weight_sum <= 0.0) {
            return tl::unexpected(detail::fail(error_code::invalid_argument, "skin vertex has no positive influence"));
        }
        output[vertex] = {static_cast<float>(x / weight_sum), static_cast<float>(y / weight_sum),
                          static_cast<float>(z / weight_sum)};
    }
    return output;
}

} // namespace skintokens

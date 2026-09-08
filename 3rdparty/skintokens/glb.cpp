// GLB validation and accessor handling are adapted from animate-any-mesh.cpp's
// independently written parser. This version additionally reads Kimodo node
// animation channels and keeps the API deliberately bounded.
#include "internal.hpp"
#include <libhv/all/json.hpp>
//#include <nlohmann/json.hpp>

#include <optional.hpp>
#include <algorithm>
#include <array>
//#include <bit>
#include <cctype>
#include <cmath>
#include <cstring>
#include <fstream>
#include <functional>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>
#include <cstdint>
#include <type_traits>

namespace nonstd {
    enum class endian {
        little = 0,
        big    = 1,
    #if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        native = little
    #elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        native = big
    #elif defined(__LITTLE_ENDIAN__)
        native = little
    #elif defined(__BIG_ENDIAN__)
        native = big
    #elif defined(_WIN32) || defined(__i386__) || defined(__x86_64__) || defined(__arm__) || defined(__aarch64__)
        native = little
    #else
    #   error "Cannot determine endianness for this platform"
    #endif
    };

    template <typename T>
    constexpr std::enable_if_t<std::is_integral<T>::value && sizeof(T) == 1, T> byteswap(T value) noexcept {
        return value;
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_integral<T>::value && sizeof(T) == 2, T> byteswap(T value) noexcept {
        auto u = static_cast<std::uint16_t>(value);
        return static_cast<T>(((u & 0x00FF) << 8) | ((u & 0xFF00) >> 8));
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_integral<T>::value && sizeof(T) == 4, T> byteswap(T value) noexcept {
        auto u = static_cast<std::uint32_t>(value);
        return static_cast<T>(
            ((u & 0x000000FF) << 24) | ((u & 0x0000FF00) << 8)  |
            ((u & 0x00FF0000) >> 8)  | ((u & 0xFF000000) >> 24));
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_integral<T>::value && sizeof(T) == 8, T> byteswap(T value) noexcept {
        auto u = static_cast<std::uint64_t>(value);
        return static_cast<T>(
            ((u & 0x00000000000000FFULL) << 56) | ((u & 0x000000000000FF00ULL) << 40) |
            ((u & 0x0000000000FF0000ULL) << 24) | ((u & 0x00000000FF000000ULL) << 8)  |
            ((u & 0x000000FF00000000ULL) >> 8)  | ((u & 0x0000FF0000000000ULL) >> 24) |
            ((u & 0x00FF000000000000ULL) >> 40) | ((u & 0xFF00000000000000ULL) >> 56));
    }
}

namespace skintokens {
namespace {

using json = nlohmann::json;
constexpr std::uint32_t glb_magic = 0x46546C67U;
constexpr std::uint32_t json_chunk = 0x4E4F534AU;
constexpr std::uint32_t bin_chunk = 0x004E4942U;
constexpr std::size_t max_file = 512U << 20U;
constexpr std::size_t max_json = 16U << 20U;
constexpr std::size_t max_vertices = 4U << 20U;

std::uint32_t u32(nonstd::span<const std::byte> bytes, std::size_t offset) {
    std::uint32_t value = 0;
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    if constexpr (nonstd::endian::native == nonstd::endian::big) value = nonstd::byteswap(value);
    return value;
}

template<class T> T little(const std::byte * data) {
    T value{};
    std::memcpy(&value, data, sizeof(value));
    if constexpr (nonstd::endian::native == nonstd::endian::big && std::is_integral_v<T>) value = nonstd::byteswap(value);
    return value;
}

struct document {
    json root;
    std::vector<std::byte> binary;
};

result<document> read_document(const std::filesystem::path & path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) return tl::unexpected(detail::fail(error_code::io, "cannot open GLB: " + path.string()));
    const auto end = stream.tellg();
    if (end < 0 || static_cast<std::uint64_t>(end) > max_file)
        return tl::unexpected(detail::fail(error_code::limit_exceeded, "GLB exceeds 512 MiB safety limit"));
    std::vector<std::byte> bytes(static_cast<std::size_t>(end));
    stream.seekg(0);
    if (!bytes.empty() && !stream.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size())))
        return tl::unexpected(detail::fail(error_code::io, "failed to read GLB"));
    if (bytes.size() < 20U || u32(bytes, 0) != glb_magic || u32(bytes, 4) != 2U || u32(bytes, 8) != bytes.size())
        return tl::unexpected(detail::fail(error_code::invalid_format, "invalid GLB v2 header"));
    const std::size_t json_size = u32(bytes, 12);
    if (u32(bytes, 16) != json_chunk || json_size == 0 || json_size > max_json || json_size > bytes.size() - 20U)
        return tl::unexpected(detail::fail(error_code::invalid_format, "invalid GLB JSON chunk"));
    document output;
    try {
        const auto * begin = reinterpret_cast<const char *>(bytes.data() + 20U);
        output.root = json::parse(begin, begin + json_size);
    } catch (const std::exception & exception) {
        return tl::unexpected(detail::fail(error_code::invalid_format, std::string{"invalid glTF JSON: "} + exception.what()));
    }
    const std::size_t next = 20U + json_size;
    if (next + 8U <= bytes.size()) {
        const std::size_t binary_size = u32(bytes, next);
        if (u32(bytes, next + 4U) != bin_chunk || binary_size > bytes.size() - next - 8U)
            return tl::unexpected(detail::fail(error_code::invalid_format, "invalid GLB binary chunk"));
        output.binary.assign(bytes.begin() + static_cast<std::ptrdiff_t>(next + 8U),
                             bytes.begin() + static_cast<std::ptrdiff_t>(next + 8U + binary_size));
    }
    return output;
}

result<const json *> item(const json & root, std::string_view array, std::size_t index) {
    auto found = root.find(array);
    if (found == root.end() || !found->is_array() || index >= found->size() || !(*found)[index].is_object())
        return tl::unexpected(detail::fail(error_code::invalid_format, "glTF " + std::string{array} + " index is invalid"));
    return &(*found)[index];
}

result<std::size_t> integer(const json & object, std::string_view name, std::size_t fallback, bool required = true) {
    auto found = object.find(name);
    if (found == object.end()) {
        if (!required) return fallback;
        return tl::unexpected(detail::fail(error_code::invalid_format, "missing glTF integer field " + std::string{name}));
    }
    if (!found->is_number_unsigned())
        return tl::unexpected(detail::fail(error_code::invalid_format, "invalid glTF integer field " + std::string{name}));
    const auto value = found->get<std::uint64_t>();
    if (value > std::numeric_limits<std::size_t>::max())
        return tl::unexpected(detail::fail(error_code::limit_exceeded, "glTF integer is out of range"));
    return static_cast<std::size_t>(value);
}

std::size_t scalar_size(std::uint32_t component) {
    if (component == 5121U) return 1U;
    if (component == 5123U) return 2U;
    if (component == 5125U || component == 5126U) return 4U;
    return 0U;
}

std::size_t components(std::string_view shape) {
    if (shape == "SCALAR") return 1U;
    if (shape == "VEC2") return 2U;
    if (shape == "VEC3") return 3U;
    if (shape == "VEC4") return 4U;
    if (shape == "MAT4") return 16U;
    return 0U;
}

struct accessor {
    nonstd::span<const std::byte> bytes;
    std::size_t count = 0;
    std::size_t stride = 0;
    std::uint32_t component = 0;
    std::string shape;
    bool normalized = false;
};

result<accessor> get_accessor(const document & doc, std::size_t index) {
    auto accessor_item = item(doc.root, "accessors", index);
    if (!accessor_item) return tl::unexpected(accessor_item.error());
    const auto & object = **accessor_item;
    if (object.contains("sparse"))
        return tl::unexpected(detail::fail(error_code::invalid_format, "sparse glTF accessors are unsupported"));
    auto view_index = integer(object, "bufferView", 0U);
    auto count = integer(object, "count", 0U);
    auto component = integer(object, "componentType", 0U);
    auto offset = integer(object, "byteOffset", 0U, false);
    if (!view_index) return tl::unexpected(view_index.error());
    if (!count) return tl::unexpected(count.error());
    if (!component) return tl::unexpected(component.error());
    if (!offset) return tl::unexpected(offset.error());
    auto shape_value = object.find("type");
    if (shape_value == object.end() || !shape_value->is_string())
        return tl::unexpected(detail::fail(error_code::invalid_format, "glTF accessor type is missing"));
    const std::string shape = shape_value->get<std::string>();
    const std::size_t element = scalar_size(static_cast<std::uint32_t>(*component)) * components(shape);
    if (element == 0U || *count > max_vertices * 256U)
        return tl::unexpected(detail::fail(error_code::limit_exceeded, "unsupported or oversized glTF accessor"));
    auto view_item = item(doc.root, "bufferViews", *view_index);
    if (!view_item) return tl::unexpected(view_item.error());
    const auto & view = **view_item;
    auto buffer = integer(view, "buffer", 0U);
    auto view_size = integer(view, "byteLength", 0U);
    auto view_offset = integer(view, "byteOffset", 0U, false);
    auto stride = integer(view, "byteStride", element, false);
    if (!buffer) return tl::unexpected(buffer.error());
    if (!view_size) return tl::unexpected(view_size.error());
    if (!view_offset) return tl::unexpected(view_offset.error());
    if (!stride) return tl::unexpected(stride.error());
    if (*buffer != 0U || *stride < element || *stride > 252U || *offset > *view_size)
        return tl::unexpected(detail::fail(error_code::invalid_format, "invalid GLB buffer view"));
    std::size_t payload = 0;
    if (*count != 0U) {
        std::size_t prefix = 0;
        if (!detail::checked_mul(*count - 1U, *stride, prefix) || prefix > std::numeric_limits<std::size_t>::max() - element)
            return tl::unexpected(detail::fail(error_code::limit_exceeded, "glTF accessor size overflows"));
        payload = prefix + element;
    }
    if (payload > *view_size - *offset || *view_offset > doc.binary.size() ||
        *offset > doc.binary.size() - *view_offset || payload > doc.binary.size() - *view_offset - *offset)
        return tl::unexpected(detail::fail(error_code::invalid_format, "glTF accessor exceeds binary buffer"));
    const std::size_t begin = *view_offset + *offset;
    return accessor{nonstd::span<const std::byte>{doc.binary}.subspan(begin, payload), *count, *stride,
                    static_cast<std::uint32_t>(*component), shape, object.value("normalized", false)};
}

result<float> scalar(const accessor & value, std::size_t index) {
    if (value.component != 5126U || value.shape != "SCALAR" || index >= value.count)
        return tl::unexpected(detail::fail(error_code::invalid_format, "animation time accessor must be float SCALAR"));
    const float output = little<float>(value.bytes.data() + index * value.stride);
    if (!std::isfinite(output)) return tl::unexpected(detail::fail(error_code::invalid_format, "accessor contains non-finite float"));
    return output;
}

result<vec3> vector3(const accessor & value, std::size_t index) {
    if (value.component != 5126U || value.shape != "VEC3" || index >= value.count)
        return tl::unexpected(detail::fail(error_code::invalid_format, "accessor must be float VEC3"));
    const auto * pointer = value.bytes.data() + index * value.stride;
    vec3 output{little<float>(pointer), little<float>(pointer + 4U), little<float>(pointer + 8U)};
    if (!std::isfinite(output.x) || !std::isfinite(output.y) || !std::isfinite(output.z))
        return tl::unexpected(detail::fail(error_code::invalid_format, "accessor contains non-finite vector"));
    return output;
}

result<quat> quaternion(const accessor & value, std::size_t index) {
    if (value.component != 5126U || value.shape != "VEC4" || index >= value.count)
        return tl::unexpected(detail::fail(error_code::invalid_format, "rotation accessor must be float VEC4"));
    const auto * pointer = value.bytes.data() + index * value.stride;
    quat output{little<float>(pointer), little<float>(pointer + 4U), little<float>(pointer + 8U), little<float>(pointer + 12U)};
    if (!std::isfinite(output.x) || !std::isfinite(output.y) || !std::isfinite(output.z) || !std::isfinite(output.w))
        return tl::unexpected(detail::fail(error_code::invalid_format, "accessor contains non-finite quaternion"));
    return output;
}

result<std::array<std::uint16_t,4>> joint_vector(const accessor & value,std::size_t index) {
    if (value.shape!="VEC4" || index>=value.count ||
        (value.component!=5121U && value.component!=5123U))
        return tl::unexpected(detail::fail(error_code::invalid_format,
            "JOINTS_0 must be an unsigned byte or unsigned short VEC4"));
    const auto * pointer=value.bytes.data()+index*value.stride;
    std::array<std::uint16_t,4> output{};
    for (std::size_t component=0;component<4U;++component)
        output[component]=value.component==5121U?
            std::to_integer<std::uint8_t>(pointer[component]):little<std::uint16_t>(pointer+component*2U);
    return output;
}

result<std::array<float,4>> weight_vector(const accessor & value,std::size_t index) {
    if (value.shape!="VEC4" || index>=value.count ||
        (value.component!=5121U && value.component!=5123U && value.component!=5126U))
        return tl::unexpected(detail::fail(error_code::invalid_format,
            "WEIGHTS_0 must be a supported VEC4"));
    if (value.component!=5126U && !value.normalized)
        return tl::unexpected(detail::fail(error_code::invalid_format,
            "integer WEIGHTS_0 must be normalized"));
    const auto * pointer=value.bytes.data()+index*value.stride;
    std::array<float,4> output{};
    for (std::size_t component=0;component<4U;++component) {
        output[component]=value.component==5126U?little<float>(pointer+component*4U):
            value.component==5123U?static_cast<float>(little<std::uint16_t>(pointer+component*2U))/65535.0F:
            static_cast<float>(std::to_integer<std::uint8_t>(pointer[component]))/255.0F;
        if (!std::isfinite(output[component]) || output[component]<0.0F)
            return tl::unexpected(detail::fail(error_code::invalid_format,
                "WEIGHTS_0 contains an invalid value"));
    }
    const float sum=output[0]+output[1]+output[2]+output[3];
    if (sum<=1.0e-12F)
        return tl::unexpected(detail::fail(error_code::invalid_format,
            "WEIGHTS_0 contains an unweighted vertex"));
    for (auto & weight:output) weight/=sum;
    return output;
}

result<color4> color(const accessor & value, std::size_t index) {
    const auto count = components(value.shape);
    if ((count != 3U && count != 4U) || index >= value.count ||
        (value.component != 5121U && value.component != 5123U && value.component != 5126U))
        return tl::unexpected(detail::fail(error_code::invalid_format, "COLOR_0 must be a supported VEC3 or VEC4"));
    const auto * pointer = value.bytes.data() + index * value.stride;
    const auto channel = [&](std::size_t component) {
        if (value.component == 5126U) return little<float>(pointer + component * 4U);
        if (value.component == 5123U)
            return static_cast<float>(little<std::uint16_t>(pointer + component * 2U)) / 65535.0F;
        return static_cast<float>(std::to_integer<std::uint8_t>(pointer[component])) / 255.0F;
    };
    if (value.component != 5126U && !value.normalized)
        return tl::unexpected(detail::fail(error_code::invalid_format, "integer COLOR_0 accessor must be normalized"));
    color4 output{channel(0U), channel(1U), channel(2U), count == 4U ? channel(3U) : 1.0F};
    if (!std::isfinite(output.r) || !std::isfinite(output.g) || !std::isfinite(output.b) || !std::isfinite(output.a))
        return tl::unexpected(detail::fail(error_code::invalid_format, "COLOR_0 contains a non-finite value"));
    return output;
}

result<std::uint32_t> index_value(const accessor & value, std::size_t index) {
    if (value.shape != "SCALAR" || index >= value.count)
        return tl::unexpected(detail::fail(error_code::invalid_format, "index accessor must be SCALAR"));
    const auto * pointer = value.bytes.data() + index * value.stride;
    if (value.component == 5121U) return std::to_integer<std::uint8_t>(*pointer);
    if (value.component == 5123U) return little<std::uint16_t>(pointer);
    if (value.component == 5125U) return little<std::uint32_t>(pointer);
    return tl::unexpected(detail::fail(error_code::invalid_format, "unsupported index component type"));
}

vec3 add(vec3 left, vec3 right) { return {left.x + right.x, left.y + right.y, left.z + right.z}; }
vec3 sub(vec3 left, vec3 right) { return {left.x - right.x, left.y - right.y, left.z - right.z}; }
vec3 cross(vec3 left, vec3 right) {
    return {left.y * right.z - left.z * right.y, left.z * right.x - left.x * right.z,
            left.x * right.y - left.y * right.x};
}

using matrix4 = std::array<float, 16>;

matrix4 identity_matrix() {
    return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
}

matrix4 multiply(const matrix4 & left, const matrix4 & right) {
    matrix4 output{};
    for (std::size_t column = 0; column < 4U; ++column)
        for (std::size_t row = 0; row < 4U; ++row)
            for (std::size_t inner = 0; inner < 4U; ++inner)
                output[column * 4U + row] += left[inner * 4U + row] * right[column * 4U + inner];
    return output;
}

result<std::vector<float>> numeric_array(const json & object, std::string_view name, std::size_t count) {
    auto found = object.find(name);
    if (found == object.end()) return std::vector<float>{};
    if (!found->is_array() || found->size() != count)
        return tl::unexpected(detail::fail(error_code::invalid_format, "invalid glTF node " + std::string{name}));
    std::vector<float> output(count);
    for (std::size_t i = 0; i < count; ++i) {
        if (!(*found)[i].is_number())
            return tl::unexpected(detail::fail(error_code::invalid_format, "non-numeric glTF node " + std::string{name}));
        output[i] = (*found)[i].get<float>();
        if (!std::isfinite(output[i]))
            return tl::unexpected(detail::fail(error_code::invalid_format, "non-finite glTF node " + std::string{name}));
    }
    return output;
}

result<matrix4> node_matrix(const json & node) {
    auto explicit_matrix = numeric_array(node, "matrix", 16U);
    if (!explicit_matrix) return tl::unexpected(explicit_matrix.error());
    if (!explicit_matrix->empty()) {
        matrix4 output{};
        std::copy(explicit_matrix->begin(), explicit_matrix->end(), output.begin());
        return output;
    }
    auto translation = numeric_array(node, "translation", 3U);
    auto rotation = numeric_array(node, "rotation", 4U);
    auto scale = numeric_array(node, "scale", 3U);
    if (!translation) return tl::unexpected(translation.error());
    if (!rotation) return tl::unexpected(rotation.error());
    if (!scale) return tl::unexpected(scale.error());
    const float x = rotation->empty() ? 0.0F : (*rotation)[0];
    const float y = rotation->empty() ? 0.0F : (*rotation)[1];
    const float z = rotation->empty() ? 0.0F : (*rotation)[2];
    const float w = rotation->empty() ? 1.0F : (*rotation)[3];
    const float sx = scale->empty() ? 1.0F : (*scale)[0];
    const float sy = scale->empty() ? 1.0F : (*scale)[1];
    const float sz = scale->empty() ? 1.0F : (*scale)[2];
    const float tx = translation->empty() ? 0.0F : (*translation)[0];
    const float ty = translation->empty() ? 0.0F : (*translation)[1];
    const float tz = translation->empty() ? 0.0F : (*translation)[2];
    const float length = std::sqrt(x*x + y*y + z*z + w*w);
    if (length <= 1e-12F)
        return tl::unexpected(detail::fail(error_code::invalid_format, "glTF node rotation is zero"));
    const float qx = x / length, qy = y / length, qz = z / length, qw = w / length;
    return matrix4{
        (1.0F - 2.0F*(qy*qy + qz*qz))*sx, 2.0F*(qx*qy + qz*qw)*sx, 2.0F*(qx*qz - qy*qw)*sx, 0,
        2.0F*(qx*qy - qz*qw)*sy, (1.0F - 2.0F*(qx*qx + qz*qz))*sy, 2.0F*(qy*qz + qx*qw)*sy, 0,
        2.0F*(qx*qz + qy*qw)*sz, 2.0F*(qy*qz - qx*qw)*sz, (1.0F - 2.0F*(qx*qx + qy*qy))*sz, 0,
        tx, ty, tz, 1
    };
}

vec3 transform_point(const matrix4 & matrix, vec3 value) {
    return {matrix[0]*value.x + matrix[4]*value.y + matrix[8]*value.z + matrix[12],
            matrix[1]*value.x + matrix[5]*value.y + matrix[9]*value.z + matrix[13],
            matrix[2]*value.x + matrix[6]*value.y + matrix[10]*value.z + matrix[14]};
}

float determinant3(const matrix4 & matrix) {
    return matrix[0]*(matrix[5]*matrix[10] - matrix[9]*matrix[6]) -
           matrix[4]*(matrix[1]*matrix[10] - matrix[9]*matrix[2]) +
           matrix[8]*(matrix[1]*matrix[6] - matrix[5]*matrix[2]);
}

template<class T>
void append_value(std::vector<std::byte> & bytes, const T & value) {
    static_assert(std::is_trivially_copyable_v<T>);
    const auto old = bytes.size();
    bytes.resize(old + sizeof(T));
    std::memcpy(bytes.data() + old, &value, sizeof(T));
}

void align4(std::vector<std::byte> & bytes) {
    while (bytes.size() % 4U != 0U) bytes.push_back(std::byte{});
}

struct glb_builder {
    json root = {
        {"asset", {{"version", "2.0"}, {"generator", "skin-tokens.cpp"}}},
        {"buffers", json::array()}, {"bufferViews", json::array()},
        {"accessors", json::array()}, {"nodes", json::array()},
    };
    std::vector<std::byte> binary;

    template<class T>
    std::size_t accessor_for(nonstd::span<const T> values, std::uint32_t component,
                             std::string_view type, nonstd::optional<std::uint32_t> target = std::nullopt,
                             json extra = {}) {
        align4(binary);
        const std::size_t offset = binary.size();
        for (const T & value : values) append_value(binary, value);
        json view{{"buffer", 0U}, {"byteOffset", offset}, {"byteLength", values.size_bytes()}};
        if (target) view["target"] = *target;
        const std::size_t view_index = root["bufferViews"].size();
        root["bufferViews"].push_back(std::move(view));
        json accessor_value{{"bufferView", view_index}, {"componentType", component},
                            {"count", values.size()}, {"type", type}};
        if (!extra.is_null()) accessor_value.update(extra);
        const std::size_t index = root["accessors"].size();
        root["accessors"].push_back(std::move(accessor_value));
        return index;
    }
};

result<void> validate_export(const mesh & geometry, const skin & binding, const motion & animation) {
    if (geometry.vertices.empty() || geometry.faces.empty() || geometry.normals.size() != geometry.vertices.size())
        return tl::unexpected(detail::fail(error_code::invalid_argument, "export mesh arrays are incomplete"));
    if (!geometry.colors.empty() && geometry.colors.size() != geometry.vertices.size())
        return tl::unexpected(detail::fail(error_code::invalid_argument, "mesh colours must match vertex count"));
    for (const auto & value : geometry.colors) {
        if (!std::isfinite(value.r) || !std::isfinite(value.g) || !std::isfinite(value.b) || !std::isfinite(value.a))
            return tl::unexpected(detail::fail(error_code::invalid_argument, "mesh contains a non-finite colour"));
    }
    if (binding.joints.size() != geometry.vertices.size() || binding.weights.size() != geometry.vertices.size())
        return tl::unexpected(detail::fail(error_code::invalid_argument, "skin influence count does not match mesh vertices"));
    if (binding.rig.names.empty() || binding.rig.names.size() > 256U ||
        binding.rig.parents.size() != binding.rig.names.size() ||
        binding.rig.rest_positions.size() != binding.rig.names.size())
        return tl::unexpected(detail::fail(error_code::invalid_argument, "skin skeleton arrays are invalid"));
    if (animation.frames == 0U || animation.rig.names != binding.rig.names ||
        animation.rig.parents != binding.rig.parents ||
        animation.root_translations.size() != animation.frames ||
        animation.local_rotations.size() != animation.frames * binding.rig.names.size())
        return tl::unexpected(detail::fail(error_code::invalid_argument, "animation does not match the skin skeleton"));
    for (std::size_t vertex = 0; vertex < binding.joints.size(); ++vertex) {
        float sum = 0.0F;
        for (std::size_t slot = 0; slot < 4U; ++slot) {
            if (binding.joints[vertex][slot] >= binding.rig.names.size() ||
                !std::isfinite(binding.weights[vertex][slot]) || binding.weights[vertex][slot] < 0.0F)
                return tl::unexpected(detail::fail(error_code::invalid_argument, "skin contains an invalid influence"));
            sum += binding.weights[vertex][slot];
        }
        if (std::abs(sum - 1.0F) > 1e-3F)
            return tl::unexpected(detail::fail(error_code::invalid_argument, "skin weights are not normalized"));
    }
    return {};
}

} // namespace

result<mesh> load_glb_file(const std::filesystem::path & path) {
    auto doc = read_document(path);
    if (!doc) return tl::unexpected(doc.error());
    auto meshes = doc->root.find("meshes");
    if (meshes == doc->root.end() || !meshes->is_array())
        return tl::unexpected(detail::fail(error_code::invalid_format, "GLB contains no meshes"));

    struct instance { std::size_t mesh_index; matrix4 transform; };
    std::vector<instance> instances;
    auto nodes = doc->root.find("nodes");
    auto scenes = doc->root.find("scenes");
    if (nodes != doc->root.end() && nodes->is_array() && scenes != doc->root.end() && scenes->is_array() && !scenes->empty()) {
        std::size_t scene_index = 0U;
        if (auto selected = doc->root.find("scene"); selected != doc->root.end()) {
            if (!selected->is_number_unsigned())
                return tl::unexpected(detail::fail(error_code::invalid_format, "glTF scene index is invalid"));
            scene_index = selected->get<std::size_t>();
        }
        if (scene_index >= scenes->size() || !(*scenes)[scene_index].is_object())
            return tl::unexpected(detail::fail(error_code::invalid_format, "glTF scene index is out of range"));
        const auto & roots = (*scenes)[scene_index].value("nodes", json::array());
        if (!roots.is_array())
            return tl::unexpected(detail::fail(error_code::invalid_format, "glTF scene roots are invalid"));
        std::vector<std::uint8_t> visiting(nodes->size(), 0U);
        std::function<result<void>(std::size_t, const matrix4 &)> visit =
            [&](std::size_t index, const matrix4 & parent) -> result<void> {
                if (index >= nodes->size() || !(*nodes)[index].is_object())
                    return tl::unexpected(detail::fail(error_code::invalid_format, "glTF node index is invalid"));
                if (visiting[index] != 0U)
                    return tl::unexpected(detail::fail(error_code::invalid_format, "glTF scene graph contains a cycle or repeated node"));
                visiting[index] = 1U;
                const auto & node = (*nodes)[index];
                auto local = node_matrix(node);
                if (!local) return tl::unexpected(local.error());
                const matrix4 world = multiply(parent, *local);
                if (auto mesh_index = node.find("mesh"); mesh_index != node.end()) {
                    if (!mesh_index->is_number_unsigned() || mesh_index->get<std::size_t>() >= meshes->size())
                        return tl::unexpected(detail::fail(error_code::invalid_format, "glTF node mesh index is invalid"));
                    if (instances.size() >= 4096U)
                        return tl::unexpected(detail::fail(error_code::limit_exceeded, "too many glTF mesh instances"));
                    instances.push_back({mesh_index->get<std::size_t>(), world});
                }
                if (auto children = node.find("children"); children != node.end()) {
                    if (!children->is_array())
                        return tl::unexpected(detail::fail(error_code::invalid_format, "glTF node children are invalid"));
                    for (const auto & child : *children) {
                        if (!child.is_number_unsigned())
                            return tl::unexpected(detail::fail(error_code::invalid_format, "glTF child index is invalid"));
                        auto nested = visit(child.get<std::size_t>(), world);
                        if (!nested) return nested;
                    }
                }
                visiting[index] = 0U;
                return {};
            };
        for (const auto & root : roots) {
            if (!root.is_number_unsigned())
                return tl::unexpected(detail::fail(error_code::invalid_format, "glTF scene root is invalid"));
            auto visited = visit(root.get<std::size_t>(), identity_matrix());
            if (!visited) return tl::unexpected(visited.error());
        }
    } else {
        for (std::size_t index = 0; index < meshes->size(); ++index)
            instances.push_back({index, identity_matrix()});
    }

    mesh output;
    bool has_colors = false;
    double metallic_sum = 0.0;
    double roughness_sum = 0.0;
    std::size_t material_vertices = 0U;
    for (const auto & instance : instances) {
        const auto & mesh_object = (*meshes)[instance.mesh_index];
        if (!mesh_object.is_object())
            return tl::unexpected(detail::fail(error_code::invalid_format, "glTF mesh is invalid"));
        auto primitives = mesh_object.find("primitives");
        if (primitives == mesh_object.end() || !primitives->is_array()) continue;
        for (const auto & primitive : *primitives) {
            if (primitive.value("mode", 4U) != 4U || !primitive.contains("attributes") || !primitive.contains("indices")) continue;
            const auto & attributes = primitive["attributes"];
            if (!attributes.contains("POSITION") || !attributes["POSITION"].is_number_unsigned()) continue;
            auto positions = get_accessor(*doc, attributes["POSITION"].get<std::size_t>());
            auto indices = get_accessor(*doc, primitive["indices"].get<std::size_t>());
            if (!positions) return tl::unexpected(positions.error());
            if (!indices) return tl::unexpected(indices.error());
            if (indices->count % 3U != 0U || output.vertices.size() + positions->count > max_vertices)
                return tl::unexpected(detail::fail(error_code::limit_exceeded, "mesh primitive exceeds supported size"));
            std::optional<accessor> colors;
            if (auto color_index = attributes.find("COLOR_0"); color_index != attributes.end()) {
                if (!color_index->is_number_unsigned())
                    return tl::unexpected(detail::fail(error_code::invalid_format, "COLOR_0 accessor index is invalid"));
                auto value = get_accessor(*doc, color_index->get<std::size_t>());
                if (!value) return tl::unexpected(value.error());
                if (value->count != positions->count)
                    return tl::unexpected(detail::fail(error_code::invalid_format, "COLOR_0 count differs from POSITION"));
                colors = *value;
                has_colors = true;
            }
            color4 factor;
            float metallic = 0.0F, roughness = 0.72F;
            if (auto material_index = primitive.find("material"); material_index != primitive.end()) {
                if (!material_index->is_number_unsigned())
                    return tl::unexpected(detail::fail(error_code::invalid_format, "glTF material index is invalid"));
                auto material = item(doc->root, "materials", material_index->get<std::size_t>());
                if (!material) return tl::unexpected(material.error());
                output.double_sided = output.double_sided || (**material).value("doubleSided", false);
                if (auto pbr = (**material).find("pbrMetallicRoughness"); pbr != (**material).end() && pbr->is_object()) {
                    metallic = pbr->value("metallicFactor", 1.0F);
                    roughness = pbr->value("roughnessFactor", 1.0F);
                    auto base = numeric_array(*pbr, "baseColorFactor", 4U);
                    if (!base) return tl::unexpected(base.error());
                    if (!base->empty()) {
                        factor = {(*base)[0], (*base)[1], (*base)[2], (*base)[3]};
                        has_colors = has_colors || factor.r != 1.0F || factor.g != 1.0F ||
                            factor.b != 1.0F || factor.a != 1.0F;
                    }
                }
            }
            if (!std::isfinite(metallic) || !std::isfinite(roughness))
                return tl::unexpected(detail::fail(error_code::invalid_format, "glTF material contains a non-finite factor"));
            metallic_sum += static_cast<double>(metallic) * static_cast<double>(positions->count);
            roughness_sum += static_cast<double>(roughness) * static_cast<double>(positions->count);
            material_vertices += positions->count;
            const auto base = static_cast<std::uint32_t>(output.vertices.size());
            for (std::size_t i = 0; i < positions->count; ++i) {
                auto value = vector3(*positions, i);
                if (!value) return tl::unexpected(value.error());
                output.vertices.push_back(transform_point(instance.transform, *value));
                color4 shade = factor;
                if (colors) {
                    auto vertex_color = color(*colors, i);
                    if (!vertex_color) return tl::unexpected(vertex_color.error());
                    shade = {vertex_color->r * factor.r, vertex_color->g * factor.g,
                             vertex_color->b * factor.b, vertex_color->a * factor.a};
                }
                output.colors.push_back(shade);
            }
            const bool mirrored = determinant3(instance.transform) < 0.0F;
            for (std::size_t i = 0; i < indices->count; i += 3U) {
                triangle face{};
                for (std::size_t j = 0; j < 3U; ++j) {
                    auto value = index_value(*indices, i + j);
                    if (!value) return tl::unexpected(value.error());
                    if (*value >= positions->count)
                        return tl::unexpected(detail::fail(error_code::invalid_format, "triangle index exceeds POSITION accessor"));
                    face[j] = base + *value;
                }
                if (mirrored) std::swap(face[1], face[2]);
                output.faces.push_back(face);
            }
        }
    }
    if (output.vertices.empty() || output.faces.empty())
        return tl::unexpected(detail::fail(error_code::invalid_format, "GLB has no indexed triangle geometry"));
    if (!has_colors) output.colors.clear();
    if (material_vertices != 0U) {
        output.metallic_factor = static_cast<float>(metallic_sum / static_cast<double>(material_vertices));
        output.roughness_factor = static_cast<float>(roughness_sum / static_cast<double>(material_vertices));
    }
    output.normals.assign(output.vertices.size(), {});
    for (const auto & face : output.faces) {
        const vec3 normal = cross(sub(output.vertices[face[1]], output.vertices[face[0]]),
                                  sub(output.vertices[face[2]], output.vertices[face[0]]));
        for (auto vertex : face) output.normals[vertex] = add(output.normals[vertex], normal);
    }
    for (auto & normal : output.normals) {
        const float length = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        if (length > 1e-12F) { normal.x /= length; normal.y /= length; normal.z /= length; }
        else normal = {0.0F, 1.0F, 0.0F};
    }
    return output;
}

result<mesh> load_trellis_mesh_file(const std::filesystem::path & path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) return tl::unexpected(detail::fail(error_code::io, "cannot open T2MESH: " + path.string()));
    const auto end = stream.tellg();
    if (end < 16 || static_cast<std::uint64_t>(end) > max_file)
        return tl::unexpected(detail::fail(error_code::limit_exceeded, "T2MESH size is outside supported limits"));
    const std::size_t file_size = static_cast<std::size_t>(end);
    stream.seekg(0);
    std::array<char, 8> magic{};
    stream.read(magic.data(), static_cast<std::streamsize>(magic.size()));
    const std::string_view signature{magic.data(), magic.size()};
    const std::size_t pbr_width = signature == "T2MESH01" ? 0U :
        signature == "T2MESH02" ? 5U : signature == "T2MESH03" ? 6U : std::numeric_limits<std::size_t>::max();
    if (pbr_width == std::numeric_limits<std::size_t>::max())
        return tl::unexpected(detail::fail(error_code::invalid_format, "unsupported T2MESH version"));
    std::array<std::byte, 8> counts{};
    stream.read(reinterpret_cast<char *>(counts.data()), static_cast<std::streamsize>(counts.size()));
    if (!stream) return tl::unexpected(detail::fail(error_code::invalid_format, "truncated T2MESH header"));
    const std::size_t vertex_count = u32(counts, 0U);
    const std::size_t triangle_count = u32(counts, 4U);
    if (vertex_count == 0U || vertex_count > max_vertices || triangle_count == 0U || triangle_count > max_vertices * 2U)
        return tl::unexpected(detail::fail(error_code::limit_exceeded, "T2MESH geometry exceeds supported limits"));
    std::size_t per_vertex = 0U, vertex_bytes = 0U, triangle_bytes = 0U;
    if (!detail::checked_mul(6U + pbr_width, sizeof(float), per_vertex) ||
        !detail::checked_mul(vertex_count, per_vertex, vertex_bytes) ||
        !detail::checked_mul(triangle_count, sizeof(std::int32_t) * 3U, triangle_bytes) ||
        vertex_bytes > std::numeric_limits<std::size_t>::max() - 16U ||
        vertex_bytes > file_size - 16U ||
        triangle_bytes != file_size - 16U - vertex_bytes)
        return tl::unexpected(detail::fail(error_code::invalid_format, "T2MESH byte length does not match its header"));

    const auto read_float = [&]() -> result<float> {
        std::array<std::byte, 4> bytes{};
        stream.read(reinterpret_cast<char *>(bytes.data()), 4);
        if (!stream) return tl::unexpected(detail::fail(error_code::invalid_format, "truncated T2MESH float stream"));
        const float value = little<float>(bytes.data());
        if (!std::isfinite(value))
            return tl::unexpected(detail::fail(error_code::invalid_format, "T2MESH contains a non-finite float"));
        return value;
    };
    mesh output;
    output.vertices.resize(vertex_count);
    output.normals.resize(vertex_count);
    for (auto & value : output.vertices) {
        auto x = read_float(), y = read_float(), z = read_float();
        if (!x) return tl::unexpected(x.error());
        if (!y) return tl::unexpected(y.error());
        if (!z) return tl::unexpected(z.error());
        value = {*x, *z, -*y}; // Trellis Z-up -> glTF/SkinTokens Y-up.
    }
    for (auto & value : output.normals) {
        auto x = read_float(), y = read_float(), z = read_float();
        if (!x) return tl::unexpected(x.error());
        if (!y) return tl::unexpected(y.error());
        if (!z) return tl::unexpected(z.error());
        value = {*x, *z, -*y};
        const float length = std::sqrt(value.x*value.x + value.y*value.y + value.z*value.z);
        if (length > 1e-12F) { value.x /= length; value.y /= length; value.z /= length; }
    }
    if (pbr_width != 0U) {
        const auto linear = [](float value) {
            value = std::clamp(value, 0.0F, 1.0F);
            return value <= 0.04045F ? value / 12.92F : std::pow((value + 0.055F) / 1.055F, 2.4F);
        };
        output.colors.resize(vertex_count);
        double metallic = 0.0, roughness = 0.0, weight = 0.0;
        for (std::size_t vertex = 0; vertex < vertex_count; ++vertex) {
            std::array<float, 6> pbr{0.7F, 0.7F, 0.7F, 0.0F, 0.6F, 1.0F};
            for (std::size_t channel = 0; channel < pbr_width; ++channel) {
                auto value = read_float();
                if (!value) return tl::unexpected(value.error());
                pbr[channel] = *value;
            }
            const float alpha = std::clamp(pbr[5], 0.0F, 1.0F);
            output.colors[vertex] = {linear(pbr[0]), linear(pbr[1]), linear(pbr[2]), alpha};
            const double visible = std::max(0.001, static_cast<double>(alpha));
            metallic += std::clamp(pbr[3], 0.0F, 1.0F) * visible;
            roughness += std::clamp(pbr[4], 0.0F, 1.0F) * visible;
            weight += visible;
        }
        output.metallic_factor = static_cast<float>(metallic / weight);
        output.roughness_factor = static_cast<float>(roughness / weight);
        output.double_sided = true;
    }
    output.faces.resize(triangle_count);
    for (auto & face : output.faces) {
        for (auto & index : face) {
            std::array<std::byte, 4> bytes{};
            stream.read(reinterpret_cast<char *>(bytes.data()), 4);
            if (!stream) return tl::unexpected(detail::fail(error_code::invalid_format, "truncated T2MESH index stream"));
            const std::int32_t signed_index = little<std::int32_t>(bytes.data());
            if (signed_index < 0 || static_cast<std::size_t>(signed_index) >= vertex_count)
                return tl::unexpected(detail::fail(error_code::invalid_format, "T2MESH triangle index is out of range"));
            index = static_cast<std::uint32_t>(signed_index);
        }
    }
    return output;
}

namespace {

std::string canonical_joint_name(std::string_view name) {
    const auto colon = name.rfind(':');
    if (colon != std::string_view::npos) name.remove_prefix(colon + 1U);
    std::string output{name};
    std::transform(output.begin(), output.end(), output.begin(),
        [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    return output;
}

result<motion> load_skeleton_document(const document & doc, bool require_animation) {
    auto nodes = doc.root.find("nodes");
    auto animations = doc.root.find("animations");
    if (nodes == doc.root.end() || !nodes->is_array() || nodes->empty() || nodes->size() > 256U)
        return tl::unexpected(detail::fail(error_code::invalid_format, "GLB has no bounded skeleton nodes"));
    if (require_animation &&
        (animations == doc.root.end() || !animations->is_array() || animations->empty()))
        return tl::unexpected(detail::fail(error_code::invalid_format, "GLB has no bounded skeleton animation"));
    const auto declared_skins = doc.root.find("skins");
    const auto declared_meshes = doc.root.find("meshes");
    const bool has_declared_skin = declared_skins != doc.root.end() && declared_skins->is_array() && !declared_skins->empty();
    const bool has_declared_animation = animations != doc.root.end() && animations->is_array() && !animations->empty();
    const bool has_declared_mesh = declared_meshes != doc.root.end() && declared_meshes->is_array() && !declared_meshes->empty();
    if (!has_declared_skin && !has_declared_animation && (has_declared_mesh || nodes->size() < 2U))
        return tl::unexpected(detail::fail(error_code::invalid_format, "GLB contains no identifiable skeleton"));
    motion output;
    std::vector<std::size_t> joint_nodes;
    const auto skins = doc.root.find("skins");
    if (skins != doc.root.end() && skins->is_array() && !skins->empty()) {
        if (!(*skins)[0].is_object() || !(*skins)[0].contains("joints") ||
            !(*skins)[0]["joints"].is_array())
            return tl::unexpected(detail::fail(error_code::invalid_format, "glTF skin has no joint array"));
        for (const auto & value : (*skins)[0]["joints"]) {
            if (!value.is_number_unsigned() || value.get<std::size_t>() >= nodes->size())
                return tl::unexpected(detail::fail(error_code::invalid_format, "skin joint node is invalid"));
            joint_nodes.push_back(value.get<std::size_t>());
        }
    } else {
        joint_nodes.resize(nodes->size());
        std::iota(joint_nodes.begin(), joint_nodes.end(), 0U);
    }
    if (joint_nodes.empty() || joint_nodes.size() > 256U)
        return tl::unexpected(detail::fail(error_code::limit_exceeded, "GLB has no bounded skin joints"));
    std::vector<std::int32_t> node_to_joint(nodes->size(), -1);
    for (std::size_t joint = 0; joint < joint_nodes.size(); ++joint) {
        if (node_to_joint[joint_nodes[joint]] >= 0)
            return tl::unexpected(detail::fail(error_code::invalid_format, "skin contains duplicate joints"));
        node_to_joint[joint_nodes[joint]] = static_cast<std::int32_t>(joint);
    }
    const std::size_t joint_count = joint_nodes.size();
    output.rig.names.resize(joint_count);
    output.rig.parents.assign(joint_count, -1);
    output.rig.rest_positions.resize(joint_count);
    std::vector<std::int32_t> node_parents(nodes->size(), -1);
    for (std::size_t i = 0; i < nodes->size(); ++i) {
        const auto & node = (*nodes)[i];
        auto children = node.find("children");
        if (children != node.end() && children->is_array()) {
            for (const auto & child_value : *children) {
                if (!child_value.is_number_unsigned() || child_value.get<std::size_t>() >= nodes->size())
                    return tl::unexpected(detail::fail(error_code::invalid_format, "skeleton child index is invalid"));
                const auto child = child_value.get<std::size_t>();
                if (node_parents[child] >= 0)
                    return tl::unexpected(detail::fail(error_code::invalid_format, "skeleton node has multiple parents"));
                node_parents[child] = static_cast<std::int32_t>(i);
            }
        }
    }
    // glTF does not require skin.joints to be topologically ordered, while
    // TokenRig and the exported flat hierarchy require parents before
    // children. Preserve sibling order but normalize arbitrary valid skins.
    std::vector<std::size_t> ordered_joints;
    ordered_joints.reserve(joint_nodes.size());
    std::vector<bool> added(nodes->size(), false);
    while (ordered_joints.size() != joint_nodes.size()) {
        const auto before = ordered_joints.size();
        for (const auto node_index : joint_nodes) {
            if (added[node_index]) continue;
            std::int32_t parent_node = node_parents[node_index];
            while (parent_node >= 0 && node_to_joint[static_cast<std::size_t>(parent_node)] < 0)
                parent_node = node_parents[static_cast<std::size_t>(parent_node)];
            if (parent_node < 0 || added[static_cast<std::size_t>(parent_node)]) {
                ordered_joints.push_back(node_index);
                added[node_index] = true;
            }
        }
        if (ordered_joints.size() == before)
            return tl::unexpected(detail::fail(error_code::invalid_format, "skin joint hierarchy contains a cycle"));
    }
    joint_nodes = std::move(ordered_joints);
    std::fill(node_to_joint.begin(), node_to_joint.end(), -1);
    for (std::size_t joint = 0; joint < joint_nodes.size(); ++joint)
        node_to_joint[joint_nodes[joint]] = static_cast<std::int32_t>(joint);
    std::vector<matrix4> node_transforms(nodes->size());
    std::vector<std::uint8_t> transform_state(nodes->size());
    std::function<result<matrix4>(std::size_t)> global_transform = [&](std::size_t node) -> result<matrix4> {
        if (transform_state[node] == 2U) return node_transforms[node];
        if (transform_state[node] == 1U)
            return tl::unexpected(detail::fail(error_code::invalid_format, "skeleton hierarchy contains a cycle"));
        transform_state[node] = 1U;
        auto local = node_matrix((*nodes)[node]);
        if (!local) return tl::unexpected(local.error());
        if (node_parents[node] < 0) node_transforms[node] = *local;
        else {
            auto parent = global_transform(static_cast<std::size_t>(node_parents[node]));
            if (!parent) return tl::unexpected(parent.error());
            node_transforms[node] = multiply(*parent, *local);
        }
        transform_state[node] = 2U;
        return node_transforms[node];
    };
    for (std::size_t joint = 0; joint < joint_count; ++joint) {
        const std::size_t node_index = joint_nodes[joint];
        output.rig.names[joint] = (*nodes)[node_index].value("name", "joint_" + std::to_string(joint));
        std::int32_t parent_node = node_parents[node_index];
        while (parent_node >= 0 && node_to_joint[static_cast<std::size_t>(parent_node)] < 0)
            parent_node = node_parents[static_cast<std::size_t>(parent_node)];
        output.rig.parents[joint] = parent_node < 0 ? -1 : node_to_joint[static_cast<std::size_t>(parent_node)];
        auto transform = global_transform(node_index);
        if (!transform) return tl::unexpected(transform.error());
        output.rig.rest_positions[joint] = {(*transform)[12], (*transform)[13], (*transform)[14]};
    }
    output.frames = 1U;
    output.frames_per_second = 30.0F;
    output.root_translations.assign(1U, output.rig.rest_positions.front());
    output.local_rotations.assign(joint_count, {});
    if (animations == doc.root.end() || !animations->is_array() || animations->empty()) return output;
    const auto & animation = (*animations)[0];
    auto samplers = animation.find("samplers");
    auto channels = animation.find("channels");
    if (samplers == animation.end() || !samplers->is_array() || channels == animation.end() || !channels->is_array())
        return tl::unexpected(detail::fail(error_code::invalid_format, "animation has no samplers/channels"));
    struct channel_data { std::size_t node; std::string path; accessor times; accessor values; };
    std::vector<channel_data> parsed;
    std::size_t frames = 0;
    float duration = 0.0F;
    for (const auto & channel : *channels) {
        auto sampler_index = integer(channel, "sampler", 0U);
        if (!sampler_index || *sampler_index >= samplers->size() || !channel.contains("target")) continue;
        const auto & sampler = (*samplers)[*sampler_index];
        const auto & target = channel["target"];
        if (!sampler.contains("input") || !sampler.contains("output") || !target.contains("node") || !target.contains("path")) continue;
        if (!sampler["input"].is_number_unsigned() || !sampler["output"].is_number_unsigned() ||
            !target["node"].is_number_unsigned() || !target["path"].is_string()) continue;
        auto times = get_accessor(doc, sampler["input"].get<std::size_t>());
        auto values = get_accessor(doc, sampler["output"].get<std::size_t>());
        if (!times) return tl::unexpected(times.error());
        if (!values) return tl::unexpected(values.error());
        if (times->count != values->count || target["node"].get<std::size_t>() >= nodes->size()) continue;
        const auto joint = node_to_joint[target["node"].get<std::size_t>()];
        if (joint < 0) continue;
        frames = std::max(frames, times->count);
        if (times->count) {
            auto last = scalar(*times, times->count - 1U);
            if (!last) return tl::unexpected(last.error());
            duration = std::max(duration, *last);
        }
        parsed.push_back({static_cast<std::size_t>(joint), target["path"].get<std::string>(), *times, *values});
    }
    if (frames > 100000U)
        return tl::unexpected(detail::fail(error_code::limit_exceeded, "animation exceeds 100000 keyframes"));
    if (frames == 0U) {
        if (require_animation)
            return tl::unexpected(detail::fail(error_code::invalid_format, "animation has no bounded keyframes"));
        return output;
    }
    output.frames = frames;
    output.frames_per_second = duration > 0.0F ? static_cast<float>(frames - 1U) / duration : 30.0F;
    output.root_translations.assign(frames, output.rig.rest_positions[0]);
    output.local_rotations.assign(frames * joint_count, {});
    for (const auto & value : parsed) {
        for (std::size_t frame = 0; frame < std::min(frames, value.values.count); ++frame) {
            if (value.path == "translation" && output.rig.parents[value.node] < 0) {
                auto translation = vector3(value.values, frame);
                if (!translation) return tl::unexpected(translation.error());
                output.root_translations[frame] = *translation;
            } else if (value.path == "rotation") {
                auto rotation = quaternion(value.values, frame);
                if (!rotation) return tl::unexpected(rotation.error());
                output.local_rotations[frame * joint_count + value.node] = *rotation;
            }
        }
    }
    return output;
}

} // namespace

rig_kind identify_rig(const skeleton & value) {
    const auto has = [&](std::string_view wanted) {
        const auto canonical = canonical_joint_name(wanted);
        return std::any_of(value.names.begin(), value.names.end(), [&](const std::string & name) {
            return canonical_joint_name(name) == canonical;
        });
    };
    const auto linked = [&](std::string_view child_name, std::string_view parent_name) {
        std::size_t child = value.names.size(), parent = value.names.size();
        const auto child_key = canonical_joint_name(child_name), parent_key = canonical_joint_name(parent_name);
        for (std::size_t index = 0; index < value.names.size(); ++index) {
            const auto name = canonical_joint_name(value.names[index]);
            if (name == child_key) child = index;
            if (name == parent_key) parent = index;
        }
        return child < value.parents.size() && parent < value.names.size() &&
               value.parents[child] == static_cast<std::int32_t>(parent);
    };
    if (value.names.size() == 30U && has("Hips") && has("Chest") && has("LeftHandMiddleEnd") &&
        has("RightHandMiddleEnd") && has("LeftToeBase") && has("RightToeBase") &&
        linked("Spine1", "Hips") && linked("LeftForeArm", "LeftArm") &&
        linked("RightForeArm", "RightArm") && linked("LeftShin", "LeftLeg") &&
        linked("RightShin", "RightLeg")) return rig_kind::soma30;
    if (value.names.size() == 52U && has("Hips") && has("Spine") && has("LeftHandThumb1") &&
        has("RightHandPinky3") && has("LeftUpLeg") && has("RightToeBase") &&
        linked("Spine", "Hips") && linked("LeftForeArm", "LeftArm") &&
        linked("RightForeArm", "RightArm") && linked("LeftLeg", "LeftUpLeg") &&
        linked("RightLeg", "RightUpLeg")) return rig_kind::mixamo52;
    return rig_kind::unknown;
}

result<motion> load_skeleton_glb_file(const std::filesystem::path & path) {
    auto doc = read_document(path);
    if (!doc) return tl::unexpected(doc.error());
    return load_skeleton_document(*doc, false);
}

result<motion> load_kimodo_glb_file(const std::filesystem::path & path) {
    auto doc = read_document(path);
    if (!doc) return tl::unexpected(doc.error());
    return load_skeleton_document(*doc, true);
}

result<skinned_asset> load_skinned_glb_file(const std::filesystem::path & path) {
    auto doc = read_document(path);
    if (!doc) return tl::unexpected(doc.error());
    auto geometry = load_glb_file(path);
    if (!geometry) return tl::unexpected(geometry.error());
    auto animation = load_skeleton_document(*doc, false);
    if (!animation) return tl::unexpected(animation.error());

    const auto skins = doc->root.find("skins");
    const auto nodes = doc->root.find("nodes");
    const auto meshes = doc->root.find("meshes");
    if (skins == doc->root.end() || !skins->is_array() || skins->size() != 1U ||
        nodes == doc->root.end() || !nodes->is_array() ||
        meshes == doc->root.end() || !meshes->is_array())
        return tl::unexpected(detail::fail(error_code::invalid_format,
            "rigged GLB must contain exactly one skin and a bounded mesh"));
    const auto & skin_object = (*skins)[0];
    if (!skin_object.is_object() || !skin_object.contains("joints") ||
        !skin_object["joints"].is_array())
        return tl::unexpected(detail::fail(error_code::invalid_format,
            "rigged GLB skin has no joint array"));

    // JOINTS_0 indexes the original skin.joints array. The skeleton loader
    // normalizes arbitrary glTF joint order so parents precede children; build
    // the same order here and explicitly remap every vertex influence.
    std::vector<std::size_t> original_nodes;
    std::vector<std::int32_t> node_to_original(nodes->size(), -1);
    for (const auto & value : skin_object["joints"]) {
        if (!value.is_number_unsigned() || value.get<std::size_t>() >= nodes->size())
            return tl::unexpected(detail::fail(error_code::invalid_format,
                "rigged GLB contains an invalid skin joint"));
        const auto node = value.get<std::size_t>();
        if (node_to_original[node] >= 0)
            return tl::unexpected(detail::fail(error_code::invalid_format,
                "rigged GLB contains duplicate skin joints"));
        node_to_original[node] = static_cast<std::int32_t>(original_nodes.size());
        original_nodes.push_back(node);
    }
    if (original_nodes.size() != animation->rig.names.size())
        return tl::unexpected(detail::fail(error_code::invalid_format,
            "skin and loaded skeleton joint counts differ"));
    std::vector<std::int32_t> node_parents(nodes->size(), -1);
    for (std::size_t parent = 0; parent < nodes->size(); ++parent) {
        const auto children = (*nodes)[parent].find("children");
        if (children == (*nodes)[parent].end()) continue;
        if (!children->is_array())
            return tl::unexpected(detail::fail(error_code::invalid_format,
                "rigged GLB node children are invalid"));
        for (const auto & child : *children) {
            if (!child.is_number_unsigned() || child.get<std::size_t>() >= nodes->size() ||
                node_parents[child.get<std::size_t>()] >= 0)
                return tl::unexpected(detail::fail(error_code::invalid_format,
                    "rigged GLB joint hierarchy is invalid"));
            node_parents[child.get<std::size_t>()] = static_cast<std::int32_t>(parent);
        }
    }
    std::vector<std::size_t> ordered_nodes;
    std::vector<bool> added(nodes->size(), false);
    while (ordered_nodes.size() != original_nodes.size()) {
        const auto before = ordered_nodes.size();
        for (const auto node : original_nodes) {
            if (added[node]) continue;
            auto parent = node_parents[node];
            while (parent >= 0 && node_to_original[static_cast<std::size_t>(parent)] < 0)
                parent = node_parents[static_cast<std::size_t>(parent)];
            if (parent < 0 || added[static_cast<std::size_t>(parent)]) {
                ordered_nodes.push_back(node);
                added[node] = true;
            }
        }
        if (ordered_nodes.size() == before)
            return tl::unexpected(detail::fail(error_code::invalid_format,
                "rigged GLB skin hierarchy contains a cycle"));
    }
    std::vector<std::uint16_t> original_to_ordered(original_nodes.size());
    for (std::size_t ordered = 0; ordered < ordered_nodes.size(); ++ordered)
        original_to_ordered[static_cast<std::size_t>(node_to_original[ordered_nodes[ordered]])] =
            static_cast<std::uint16_t>(ordered);

    const json * primitive = nullptr;
    for (const auto & node : *nodes) {
        if (!node.is_object() || !node.contains("skin") || !node.contains("mesh")) continue;
        if (!node["skin"].is_number_unsigned() || node["skin"].get<std::size_t>() != 0U ||
            !node["mesh"].is_number_unsigned() || node["mesh"].get<std::size_t>() >= meshes->size())
            return tl::unexpected(detail::fail(error_code::invalid_format,
                "rigged GLB mesh node references an invalid skin or mesh"));
        const auto & mesh_object = (*meshes)[node["mesh"].get<std::size_t>()];
        if (!mesh_object.is_object() || !mesh_object.contains("primitives") ||
            !mesh_object["primitives"].is_array())
            return tl::unexpected(detail::fail(error_code::invalid_format,
                "rigged GLB mesh has no primitives"));
        for (const auto & candidate : mesh_object["primitives"]) {
            if (!candidate.is_object() || candidate.value("mode", 4U) != 4U ||
                !candidate.contains("attributes")) continue;
            const auto & attributes = candidate["attributes"];
            if (!attributes.is_object() || !attributes.contains("POSITION") ||
                !attributes.contains("JOINTS_0") || !attributes.contains("WEIGHTS_0")) continue;
            if (primitive != nullptr)
                return tl::unexpected(detail::fail(error_code::invalid_format,
                    "multiple skinned mesh primitives are not supported"));
            primitive = &candidate;
        }
    }
    if (primitive == nullptr)
        return tl::unexpected(detail::fail(error_code::invalid_format,
            "rigged GLB has no JOINTS_0/WEIGHTS_0 triangle primitive"));
    const auto & attributes = (*primitive)["attributes"];
    for (const auto * name : {"POSITION", "JOINTS_0", "WEIGHTS_0"})
        if (!attributes[name].is_number_unsigned())
            return tl::unexpected(detail::fail(error_code::invalid_format,
                std::string{"invalid skinned attribute "} + name));
    auto positions = get_accessor(*doc, attributes["POSITION"].get<std::size_t>());
    auto joints = get_accessor(*doc, attributes["JOINTS_0"].get<std::size_t>());
    auto weights = get_accessor(*doc, attributes["WEIGHTS_0"].get<std::size_t>());
    if (!positions) return tl::unexpected(positions.error());
    if (!joints) return tl::unexpected(joints.error());
    if (!weights) return tl::unexpected(weights.error());
    if (positions->count != geometry->vertices.size() || joints->count != positions->count ||
        weights->count != positions->count)
        return tl::unexpected(detail::fail(error_code::invalid_format,
            "skinned attribute counts differ from mesh vertex count"));

    skinned_asset output;
    output.geometry = std::move(*geometry);
    output.animation = std::move(*animation);
    output.binding.rig = output.animation.rig;
    output.binding.joints.resize(positions->count);
    output.binding.weights.resize(positions->count);
    output.binding.learned = skin_object.value("name", std::string{}).find("learned") != std::string::npos;
    for (std::size_t vertex = 0; vertex < positions->count; ++vertex) {
        auto joint = joint_vector(*joints, vertex);
        auto weight = weight_vector(*weights, vertex);
        if (!joint) return tl::unexpected(joint.error());
        if (!weight) return tl::unexpected(weight.error());
        for (auto & index : *joint) {
            if (index >= original_to_ordered.size())
                return tl::unexpected(detail::fail(error_code::invalid_format,
                    "JOINTS_0 influence exceeds skin joint count"));
            index = original_to_ordered[index];
        }
        output.binding.joints[vertex] = *joint;
        output.binding.weights[vertex] = *weight;
    }
    return output;
}

result<glb_info> inspect_glb_file(const std::filesystem::path & path) {
    auto doc = read_document(path);
    if (!doc) return tl::unexpected(doc.error());
    glb_info output;
    const auto meshes = doc->root.find("meshes");
    const auto skins = doc->root.find("skins");
    output.has_mesh = meshes != doc->root.end() && meshes->is_array() && !meshes->empty();
    output.has_skin = skins != doc->root.end() && skins->is_array() && !skins->empty();
    auto skeleton = load_skeleton_document(*doc, false);
    if (skeleton) {
        output.has_skeleton = true;
        output.joint_count = skeleton->rig.names.size();
        output.frame_count = skeleton->frames;
        output.frames_per_second = skeleton->frames_per_second;
        output.has_animation = skeleton->frames > 1U;
        output.rig = identify_rig(skeleton->rig);
    } else if (output.has_skin) {
        return tl::unexpected(skeleton.error());
    }
    return output;
}

result<void> save_skinned_animation_glb_file(
    const std::filesystem::path & path, const mesh & geometry, const skin & binding,
    const motion & animation) {
    auto valid = validate_export(geometry, binding, animation);
    if (!valid) return tl::unexpected(valid.error());

    glb_builder builder;
    const std::size_t joint_count = binding.rig.names.size();
    const auto position_minmax = [&] {
        vec3 low = geometry.vertices.front();
        vec3 high = geometry.vertices.front();
        for (const auto & value : geometry.vertices) {
            low.x = std::min(low.x, value.x); low.y = std::min(low.y, value.y); low.z = std::min(low.z, value.z);
            high.x = std::max(high.x, value.x); high.y = std::max(high.y, value.y); high.z = std::max(high.z, value.z);
        }
        return json{{"min", {low.x, low.y, low.z}}, {"max", {high.x, high.y, high.z}}};
    }();
    const auto positions = builder.accessor_for<vec3>(geometry.vertices, 5126U, "VEC3", 34962U, position_minmax);
    const auto normals = builder.accessor_for<vec3>(geometry.normals, 5126U, "VEC3", 34962U);
    std::optional<std::size_t> colors;
    if (!geometry.colors.empty())
        colors = builder.accessor_for<color4>(geometry.colors, 5126U, "VEC4", 34962U);
    const auto joints = builder.accessor_for<std::array<std::uint16_t, 4>>(binding.joints, 5123U, "VEC4", 34962U);
    const auto weights = builder.accessor_for<std::array<float, 4>>(binding.weights, 5126U, "VEC4", 34962U);
    std::vector<std::uint32_t> indices;
    indices.reserve(geometry.faces.size() * 3U);
    for (const auto & face : geometry.faces) indices.insert(indices.end(), face.begin(), face.end());
    const auto index_accessor = builder.accessor_for<std::uint32_t>(indices, 5125U, "SCALAR", 34963U);

    std::vector<std::array<float, 16>> inverse_bind(joint_count);
    for (std::size_t joint = 0; joint < joint_count; ++joint) {
        auto & matrix = inverse_bind[joint];
        matrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0,
                  -binding.rig.rest_positions[joint].x,
                  -binding.rig.rest_positions[joint].y,
                  -binding.rig.rest_positions[joint].z, 1};
    }
    const auto inverse_accessor = builder.accessor_for<std::array<float, 16>>(inverse_bind, 5126U, "MAT4");

    std::vector<float> times(animation.frames);
    const float fps = animation.frames_per_second > 0.0F ? animation.frames_per_second : 30.0F;
    for (std::size_t frame = 0; frame < animation.frames; ++frame) times[frame] = static_cast<float>(frame) / fps;
    const auto time_accessor = builder.accessor_for<float>(times, 5126U, "SCALAR", std::nullopt,
        json{{"min", {times.front()}}, {"max", {times.back()}}});

    for (std::size_t joint = 0; joint < joint_count; ++joint) {
        const std::int32_t parent = binding.rig.parents[joint];
        vec3 local = binding.rig.rest_positions[joint];
        if (parent >= 0) local = sub(local, binding.rig.rest_positions[static_cast<std::size_t>(parent)]);
        json node{{"name", binding.rig.names[joint]}, {"translation", {local.x, local.y, local.z}}};
        json children = json::array();
        for (std::size_t child = 0; child < joint_count; ++child)
            if (binding.rig.parents[child] == static_cast<std::int32_t>(joint)) children.push_back(child);
        if (!children.empty()) node["children"] = std::move(children);
        builder.root["nodes"].push_back(std::move(node));
    }
    const std::size_t mesh_node = builder.root["nodes"].size();
    builder.root["nodes"].push_back({{"name", "SkinnedMesh"}, {"mesh", 0U}, {"skin", 0U}});
    const bool transparent = std::any_of(geometry.colors.begin(), geometry.colors.end(),
        [](const color4 & value) { return value.a < 0.999F; });
    json material{{"name", "SkinTokens material"},
        {"pbrMetallicRoughness", {{"baseColorFactor", colors ? json::array({1.0, 1.0, 1.0, 1.0}) :
                                                        json::array({0.55, 0.68, 0.78, 1.0})},
                                  {"metallicFactor", std::clamp(geometry.metallic_factor, 0.0F, 1.0F)},
                                  {"roughnessFactor", std::clamp(geometry.roughness_factor, 0.0F, 1.0F)}}},
        {"doubleSided", geometry.double_sided}};
    if (transparent) material["alphaMode"] = "BLEND";
    builder.root["materials"] = json::array({std::move(material)});
    json attributes{{"POSITION", positions}, {"NORMAL", normals},
                    {"JOINTS_0", joints}, {"WEIGHTS_0", weights}};
    if (colors) attributes["COLOR_0"] = *colors;
    builder.root["meshes"] = json::array({{{"name", "Imported mesh"}, {"primitives", json::array({{
        {"attributes", std::move(attributes)},
        {"indices", index_accessor}, {"material", 0U}, {"mode", 4U}
    }})}}});
    json skin_object{{"name", binding.learned ? "SkinTokens learned binding" : "Geometric baseline binding"},
                     {"inverseBindMatrices", inverse_accessor}, {"skeleton", 0U}, {"joints", json::array()}};
    for (std::size_t joint = 0; joint < joint_count; ++joint) skin_object["joints"].push_back(joint);
    builder.root["skins"] = json::array({std::move(skin_object)});

    json samplers = json::array();
    json channels = json::array();
    std::vector<vec3> root_values = animation.root_translations;
    const auto root_accessor = builder.accessor_for<vec3>(root_values, 5126U, "VEC3");
    samplers.push_back({{"input", time_accessor}, {"output", root_accessor}, {"interpolation", "LINEAR"}});
    channels.push_back({{"sampler", 0U}, {"target", {{"node", 0U}, {"path", "translation"}}}});
    for (std::size_t joint = 0; joint < joint_count; ++joint) {
        std::vector<quat> values(animation.frames);
        for (std::size_t frame = 0; frame < animation.frames; ++frame)
            values[frame] = animation.local_rotations[frame * joint_count + joint];
        const auto output_accessor = builder.accessor_for<quat>(values, 5126U, "VEC4");
        const auto sampler = samplers.size();
        samplers.push_back({{"input", time_accessor}, {"output", output_accessor}, {"interpolation", "LINEAR"}});
        channels.push_back({{"sampler", sampler}, {"target", {{"node", joint}, {"path", "rotation"}}}});
    }
    builder.root["animations"] = json::array({{{"name", "Kimodo motion"},
        {"samplers", std::move(samplers)}, {"channels", std::move(channels)}}});
    builder.root["scenes"] = json::array({{{"nodes", {0U, mesh_node}}}});
    builder.root["scene"] = 0U;
    builder.root["buffers"].push_back({{"byteLength", builder.binary.size()}});

    std::string json_text = builder.root.dump();
    while (json_text.size() % 4U != 0U) json_text.push_back(' ');
    align4(builder.binary);
    const std::uint64_t total64 = 12ULL + 8ULL + json_text.size() + 8ULL + builder.binary.size();
    if (total64 > std::numeric_limits<std::uint32_t>::max())
        return tl::unexpected(detail::fail(error_code::limit_exceeded, "exported GLB exceeds 4 GiB"));
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) return tl::unexpected(detail::fail(error_code::io, "cannot create GLB: " + path.string()));
    const auto write_u32 = [&](std::uint32_t value) {
        stream.write(reinterpret_cast<const char *>(&value), sizeof(value));
    };
    write_u32(glb_magic); write_u32(2U); write_u32(static_cast<std::uint32_t>(total64));
    write_u32(static_cast<std::uint32_t>(json_text.size())); write_u32(json_chunk);
    stream.write(json_text.data(), static_cast<std::streamsize>(json_text.size()));
    write_u32(static_cast<std::uint32_t>(builder.binary.size())); write_u32(bin_chunk);
    stream.write(reinterpret_cast<const char *>(builder.binary.data()), static_cast<std::streamsize>(builder.binary.size()));
    if (!stream) return tl::unexpected(detail::fail(error_code::io, "failed while writing GLB"));
    return {};
}

} // namespace skintokens

#include "internal.hpp"

#include <array>
#include <fstream>
#include <unordered_map>

namespace skintokens::detail {
namespace {

constexpr std::uint32_t gguf_magic = 0x46554747U;
constexpr std::uint64_t max_string = 16U * 1024U * 1024U;
constexpr std::uint64_t max_metadata = 100000U;
constexpr std::uint64_t max_tensors = 1000000U;

template<class T> bool read(std::istream & stream, T & value) {
    return static_cast<bool>(stream.read(reinterpret_cast<char *>(&value), sizeof(value)));
}

bool read_string(std::istream & stream, std::string & output) {
    std::uint64_t size = 0;
    if (!read(stream, size) || size > max_string) return false;
    output.resize(static_cast<std::size_t>(size));
    return size == 0 || static_cast<bool>(stream.read(output.data(), static_cast<std::streamsize>(size)));
}

bool skip_scalar(std::istream & stream, std::uint32_t kind) {
    static constexpr std::array<unsigned, 12> widths{1, 1, 2, 2, 4, 4, 4, 1, 0, 0, 8, 8};
    if (kind >= widths.size() || widths[kind] == 0) return false;
    stream.seekg(static_cast<std::streamoff>(widths[kind]), std::ios::cur);
    return static_cast<bool>(stream);
}

} // namespace

result<bundle_metadata> inspect_gguf(const std::filesystem::path & path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) return tl::unexpected(fail(error_code::io, "cannot open GGUF component: " + path.string()));
    std::uint32_t magic = 0;
    std::uint32_t version = 0;
    std::uint64_t tensors = 0;
    std::uint64_t metadata = 0;
    if (!read(stream, magic) || !read(stream, version) || !read(stream, tensors) || !read(stream, metadata))
        return tl::unexpected(fail(error_code::invalid_format, "truncated GGUF header"));
    if (magic != gguf_magic || version < 2 || version > 3)
        return tl::unexpected(fail(error_code::invalid_format, "unsupported GGUF header"));
    if (tensors > max_tensors || metadata > max_metadata)
        return tl::unexpected(fail(error_code::limit_exceeded, "GGUF directory count exceeds safety limit"));

    bundle_metadata output;
    output.tensor_count = tensors;
    std::unordered_map<std::string, std::string *> strings{
        {"general.architecture", &output.architecture},
        {"skintokens.component", &output.component},
        {"skintokens.upstream_revision", &output.upstream_revision},
        {"skintokens.tokenrig_sha256", &output.tokenrig_sha256},
        {"skintokens.skin_vae_sha256", &output.skin_vae_sha256},
    };
    for (std::uint64_t i = 0; i < metadata; ++i) {
        std::string key;
        std::uint32_t kind = 0;
        if (!read_string(stream, key) || key.empty() || key.size() > 4096U || !read(stream, kind))
            return tl::unexpected(fail(error_code::invalid_format, "invalid GGUF metadata entry"));
        if (kind == 8U) {
            std::string value;
            if (!read_string(stream, value))
                return tl::unexpected(fail(error_code::invalid_format, "truncated GGUF string metadata"));
            if (auto found = strings.find(key); found != strings.end()) *found->second = std::move(value);
        } else if (key == "skintokens.format_version" && kind == 4U) {
            std::uint32_t value = 0;
            if (!read(stream, value))
                return tl::unexpected(fail(error_code::invalid_format, "truncated GGUF version metadata"));
            output.format_version = value;
        } else if (key == "skintokens.format_version" && (kind == 10U || kind == 11U)) {
            if (!read(stream, output.format_version))
                return tl::unexpected(fail(error_code::invalid_format, "truncated GGUF version metadata"));
        } else if (!skip_scalar(stream, kind)) {
            // Arrays are unnecessary in the manifest and deliberately rejected.
            return tl::unexpected(fail(error_code::invalid_format, "unsupported GGUF metadata type"));
        }
    }
    if (output.architecture != "skintokens" || output.format_version != 1U || output.component.empty())
        return tl::unexpected(fail(error_code::incompatible_model, "GGUF is not a compatible SkinTokens component"));
    if (output.upstream_revision.size() != 40U || output.tokenrig_sha256.size() != 64U ||
        output.skin_vae_sha256.size() != 64U)
        return tl::unexpected(fail(error_code::incompatible_model, "GGUF model identity is incomplete"));
    return output;
}

} // namespace skintokens::detail

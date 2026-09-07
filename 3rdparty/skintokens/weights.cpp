#include "internal.hpp"

#include <algorithm>
#include <fstream>

namespace skintokens::detail {

weight_component::~weight_component() {
    if (buffer != nullptr) ggml_backend_buffer_free(buffer);
    if (file != nullptr) gguf_free(file);
    if (context != nullptr) ggml_free(context);
}

ggml_tensor * weight_component::tensor(std::string_view name) const {
    return context == nullptr ? nullptr : ggml_get_tensor(context, std::string{name}.c_str());
}

result<std::unique_ptr<weight_component>> load_component(
    const std::filesystem::path & path, ggml_backend_t backend) {
    auto output = std::make_unique<weight_component>();
    output->path = path;
    gguf_init_params parameters{true, &output->context};
    output->file = gguf_init_from_file(path.string().c_str(), parameters);
    if (output->file == nullptr || output->context == nullptr)
        return tl::unexpected(fail(error_code::invalid_format, "cannot map GGUF tensors: " + path.string()));
    output->buffer = ggml_backend_alloc_ctx_tensors(output->context, backend);
    if (output->buffer == nullptr)
        return tl::unexpected(fail(error_code::allocation, "cannot allocate model component on backend: " + path.string()));

    std::ifstream stream(path, std::ios::binary);
    if (!stream) return tl::unexpected(fail(error_code::io, "cannot reopen GGUF component"));
    const std::size_t data_offset = gguf_get_data_offset(output->file);
    std::vector<char> scratch(8U << 20U);
    for (std::int64_t i = 0; i < gguf_get_n_tensors(output->file); ++i) {
        const char * name = gguf_get_tensor_name(output->file, i);
        auto * tensor = ggml_get_tensor(output->context, name);
        if (tensor == nullptr || (tensor->type != GGML_TYPE_F32 && tensor->type != GGML_TYPE_F16))
            return tl::unexpected(fail(error_code::incompatible_model, "GGUF component has unsupported tensor type"));
        const std::size_t bytes = ggml_nbytes(tensor);
        const std::size_t offset = gguf_get_tensor_offset(output->file, i);
        stream.seekg(static_cast<std::streamoff>(data_offset + offset));
        for (std::size_t done = 0; done < bytes;) {
            const std::size_t amount = std::min(scratch.size(), bytes - done);
            stream.read(scratch.data(), static_cast<std::streamsize>(amount));
            if (!stream) return tl::unexpected(fail(error_code::invalid_format, "GGUF tensor data is truncated"));
            ggml_backend_tensor_set(tensor, scratch.data(), done, amount);
            done += amount;
        }
    }
    return output;
}

} // namespace skintokens::detail

#pragma once

#include <algorithm>
//#include <bit>
//#include <ranges> // IWYU pragma: keep - Needed by MSVC
#include <utility>
#include <type_traits>

namespace std {
#if !__has_cpp_attribute(__cpp_lib_to_underlying)
template <typename E>
constexpr auto to_underlying(E e) noexcept {
    return static_cast<std::underlying_type_t<E>>(e);
}
#endif

#if !__has_cpp_attribute(__cpp_lib_byteswap)
/*template <std::integral T>
constexpr T byteswap(T value) noexcept {
    static_assert(std::has_unique_object_representations_v<T>, "T may not have padding bits");
    auto value_representation = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
    std::ranges::reverse(value_representation);
    return std::bit_cast<T>(value_representation);
}
*/  // WangRui 202602: downgrade to C++17
template <typename T>
constexpr std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, T>
byteswap(T value) noexcept {
    static_assert(std::is_integral_v<T>, "T must be integral");
    T result = 0;
    for (size_t i = 0; i < sizeof(T); ++i) {
        result = (result << 8) | ((value >> (8 * i)) & 0xFF);
    }
    return result;
}

constexpr uint16_t byteswap(uint16_t value) noexcept {
    return (value >> 8) | (value << 8);
}

constexpr uint32_t byteswap(uint32_t value) noexcept {
    return ((value >> 24) & 0xFF) |
        ((value >> 8) & 0xFF00) |
        ((value << 8) & 0xFF0000) |
        ((value << 24) & 0xFF000000);
}

constexpr uint64_t byteswap(uint64_t value) noexcept {
    return ((value >> 56) & 0xFFULL) |
        ((value >> 40) & 0xFF00ULL) |
        ((value >> 24) & 0xFF0000ULL) |
        ((value >> 8) & 0xFF000000ULL) |
        ((value << 8) & 0xFF00000000ULL) |
        ((value << 24) & 0xFF0000000000ULL) |
        ((value << 40) & 0xFF000000000000ULL) |
        ((value << 56) & 0xFF00000000000000ULL);
}
////
#endif
} // namespace std

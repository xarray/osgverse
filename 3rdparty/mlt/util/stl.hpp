#pragma once

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <vector>
#include <type_traits>

namespace mlt::util {

/// Create a vector of N items by invoking the given function N times
/*template <typename T, typename F, typename I = std::size_t>
    requires requires(F f, I i) {
        { f(i) } -> std::same_as<T>;
    }*/  // WangRui 202602: downgrade to C++17
template <typename T, typename F, typename I = std::size_t,
        std::enable_if_t<std::is_invocable_r_v<T, F, I>, int> = 0>
////
std::vector<T> generateVector(const std::size_t count, F generator) {
    std::vector<T> result;
    result.reserve(count);
    std::generate_n(
        std::back_inserter(result), count, [i = I{0}, f = std::move(generator)]() mutable { return f(i++); });
    return result;
}

// Helper for using lambdas with `std::variant`
// See https://en.cppreference.com/w/cpp/utility/variant/visit
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

// explicit deduction guide (not needed as of C++20)
// (but seems to be needed by MSVC)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

} // namespace mlt::util

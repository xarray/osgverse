#pragma once

//#include <bit>
#include <cassert>
#include <cstdint>
#include <numeric>
#include <optional>
#include <vector>

// WangRui 202602: downgrade to C++17
template <typename T>
constexpr std::enable_if_t<std::is_unsigned_v<T>, int>
downgraded_popcount(T x) noexcept {
    int count = 0;
    while (x) { x &= x - 1; ++count; }
    return count;
}
////

namespace mlt {

// `std::vector<bool>` is not great, just use a vector of bytes
using PackedBitset = std::vector<std::uint8_t>;

/// Test a specific bit
static inline bool testBit(const PackedBitset& bitset, std::size_t i) noexcept {
    return ((i / 8) < bitset.size()) && (bitset[i / 8] & (1 << (i % 8)));
}

/// Get the total number of bits set
static inline std::size_t countSetBits(const PackedBitset& bitset) {
    // NOLINTNEXTLINE(boost-use-ranges)
    return std::accumulate(
        bitset.begin(), bitset.end(), static_cast<std::size_t>(0), [](const auto total, const auto byte) {
            return total + static_cast<unsigned>(downgraded_popcount(byte));
        });
}

// WangRui 202602: downgrade to C++17
template <typename T> constexpr std::enable_if_t<std::is_unsigned_v<T>, int>
downgraded_countr_zero(T x) noexcept {
    if (x == 0) return sizeof(T) * 8; int count = 0;
    constexpr int bits = sizeof(T) * 8;

    if constexpr (bits >= 64) {
        if ((x & 0xFFFFFFFFULL) == 0) { count += 32; x >>= 32; }
    }
    if constexpr (bits >= 32) {
        if ((x & 0xFFFFU) == 0) { count += 16; x >>= 16; }
    }
    if ((x & 0xFFU) == 0) { count += 8; x >>= 8; }
    if ((x & 0xFU) == 0) { count += 4; x >>= 4; }
    if ((x & 0x3U) == 0) { count += 2; x >>= 2; }
    if ((x & 0x1U) == 0) { count += 1; }
    return count;
}
////

/// Return the index of the next set bit within the bitstream
/// @param bits The bitset
/// @param afterIndex The bit index to start with
/// @return The index of the next set bit (including the starting index)
static inline std::optional<std::size_t> nextSetBit(const PackedBitset& bits,
                                                    const std::size_t afterIndex = 0) noexcept {
    if (std::size_t byteIndex = (afterIndex / 8); byteIndex < bits.size()) {
        auto byte = bits[byteIndex];

        // If we're mid-byte, shift it down so the next bit is in the 1 position
        std::size_t result = afterIndex;
        if (const auto partialBits = result & 7; partialBits) {
            byte >>= partialBits;
            if (!byte) {
                // skip to the next byte
                if (++byteIndex == bits.size()) {
                    return {};
                }
                result += (8 - partialBits);
                byte = bits[byteIndex];
            }
        }

        while (byteIndex < bits.size()) {
            // If this byte is non-zero, the next bit is within it
            if (byte) {
                const auto ffs = downgraded_countr_zero(byte);
                return result + ffs;
            }
            // Continue to the next byte
            if (++byteIndex < bits.size()) {
                byte = bits[byteIndex];
                result += 8;
            }
        }
    }
    return {};
}

} // namespace mlt

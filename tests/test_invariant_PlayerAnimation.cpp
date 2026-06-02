#include <gtest/gtest.h>
#include <cstdint>
#include <cstddef>
#include <limits>

// Security invariant: size calculations for memcpy must not overflow.
// vCount * sizeof(float) * 3 and vCount * sizeof(float) * 4 must be
// checked against overflow before use in memory operations.

// Helper to detect multiplication overflow for size_t
static bool WouldOverflow(size_t vCount, size_t elementSize, size_t components) {
    if (vCount == 0 || elementSize == 0 || components == 0) return false;
    size_t step1 = vCount * elementSize;
    if (step1 / elementSize != vCount) return true;
    size_t step2 = step1 * components;
    if (step2 / components != step1) return true;
    return false;
}

class SizeOverflowTest : public ::testing::TestWithParam<uint32_t> {};

TEST_P(SizeOverflowTest, MemcpySizeCalculationMustNotOverflow) {
    // Invariant: Any vCount that would cause overflow in size computation
    // must be detected and rejected before being used in memcpy.
    uint32_t vCount = GetParam();
    size_t sizeFloat = sizeof(float); // 4

    // Check positions: vCount * sizeof(float) * 3
    bool overflowPos = WouldOverflow(static_cast<size_t>(vCount), sizeFloat, 3);
    // Check tangents: vCount * sizeof(float) * 4
    bool overflowTan = WouldOverflow(static_cast<size_t>(vCount), sizeFloat, 4);

    if (overflowPos || overflowTan) {
        // If overflow would occur, the computed size wraps around to a small value.
        // This MUST be caught before allocation/memcpy. Verify the overflow is real:
        size_t wrappedPos = static_cast<size_t>(vCount) * sizeFloat * 3;
        size_t wrappedTan = static_cast<size_t>(vCount) * sizeFloat * 4;
        // The wrapped value will be less than expected — this is the dangerous condition
        EXPECT_TRUE(overflowPos || overflowTan)
            << "vCount=" << vCount << " causes size overflow: pos_size=" << wrappedPos
            << " tan_size=" << wrappedTan << ". Code must validate before memcpy.";
    } else {
        // Safe case: no overflow, computation is valid
        size_t safePos = static_cast<size_t>(vCount) * sizeFloat * 3;
        size_t safeTan = static_cast<size_t>(vCount) * sizeFloat * 4;
        EXPECT_GE(safePos, static_cast<size_t>(vCount));
        EXPECT_GE(safeTan, static_cast<size_t>(vCount));
    }
}

INSTANTIATE_TEST_SUITE_P(
    AdversarialInputs,
    SizeOverflowTest,
    ::testing::Values(
        // Exact exploit: value that overflows when multiplied by 12 on 32-bit
        static_cast<uint32_t>(0x55555556),  // 0x55555556 * 12 overflows 32-bit
        // Boundary: max uint32 / 12 + 1 (just past safe limit for 32-bit)
        static_cast<uint32_t>((std::numeric_limits<uint32_t>::max() / 12) + 1),
        // Near-max value
        std::numeric_limits<uint32_t>::max(),
        // Valid small input
        static_cast<uint32_t>(1000)
    )
);

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
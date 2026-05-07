#include "gf/core/errors.h"
#include "gf/core/gauss_ir.h"
#include "gf/core/validate.h"
#include "gf/io/writer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

namespace gf {

namespace {

constexpr int BYTES_PER_SPLAT = 32;
constexpr float SH_C0 = 0.28209479177387814f;

// Write little-endian float32
void WriteFloat32LE(uint8_t *data, float value) {
  const uint32_t bits = *reinterpret_cast<const uint32_t *>(&value);
  data[0] = static_cast<uint8_t>(bits & 0xFF);
  data[1] = static_cast<uint8_t>((bits >> 8) & 0xFF);
  data[2] = static_cast<uint8_t>((bits >> 16) & 0xFF);
  data[3] = static_cast<uint8_t>((bits >> 24) & 0xFF);
}

} // namespace

class SplatWriter : public IGaussWriter {
public:
  Expected<std::vector<uint8_t>> Write(const GaussianCloudIR &ir,
                                       const WriteOptions &options) override {
    const auto err = ValidateBasic(ir, options.strict);
    if (!err.message.empty() && options.strict) {
      return Expected<std::vector<uint8_t>>(err);
    }

    const int32_t N = ir.numPoints;
    if (N == 0) {
      return Expected<std::vector<uint8_t>>(
          MakeError("splat write failed: no points to write"));
    }

    // Check data consistency
    const size_t expectedPositions = static_cast<size_t>(N) * 3;
    const size_t expectedScales = static_cast<size_t>(N) * 3;
    const size_t expectedRotations = static_cast<size_t>(N) * 4;
    const size_t expectedAlphas = static_cast<size_t>(N);
    const size_t expectedColors = static_cast<size_t>(N) * 3;

    if (ir.positions.size() != expectedPositions ||
        ir.scales.size() != expectedScales ||
        ir.rotations.size() != expectedRotations ||
        ir.alphas.size() != expectedAlphas ||
        ir.colors.size() != expectedColors) {
      return Expected<std::vector<uint8_t>>(
          MakeError("splat write failed: inconsistent data sizes"));
    }

    // .splat format only supports SH degree 0
    if (ir.meta.shDegree > 0 && !ir.sh.empty()) {
      // Higher-order SH coefficients are ignored in .splat format
      // This is acceptable, but we could warn in strict mode
    }

    // Prepare output buffer
    std::vector<uint8_t> result(static_cast<size_t>(N) * BYTES_PER_SPLAT);

    // Write data in chunks for better performance
    const size_t chunkSize = 1024;
    const size_t numChunks =
        (static_cast<size_t>(N) + chunkSize - 1) / chunkSize;
    std::vector<uint8_t> chunkData(chunkSize * BYTES_PER_SPLAT);

    for (size_t c = 0; c < numChunks; ++c) {
      const size_t numRows =
          std::min(chunkSize, static_cast<size_t>(N) - c * chunkSize);
      const size_t chunkStartIdx = c * chunkSize;

      // Encode each splat in the chunk
      for (size_t r = 0; r < numRows; ++r) {
        const size_t idx = chunkStartIdx + r;
        const size_t offset = r * BYTES_PER_SPLAT;
        uint8_t *splatBytes = &chunkData[offset];

        // Write position (3 × float32, offset 0-11)
        WriteFloat32LE(&splatBytes[0], ir.positions[idx * 3 + 0]);
        WriteFloat32LE(&splatBytes[4], ir.positions[idx * 3 + 1]);
        WriteFloat32LE(&splatBytes[8], ir.positions[idx * 3 + 2]);

        // Write scale (3 × float32, offset 12-23)
        // Convert from log-scale to linear scale
        const float scaleX = std::exp(ir.scales[idx * 3 + 0]);
        const float scaleY = std::exp(ir.scales[idx * 3 + 1]);
        const float scaleZ = std::exp(ir.scales[idx * 3 + 2]);
        WriteFloat32LE(&splatBytes[12], scaleX);
        WriteFloat32LE(&splatBytes[16], scaleY);
        WriteFloat32LE(&splatBytes[20], scaleZ);

        // Write color and opacity (4 × uint8, offset 24-27)
        // Convert from SH 0-order coefficients to uint8
        // Inverse of: (red / 255.0f - 0.5f) / SH_C0
        const float colorR = ir.colors[idx * 3 + 0];
        const float colorG = ir.colors[idx * 3 + 1];
        const float colorB = ir.colors[idx * 3 + 2];
        const uint8_t red = static_cast<uint8_t>(
            gf::clamp((colorR * SH_C0 + 0.5f) * 255.0f, 0.0f, 255.0f));
        const uint8_t green = static_cast<uint8_t>(
            gf::clamp((colorG * SH_C0 + 0.5f) * 255.0f, 0.0f, 255.0f));
        const uint8_t blue = static_cast<uint8_t>(
            gf::clamp((colorB * SH_C0 + 0.5f) * 255.0f, 0.0f, 255.0f));

        // Convert from pre-sigmoid alpha to uint8
        // Inverse of: log(normalizedOpacity / (1.0f - normalizedOpacity))
        const float alpha = ir.alphas[idx];
        const float sigmoidAlpha = 1.0f / (1.0f + std::exp(-alpha));
        const uint8_t opacity = static_cast<uint8_t>(
            gf::clamp(sigmoidAlpha * 255.0f, 0.0f, 255.0f));

        splatBytes[24] = red;
        splatBytes[25] = green;
        splatBytes[26] = blue;
        splatBytes[27] = opacity;

        // Write rotation quaternion (4 × uint8, offset 28-31)
        // IR format: [w, x, y, z]
        // .splat format: [w, x, y, z] (same order)
        float rotW = ir.rotations[idx * 4 + 0];
        float rotX = ir.rotations[idx * 4 + 1];
        float rotY = ir.rotations[idx * 4 + 2];
        float rotZ = ir.rotations[idx * 4 + 3];

        // Normalize quaternion (should already be normalized, but ensure it)
        const float length =
            std::sqrt(rotX * rotX + rotY * rotY + rotZ * rotZ + rotW * rotW);
        if (length > 1e-8f) {
          const float invLength = 1.0f / length;
          rotW *= invLength;
          rotX *= invLength;
          rotY *= invLength;
          rotZ *= invLength;
        } else {
          // Default to identity quaternion if invalid
          rotW = 1.0f;
          rotX = 0.0f;
          rotY = 0.0f;
          rotZ = 0.0f;
        }

        // Convert from float [-1,1] to uint8 [0,255]
        // Match supersplat-main: value * 128 + 128
        splatBytes[28] = static_cast<uint8_t>(
            gf::clamp(std::round(rotW * 128.0f + 128.0f), 0.0f, 255.0f));
        splatBytes[29] = static_cast<uint8_t>(
            gf::clamp(std::round(rotX * 128.0f + 128.0f), 0.0f, 255.0f));
        splatBytes[30] = static_cast<uint8_t>(
            gf::clamp(std::round(rotY * 128.0f + 128.0f), 0.0f, 255.0f));
        splatBytes[31] = static_cast<uint8_t>(
            gf::clamp(std::round(rotZ * 128.0f + 128.0f), 0.0f, 255.0f));
      }

      // Copy chunk to result buffer
      const size_t bytesToWrite = numRows * BYTES_PER_SPLAT;
      const size_t resultOffset = chunkStartIdx * BYTES_PER_SPLAT;
      std::memcpy(&result[resultOffset], chunkData.data(), bytesToWrite);
    }

    return Expected<std::vector<uint8_t>>(std::move(result));
  }
};

std::unique_ptr<IGaussWriter> MakeSplatWriter() {
  return std::make_unique<SplatWriter>();
}

} // namespace gf

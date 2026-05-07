#include "gf/io/reader.h"
#include "gf/core/errors.h"
#include "gf/core/gauss_ir.h"
#include "gf/core/validate.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace gf {

namespace {

constexpr int BYTES_PER_SPLAT = 32;
constexpr float SH_C0 = 0.28209479177387814f;
constexpr float MAX_LOGIT = 10.0f;

// Read little-endian float32
float ReadFloat32LE(const uint8_t *data) {
  uint32_t bits = static_cast<uint32_t>(data[0]) |
                  (static_cast<uint32_t>(data[1]) << 8) |
                  (static_cast<uint32_t>(data[2]) << 16) |
                  (static_cast<uint32_t>(data[3]) << 24);
  return *reinterpret_cast<const float *>(&bits);
}

} // namespace

class SplatReader : public IGaussReader {
public:
  Expected<GaussianCloudIR> Read(const uint8_t *data, size_t size,
                                 const ReadOptions &options) override {
    try {
      if (data == nullptr || size == 0) {
        return Expected<GaussianCloudIR>(
            MakeError("splat read failed: empty input"));
      }

      // Check if file size is a multiple of 32 bytes
      if (size % BYTES_PER_SPLAT != 0) {
        return Expected<GaussianCloudIR>(
            MakeError("splat read failed: file size is not a multiple of 32 "
                      "bytes"));
      }

      const size_t numSplats = size / BYTES_PER_SPLAT;
      if (numSplats == 0) {
        return Expected<GaussianCloudIR>(
            MakeError("splat read failed: file is empty"));
      }

      // Initialize IR
      GaussianCloudIR ir;
      ir.numPoints = static_cast<int32_t>(numSplats);
      ir.meta.shDegree = 0; // .splat format doesn't have higher-order SH
      ir.meta.sourceFormat = "splat";

      // Pre-allocate all vectors to avoid reallocation overhead
      // This helps compiler optimize memory access patterns
      ir.positions.resize(numSplats * 3);
      ir.scales.resize(numSplats * 3);
      ir.rotations.resize(numSplats * 4);
      ir.alphas.resize(numSplats);
      ir.colors.resize(numSplats * 3);

      // Use pointers for direct writes to enable better compiler optimization
      // __restrict__ hints compiler that pointers don't alias
      float *__restrict__ posPtr = ir.positions.data();
      float *__restrict__ scalePtr = ir.scales.data();
      float *__restrict__ rotPtr = ir.rotations.data();
      float *__restrict__ alphaPtr = ir.alphas.data();
      float *__restrict__ colorPtr = ir.colors.data();

      // Cache constants for better register allocation
      const float inv255 = 1.0f / 255.0f;
      const float inv128 = 1.0f / 128.0f;
      const float neg128 = -128.0f;
      const float invShC0 = 1.0f / SH_C0;
      const float float255 = 255.0f;

      // Optimized loop: direct writes with pointer arithmetic
      // Compiler can auto-vectorize this pattern easily
      for (size_t i = 0; i < numSplats; ++i) {
        const size_t offset = i * BYTES_PER_SPLAT;
        const uint8_t *splatBytes = &data[offset];

        // Read position (3 × float32, offset 0-11)
        posPtr[0] = ReadFloat32LE(&splatBytes[0]);
        posPtr[1] = ReadFloat32LE(&splatBytes[4]);
        posPtr[2] = ReadFloat32LE(&splatBytes[8]);
        posPtr += 3;

        // Read scale (3 × float32, offset 12-23)
        // Convert from linear to log scale - use std::max for comparison
        const float scaleX = ReadFloat32LE(&splatBytes[12]);
        const float scaleY = ReadFloat32LE(&splatBytes[16]);
        const float scaleZ = ReadFloat32LE(&splatBytes[20]);
        scalePtr[0] = (scaleX > 0.0f) ? std::log(scaleX) : -10.0f;
        scalePtr[1] = (scaleY > 0.0f) ? std::log(scaleY) : -10.0f;
        scalePtr[2] = (scaleZ > 0.0f) ? std::log(scaleZ) : -10.0f;
        scalePtr += 3;

        // Read color and opacity (4 × uint8, offset 24-27)
        // Convert from uint8 back to spherical harmonics
        colorPtr[0] =
            (static_cast<float>(splatBytes[24]) * inv255 - 0.5f) * invShC0;
        colorPtr[1] =
            (static_cast<float>(splatBytes[25]) * inv255 - 0.5f) * invShC0;
        colorPtr[2] =
            (static_cast<float>(splatBytes[26]) * inv255 - 0.5f) * invShC0;
        colorPtr += 3;

        // Store opacity (convert from uint8 to float and apply inverse sigmoid)
        // Match supersplat-main formula but clamp to finite range
        const uint8_t opacity = splatBytes[27];
        if (opacity == 0) {
          *alphaPtr++ = -MAX_LOGIT;
        } else if (opacity == 255) {
          *alphaPtr++ = MAX_LOGIT;
        } else {
          const float alpha =
              -std::log(float255 / static_cast<float>(opacity) - 1.0f);
          *alphaPtr++ = gf::clamp(alpha, -MAX_LOGIT, MAX_LOGIT);
        }

        // Read rotation quaternion (4 × uint8, offset 28-31)
        // Convert from uint8 [0,255] to float [-1,1] and normalize
        // .splat format stores quaternion as [w, x, y, z] (same as IR format)
        const float rotW =
            (static_cast<float>(splatBytes[28]) + neg128) * inv128;
        const float rotX =
            (static_cast<float>(splatBytes[29]) + neg128) * inv128;
        const float rotY =
            (static_cast<float>(splatBytes[30]) + neg128) * inv128;
        const float rotZ =
            (static_cast<float>(splatBytes[31]) + neg128) * inv128;

        // Normalize quaternion - compute squared length for better performance
        const float lenSq =
            rotX * rotX + rotY * rotY + rotZ * rotZ + rotW * rotW;
        if (lenSq > 0.0f) {
          const float invLength = 1.0f / std::sqrt(lenSq);
          // Store in IR format as [w, x, y, z] - contiguous writes enable SIMD
          rotPtr[0] = rotW * invLength; // w
          rotPtr[1] = rotX * invLength; // x
          rotPtr[2] = rotY * invLength; // y
          rotPtr[3] = rotZ * invLength; // z
        } else {
          // Default to identity quaternion if invalid
          rotPtr[0] = 1.0f; // w
          rotPtr[1] = 0.0f; // x
          rotPtr[2] = 0.0f; // y
          rotPtr[3] = 0.0f; // z
        }
        rotPtr += 4;
      }

      const auto err = ValidateBasic(ir, options.strict);
      if (!err.message.empty() && options.strict) {
        return Expected<GaussianCloudIR>(err);
      }

      return Expected<GaussianCloudIR>(std::move(ir));
    } catch (const std::exception &e) {
      return Expected<GaussianCloudIR>(
          MakeError(std::string("splat read failed: ") + e.what()));
    }
  }
};

std::unique_ptr<IGaussReader> MakeSplatReader() {
  return std::make_unique<SplatReader>();
}

} // namespace gf

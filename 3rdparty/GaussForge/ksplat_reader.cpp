#include "gf/io/ksplat.h"
#include "gf/io/reader.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

#include "gf/core/errors.h"
#include "gf/core/gauss_ir.h"
#include "gf/core/validate.h"

namespace gf {

namespace {

constexpr size_t MAIN_HEADER_SIZE = 4096;
constexpr size_t SECTION_HEADER_SIZE = 1024;
constexpr float SH_C0 = 0.28209479177387814f;

// Compression mode configuration
struct CompressionConfig {
  size_t centerBytes;
  size_t scaleBytes;
  size_t rotationBytes;
  size_t colorBytes;
  size_t harmonicsBytes;
  size_t scaleStartByte;
  size_t rotationStartByte;
  size_t colorStartByte;
  size_t harmonicsStartByte;
  uint32_t scaleQuantRange;
};

const CompressionConfig COMPRESSION_MODES[] = {
    {12, 12, 16, 4, 4, 12, 24, 40, 44, 1},
    {6, 6, 8, 4, 2, 6, 12, 20, 24, 32767},
    {6, 6, 8, 4, 1, 6, 12, 20, 24, 32767}};

const int HARMONICS_COMPONENT_COUNT[] = {0, 9, 24, 45};

// Read little-endian uint16
uint16_t ReadUint16LE(const uint8_t *data) {
  return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
}

// Read little-endian uint32
uint32_t ReadUint32LE(const uint8_t *data) {
  return static_cast<uint32_t>(data[0]) |
         (static_cast<uint32_t>(data[1]) << 8) |
         (static_cast<uint32_t>(data[2]) << 16) |
         (static_cast<uint32_t>(data[3]) << 24);
}

// Read little-endian float32
float ReadFloat32LE(const uint8_t *data) {
  uint32_t bits = ReadUint32LE(data);
  return *reinterpret_cast<const float *>(&bits);
}

// Decode half-precision floating point (float16)
float DecodeFloat16(uint16_t encoded) {
  const uint16_t signBit = (encoded >> 15) & 1;
  const uint16_t exponent = (encoded >> 10) & 0x1f;
  const uint16_t mantissa = encoded & 0x3ff;

  if (exponent == 0) {
    if (mantissa == 0) {
      return signBit ? -0.0f : 0.0f;
    }
    // Denormalized number
    uint16_t m = mantissa;
    int exp = -14;
    while (!(m & 0x400)) {
      m <<= 1;
      exp--;
    }
    m &= 0x3ff;
    const uint32_t finalExp = static_cast<uint32_t>(exp + 127);
    const uint32_t finalMantissa = static_cast<uint32_t>(m) << 13;
    const uint32_t bits = (static_cast<uint32_t>(signBit) << 31) |
                          (finalExp << 23) | finalMantissa;
    return *reinterpret_cast<const float *>(&bits);
  }

  if (exponent == 0x1f) {
    return mantissa == 0 ? (signBit ? -std::numeric_limits<float>::infinity()
                                    : std::numeric_limits<float>::infinity())
                         : std::numeric_limits<float>::quiet_NaN();
  }

  const uint32_t finalExp = static_cast<uint32_t>(exponent - 15 + 127);
  const uint32_t finalMantissa = static_cast<uint32_t>(mantissa) << 13;
  const uint32_t bits =
      (static_cast<uint32_t>(signBit) << 31) | (finalExp << 23) | finalMantissa;
  return *reinterpret_cast<const float *>(&bits);
}

// Unpack normalized value from N bits
inline float UnpackUnorm(uint32_t value, int bits) {
  const uint32_t mask = (1u << bits) - 1;
  const uint32_t extracted = value & mask;
  return static_cast<float>(extracted) / static_cast<float>(mask);
}

// Unpack 32-bit packed quaternion (smallest-three encoding: 2-10-10-10 bits)
// Format: [largest_index(2 bits)][comp1(10 bits)][comp2(10 bits)][comp3(10
// bits)]
struct Quat {
  float x, y, z, w;
};

Quat UnpackRot32(uint32_t packed) {
  const float norm = 1.0f / (std::sqrt(2.0f) * 0.5f);
  const uint32_t which = (packed >> 30) & 0x3; // 0:w, 1:x, 2:y, 3:z

  // Extract the three stored components
  const float a = (UnpackUnorm(packed >> 20, 10) - 0.5f) * norm;
  const float b = (UnpackUnorm(packed >> 10, 10) - 0.5f) * norm;
  const float c = (UnpackUnorm(packed, 10) - 0.5f) * norm;

  // Reconstruct the largest component from normalization constraint
  const float m = std::sqrt(std::max(0.0f, 1.0f - (a * a + b * b + c * c)));

  Quat result;
  switch (which) {
  case 0: // w is largest, stored components are x, y, z
    result.w = m;
    result.x = a;
    result.y = b;
    result.z = c;
    break;
  case 1: // x is largest, stored components are w, y, z
    result.x = m;
    result.w = a;
    result.y = b;
    result.z = c;
    break;
  case 2: // y is largest, stored components are w, x, z
    result.y = m;
    result.w = a;
    result.x = b;
    result.z = c;
    break;
  case 3: // z is largest, stored components are w, x, y
    result.z = m;
    result.w = a;
    result.x = b;
    result.y = c;
    break;
  }
  return result;
}

} // namespace

class KsplatReader : public IGaussReader {
public:
  Expected<GaussianCloudIR> Read(const uint8_t *data, size_t size,
                                 const ReadOptions &options) override {
    try {
      if (data == nullptr || size == 0) {
        return Expected<GaussianCloudIR>(
            MakeError("ksplat read failed: empty input"));
      }

      if (size < MAIN_HEADER_SIZE) {
        return Expected<GaussianCloudIR>(
            MakeError("ksplat read failed: file too small to be valid .ksplat "
                      "format"));
      }

      // Parse main header
      const uint8_t majorVersion = data[0];
      const uint8_t minorVersion = data[1];
      if (majorVersion != 0 || minorVersion < 1) {
        return Expected<GaussianCloudIR>(MakeError(
            "ksplat read failed: unsupported version " +
            std::to_string(majorVersion) + "." + std::to_string(minorVersion)));
      }

      const uint32_t maxSections = ReadUint32LE(&data[4]);
      const uint32_t numSplats = ReadUint32LE(&data[16]);
      const uint16_t compressionMode = ReadUint16LE(&data[20]);

      if (compressionMode > 2) {
        return Expected<GaussianCloudIR>(
            MakeError("ksplat read failed: invalid compression mode " +
                      std::to_string(compressionMode)));
      }

      float minHarmonicsValue = ReadFloat32LE(&data[36]);
      float maxHarmonicsValue = ReadFloat32LE(&data[40]);
      if (minHarmonicsValue == 0.0f && maxHarmonicsValue == 0.0f) {
        minHarmonicsValue = -1.5f;
        maxHarmonicsValue = 1.5f;
      }

      if (numSplats == 0) {
        return Expected<GaussianCloudIR>(
            MakeError("ksplat read failed: file is empty"));
      }

      const CompressionConfig &config = COMPRESSION_MODES[compressionMode];

      // First pass: scan all sections to find maximum harmonics degree
      int maxHarmonicsDegree = 0;
      for (uint32_t sectionIdx = 0; sectionIdx < maxSections; sectionIdx++) {
        const size_t sectionHeaderOffset =
            MAIN_HEADER_SIZE + sectionIdx * SECTION_HEADER_SIZE;
        if (sectionHeaderOffset + SECTION_HEADER_SIZE > size) {
          return Expected<GaussianCloudIR>(
              MakeError("ksplat read failed: insufficient data for section "
                        "header"));
        }

        const uint32_t sectionSplatCount =
            ReadUint32LE(&data[sectionHeaderOffset]);
        if (sectionSplatCount == 0)
          continue; // Skip empty sections

        const uint16_t harmonicsDegree =
            ReadUint16LE(&data[sectionHeaderOffset + 40]);
        maxHarmonicsDegree =
            std::max(maxHarmonicsDegree, static_cast<int>(harmonicsDegree));
      }

      // Initialize IR
      GaussianCloudIR ir;
      ir.numPoints = static_cast<int32_t>(numSplats);
      ir.meta.shDegree = maxHarmonicsDegree;
      ir.meta.sourceFormat = "ksplat";

      const int maxHarmonicsComponentCount =
          HARMONICS_COMPONENT_COUNT[maxHarmonicsDegree];
      const int shCoeffsPerPoint = ShCoeffsPerPoint(maxHarmonicsDegree);

      // Pre-allocate all vectors to avoid reallocation overhead
      // This helps compiler optimize memory access patterns
      ir.positions.resize(numSplats * 3);
      ir.scales.resize(numSplats * 3);
      ir.rotations.resize(numSplats * 4);
      ir.alphas.resize(numSplats);
      ir.colors.resize(numSplats * 3);
      ir.sh.resize(numSplats * shCoeffsPerPoint);

      // Use pointers for direct writes to enable better compiler optimization
      // __restrict__ hints compiler that pointers don't alias
      float *__restrict__ posPtr = ir.positions.data();
      float *__restrict__ scalePtr = ir.scales.data();
      float *__restrict__ rotPtr = ir.rotations.data();
      float *__restrict__ alphaPtr = ir.alphas.data();
      float *__restrict__ colorPtr = ir.colors.data();
      float *__restrict__ shPtr = ir.sh.data();

      // Initialize all SH coefficients to 0 in one bulk operation
      std::fill(shPtr, shPtr + numSplats * shCoeffsPerPoint, 0.0f);

      size_t currentSectionDataOffset =
          MAIN_HEADER_SIZE + maxSections * SECTION_HEADER_SIZE;
      size_t splatIndex = 0;

      // Process each section
      for (uint32_t sectionIdx = 0; sectionIdx < maxSections; sectionIdx++) {
        const size_t sectionHeaderOffset =
            MAIN_HEADER_SIZE + sectionIdx * SECTION_HEADER_SIZE;
        if (sectionHeaderOffset + SECTION_HEADER_SIZE > size) {
          return Expected<GaussianCloudIR>(
              MakeError("ksplat read failed: insufficient data for section "
                        "header"));
        }

        const uint32_t sectionSplatCount =
            ReadUint32LE(&data[sectionHeaderOffset]);
        const uint32_t maxSectionSplats =
            ReadUint32LE(&data[sectionHeaderOffset + 4]);
        const uint32_t bucketCapacity =
            ReadUint32LE(&data[sectionHeaderOffset + 8]);
        const uint32_t bucketCount =
            ReadUint32LE(&data[sectionHeaderOffset + 12]);
        const float spatialBlockSize =
            ReadFloat32LE(&data[sectionHeaderOffset + 16]);
        const uint16_t bucketStorageSize =
            ReadUint16LE(&data[sectionHeaderOffset + 20]);
        uint32_t quantizationRange =
            ReadUint32LE(&data[sectionHeaderOffset + 24]);
        if (quantizationRange == 0) {
          quantizationRange = config.scaleQuantRange;
        }
        const uint32_t fullBuckets =
            ReadUint32LE(&data[sectionHeaderOffset + 32]);
        const uint32_t partialBuckets =
            ReadUint32LE(&data[sectionHeaderOffset + 36]);
        const uint16_t harmonicsDegree =
            ReadUint16LE(&data[sectionHeaderOffset + 40]);

        // Calculate layout
        const uint32_t fullBucketSplats = fullBuckets * bucketCapacity;
        const size_t partialBucketMetaSize = partialBuckets * 4;
        const size_t totalBucketStorageSize =
            bucketStorageSize * bucketCount + partialBucketMetaSize;
        const int harmonicsComponentCount =
            HARMONICS_COMPONENT_COUNT[harmonicsDegree];
        // Calculate bytesPerSplat with 4-byte alignment padding
        const size_t rawBytesPerSplat =
            config.centerBytes + config.scaleBytes + config.rotationBytes +
            config.colorBytes + harmonicsComponentCount * config.harmonicsBytes;
        // Align to 4-byte boundary (padding)
        const size_t bytesPerSplat = (rawBytesPerSplat + 3) & ~3;
        const size_t sectionDataSize = bytesPerSplat * maxSectionSplats;

        if (currentSectionDataOffset + totalBucketStorageSize +
                sectionDataSize >
            size) {
          return Expected<GaussianCloudIR>(
              MakeError("ksplat read failed: insufficient data for section"));
        }

        // Calculate decompression parameters
        const float positionScale =
            spatialBlockSize / 2.0f / static_cast<float>(quantizationRange);

        // Get bucket centers
        const size_t bucketCentersOffset =
            currentSectionDataOffset + partialBucketMetaSize;
        if (bucketCentersOffset + bucketCount * 3 * sizeof(float) > size) {
          return Expected<GaussianCloudIR>(
              MakeError("ksplat read failed: insufficient data for bucket "
                        "centers"));
        }
        const float *bucketCenters =
            reinterpret_cast<const float *>(&data[bucketCentersOffset]);

        // Get partial bucket sizes
        const uint32_t *partialBucketSizes =
            reinterpret_cast<const uint32_t *>(&data[currentSectionDataOffset]);

        // Get splat data
        const size_t splatDataOffset =
            currentSectionDataOffset + totalBucketStorageSize;
        if (splatDataOffset + sectionDataSize > size) {
          return Expected<GaussianCloudIR>(MakeError(
              "ksplat read failed: insufficient data for splat data"));
        }
        const uint8_t *splatData = &data[splatDataOffset];

        // Track partial bucket processing
        uint32_t currentPartialBucket = fullBuckets;
        size_t currentPartialBase = fullBucketSplats;

        // Process splats in this section
        for (uint32_t splatIdx = 0; splatIdx < sectionSplatCount; splatIdx++) {
          const size_t splatByteOffset = splatIdx * bytesPerSplat;

          // Determine which bucket this splat belongs to
          uint32_t bucketIdx;
          if (splatIdx < fullBucketSplats) {
            bucketIdx = splatIdx / bucketCapacity;
          } else {
            const uint32_t currentBucketSize =
                partialBucketSizes[currentPartialBucket - fullBuckets];
            if (splatIdx >= currentPartialBase + currentBucketSize) {
              currentPartialBucket++;
              currentPartialBase += currentBucketSize;
            }
            bucketIdx = currentPartialBucket;
          }

          // Decode position
          float x, y, z;
          if (compressionMode == 0) {
            x = ReadFloat32LE(&splatData[splatByteOffset]);
            y = ReadFloat32LE(&splatData[splatByteOffset + 4]);
            z = ReadFloat32LE(&splatData[splatByteOffset + 8]);
          } else {
            const int16_t xQuant =
                static_cast<int16_t>(ReadUint16LE(&splatData[splatByteOffset]));
            const int16_t yQuant = static_cast<int16_t>(
                ReadUint16LE(&splatData[splatByteOffset + 2]));
            const int16_t zQuant = static_cast<int16_t>(
                ReadUint16LE(&splatData[splatByteOffset + 4]));
            x = (static_cast<float>(xQuant) -
                 static_cast<float>(quantizationRange)) *
                    positionScale +
                bucketCenters[bucketIdx * 3];
            y = (static_cast<float>(yQuant) -
                 static_cast<float>(quantizationRange)) *
                    positionScale +
                bucketCenters[bucketIdx * 3 + 1];
            z = (static_cast<float>(zQuant) -
                 static_cast<float>(quantizationRange)) *
                    positionScale +
                bucketCenters[bucketIdx * 3 + 2];
          }

          // Decode scales
          float scaleX, scaleY, scaleZ;
          if (compressionMode == 0) {
            scaleX = ReadFloat32LE(
                &splatData[splatByteOffset + config.scaleStartByte]);
            scaleY = ReadFloat32LE(
                &splatData[splatByteOffset + config.scaleStartByte + 4]);
            scaleZ = ReadFloat32LE(
                &splatData[splatByteOffset + config.scaleStartByte + 8]);
          } else {
            scaleX = DecodeFloat16(ReadUint16LE(
                &splatData[splatByteOffset + config.scaleStartByte]));
            scaleY = DecodeFloat16(ReadUint16LE(
                &splatData[splatByteOffset + config.scaleStartByte + 2]));
            scaleZ = DecodeFloat16(ReadUint16LE(
                &splatData[splatByteOffset + config.scaleStartByte + 4]));
          }

          // Decode rotation quaternion
          float rot0, rot1, rot2, rot3;
          if (config.rotationBytes == 4) {
            // 32-bit packed quaternion (smallest-three encoding)
            const uint32_t packed = ReadUint32LE(
                &splatData[splatByteOffset + config.rotationStartByte]);
            const Quat q = UnpackRot32(packed);
            rot0 = q.w; // IR format: [w, x, y, z]
            rot1 = q.x;
            rot2 = q.y;
            rot3 = q.z;
          } else if (compressionMode == 0) {
            // 4 × float32 (16 bytes)
            rot0 = ReadFloat32LE(
                &splatData[splatByteOffset + config.rotationStartByte]);
            rot1 = ReadFloat32LE(
                &splatData[splatByteOffset + config.rotationStartByte + 4]);
            rot2 = ReadFloat32LE(
                &splatData[splatByteOffset + config.rotationStartByte + 8]);
            rot3 = ReadFloat32LE(
                &splatData[splatByteOffset + config.rotationStartByte + 12]);
          } else {
            // 4 × float16 (8 bytes)
            rot0 = DecodeFloat16(ReadUint16LE(
                &splatData[splatByteOffset + config.rotationStartByte]));
            rot1 = DecodeFloat16(ReadUint16LE(
                &splatData[splatByteOffset + config.rotationStartByte + 2]));
            rot2 = DecodeFloat16(ReadUint16LE(
                &splatData[splatByteOffset + config.rotationStartByte + 4]));
            rot3 = DecodeFloat16(ReadUint16LE(
                &splatData[splatByteOffset + config.rotationStartByte + 6]));
          }

          // Decode color and opacity
          const uint8_t red =
              splatData[splatByteOffset + config.colorStartByte];
          const uint8_t green =
              splatData[splatByteOffset + config.colorStartByte + 1];
          const uint8_t blue =
              splatData[splatByteOffset + config.colorStartByte + 2];
          const uint8_t opacity =
              splatData[splatByteOffset + config.colorStartByte + 3];

          // Store position (3 floats) - contiguous writes enable SIMD
          posPtr[0] = x;
          posPtr[1] = y;
          posPtr[2] = z;
          posPtr += 3;

          // Store scale (convert from linear to log scale, 3 floats)
          scalePtr[0] = scaleX > 0.0f ? std::log(scaleX) : -10.0f;
          scalePtr[1] = scaleY > 0.0f ? std::log(scaleY) : -10.0f;
          scalePtr[2] = scaleZ > 0.0f ? std::log(scaleZ) : -10.0f;
          scalePtr += 3;

          // Store color (convert from uint8 back to spherical harmonics, 3
          // floats)
          colorPtr[0] = (red / 255.0f - 0.5f) / SH_C0;
          colorPtr[1] = (green / 255.0f - 0.5f) / SH_C0;
          colorPtr[2] = (blue / 255.0f - 0.5f) / SH_C0;
          colorPtr += 3;

          // Store opacity (convert from uint8 to float and apply inverse
          // sigmoid)
          const float epsilon = 1e-6f;
          const float normalizedOpacity =
              std::max(epsilon, std::min(1.0f - epsilon, opacity / 255.0f));
          *alphaPtr++ =
              std::log(normalizedOpacity / (1.0f - normalizedOpacity));

          // Store quaternion (IR format stores as [w, x, y, z], 4 floats)
          rotPtr[0] = rot0; // w
          rotPtr[1] = rot1; // x
          rotPtr[2] = rot2; // y
          rotPtr[3] = rot3; // z
          rotPtr += 4;

          // Decode and store spherical harmonics
          // ksplat format stores SH as: R_band0, G_band0, B_band0, R_band1,
          // G_band1, B_band1, ... IR format stores as: coeff1_R, coeff1_G,
          // coeff1_B, coeff2_R, coeff2_G, coeff2_B, ... where coefficients are
          // ordered by band: band1, band2, band3
          // Optimized: direct pointer writes to pre-initialized zero array
          const int coeffsPerChannel = harmonicsComponentCount / 3;

          for (int i = 0; i < harmonicsComponentCount; i++) {
            float harmonicsValue;
            if (compressionMode == 0) {
              harmonicsValue =
                  ReadFloat32LE(&splatData[splatByteOffset +
                                           config.harmonicsStartByte + i * 4]);
            } else if (compressionMode == 1) {
              harmonicsValue = DecodeFloat16(
                  ReadUint16LE(&splatData[splatByteOffset +
                                          config.harmonicsStartByte + i * 2]));
            } else {
              const float normalized =
                  splatData[splatByteOffset + config.harmonicsStartByte + i] /
                  255.0f;
              harmonicsValue =
                  minHarmonicsValue +
                  normalized * (maxHarmonicsValue - minHarmonicsValue);
            }

            // Map from ksplat layout to IR layout
            // ksplat format: Channel-First layout [R1...R15, G1...G15,
            // B1...B15] IR format: Coefficient-First layout [coeff1_RGB,
            // coeff2_RGB, ...]
            const int channel = i / coeffsPerChannel; // R=0, G=1, B=2
            const int coeffInChannel =
                i % coeffsPerChannel; // coefficient index within channel

            // IR format: coeff_R, coeff_G, coeff_B for each coefficient
            // coeffInChannel directly maps to IR coefficient index (0-based)
            // Direct write using pointer arithmetic for better optimization
            float *__restrict__ shTargetPtr =
                shPtr + coeffInChannel * 3 + channel;
            *shTargetPtr = harmonicsValue;
          }

          shPtr += shCoeffsPerPoint;

          splatIndex++;
        }

        currentSectionDataOffset += sectionDataSize + totalBucketStorageSize;
      }

      if (splatIndex != numSplats) {
        return Expected<GaussianCloudIR>(
            MakeError("ksplat read failed: splat count mismatch, expected " +
                      std::to_string(numSplats) + ", processed " +
                      std::to_string(splatIndex)));
      }

      const auto err = ValidateBasic(ir, options.strict);
      if (!err.message.empty() && options.strict) {
        return Expected<GaussianCloudIR>(err);
      }

      return Expected<GaussianCloudIR>(std::move(ir));
    } catch (const std::exception &e) {
      return Expected<GaussianCloudIR>(
          MakeError(std::string("ksplat read failed: ") + e.what()));
    }
  }
};

std::unique_ptr<IGaussReader> MakeKsplatReader() {
  return std::make_unique<KsplatReader>();
}

} // namespace gf

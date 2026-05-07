#include "gf/core/errors.h"
#include "gf/core/gauss_ir.h"
#include "gf/core/validate.h"
#include "gf/io/ksplat.h"
#include "gf/io/writer.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace gf {

namespace {

constexpr size_t MAIN_HEADER_SIZE = 4096;
constexpr size_t SECTION_HEADER_SIZE = 1024;
constexpr float SH_C0 = 0.28209479177387814f;

// Compression mode configuration (using mode 0: full precision)
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
};

const CompressionConfig COMPRESSION_MODE_0 = {12, 12, 16, 4, 4, 12, 24, 40, 44};

const int HARMONICS_COMPONENT_COUNT[] = {0, 9, 24, 45};

// Write little-endian uint8
void WriteUint8(uint8_t *data, uint8_t value) { data[0] = value; }

// Write little-endian uint16
void WriteUint16LE(uint8_t *data, uint16_t value) {
  data[0] = static_cast<uint8_t>(value & 0xFF);
  data[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

// Write little-endian uint32
void WriteUint32LE(uint8_t *data, uint32_t value) {
  data[0] = static_cast<uint8_t>(value & 0xFF);
  data[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
  data[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
  data[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

// Write little-endian float32
void WriteFloat32LE(uint8_t *data, float value) {
  const uint32_t bits = *reinterpret_cast<const uint32_t *>(&value);
  WriteUint32LE(data, bits);
}

} // namespace

class KsplatWriter : public IGaussWriter {
public:
  Expected<std::vector<uint8_t>> Write(const GaussianCloudIR &ir,
                                       const WriteOptions &options) override {
    try {
      const auto err = ValidateBasic(ir, options.strict);
      if (!err.message.empty() && options.strict) {
        return Expected<std::vector<uint8_t>>(err);
      }

      const int32_t N = ir.numPoints;
      if (N == 0) {
        return Expected<std::vector<uint8_t>>(
            MakeError("ksplat write failed: no points to write"));
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
            MakeError("ksplat write failed: inconsistent data sizes"));
      }

      // Use compression mode 0 (full precision) and single section
      const uint16_t compressionMode = 0;
      const uint32_t maxSections = 1;
      const int harmonicsDegree = ir.meta.shDegree;
      const int harmonicsComponentCount =
          HARMONICS_COMPONENT_COUNT[harmonicsDegree];
      const int shCoeffsPerPoint = ShCoeffsPerPoint(harmonicsDegree);

      // Check SH data
      const size_t expectedSH = static_cast<size_t>(N) * shCoeffsPerPoint;
      if (ir.sh.size() != expectedSH && !ir.sh.empty()) {
        return Expected<std::vector<uint8_t>>(
            MakeError("ksplat write failed: inconsistent SH data size"));
      }

      const CompressionConfig &config = COMPRESSION_MODE_0;
      // Calculate bytesPerSplat with 4-byte alignment padding
      const size_t rawBytesPerSplat =
          config.centerBytes + config.scaleBytes + config.rotationBytes +
          config.colorBytes + harmonicsComponentCount * config.harmonicsBytes;
      // Align to 4-byte boundary (padding)
      const size_t bytesPerSplat = (rawBytesPerSplat + 3) & ~3;
      const size_t sectionDataSize = bytesPerSplat * static_cast<size_t>(N);

      // Calculate output size
      const size_t totalSize = MAIN_HEADER_SIZE +
                               maxSections * SECTION_HEADER_SIZE +
                               sectionDataSize;

      std::vector<uint8_t> result(totalSize, 0);

      // Write main header
      uint8_t *mainHeader = result.data();
      WriteUint8(&mainHeader[0], 0); // majorVersion
      WriteUint8(&mainHeader[1], 1); // minorVersion
      WriteUint32LE(&mainHeader[4], maxSections);
      WriteUint32LE(&mainHeader[16], static_cast<uint32_t>(N));
      WriteUint16LE(&mainHeader[20], compressionMode);
      WriteFloat32LE(&mainHeader[36], -1.5f); // minHarmonicsValue
      WriteFloat32LE(&mainHeader[40], 1.5f);  // maxHarmonicsValue

      // Write section header
      uint8_t *sectionHeader = &result[MAIN_HEADER_SIZE];
      WriteUint32LE(&sectionHeader[0],
                    static_cast<uint32_t>(N)); // sectionSplatCount
      WriteUint32LE(&sectionHeader[4],
                    static_cast<uint32_t>(N)); // maxSectionSplats
      WriteUint32LE(&sectionHeader[8],
                    static_cast<uint32_t>(N));  // bucketCapacity
      WriteUint32LE(&sectionHeader[12], 0);     // bucketCount (no buckets)
      WriteFloat32LE(&sectionHeader[16], 1.0f); // spatialBlockSize
      WriteUint16LE(&sectionHeader[20], 0);     // bucketStorageSize
      WriteUint32LE(&sectionHeader[24], 1);     // quantizationRange
      WriteUint32LE(&sectionHeader[32], 0);     // fullBuckets
      WriteUint32LE(&sectionHeader[36], 0);     // partialBuckets
      WriteUint16LE(&sectionHeader[40], static_cast<uint16_t>(harmonicsDegree));

      // Write splat data
      const size_t splatDataOffset =
          MAIN_HEADER_SIZE + maxSections * SECTION_HEADER_SIZE;
      uint8_t *splatData = &result[splatDataOffset];

      for (int32_t i = 0; i < N; i++) {
        const size_t splatByteOffset = static_cast<size_t>(i) * bytesPerSplat;

        // Write position (3 × float32)
        WriteFloat32LE(&splatData[splatByteOffset], ir.positions[i * 3 + 0]);
        WriteFloat32LE(&splatData[splatByteOffset + 4],
                       ir.positions[i * 3 + 1]);
        WriteFloat32LE(&splatData[splatByteOffset + 8],
                       ir.positions[i * 3 + 2]);

        // Write scale (3 × float32, convert from log-scale to linear)
        const float scaleX = std::exp(ir.scales[i * 3 + 0]);
        const float scaleY = std::exp(ir.scales[i * 3 + 1]);
        const float scaleZ = std::exp(ir.scales[i * 3 + 2]);
        WriteFloat32LE(&splatData[splatByteOffset + config.scaleStartByte],
                       scaleX);
        WriteFloat32LE(&splatData[splatByteOffset + config.scaleStartByte + 4],
                       scaleY);
        WriteFloat32LE(&splatData[splatByteOffset + config.scaleStartByte + 8],
                       scaleZ);

        // Write rotation quaternion (4 × float32)
        // IR format: [w, x, y, z]
        float rotW = ir.rotations[i * 4 + 0];
        float rotX = ir.rotations[i * 4 + 1];
        float rotY = ir.rotations[i * 4 + 2];
        float rotZ = ir.rotations[i * 4 + 3];

        // Normalize quaternion
        const float length =
            std::sqrt(rotX * rotX + rotY * rotY + rotZ * rotZ + rotW * rotW);
        if (length > 1e-8f) {
          const float invLength = 1.0f / length;
          rotW *= invLength;
          rotX *= invLength;
          rotY *= invLength;
          rotZ *= invLength;
        } else {
          rotW = 1.0f;
          rotX = 0.0f;
          rotY = 0.0f;
          rotZ = 0.0f;
        }

        WriteFloat32LE(&splatData[splatByteOffset + config.rotationStartByte],
                       rotW);
        WriteFloat32LE(
            &splatData[splatByteOffset + config.rotationStartByte + 4], rotX);
        WriteFloat32LE(
            &splatData[splatByteOffset + config.rotationStartByte + 8], rotY);
        WriteFloat32LE(
            &splatData[splatByteOffset + config.rotationStartByte + 12], rotZ);

        // Write color and opacity (4 × uint8)
        // Convert from SH 0-order coefficients to uint8
        const float colorR = ir.colors[i * 3 + 0];
        const float colorG = ir.colors[i * 3 + 1];
        const float colorB = ir.colors[i * 3 + 2];
        const uint8_t red = static_cast<uint8_t>(
            gf::clamp((colorR * SH_C0 + 0.5f) * 255.0f, 0.0f, 255.0f));
        const uint8_t green = static_cast<uint8_t>(
            gf::clamp((colorG * SH_C0 + 0.5f) * 255.0f, 0.0f, 255.0f));
        const uint8_t blue = static_cast<uint8_t>(
            gf::clamp((colorB * SH_C0 + 0.5f) * 255.0f, 0.0f, 255.0f));

        // Convert from pre-sigmoid alpha to uint8
        const float alpha = ir.alphas[i];
        const float sigmoidAlpha = 1.0f / (1.0f + std::exp(-alpha));
        const uint8_t opacity = static_cast<uint8_t>(
            gf::clamp(sigmoidAlpha * 255.0f, 0.0f, 255.0f));

        splatData[splatByteOffset + config.colorStartByte] = red;
        splatData[splatByteOffset + config.colorStartByte + 1] = green;
        splatData[splatByteOffset + config.colorStartByte + 2] = blue;
        splatData[splatByteOffset + config.colorStartByte + 3] = opacity;

        // Write spherical harmonics
        if (harmonicsComponentCount > 0 && !ir.sh.empty()) {
          // ksplat format: Channel-First layout [R1...R15, G1...G15, B1...B15]
          // IR format: Coefficient-First layout [coeff1_RGB, coeff2_RGB, ...]
          const int coeffsPerChannel = harmonicsComponentCount / 3;
          for (int channel = 0; channel < 3; channel++) {
            for (int coeffInChannel = 0; coeffInChannel < coeffsPerChannel;
                 coeffInChannel++) {
              // Calculate ksplat index: channel-first layout
              const int ksplatIndex =
                  channel * coeffsPerChannel + coeffInChannel;

              // IR format: coeff_R, coeff_G, coeff_B for each coefficient
              const int shOffset =
                  i * shCoeffsPerPoint + coeffInChannel * 3 + channel;
              float harmonicsValue = 0.0f;
              if (shOffset < static_cast<int>(ir.sh.size())) {
                harmonicsValue = ir.sh[shOffset];
              }

              WriteFloat32LE(
                  &splatData[splatByteOffset + config.harmonicsStartByte +
                             ksplatIndex * 4],
                  harmonicsValue);
            }
          }
        }
      }

      return Expected<std::vector<uint8_t>>(std::move(result));
    } catch (const std::exception &e) {
      return Expected<std::vector<uint8_t>>(
          MakeError(std::string("ksplat write failed: ") + e.what()));
    }
  }
};

std::unique_ptr<IGaussWriter> MakeKsplatWriter() {
  return std::make_unique<KsplatWriter>();
}

} // namespace gf

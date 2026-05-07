#include "gf/core/errors.h"
#include "gf/core/gauss_ir.h"
#include "gf/core/metadata.h"
#include "gf/io/writer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace gf {

namespace {

constexpr int CHUNK_SIZE = 256;
constexpr float SH_C0 = 0.28209479177387814f;

// Pack normalized value to N bits
inline uint32_t packUnorm(float value, int bits) {
  const uint32_t maxVal = (1u << bits) - 1;
  return std::max(0u, std::min(maxVal, static_cast<uint32_t>(
                                           std::floor(value * maxVal + 0.5f))));
}

// Pack position/scale (11-10-11 bits)
uint32_t pack111011(float x, float y, float z) {
  return (packUnorm(x, 11) << 21) | (packUnorm(y, 10) << 11) | packUnorm(z, 11);
}

// Pack color (8-8-8-8 bits)
uint32_t pack8888(float x, float y, float z, float w) {
  return (packUnorm(x, 8) << 24) | (packUnorm(y, 8) << 16) |
         (packUnorm(z, 8) << 8) | packUnorm(w, 8);
}

// Pack rotation (2-10-10-10 bits)
uint32_t packRot(float x, float y, float z, float w) {
  // Normalize quaternion
  const float norm = std::sqrt(x * x + y * y + z * z + w * w);
  if (norm < 1e-8f) {
    x = 1.0f;
    y = z = w = 0.0f;
  } else {
    x /= norm;
    y /= norm;
    z /= norm;
    w /= norm;
  }

  float a[4] = {x, y, z, w};

  // Find largest component
  int largest = 0;
  for (int i = 1; i < 4; ++i) {
    if (std::abs(a[i]) > std::abs(a[largest])) {
      largest = i;
    }
  }

  // Ensure largest component is positive
  if (a[largest] < 0) {
    a[0] = -a[0];
    a[1] = -a[1];
    a[2] = -a[2];
    a[3] = -a[3];
  }

  // Pack other 3 components
  const float packNorm = std::sqrt(2.0f) * 0.5f;
  uint32_t result = largest;
  for (int i = 0; i < 4; ++i) {
    if (i != largest) {
      result = (result << 10) | packUnorm(a[i] * packNorm + 0.5f, 10);
    }
  }

  return result;
}

struct MinMax {
  float min, max;
};

MinMax calcMinMax(const float *data, int count) {
  MinMax result;
  result.min = result.max = data[0];
  for (int i = 1; i < count; ++i) {
    result.min = std::min(result.min, data[i]);
    result.max = std::max(result.max, data[i]);
  }
  return result;
}

float normalize(float x, float min, float max) {
  if (x <= min)
    return 0.0f;
  if (x >= max)
    return 1.0f;
  return (max - min < 0.00001f) ? 0.0f : (x - min) / (max - min);
}

float sigmoid(float x) { return 1.0f / (1.0f + std::exp(-x)); }

} // anonymous namespace

class PlyCompressedWriter : public IGaussWriter {
public:
  Expected<std::vector<uint8_t>> Write(const GaussianCloudIR &ir,
                                       const WriteOptions &options) override {
    try {
      const int numPoints = ir.numPoints;
      if (numPoints <= 0) {
        return Expected<std::vector<uint8_t>>(
            MakeError("compressed ply write failed: no points to write"));
      }

      const int numChunks = (numPoints + CHUNK_SIZE - 1) / CHUNK_SIZE;

      // Prepare chunk data storage
      std::vector<float> chunkData(numChunks * 18);
      std::vector<uint32_t> packedData(numPoints * 4);

      // Calculate SH coefficients count
      const int shDegree = ir.meta.shDegree;
      const int shDim =
          (shDegree <= 0) ? 0 : ((shDegree + 1) * (shDegree + 1) - 1);
      const int shCoeffs = shDim * 3;
      std::vector<uint8_t> shData;
      if (shCoeffs > 0) {
        shData.resize(numPoints * shCoeffs);
      }

      // Process each chunk
      std::vector<float> tempPositions(CHUNK_SIZE * 3);
      std::vector<float> tempScales(CHUNK_SIZE * 3);
      std::vector<float> tempColors(CHUNK_SIZE * 3);
      std::vector<float> tempRotations(CHUNK_SIZE * 4);

      for (int chunkIdx = 0; chunkIdx < numChunks; ++chunkIdx) {
        const int startIdx = chunkIdx * CHUNK_SIZE;
        const int count = std::min(CHUNK_SIZE, numPoints - startIdx);

        // Extract chunk data
        for (int i = 0; i < count; ++i) {
          const int idx = startIdx + i;

          // Position
          tempPositions[i * 3 + 0] = ir.positions[idx * 3 + 0];
          tempPositions[i * 3 + 1] = ir.positions[idx * 3 + 1];
          tempPositions[i * 3 + 2] = ir.positions[idx * 3 + 2];

          // Scale
          tempScales[i * 3 + 0] = ir.scales[idx * 3 + 0];
          tempScales[i * 3 + 1] = ir.scales[idx * 3 + 1];
          tempScales[i * 3 + 2] = ir.scales[idx * 3 + 2];

          // Color (convert f_dc to RGB [0,1])
          tempColors[i * 3 + 0] = ir.colors[idx * 3 + 0] * SH_C0 + 0.5f;
          tempColors[i * 3 + 1] = ir.colors[idx * 3 + 1] * SH_C0 + 0.5f;
          tempColors[i * 3 + 2] = ir.colors[idx * 3 + 2] * SH_C0 + 0.5f;

          tempRotations[i * 4 + 0] = ir.rotations[idx * 4 + 0]; // w
          tempRotations[i * 4 + 1] = ir.rotations[idx * 4 + 1]; // x
          tempRotations[i * 4 + 2] = ir.rotations[idx * 4 + 2]; // z
          tempRotations[i * 4 + 3] = ir.rotations[idx * 4 + 3]; // x
        }

        // Pad last chunk
        for (int i = count; i < CHUNK_SIZE; ++i) {
          const int lastIdx = count - 1;
          tempPositions[i * 3 + 0] = tempPositions[lastIdx * 3 + 0];
          tempPositions[i * 3 + 1] = tempPositions[lastIdx * 3 + 1];
          tempPositions[i * 3 + 2] = tempPositions[lastIdx * 3 + 2];
          tempScales[i * 3 + 0] = tempScales[lastIdx * 3 + 0];
          tempScales[i * 3 + 1] = tempScales[lastIdx * 3 + 1];
          tempScales[i * 3 + 2] = tempScales[lastIdx * 3 + 2];
          tempColors[i * 3 + 0] = tempColors[lastIdx * 3 + 0];
          tempColors[i * 3 + 1] = tempColors[lastIdx * 3 + 1];
          tempColors[i * 3 + 2] = tempColors[lastIdx * 3 + 2];
          tempRotations[i * 4 + 0] = tempRotations[lastIdx * 4 + 0];
          tempRotations[i * 4 + 1] = tempRotations[lastIdx * 4 + 1];
          tempRotations[i * 4 + 2] = tempRotations[lastIdx * 4 + 2];
          tempRotations[i * 4 + 3] = tempRotations[lastIdx * 4 + 3];
        }

        // Calculate min/max for positions
        MinMax px = calcMinMax(&tempPositions[0], CHUNK_SIZE);
        MinMax py = calcMinMax(&tempPositions[1], CHUNK_SIZE);
        MinMax pz = calcMinMax(&tempPositions[2], CHUNK_SIZE);
        for (int i = 0; i < CHUNK_SIZE; ++i) {
          px = calcMinMax(&tempPositions[i * 3 + 0], 1);
          py = calcMinMax(&tempPositions[i * 3 + 1], 1);
          pz = calcMinMax(&tempPositions[i * 3 + 2], 1);
        }

        // Recalculate properly
        px.min = px.max = tempPositions[0];
        py.min = py.max = tempPositions[1];
        pz.min = pz.max = tempPositions[2];
        for (int i = 1; i < CHUNK_SIZE; ++i) {
          px.min = std::min(px.min, tempPositions[i * 3 + 0]);
          px.max = std::max(px.max, tempPositions[i * 3 + 0]);
          py.min = std::min(py.min, tempPositions[i * 3 + 1]);
          py.max = std::max(py.max, tempPositions[i * 3 + 1]);
          pz.min = std::min(pz.min, tempPositions[i * 3 + 2]);
          pz.max = std::max(pz.max, tempPositions[i * 3 + 2]);
        }

        // Calculate min/max for scales (with clamping)
        MinMax sx = {1e9f, -1e9f};
        MinMax sy = {1e9f, -1e9f};
        MinMax sz = {1e9f, -1e9f};
        for (int i = 0; i < CHUNK_SIZE; ++i) {
          sx.min = std::min(sx.min, tempScales[i * 3 + 0]);
          sx.max = std::max(sx.max, tempScales[i * 3 + 0]);
          sy.min = std::min(sy.min, tempScales[i * 3 + 1]);
          sy.max = std::max(sy.max, tempScales[i * 3 + 1]);
          sz.min = std::min(sz.min, tempScales[i * 3 + 2]);
          sz.max = std::max(sz.max, tempScales[i * 3 + 2]);
        }
        // Clamp scales
        sx.min = std::max(-20.0f, std::min(20.0f, sx.min));
        sx.max = std::max(-20.0f, std::min(20.0f, sx.max));
        sy.min = std::max(-20.0f, std::min(20.0f, sy.min));
        sy.max = std::max(-20.0f, std::min(20.0f, sy.max));
        sz.min = std::max(-20.0f, std::min(20.0f, sz.min));
        sz.max = std::max(-20.0f, std::min(20.0f, sz.max));

        // Calculate min/max for colors
        MinMax cr = {1e9f, -1e9f};
        MinMax cg = {1e9f, -1e9f};
        MinMax cb = {1e9f, -1e9f};
        for (int i = 0; i < CHUNK_SIZE; ++i) {
          cr.min = std::min(cr.min, tempColors[i * 3 + 0]);
          cr.max = std::max(cr.max, tempColors[i * 3 + 0]);
          cg.min = std::min(cg.min, tempColors[i * 3 + 1]);
          cg.max = std::max(cg.max, tempColors[i * 3 + 1]);
          cb.min = std::min(cb.min, tempColors[i * 3 + 2]);
          cb.max = std::max(cb.max, tempColors[i * 3 + 2]);
        }

        // Store chunk metadata
        float *chunk = &chunkData[chunkIdx * 18];
        chunk[0] = px.min;
        chunk[1] = py.min;
        chunk[2] = pz.min;
        chunk[3] = px.max;
        chunk[4] = py.max;
        chunk[5] = pz.max;
        chunk[6] = sx.min;
        chunk[7] = sy.min;
        chunk[8] = sz.min;
        chunk[9] = sx.max;
        chunk[10] = sy.max;
        chunk[11] = sz.max;
        chunk[12] = cr.min;
        chunk[13] = cg.min;
        chunk[14] = cb.min;
        chunk[15] = cr.max;
        chunk[16] = cg.max;
        chunk[17] = cb.max;

        // Pack vertex data
        for (int i = 0; i < count; ++i) {
          const int idx = startIdx + i;
          const int outIdx = idx * 4;

          // Pack position
          packedData[outIdx + 0] =
              pack111011(normalize(tempPositions[i * 3 + 0], px.min, px.max),
                         normalize(tempPositions[i * 3 + 1], py.min, py.max),
                         normalize(tempPositions[i * 3 + 2], pz.min, pz.max));

          // Pack rotation
          packedData[outIdx + 1] =
              packRot(tempRotations[i * 4 + 0], tempRotations[i * 4 + 1],
                      tempRotations[i * 4 + 2], tempRotations[i * 4 + 3]);

          // Pack scale
          packedData[outIdx + 2] =
              pack111011(normalize(tempScales[i * 3 + 0], sx.min, sx.max),
                         normalize(tempScales[i * 3 + 1], sy.min, sy.max),
                         normalize(tempScales[i * 3 + 2], sz.min, sz.max));

          // Pack color and opacity
          const float opacity = sigmoid(ir.alphas[idx]);
          packedData[outIdx + 3] = pack8888(
              normalize(tempColors[i * 3 + 0], cr.min, cr.max),
              normalize(tempColors[i * 3 + 1], cg.min, cg.max),
              normalize(tempColors[i * 3 + 2], cb.min, cb.max), opacity);
        }

        // Process SH data
        if (shCoeffs > 0) {
          for (int i = 0; i < count; ++i) {
            const int idx = startIdx + i;
            for (int k = 0; k < shCoeffs; ++k) {
              const float value = ir.sh[idx * shCoeffs + k];
              const float nvalue = value / 8.0f + 0.5f;
              shData[idx * shCoeffs + k] = static_cast<uint8_t>(std::max(
                  0.0f, std::min(255.0f, std::floor(nvalue * 256.0f))));
            }
          }
        }
      }

      // Build header
      std::ostringstream header;
      header << "ply\n";
      header << "format binary_little_endian 1.0\n";
      header << "comment Generated by GaussForge\n";
      header << "element chunk " << numChunks << "\n";
      header << "property float min_x\n";
      header << "property float min_y\n";
      header << "property float min_z\n";
      header << "property float max_x\n";
      header << "property float max_y\n";
      header << "property float max_z\n";
      header << "property float min_scale_x\n";
      header << "property float min_scale_y\n";
      header << "property float min_scale_z\n";
      header << "property float max_scale_x\n";
      header << "property float max_scale_y\n";
      header << "property float max_scale_z\n";
      header << "property float min_r\n";
      header << "property float min_g\n";
      header << "property float min_b\n";
      header << "property float max_r\n";
      header << "property float max_g\n";
      header << "property float max_b\n";
      header << "element vertex " << numPoints << "\n";
      header << "property uint packed_position\n";
      header << "property uint packed_rotation\n";
      header << "property uint packed_scale\n";
      header << "property uint packed_color\n";

      if (shCoeffs > 0) {
        header << "element sh " << numPoints << "\n";
        for (int i = 0; i < shCoeffs; ++i) {
          header << "property uchar f_rest_" << i << "\n";
        }
      }

      header << "end_header\n";

      std::string headerStr = header.str();

      // Build output buffer
      std::vector<uint8_t> result;
      result.reserve(headerStr.size() + chunkData.size() * sizeof(float) +
                     packedData.size() * sizeof(uint32_t) + shData.size());

      // Write header
      result.insert(result.end(), headerStr.begin(), headerStr.end());

      // Write chunk data
      const uint8_t *chunkPtr =
          reinterpret_cast<const uint8_t *>(chunkData.data());
      result.insert(result.end(), chunkPtr,
                    chunkPtr + chunkData.size() * sizeof(float));

      // Write vertex data
      const uint8_t *packedPtr =
          reinterpret_cast<const uint8_t *>(packedData.data());
      result.insert(result.end(), packedPtr,
                    packedPtr + packedData.size() * sizeof(uint32_t));

      // Write SH data
      if (shCoeffs > 0) {
        result.insert(result.end(), shData.begin(), shData.end());
      }

      return Expected<std::vector<uint8_t>>(std::move(result));

    } catch (const std::exception &e) {
      return MakeError(std::string("compressed ply write failed: ") + e.what());
    }
  }
};

std::unique_ptr<IGaussWriter> MakePlyCompressedWriter() {
  return std::make_unique<PlyCompressedWriter>();
}

} // namespace gf

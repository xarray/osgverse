#include "gf/core/errors.h"
#include "gf/core/gauss_ir.h"
#include "gf/core/validate.h"
#include "gf/io/reader.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace gf {

namespace {

constexpr int CHUNK_SIZE = 256;
constexpr float SH_C0 = 0.28209479177387814f;

// Helper function to read lines and skip comments from memory buffer
bool getlineSkipComment(const uint8_t *&data, size_t &remaining,
                        std::string &line) {
  while (remaining > 0) {
    // Find line end
    size_t lineEnd = 0;
    bool found = false;
    for (size_t i = 0; i < remaining; ++i) {
      if (data[i] == '\n') {
        lineEnd = i;
        found = true;
        break;
      }
    }
    if (!found) {
      lineEnd = remaining;
    }

    // Extract line content
    line.assign(reinterpret_cast<const char *>(data), lineEnd);
    data += lineEnd + (found ? 1 : 0);
    remaining -= lineEnd + (found ? 1 : 0);

    // Remove trailing whitespace
    auto end = line.find_last_not_of(" \t\r\n");
    if (end != std::string::npos) {
      line = line.substr(0, end + 1);
    } else {
      line.clear();
    }

    auto start = line.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
      continue;
    std::string trimmed = line.substr(start);
    if (trimmed.rfind("comment", 0) == 0)
      continue;
    line = trimmed;
    return true;
  }
  return false;
}

// Linear interpolation
inline float lerp(float a, float b, float t) { return a * (1.0f - t) + b * t; }

// Unpack normalized value from N bits
inline float unpackUnorm(uint32_t value, int bits) {
  const uint32_t mask = (1u << bits) - 1;
  const uint32_t extracted = value & mask;
  return static_cast<float>(extracted) / static_cast<float>(mask);
}

// Unpack position/scale (11-10-11 bits: X=11, Y=10, Z=11)
struct Vec3 {
  float x, y, z;
};

Vec3 unpack111011(uint32_t value) {
  Vec3 result;
  result.x = unpackUnorm(value >> 21, 11);
  result.y = unpackUnorm(value >> 11, 10);
  result.z = unpackUnorm(value, 11);
  return result;
}

// Unpack rotation (2-10-10-10 bits)
struct Quat {
  float x, y, z, w;
};

Quat unpackRot(uint32_t value) {
  const float norm = 1.0f / (std::sqrt(2.0f) * 0.5f); // Restore normalized range
  const uint32_t which = value >> 30;                 // 0:w, 1:x, 2:y, 3:z

  // a, b, c are the three smaller stored components
  const float a = (unpackUnorm(value >> 20, 10) - 0.5f) * norm;
  const float b = (unpackUnorm(value >> 10, 10) - 0.5f) * norm;
  const float c = (unpackUnorm(value, 10) - 0.5f) * norm;

  // Calculate the discarded maximum component m
  const float m = std::sqrt(std::max(0.0f, 1.0f - (a * a + b * b + c * c)));

  Quat result;
  // Core correction: restore according to which index definition
  switch (which) {
  case 0: // w is the maximum component, stored are x, y, z
    result.w = m;
    result.x = a;
    result.y = b;
    result.z = c;
    break;
  case 1: // x is the maximum component, stored are w, y, z
    result.x = m;
    result.w = a;
    result.y = b;
    result.z = c;
    break;
  case 2: // y is the maximum component, stored are w, x, z
    result.y = m;
    result.w = a;
    result.x = b;
    result.z = c;
    break;
  case 3: // z is the maximum component, stored are w, x, y
    result.w = a;
    result.x = b;
    result.y = c;
    result.z = m;
    break;
  }
  return result;
}

// Unpack color (8-8-8-8 bits: R, G, B, Alpha)
struct Color4 {
  float x, y, z, w;
};

Color4 unpack8888(uint32_t value) {
  Color4 result;
  result.x = unpackUnorm(value >> 24, 8);
  result.y = unpackUnorm(value >> 16, 8);
  result.z = unpackUnorm(value >> 8, 8);
  result.w = unpackUnorm(value, 8);
  return result;
}

int degreeForShCoeffs(int numCoeffs) {
  if (numCoeffs == 0)
    return 0;
  if (numCoeffs == 9)
    return 1;
  if (numCoeffs == 24)
    return 2;
  if (numCoeffs == 45)
    return 3;
  return 0;
}

} // anonymous namespace

class PlyCompressedReader : public IGaussReader {
public:
  Expected<GaussianCloudIR> Read(const uint8_t *data, size_t size,
                                 const ReadOptions &options) override {
    try {
      if (data == nullptr || size == 0) {
        return Expected<GaussianCloudIR>(
            MakeError("compressed ply read failed: empty input"));
      }

      const uint8_t *current = data;
      size_t remaining = size;

      // Parse header
      std::string line;
      if (!getlineSkipComment(current, remaining, line) || line != "ply") {
        return Expected<GaussianCloudIR>(
            MakeError("compressed ply read failed: not a ply file"));
      }

      if (!getlineSkipComment(current, remaining, line) ||
          line != "format binary_little_endian 1.0") {
        return Expected<GaussianCloudIR>(
            MakeError("compressed ply read failed: unsupported format"));
      }

      // Parse elements
      int numChunks = 0;
      int numVertices = 0;
      int numSh = 0;
      int shCoeffs = 0;

      std::string currentElement;

      while (getlineSkipComment(current, remaining, line)) {
        if (line == "end_header")
          break;

        if (line.rfind("element ", 0) == 0) {
          std::string rest = line.substr(8);
          size_t spacePos = rest.find(' ');
          if (spacePos == std::string::npos) {
            return Expected<GaussianCloudIR>(
                MakeError("compressed ply read failed: invalid element line"));
          }
          currentElement = rest.substr(0, spacePos);
          int count = std::stoi(rest.substr(spacePos + 1));

          if (currentElement == "chunk") {
            numChunks = count;
          } else if (currentElement == "vertex") {
            numVertices = count;
          } else if (currentElement == "sh") {
            numSh = count;
          }
        } else if (line.rfind("property ", 0) == 0) {
          if (currentElement == "sh") {
            shCoeffs++;
          }
        }
      }

      if (numVertices <= 0) {
        return Expected<GaussianCloudIR>(
            MakeError("compressed ply read failed: invalid vertex count"));
      }

      if (numChunks != (numVertices + CHUNK_SIZE - 1) / CHUNK_SIZE) {
        return Expected<GaussianCloudIR>(
            MakeError("compressed ply read failed: chunk count mismatch"));
      }

      // Read chunk data (18 floats per chunk)
      const size_t chunkDataSize = numChunks * 18 * sizeof(float);
      if (remaining < chunkDataSize) {
        return Expected<GaussianCloudIR>(MakeError(
            "compressed ply read failed: insufficient data for chunks"));
      }
      std::vector<float> chunkData(numChunks * 18);
      std::memcpy(chunkData.data(), current, chunkDataSize);
      current += chunkDataSize;
      remaining -= chunkDataSize;

      // Read vertex data (4 uint32s per vertex)
      const size_t vertexDataSize = numVertices * 4 * sizeof(uint32_t);
      if (remaining < vertexDataSize) {
        return Expected<GaussianCloudIR>(MakeError(
            "compressed ply read failed: insufficient data for vertices"));
      }
      std::vector<uint32_t> vertexData(numVertices * 4);
      std::memcpy(vertexData.data(), current, vertexDataSize);
      current += vertexDataSize;
      remaining -= vertexDataSize;

      // Read SH data if present
      std::vector<uint8_t> shData;
      if (numSh > 0 && shCoeffs > 0) {
        const size_t shDataSize = numSh * shCoeffs;
        if (remaining < shDataSize) {
          return Expected<GaussianCloudIR>(MakeError(
              "compressed ply read failed: insufficient data for SH"));
        }
        shData.resize(shDataSize);
        std::memcpy(shData.data(), current, shDataSize);
        current += shDataSize;
        remaining -= shDataSize;
      }

      // Build GaussianCloudIR
      GaussianCloudIR ir;
      ir.numPoints = numVertices;
      ir.meta.shDegree = degreeForShCoeffs(shCoeffs);
      ir.meta.sourceFormat = "compressed.ply";

      ir.positions.resize(numVertices * 3);
      ir.scales.resize(numVertices * 3);
      ir.rotations.resize(numVertices * 4);
      ir.alphas.resize(numVertices);
      ir.colors.resize(numVertices * 3);
      if (shCoeffs > 0) {
        ir.sh.resize(numVertices * shCoeffs);
      }

      // Decompress vertex data
      for (int i = 0; i < numVertices; ++i) {
        const int chunkIdx = i / CHUNK_SIZE;
        const float *chunk = &chunkData[chunkIdx * 18];

        const float min_x = chunk[0];
        const float min_y = chunk[1];
        const float min_z = chunk[2];
        const float max_x = chunk[3];
        const float max_y = chunk[4];
        const float max_z = chunk[5];
        const float min_scale_x = chunk[6];
        const float min_scale_y = chunk[7];
        const float min_scale_z = chunk[8];
        const float max_scale_x = chunk[9];
        const float max_scale_y = chunk[10];
        const float max_scale_z = chunk[11];
        const float min_r = chunk[12];
        const float min_g = chunk[13];
        const float min_b = chunk[14];
        const float max_r = chunk[15];
        const float max_g = chunk[16];
        const float max_b = chunk[17];

        const uint32_t packed_position = vertexData[i * 4 + 0];
        const uint32_t packed_rotation = vertexData[i * 4 + 1];
        const uint32_t packed_scale = vertexData[i * 4 + 2];
        const uint32_t packed_color = vertexData[i * 4 + 3];

        // Unpack position
        Vec3 pos = unpack111011(packed_position);
        ir.positions[i * 3 + 0] = lerp(min_x, max_x, pos.x);
        ir.positions[i * 3 + 1] = lerp(min_y, max_y, pos.y);
        ir.positions[i * 3 + 2] = lerp(min_z, max_z, pos.z);

        // Unpack rotation
        // unpackRot returns quaternion as (x, y, z, w)
        // Store in order [y, z, w, x] so that ply_writer outputs [x, y, z, w]
        // (ply_writer writes IR[3,0,1,2] as PLY[rot_0,rot_1,rot_2,rot_3])
        Quat rot = unpackRot(packed_rotation);
        ir.rotations[i * 4 + 0] = rot.w;
        ir.rotations[i * 4 + 1] = rot.x;
        ir.rotations[i * 4 + 2] = rot.y;
        ir.rotations[i * 4 + 3] = rot.z;

        // Unpack scale
        Vec3 scale = unpack111011(packed_scale);
        ir.scales[i * 3 + 0] = lerp(min_scale_x, max_scale_x, scale.x);
        ir.scales[i * 3 + 1] = lerp(min_scale_y, max_scale_y, scale.y);
        ir.scales[i * 3 + 2] = lerp(min_scale_z, max_scale_z, scale.z);

        // Unpack color and opacity
        Color4 color = unpack8888(packed_color);
        const float cr = lerp(min_r, max_r, color.x);
        const float cg = lerp(min_g, max_g, color.y);
        const float cb = lerp(min_b, max_b, color.z);

        // Convert from RGB [0,1] back to f_dc (SH coefficient)
        ir.colors[i * 3 + 0] = (cr - 0.5f) / SH_C0;
        ir.colors[i * 3 + 1] = (cg - 0.5f) / SH_C0;
        ir.colors[i * 3 + 2] = (cb - 0.5f) / SH_C0;

        // Convert from sigmoid space back to pre-sigmoid (logit)
        // Clamp opacity to avoid numerical issues at boundaries
        const float opacity = std::max(0.001f, std::min(0.999f, color.w));
        ir.alphas[i] = -std::log(1.0f / opacity - 1.0f);
      }

      // Decompress SH data
      // In PLY format, SH is stored as: all R channels, then all G, then all B
      // In IR format, SH should be interleaved: R,G,B per coefficient
      if (shCoeffs > 0) {
        const int shDim = shCoeffs / 3; // Number of SH coefficients per channel
        for (int i = 0; i < numVertices; ++i) {
          for (int j = 0; j < shDim; ++j) {
            // Read from separate R,G,B channels and interleave
            const uint8_t value_r = shData[i * shCoeffs + j];
            const uint8_t value_g = shData[i * shCoeffs + j + shDim];
            const uint8_t value_b = shData[i * shCoeffs + j + 2 * shDim];

            const float n_r = (value_r == 0)     ? 0.0f
                              : (value_r == 255) ? 1.0f
                                                 : (value_r + 0.5f) / 256.0f;
            const float n_g = (value_g == 0)     ? 0.0f
                              : (value_g == 255) ? 1.0f
                                                 : (value_g + 0.5f) / 256.0f;
            const float n_b = (value_b == 0)     ? 0.0f
                              : (value_b == 255) ? 1.0f
                                                 : (value_b + 0.5f) / 256.0f;

            ir.sh[i * shCoeffs + j * 3 + 0] = (n_r - 0.5f) * 8.0f;
            ir.sh[i * shCoeffs + j * 3 + 1] = (n_g - 0.5f) * 8.0f;
            ir.sh[i * shCoeffs + j * 3 + 2] = (n_b - 0.5f) * 8.0f;
          }
        }
      }

      const auto err = ValidateBasic(ir, options.strict);
      if (!err.message.empty() && options.strict)
        return Expected<GaussianCloudIR>(err);

      return Expected<GaussianCloudIR>(std::move(ir));

    } catch (const std::exception &e) {
      return Expected<GaussianCloudIR>(
          MakeError(std::string("compressed ply read failed: ") + e.what()));
    }
  }
};

std::unique_ptr<IGaussReader> MakePlyCompressedReader() {
  return std::make_unique<PlyCompressedReader>();
}

} // namespace gf

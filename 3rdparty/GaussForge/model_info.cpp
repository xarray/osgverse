#include "gf/core/model_info.h"

#include <algorithm>
#include <cstring>
#include <numeric>

namespace gf {

std::string FormatBytes(size_t bytes) {
  const char *suffix[] = {"B", "KB", "MB", "GB"};
  int exp = 0;
  double value = static_cast<double>(bytes);
  while (value >= 1024.0 && exp < 3) {
    value /= 1024.0;
    exp++;
  }
  char buf[32];
  snprintf(buf, sizeof(buf), "%.2f %s", value, suffix[exp]);
  return std::string(buf);
}

std::string HandednessToString(Handedness h) {
  switch (h) {
  case Handedness::kLeft:
    return "Left";
  case Handedness::kRight:
    return "Right";
  default:
    return "Unknown";
  }
}

std::string UpAxisToString(UpAxis up) {
  switch (up) {
  case UpAxis::kY:
    return "Y";
  case UpAxis::kZ:
    return "Z";
  default:
    return "Unknown";
  }
}

std::string LengthUnitToString(LengthUnit unit) {
  switch (unit) {
  case LengthUnit::kMeter:
    return "Meter";
  case LengthUnit::kCentimeter:
    return "Centimeter";
  default:
    return "Unknown";
  }
}

std::string ColorSpaceToString(ColorSpace color) {
  switch (color) {
  case ColorSpace::kLinear:
    return "Linear";
  case ColorSpace::kSRGB:
    return "sRGB";
  default:
    return "Unknown";
  }
}

namespace {

FloatStats ComputeFloatStats(const std::vector<float> &data) {
  FloatStats stats;
  if (data.empty()) {
    return stats;
  }

  stats.count = data.size();
  stats.min = data[0];
  stats.max = data[0];
  double sum = 0.0;

  for (float v : data) {
    stats.min = std::min(stats.min, v);
    stats.max = std::max(stats.max, v);
    sum += v;
  }

  stats.avg = static_cast<float>(sum / data.size());
  return stats;
}

BoundingBox ComputeBounds(const std::vector<float> &positions) {
  BoundingBox bounds;
  if (positions.size() < 3) {
    return bounds;
  }

  bounds.minX = bounds.maxX = positions[0];
  bounds.minY = bounds.maxY = positions[1];
  bounds.minZ = bounds.maxZ = positions[2];

  for (size_t i = 3; i < positions.size(); i += 3) {
    bounds.minX = std::min(bounds.minX, positions[i]);
    bounds.maxX = std::max(bounds.maxX, positions[i]);
    bounds.minY = std::min(bounds.minY, positions[i + 1]);
    bounds.maxY = std::max(bounds.maxY, positions[i + 1]);
    bounds.minZ = std::min(bounds.minZ, positions[i + 2]);
    bounds.maxZ = std::max(bounds.maxZ, positions[i + 2]);
  }

  return bounds;
}

} // namespace

ModelInfo GetModelInfo(const GaussianCloudIR &ir, size_t file_size) {
  ModelInfo info;

  // Basic info
  info.numPoints = ir.numPoints;
  info.fileSize = file_size;
  info.sourceFormat = ir.meta.sourceFormat;

  // Metadata
  info.handedness = ir.meta.handedness;
  info.upAxis = ir.meta.up;
  info.unit = ir.meta.unit;
  info.colorSpace = ir.meta.color;
  info.shDegree = ir.meta.shDegree;
  info.antialiased = ir.meta.antialiased;

  // Statistics
  info.bounds = ComputeBounds(ir.positions);
  info.scaleStats = ComputeFloatStats(ir.scales);
  info.alphaStats = ComputeFloatStats(ir.alphas);

  // Data size breakdown
  info.positionsSize = ir.positions.size() * sizeof(float);
  info.scalesSize = ir.scales.size() * sizeof(float);
  info.rotationsSize = ir.rotations.size() * sizeof(float);
  info.alphasSize = ir.alphas.size() * sizeof(float);
  info.colorsSize = ir.colors.size() * sizeof(float);
  info.shSize = ir.sh.size() * sizeof(float);

  info.totalSize = info.positionsSize + info.scalesSize + info.rotationsSize +
                  info.alphasSize + info.colorsSize + info.shSize;

  // Extra attributes
  for (std::unordered_map<std::string, AttributeArray>::const_iterator it = ir.extras.begin();
       it != ir.extras.end(); ++it) {
    const std::string& name = it->first;  const AttributeArray& arr = it->second;
  //for (const auto &[name, arr] : ir.extras) {
    info.extraAttrs.push_back({name, arr.size() * sizeof(float)});
    info.totalSize += arr.size() * sizeof(float);
  }

  return info;
}

} // namespace gf

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "gf/core/gauss_ir.h"
#include "gf/core/metadata.h"

namespace gf {

// Statistics for a single float attribute
struct FloatStats {
  float min = 0.0f;
  float max = 0.0f;
  float avg = 0.0f;
  size_t count = 0;
};

// Bounding box for positions
struct BoundingBox {
  float minX = 0.0f, maxX = 0.0f;
  float minY = 0.0f, maxY = 0.0f;
  float minZ = 0.0f, maxZ = 0.0f;
};

// Model information summary
struct ModelInfo {
  // Basic info
  int32_t numPoints = 0;
  size_t fileSize = 0;
  std::string sourceFormat;

  // Metadata
  Handedness handedness = Handedness::kUnknown;
  UpAxis upAxis = UpAxis::kUnknown;
  LengthUnit unit = LengthUnit::kUnknown;
  ColorSpace colorSpace = ColorSpace::kUnknown;
  int shDegree = 0;
  bool antialiased = false;

  // Geometry statistics
  BoundingBox bounds;
  FloatStats scaleStats;
  FloatStats alphaStats;

  // Data size breakdown (in bytes)
  size_t positionsSize = 0;
  size_t scalesSize = 0;
  size_t rotationsSize = 0;
  size_t alphasSize = 0;
  size_t colorsSize = 0;
  size_t shSize = 0;
  size_t totalSize = 0;

  // Extra attribute names and sizes
  std::vector<std::pair<std::string, size_t>> extraAttrs;
};

// Calculate model information from a GaussianCloudIR
ModelInfo GetModelInfo(const GaussianCloudIR &ir, size_t file_size = 0);

// Helper functions for formatting
std::string FormatBytes(size_t bytes);
std::string HandednessToString(Handedness h);
std::string UpAxisToString(UpAxis up);
std::string LengthUnitToString(LengthUnit unit);
std::string ColorSpaceToString(ColorSpace color);

} // namespace gf

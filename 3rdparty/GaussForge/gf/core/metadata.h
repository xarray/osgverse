#pragma once

#include <string>

namespace gf {

enum class Handedness { kUnknown, kLeft, kRight };
enum class UpAxis { kUnknown, kY, kZ };
enum class LengthUnit { kUnknown, kMeter, kCentimeter };
enum class ColorSpace { kUnknown, kLinear, kSRGB };

struct GaussMetadata {
  Handedness handedness = Handedness::kUnknown;
  UpAxis up = UpAxis::kUnknown;
  LengthUnit unit = LengthUnit::kUnknown;
  ColorSpace color = ColorSpace::kUnknown;
  int shDegree = 0;
  bool antialiased = false;
  std::string sourceFormat;
};

} // namespace gf

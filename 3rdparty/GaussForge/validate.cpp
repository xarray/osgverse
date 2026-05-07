#include "gf/core/validate.h"

#include <cmath>

namespace gf {

namespace {
bool IsFinite(const float v) { return std::isfinite(v); }
} // namespace

Error ValidateBasic(const GaussianCloudIR &ir, bool strict) {
  if (ir.numPoints < 0)
    return MakeError("numPoints is negative");

  const auto expect_size = [&](size_t got, size_t expect,
                               const char *name) -> Error {
    if (got != expect) {
      return MakeError(std::string(name) + " size mismatch, got " +
                       std::to_string(got) + ", expect " +
                       std::to_string(expect));
    }
    return Error{};
  };

  auto err = expect_size(ir.positions.size(), static_cast<size_t>(ir.numPoints) * 3, "positions");
  if (!err.message.empty()) return err;
  
  err = expect_size(ir.scales.size(), static_cast<size_t>(ir.numPoints) * 3, "scales");
  if (!err.message.empty()) return err;

  err = expect_size(ir.rotations.size(), static_cast<size_t>(ir.numPoints) * 4, "rotations");
  if (!err.message.empty()) return err;

  err = expect_size(ir.alphas.size(), static_cast<size_t>(ir.numPoints), "alphas");
  if (!err.message.empty()) return err;

  err = expect_size(ir.colors.size(), static_cast<size_t>(ir.numPoints) * 3, "colors");
  if (!err.message.empty()) return err;

  err = expect_size(ir.sh.size(), static_cast<size_t>(ir.numPoints) *
                    ShCoeffsPerPoint(ir.meta.shDegree), "sh");
  if (!err.message.empty()) return err;

  if (strict) {
    for (const float v : ir.positions) {
      if (!IsFinite(v))
        return MakeError("positions contains non-finite value");
    }
    for (const float v : ir.scales) {
      if (!IsFinite(v))
        return MakeError("scales contains non-finite value");
    }
    for (const float v : ir.rotations) {
      if (!IsFinite(v))
        return MakeError("rotations contains non-finite value");
    }
    for (const float v : ir.alphas) {
      if (!IsFinite(v))
        return MakeError("alphas contains non-finite value");
    }
    for (const float v : ir.colors) {
      if (!IsFinite(v))
        return MakeError("colors contains non-finite value");
    }
    for (const float v : ir.sh) {
      if (!IsFinite(v))
        return MakeError("sh contains non-finite value");
    }
  }

  return Error{}; // empty message indicates success.
}

} // namespace gf

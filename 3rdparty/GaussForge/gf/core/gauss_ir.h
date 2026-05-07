#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "gf/core/metadata.h"

namespace gf {

// TODO: Generalize AttributeArray to support multiple numeric types and shapes.
using AttributeArray = std::vector<float>;

struct GaussianCloudIR {
  int32_t numPoints = 0;

  // Structure-of-arrays layout for better cache/SIMD behavior.
  std::vector<float> positions; // [x0, y0, z0, x1, y1, z1, ..., x_{N-1},
                                // y_{N-1}, z_{N-1}]  (3 * N)

  std::vector<float>
      scales; // Log-scale values [sx0, sy0, sz0, sx1, sy1, sz1, ...]  (3 * N)

  std::vector<float> rotations; // Quaternions stored as [w, x, y, z] per point
                                // [qw0, qx0, qy0, qz0, qw1, qx1, qy1, qz1, ...]
                                // This order allows PLY writers to output as
                                // [w, x, y, z] by reading IR[0,1,2,3].  (4 * N)

  std::vector<float>
      alphas; // Pre-sigmoid opacity values [a0, a1, ..., a_{N-1}]  (N)

  std::vector<float> colors; // Spherical Harmonics degree-0 (DC) coefficients.
                             // Layout: RGB interleaved per point [r0, g0, b0,
                             // r1, g1, b1, ...]  (3 * N)
  std::vector<float>
      sh; // Higher-order Spherical Harmonics coefficients (degree ≥ 1).
          // Total coeffs per point = ((shDegree+1)^2 - 1)
          // Layout: RGB interleaved per coefficient, coefficients contiguous
          // per point. Order per point: coeff1_R, coeff1_G, coeff1_B, coeff2_R,
          // coeff2_G, coeff2_B, ... Overall size: (num_higher_coeffs_per_point
          // * 3) * N
  std::unordered_map<std::string, AttributeArray> extras;
  GaussMetadata meta;
};

template <typename T>
inline constexpr T clamp(T v, T lo, T hi)
{ return (v < lo) ? lo : (hi < v) ? hi : v; }

inline int ShCoeffsPerPoint(int degree) {
  if (degree <= 0)
    return 0;
  const int per_channel = (degree + 1) * (degree + 1) - 1;
  return per_channel * 3;
}

} // namespace gf

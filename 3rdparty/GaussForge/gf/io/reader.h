#pragma once

#include <cstddef>
#include <cstdint>

#include "gf/core/errors.h"
#include "gf/core/gauss_ir.h"

namespace gf {

struct ReadOptions {
  bool strict = false;
};

class IGaussReader {
public:
  virtual ~IGaussReader() = default;
  // Read data from memory buffer
  virtual Expected<GaussianCloudIR> Read(const uint8_t *data, size_t size,
                                         const ReadOptions &options) = 0;
};

} // namespace gf

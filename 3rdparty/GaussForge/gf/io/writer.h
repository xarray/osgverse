#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "gf/core/errors.h"
#include "gf/core/gauss_ir.h"

namespace gf {

struct WriteOptions {
  bool strict = false;
};

class IGaussWriter {
public:
  virtual ~IGaussWriter() = default;
  // Write data to memory buffer, return serialized byte data
  virtual Expected<std::vector<uint8_t>> Write(const GaussianCloudIR &ir,
                                               const WriteOptions &options) = 0;
};

} // namespace gf

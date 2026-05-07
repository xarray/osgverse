#pragma once

#include "gf/core/errors.h"
#include "gf/core/gauss_ir.h"

namespace gf {

// Perform basic length and value validation. In strict mode, returns error on first issue encountered.
Error ValidateBasic(const GaussianCloudIR& ir, bool strict = false);

}  // namespace gf


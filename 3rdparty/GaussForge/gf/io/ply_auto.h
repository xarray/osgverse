#pragma once

#include <memory>

#include "gf/io/reader.h"

namespace gf {

std::unique_ptr<IGaussReader> MakePlyAutoReader();

} // namespace gf

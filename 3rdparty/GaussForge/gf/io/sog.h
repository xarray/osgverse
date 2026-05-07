#pragma once

#include <memory>
#include "gf/io/reader.h"
#include "gf/io/writer.h"

namespace gf {

std::unique_ptr<IGaussReader> MakeSogReader();
std::unique_ptr<IGaussWriter> MakeSogWriter();

}  // namespace gf

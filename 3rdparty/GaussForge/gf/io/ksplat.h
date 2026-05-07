#pragma once

#include <memory>

#include "gf/io/reader.h"
#include "gf/io/writer.h"

namespace gf {

std::unique_ptr<IGaussReader> MakeKsplatReader();
std::unique_ptr<IGaussWriter> MakeKsplatWriter();

} // namespace gf

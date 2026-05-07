#pragma once

#include <memory>

#include "gf/io/reader.h"
#include "gf/io/writer.h"

namespace gf {

std::unique_ptr<IGaussReader> MakeSplatReader();
std::unique_ptr<IGaussWriter> MakeSplatWriter();

} // namespace gf

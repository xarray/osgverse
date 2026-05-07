#pragma once

#include <memory>

#include "gf/io/reader.h"
#include "gf/io/writer.h"

namespace gf {

std::unique_ptr<IGaussReader> MakePlyCompressedReader();
std::unique_ptr<IGaussWriter> MakePlyCompressedWriter();

} // namespace gf

#pragma once

#ifndef _MSC_VER
#   include <cstdint>
#endif
#include <vector>
#include <xmmintrin.h>

class QuadDecomposition
{
public:
  static std::vector<uint32_t> decompose(const std::vector<uint32_t>& indices, const std::vector<__m128>& vertices);
};



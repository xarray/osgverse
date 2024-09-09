#pragma once

#include <vector>

struct Aabb;

class SurfaceAreaHeuristic
{
public:
  static std::vector<std::vector<uint32_t>> generateBatches(const std::vector<Aabb>& aabbs, uint32_t targetSize, uint32_t splitGranularity);
};


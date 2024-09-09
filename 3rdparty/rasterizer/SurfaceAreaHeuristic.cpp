#include "SurfaceAreaHeuristic.h"

#include "VectorMath.h"

#include <algorithm>
#include <numeric>

namespace
{
  uint32_t sahSplit(const std::vector<Aabb>& aabbsIn, uint32_t splitGranularity, uint32_t* indicesStart, uint32_t* indicesEnd)
  {
    uint32_t numIndices = uint32_t(indicesEnd - indicesStart);

    __m128 bestCost = _mm_set1_ps(std::numeric_limits<float>::infinity());

    int bestAxis = -1;
    int bestIndex = -1;

    for (int splitAxis = 0; splitAxis < 3; ++splitAxis)
    {
      // Sort along center position
      std::stable_sort(indicesStart, indicesEnd, [&](auto i0, auto i1) {
        return _mm_movemask_ps(_mm_cmplt_ps(aabbsIn[i0].getCenter(), aabbsIn[i1].getCenter())) & (1 << splitAxis);
      });

      std::vector<__m128> areasFromLeft;
      areasFromLeft.resize(numIndices);

      std::vector<__m128> areasFromRight;
      areasFromRight.resize(numIndices);

      Aabb fromLeft;
      for (uint32_t i = 0; i < numIndices; ++i)
      {
        fromLeft.include(aabbsIn[indicesStart[i]]);
        areasFromLeft[i] = fromLeft.surfaceArea();
      }

      Aabb fromRight;
      for (int i = numIndices - 1; i >= 0; --i)
      {
        fromRight.include(aabbsIn[indicesStart[i]]);
        areasFromRight[i] = fromRight.surfaceArea();
      }

      for (uint32_t splitIndex = splitGranularity; splitIndex < numIndices - splitGranularity; splitIndex += splitGranularity)
      {
        int countLeft = static_cast<int>(splitIndex);
        int countRight = static_cast<int>(numIndices - splitIndex);

        __m128 areaLeft = areasFromLeft[splitIndex - 1];
        __m128 areaRight = areasFromRight[splitIndex];
        __m128 scaledAreaLeft = _mm_mul_ss(areaLeft, _mm_cvtsi32_ss(_mm_setzero_ps(), countLeft));
        __m128 scaledAreaRight = _mm_mul_ss(areaRight, _mm_cvtsi32_ss(_mm_setzero_ps(), countRight));

        __m128 cost = _mm_add_ss(scaledAreaLeft, scaledAreaRight);

        if (_mm_comilt_ss(cost, bestCost))
        {
          bestCost = cost;
          bestAxis = splitAxis;
          bestIndex = splitIndex;
        }
      }
    }

    // Sort again according to best axis
    std::stable_sort(indicesStart, indicesEnd, [&](auto i0, auto i1) {
      return _mm_movemask_ps(_mm_cmplt_ps(aabbsIn[i0].getCenter(), aabbsIn[i1].getCenter())) & (1 << bestAxis);
    });

    return bestIndex;
  }

  void generateBatchesRecursive(const std::vector<Aabb>& aabbsIn, uint32_t targetSize, uint32_t splitGranularity, uint32_t* indicesStart, uint32_t* indicesEnd, std::vector<std::vector<uint32_t>>& result)
  {
    auto splitIndex = sahSplit(aabbsIn, splitGranularity, indicesStart, indicesEnd);

    uint32_t* range[] = { indicesStart, indicesStart + splitIndex, indicesEnd };

    for (int i = 0; i < 2; ++i)
    {
      auto batchSize = range[i + 1] - range[i];
      if (batchSize < targetSize)
      {
        result.push_back({ range[i], range[i + 1] });
      }
      else
      {
        generateBatchesRecursive(aabbsIn, targetSize, splitGranularity, range[i], range[i + 1], result);
      }
    }
  }
}

std::vector<std::vector<uint32_t>> SurfaceAreaHeuristic::generateBatches(const std::vector<Aabb>& aabbs, uint32_t targetSize, uint32_t splitGranularity)
{
  std::vector<uint32_t> indices(aabbs.size());
  std::iota(begin(indices), end(indices), 0);

  std::vector<std::vector<uint32_t>> result;
  generateBatchesRecursive(aabbs, targetSize, splitGranularity, &indices[0], &indices[0] + indices.size(), result);
  return result;
}

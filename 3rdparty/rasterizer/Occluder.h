#pragma once

#include <memory>
#include <vector>
#include <cstring>

#include "Rasterizer.h"
#if defined(_MSC_VER) || defined(__CYGWIN__) || defined(__MINGW32__)
#   include <intrin.h>
#else
#   define __forceinline inline
#   define _aligned_malloc(size, alignment) aligned_alloc(alignment, size)
#   define _aligned_free(ptr) free(ptr)
#   include "linux/intrin.h"
#endif

struct Occluder
{
	static std::unique_ptr<Occluder> bake(const std::vector<__m128>& vertices, __m128 refMin, __m128 refMax);

	__m128 m_center;

	__m128 m_refMin;
	__m128 m_refMax;

	__m128 m_boundsMin;
	__m128 m_boundsMax;

	__m256i* m_vertexData;
	uint32_t m_packetCount;
};



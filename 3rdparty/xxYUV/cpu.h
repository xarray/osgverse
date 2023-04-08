//==============================================================================
// xxYUV : cpu Header
//
// Copyright (c) 2020-2021 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================
#pragma once

#ifndef xxYUV_EXPORT
#define xxYUV_EXPORT
#endif

//------------------------------------------------------------------------------
inline bool neon() { return true; }
//------------------------------------------------------------------------------
inline bool sse2() { return true; }
xxYUV_EXPORT bool ssse3();
xxYUV_EXPORT bool avx2();
xxYUV_EXPORT bool avx512bw();
//------------------------------------------------------------------------------

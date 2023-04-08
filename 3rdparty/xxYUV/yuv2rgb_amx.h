//==============================================================================
// xxYUV : yuv2rgb_amx Header
//
// Copyright (c) 2021 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================
#pragma once

#ifndef xxYUV_EXPORT
#define xxYUV_EXPORT
#endif

//------------------------------------------------------------------------------
template<int rgbWidth, bool rgbSwizzle, bool interleaved, bool firstU, bool fullRange>
xxYUV_EXPORT void yuv2rgb_amx(int width, int height, const void* y, const void* u, const void* v, int strideY, int strideU, int strideV, void* rgb, int strideRGB);
//------------------------------------------------------------------------------
xxYUV_EXPORT void yyy2rgb_amx(int width, int height, const void* y, int strideY, void* rgb, int strideRGB);
//------------------------------------------------------------------------------
xxYUV_EXPORT void yuv2rgb_yu12_amx(int width, int height, const void* yuv, void* rgb, bool fullRange = true, int rgbWidth = 3, bool rgbSwizzle = false, int strideRGB = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1);
xxYUV_EXPORT void yuv2rgb_yv12_amx(int width, int height, const void* yuv, void* rgb, bool fullRange = true, int rgbWidth = 3, bool rgbSwizzle = false, int strideRGB = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1);
xxYUV_EXPORT void yuv2rgb_nv12_amx(int width, int height, const void* yuv, void* rgb, bool fullRange = true, int rgbWidth = 3, bool rgbSwizzle = false, int strideRGB = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1);
xxYUV_EXPORT void yuv2rgb_nv21_amx(int width, int height, const void* yuv, void* rgb, bool fullRange = true, int rgbWidth = 3, bool rgbSwizzle = false, int strideRGB = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1);
//------------------------------------------------------------------------------

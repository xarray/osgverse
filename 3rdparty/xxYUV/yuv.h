//==============================================================================
// xxYUV : yuv Header
//
// Copyright (c) 2020-2021 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================
#pragma once

#ifndef xxYUV_EXPORT
#define xxYUV_EXPORT
#endif

//------------------------------------------------------------------------------
#ifndef xxYUV_DEPRECATED
#include "yuv2yuva.h"
//------------------------------------------------------------------------------
inline void yuv_yu12_to_yuva(int width, int height, const void* input, void* output, bool yuvSwizzle = false, int strideOutput = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1)
{
    yuv2yuva_parameter parameter =
    {
        .width = width,
        .height = height,
        .y = input,
        .alignWidth = alignWidth,
        .alignHeight = alignHeight,
        .alignSize = alignSize,
        .output = output,
        .strideOutput = strideOutput,
        .swizzleOutput = yuvSwizzle,
    };
    yuv2yuva_yu12(&parameter);
}
//------------------------------------------------------------------------------
inline void yuv_yv12_to_yuva(int width, int height, const void* input, void* output, bool yuvSwizzle = false, int strideOutput = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1)
{
    yuv2yuva_parameter parameter =
    {
        .width = width,
        .height = height,
        .y = input,
        .alignWidth = alignWidth,
        .alignHeight = alignHeight,
        .alignSize = alignSize,
        .output = output,
        .strideOutput = strideOutput,
        .swizzleOutput = yuvSwizzle,
    };
    yuv2yuva_yv12(&parameter);
}
//------------------------------------------------------------------------------
inline void yuv_nv12_to_yuva(int width, int height, const void* input, void* output, bool yuvSwizzle = false, int strideOutput = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1)
{
    yuv2yuva_parameter parameter =
    {
        .width = width,
        .height = height,
        .y = input,
        .alignWidth = alignWidth,
        .alignHeight = alignHeight,
        .alignSize = alignSize,
        .output = output,
        .strideOutput = strideOutput,
        .swizzleOutput = yuvSwizzle,
    };
    yuv2yuva_nv12(&parameter);
}
//------------------------------------------------------------------------------
inline void yuv_nv21_to_yuva(int width, int height, const void* input, void* output, bool yuvSwizzle = false, int strideOutput = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1)
{
    yuv2yuva_parameter parameter =
    {
        .width = width,
        .height = height,
        .y = input,
        .alignWidth = alignWidth,
        .alignHeight = alignHeight,
        .alignSize = alignSize,
        .output = output,
        .strideOutput = strideOutput,
        .swizzleOutput = yuvSwizzle,
    };
    yuv2yuva_nv21(&parameter);
}
//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------

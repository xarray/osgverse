//==============================================================================
// xxYUV : yuv2rgb Header
//
// Copyright (c) 2020-2021 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================
#pragma once

#define xxYUV_DEPRECATED
#ifndef xxYUV_EXPORT
#define xxYUV_EXPORT
#endif

//------------------------------------------------------------------------------
typedef struct _yuv2rgb_parameter
{
    int width;
    int height;

    const void* y;
    const void* u;
    const void* v;
    int alignWidth;
    int alignHeight;
    int alignSize;
    int strideY;
    int strideU;
    int strideV;
    bool videoRange;

    void* rgb;
    int componentRGB;
    int strideRGB;
    bool swizzleRGB;
} yuv2rgb_parameter;
//------------------------------------------------------------------------------
xxYUV_EXPORT void yuv2rgb_yu12(const yuv2rgb_parameter* parameter);
xxYUV_EXPORT void yuv2rgb_yv12(const yuv2rgb_parameter* parameter);
xxYUV_EXPORT void yuv2rgb_nv12(const yuv2rgb_parameter* parameter);
xxYUV_EXPORT void yuv2rgb_nv21(const yuv2rgb_parameter* parameter);
//------------------------------------------------------------------------------
#ifndef xxYUV_DEPRECATED
//------------------------------------------------------------------------------
inline void yuv2rgb_yu12(int width, int height, const void* yuv, void* rgb, bool fullRange = true, int rgbWidth = 3, bool rgbSwizzle = false, int strideRGB = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1)
{
    yuv2rgb_parameter parameter =
    {
        .width = width,
        .height = height,
        .y = yuv,
        .alignWidth = alignWidth,
        .alignHeight = alignHeight,
        .alignSize = alignSize,
        .videoRange = !fullRange,
        .rgb = rgb,
        .componentRGB = rgbWidth,
        .strideRGB = strideRGB,
        .swizzleRGB = rgbSwizzle,
    };
    yuv2rgb_yu12(&parameter);
}
//------------------------------------------------------------------------------
inline void yuv2rgb_yv12(int width, int height, const void* yuv, void* rgb, bool fullRange = true, int rgbWidth = 3, bool rgbSwizzle = false, int strideRGB = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1)
{
    yuv2rgb_parameter parameter =
    {
        .width = width,
        .height = height,
        .y = yuv,
        .alignWidth = alignWidth,
        .alignHeight = alignHeight,
        .alignSize = alignSize,
        .videoRange = !fullRange,
        .rgb = rgb,
        .componentRGB = rgbWidth,
        .strideRGB = strideRGB,
        .swizzleRGB = rgbSwizzle,
    };
    yuv2rgb_yv12(&parameter);
}
//------------------------------------------------------------------------------
inline void yuv2rgb_nv12(int width, int height, const void* yuv, void* rgb, bool fullRange = true, int rgbWidth = 3, bool rgbSwizzle = false, int strideRGB = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1)
{
    yuv2rgb_parameter parameter =
    {
        .width = width,
        .height = height,
        .y = yuv,
        .alignWidth = alignWidth,
        .alignHeight = alignHeight,
        .alignSize = alignSize,
        .videoRange = !fullRange,
        .rgb = rgb,
        .componentRGB = rgbWidth,
        .strideRGB = strideRGB,
        .swizzleRGB = rgbSwizzle,
    };
    yuv2rgb_nv12(&parameter);
}
//------------------------------------------------------------------------------
inline void yuv2rgb_nv21(int width, int height, const void* yuv, void* rgb, bool fullRange = true, int rgbWidth = 3, bool rgbSwizzle = false, int strideRGB = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1)
{
    yuv2rgb_parameter parameter =
    {
        .width = width,
        .height = height,
        .y = yuv,
        .alignWidth = alignWidth,
        .alignHeight = alignHeight,
        .alignSize = alignSize,
        .videoRange = !fullRange,
        .rgb = rgb,
        .componentRGB = rgbWidth,
        .strideRGB = strideRGB,
        .swizzleRGB = rgbSwizzle,
    };
    yuv2rgb_nv21(&parameter);
}
//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------

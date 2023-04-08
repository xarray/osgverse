//==============================================================================
// xxYUV : rgb2yuv Header
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
typedef struct _rgb2yuv_parameter
{
    int width;
    int height;

    const void* rgb;
    int componentRGB;
    int strideRGB;
    bool swizzleRGB;

    void* y;
    void* u;
    void* v;
    int alignWidth;
    int alignHeight;
    int alignSize;
    int strideY;
    int strideU;
    int strideV;
    bool videoRange;
} rgb2yuv_parameter;
//------------------------------------------------------------------------------
xxYUV_EXPORT void rgb2yuv_yu12(const rgb2yuv_parameter* parameter);
xxYUV_EXPORT void rgb2yuv_yv12(const rgb2yuv_parameter* parameter);
xxYUV_EXPORT void rgb2yuv_nv12(const rgb2yuv_parameter* parameter);
xxYUV_EXPORT void rgb2yuv_nv21(const rgb2yuv_parameter* parameter);
//------------------------------------------------------------------------------
#ifndef xxYUV_DEPRECATED
//------------------------------------------------------------------------------
inline void rgb2yuv_yu12(int width, int height, const void* rgb, void* yuv, int rgbWidth = 3, bool rgbSwizzle = false, bool fullRange = true, int strideRGB = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1)
{
    rgb2yuv_parameter parameter =
    {
        .width = width,
        .height = height,
        .rgb = rgb,
        .componentRGB = rgbWidth,
        .strideRGB = strideRGB,
        .swizzleRGB = rgbSwizzle,
        .y = yuv,
        .alignWidth = alignWidth,
        .alignHeight = alignHeight,
        .alignSize = alignSize,
        .videoRange = !fullRange,
    };
    rgb2yuv_yu12(&parameter);
}
//------------------------------------------------------------------------------
inline void rgb2yuv_yv12(int width, int height, const void* rgb, void* yuv, int rgbWidth = 3, bool rgbSwizzle = false, bool fullRange = true, int strideRGB = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1)
{
    rgb2yuv_parameter parameter =
    {
        .width = width,
        .height = height,
        .rgb = rgb,
        .componentRGB = rgbWidth,
        .strideRGB = strideRGB,
        .swizzleRGB = rgbSwizzle,
        .y = yuv,
        .alignWidth = alignWidth,
        .alignHeight = alignHeight,
        .alignSize = alignSize,
        .videoRange = !fullRange,
    };
    rgb2yuv_yv12(&parameter);
}
//------------------------------------------------------------------------------
inline void rgb2yuv_nv12(int width, int height, const void* rgb, void* yuv, int rgbWidth = 3, bool rgbSwizzle = false, bool fullRange = true, int strideRGB = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1)
{
    rgb2yuv_parameter parameter =
    {
        .width = width,
        .height = height,
        .rgb = rgb,
        .componentRGB = rgbWidth,
        .strideRGB = strideRGB,
        .swizzleRGB = rgbSwizzle,
        .y = yuv,
        .alignWidth = alignWidth,
        .alignHeight = alignHeight,
        .alignSize = alignSize,
        .videoRange = !fullRange,
    };
    rgb2yuv_nv12(&parameter);
}
//------------------------------------------------------------------------------
inline void rgb2yuv_nv21(int width, int height, const void* rgb, void* yuv, int rgbWidth = 3, bool rgbSwizzle = false, bool fullRange = true, int strideRGB = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1)
{
    rgb2yuv_parameter parameter =
    {
        .width = width,
        .height = height,
        .rgb = rgb,
        .componentRGB = rgbWidth,
        .strideRGB = strideRGB,
        .swizzleRGB = rgbSwizzle,
        .y = yuv,
        .alignWidth = alignWidth,
        .alignHeight = alignHeight,
        .alignSize = alignSize,
        .videoRange = !fullRange,
    };
    rgb2yuv_nv21(&parameter);
}
//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------

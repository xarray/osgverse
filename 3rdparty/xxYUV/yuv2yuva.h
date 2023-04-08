//==============================================================================
// xxYUV : yuv2yuva Header
//
// Copyright (c) 2020-2021 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================
#pragma once

#ifndef xxYUV_EXPORT
#define xxYUV_EXPORT
#endif

//------------------------------------------------------------------------------
typedef struct _yuv2yuva_parameter
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

    void* output;
    int strideOutput;
    bool swizzleOutput;
} yuv2yuva_parameter;
//------------------------------------------------------------------------------
xxYUV_EXPORT void yuv2yuva_yu12(const yuv2yuva_parameter* parameter);
xxYUV_EXPORT void yuv2yuva_yv12(const yuv2yuva_parameter* parameter);
xxYUV_EXPORT void yuv2yuva_nv12(const yuv2yuva_parameter* parameter);
xxYUV_EXPORT void yuv2yuva_nv21(const yuv2yuva_parameter* parameter);
//------------------------------------------------------------------------------

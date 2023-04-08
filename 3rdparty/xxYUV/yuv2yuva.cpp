//==============================================================================
// xxYUV : yuv2yuva Source
//
// Copyright (c) 2020-2021 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================
#if defined(__llvm__)
#   pragma clang diagnostic ignored "-Wunused-variable"
#endif
#include "cpu.h"
#include "yuv2yuva.inl"
#include "yuv2yuva.h"

#define align(v, a) ((v) + ((a) - 1) & ~((a) - 1))

//------------------------------------------------------------------------------
void yuv2yuva_yu12(const yuv2yuva_parameter* parameter)
{
    int width = parameter->width;
    int height = parameter->height;

    const void* y = parameter->y;
    const void* u = parameter->u;
    const void* v = parameter->v;
    int alignWidth = parameter->alignWidth ? parameter->alignWidth : 16;
    int alignHeight = parameter->alignHeight ? parameter->alignHeight : 1;
    int alignSize = parameter->alignSize ? parameter->alignSize : 1;
    int strideY = parameter->strideY ? parameter->strideY : align(width, alignWidth);
    int strideU = parameter->strideU ? parameter->strideU : align(width, alignWidth) / 2;
    int strideV = parameter->strideV ? parameter->strideV : align(width, alignWidth) / 2;
    int sizeY = align(strideY * align(height, alignHeight), alignSize);
    int sizeU = align(strideU * align(height, alignHeight) / 2, alignSize);

    void* output = parameter->output;
    bool swizzleOutput = parameter->swizzleOutput;
    int strideOutput = parameter->strideOutput ? parameter->strideOutput : 4 * width;
    if (strideOutput < 0)
    {
        output = (char*)output - (strideOutput * (height - 1));
    }

    u = u ? u : (char*)y + sizeY;
    v = v ? v : (char*)y + sizeY + sizeU;

    void (*converter)(int width, int height, const void* y, const void* u, const void* v, int strideY, int strideU, int strideV, void* output, int strideOutput);

    if (swizzleOutput)
    {
        static auto select = yuv2yuva_select(false, false, 2, 1, 0, 3);
        converter = select;
    }
    else
    {
        static auto select = yuv2yuva_select(false, false, 0, 1, 2, 3);
        converter = select;
    }

    converter(width, height, y, u, v, strideY, strideU, strideV, output, strideOutput);
}
//------------------------------------------------------------------------------
void yuv2yuva_yv12(const yuv2yuva_parameter* parameter)
{
    int width = parameter->width;
    int height = parameter->height;

    const void* y = parameter->y;
    const void* u = parameter->u;
    const void* v = parameter->v;
    int alignWidth = parameter->alignWidth ? parameter->alignWidth : 16;
    int alignHeight = parameter->alignHeight ? parameter->alignHeight : 1;
    int alignSize = parameter->alignSize ? parameter->alignSize : 1;
    int strideY = parameter->strideY ? parameter->strideY : align(width, alignWidth);
    int strideU = parameter->strideU ? parameter->strideU : align(width, alignWidth) / 2;
    int strideV = parameter->strideV ? parameter->strideV : align(width, alignWidth) / 2;
    int sizeY = align(strideY * align(height, alignHeight), alignSize);
    int sizeU = align(strideU * align(height, alignHeight) / 2, alignSize);

    void* output = parameter->output;
    bool swizzleOutput = parameter->swizzleOutput;
    int strideOutput = parameter->strideOutput ? parameter->strideOutput : 4 * width;
    if (strideOutput < 0)
    {
        output = (char*)output - (strideOutput * (height - 1));
    }

    u = u ? u : (char*)y + sizeY + sizeU;
    v = v ? v : (char*)y + sizeY;

    void (*converter)(int width, int height, const void* y, const void* u, const void* v, int strideY, int strideU, int strideV, void* output, int strideOutput);

    if (swizzleOutput)
    {
        static auto select = yuv2yuva_select(false, false, 2, 1, 0, 3);
        converter = select;
    }
    else
    {
        static auto select = yuv2yuva_select(false, false, 0, 1, 2, 3);
        converter = select;
    }

    converter(width, height, y, u, v, strideY, strideU, strideV, output, strideOutput);
}
//------------------------------------------------------------------------------
void yuv2yuva_nv12(const yuv2yuva_parameter* parameter)
{
    int width = parameter->width;
    int height = parameter->height;

    const void* y = parameter->y;
    const void* u = parameter->u;
    const void* v = parameter->v;
    int alignWidth = parameter->alignWidth ? parameter->alignWidth : 16;
    int alignHeight = parameter->alignHeight ? parameter->alignHeight : 1;
    int alignSize = parameter->alignSize ? parameter->alignSize : 1;
    int strideY = parameter->strideY ? parameter->strideY : align(width, alignWidth);
    int sizeY = align(strideY * align(height, alignHeight), alignSize);
    int sizeUV = align(strideY * align(height, alignHeight) / 2, alignSize);

    void* output = parameter->output;
    bool swizzleOutput = parameter->swizzleOutput;
    int strideOutput = parameter->strideOutput ? parameter->strideOutput : 4 * width;
    if (strideOutput < 0)
    {
        output = (char*)output - (strideOutput * (height - 1));
    }

    u = u ? u : (char*)y + sizeY;
    v = v ? v : (char*)y + sizeY + 1;

    void (*converter)(int width, int height, const void* y, const void* u, const void* v, int strideY, int strideU, int strideV, void* output, int strideOutput);

    if (swizzleOutput)
    {
        static auto select = yuv2yuva_select(true, true, 2, 1, 0, 3);
        converter = select;
    }
    else
    {
        static auto select = yuv2yuva_select(true, true, 0, 1, 2, 3);
        converter = select;
    }

    converter(width, height, y, u, v, strideY, strideY, strideY, output, strideOutput);
}
//------------------------------------------------------------------------------
void yuv2yuva_nv21(const yuv2yuva_parameter* parameter)
{
    int width = parameter->width;
    int height = parameter->height;

    const void* y = parameter->y;
    const void* u = parameter->u;
    const void* v = parameter->v;
    int alignWidth = parameter->alignWidth ? parameter->alignWidth : 16;
    int alignHeight = parameter->alignHeight ? parameter->alignHeight : 1;
    int alignSize = parameter->alignSize ? parameter->alignSize : 1;
    int strideY = parameter->strideY ? parameter->strideY : align(width, alignWidth);
    int sizeY = align(strideY * align(height, alignHeight), alignSize);
    int sizeUV = align(strideY * align(height, alignHeight) / 2, alignSize);

    void* output = parameter->output;
    bool swizzleOutput = parameter->swizzleOutput;
    int strideOutput = parameter->strideOutput ? parameter->strideOutput : 4 * width;
    if (strideOutput < 0)
    {
        output = (char*)output - (strideOutput * (height - 1));
    }

    u = u ? u : (char*)y + sizeY + 1;
    v = v ? v : (char*)y + sizeY;

    void (*converter)(int width, int height, const void* y, const void* u, const void* v, int strideY, int strideU, int strideV, void* output, int strideOutput);

    if (swizzleOutput)
    {
        static auto select = yuv2yuva_select(true, false, 2, 1, 0, 3);
        converter = select;
    }
    else
    {
        static auto select = yuv2yuva_select(true, false, 0, 1, 2, 3);
        converter = select;
    }

    converter(width, height, y, u, v, strideY, strideY, strideY, output, strideOutput);
}
//------------------------------------------------------------------------------

//==============================================================================
// xxYUV : yuv2rgb Source
//
// Copyright (c) 2020-2021 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================
#if defined(__llvm__)
#   pragma clang diagnostic ignored "-Wunused-variable"
#endif
#include "cpu.h"
#include "yuv2rgb.inl"
#include "yuv2rgb.h"

#define align(v, a) ((v) + ((a) - 1) & ~((a) - 1))

//------------------------------------------------------------------------------
void yuv2rgb_yu12(const yuv2rgb_parameter* parameter)
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
    bool videoRange = parameter->videoRange;

    void* rgb = parameter->rgb;
    int componentRGB = parameter->componentRGB;
    int strideRGB = parameter->strideRGB ? parameter->strideRGB : componentRGB * width;
    bool swizzleRGB = parameter->swizzleRGB;
    if (strideRGB < 0)
    {
        rgb = (char*)rgb - (strideRGB * (height - 1));
    }

    u = u ? u : (char*)y + sizeY;
    v = v ? v : (char*)y + sizeY + sizeU;

    void (*converter)(int width, int height, const void* y, const void* u, const void* v, int strideY, int strideU, int strideV, void* rgb, int strideRGB);

    if (componentRGB == 3)
    {
        if (swizzleRGB)
        {
            if (videoRange)
            {
                static auto select = yuv2rgb_select(3, true, false, false, true);
                converter = select;
            }
            else
            {
                static auto select = yuv2rgb_select(3, true, false, false, false);
                converter = select;
            }
        }
        else
        {
            if (videoRange)
            {
                static auto select = yuv2rgb_select(3, false, false, false, true);
                converter = select;
            }
            else
            {
                static auto select = yuv2rgb_select(3, false, false, false, false);
                converter = select;
            }
        }
    }
    else if (componentRGB == 4)
    {
        if (swizzleRGB)
        {
            if (videoRange)
            {
                static auto select = yuv2rgb_select(4, true, false, false, true);
                converter = select;
            }
            else
            {
                static auto select = yuv2rgb_select(4, true, false, false, false);
                converter = select;
            }
        }
        else
        {
            if (videoRange)
            {
                static auto select = yuv2rgb_select(4, false, false, false, true);
                converter = select;
            }
            else
            {
                static auto select = yuv2rgb_select(4, false, false, false, false);
                converter = select;
            }
        }
    }
    else
    {
        return;
    }

    converter(width, height, y, u, v, strideY, strideU, strideU, rgb, strideRGB);
}
//------------------------------------------------------------------------------
void yuv2rgb_yv12(const yuv2rgb_parameter* parameter)
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
    bool videoRange = parameter->videoRange;

    void* rgb = parameter->rgb;
    int componentRGB = parameter->componentRGB;
    int strideRGB = parameter->strideRGB ? parameter->strideRGB : componentRGB * width;
    bool swizzleRGB = parameter->swizzleRGB;
    if (strideRGB < 0)
    {
        rgb = (char*)rgb - (strideRGB * (height - 1));
    }

    u = u ? u : (char*)y + sizeY + sizeU;
    v = v ? v : (char*)y + sizeY;

    void (*converter)(int width, int height, const void* y, const void* u, const void* v, int strideY, int strideU, int strideV, void* rgb, int strideRGB);

    if (componentRGB == 3)
    {
        if (swizzleRGB)
        {
            if (videoRange)
            {
                static auto select = yuv2rgb_select(3, true, false, false, true);
                converter = select;
            }
            else
            {
                static auto select = yuv2rgb_select(3, true, false, false, false);
                converter = select;
            }
        }
        else
        {
            if (videoRange)
            {
                static auto select = yuv2rgb_select(3, false, false, false, true);
                converter = select;
            }
            else
            {
                static auto select = yuv2rgb_select(3, false, false, false, false);
                converter = select;
            }
        }
    }
    else if (componentRGB == 4)
    {
        if (swizzleRGB)
        {
            if (videoRange)
            {
                static auto select = yuv2rgb_select(4, true, false, false, true);
                converter = select;
            }
            else
            {
                static auto select = yuv2rgb_select(4, true, false, false, false);
                converter = select;
            }
        }
        else
        {
            if (videoRange)
            {
                static auto select = yuv2rgb_select(4, false, false, false, true);
                converter = select;
            }
            else
            {
                static auto select = yuv2rgb_select(4, false, false, false, false);
                converter = select;
            }
        }
    }
    else
    {
        return;
    }

    converter(width, height, y, u, v, strideY, strideU, strideU, rgb, strideRGB);
}
//------------------------------------------------------------------------------
void yuv2rgb_nv12(const yuv2rgb_parameter* parameter)
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
    bool videoRange = parameter->videoRange;

    void* rgb = parameter->rgb;
    int componentRGB = parameter->componentRGB;
    int strideRGB = parameter->strideRGB ? parameter->strideRGB : componentRGB * width;
    bool swizzleRGB = parameter->swizzleRGB;
    if (strideRGB < 0)
    {
        rgb = (char*)rgb - (strideRGB * (height - 1));
    }

    u = u ? u : (char*)y + sizeY;
    v = v ? v : (char*)y + sizeY + 1;

    void (*converter)(int width, int height, const void* y, const void* u, const void* v, int strideY, int strideU, int strideV, void* rgb, int strideRGB);

    if (componentRGB == 3)
    {
        if (swizzleRGB)
        {
            if (videoRange)
            {
                static auto select = yuv2rgb_select(3, true, true, true, true);
                converter = select;
            }
            else
            {
                static auto select = yuv2rgb_select(3, true, true, true, false);
                converter = select;
            }
        }
        else
        {
            if (videoRange)
            {
                static auto select = yuv2rgb_select(3, false, true, true, true);
                converter = select;
            }
            else
            {
                static auto select = yuv2rgb_select(3, false, true, true, false);
                converter = select;
            }
        }
    }
    else if (componentRGB == 4)
    {
        if (swizzleRGB)
        {
            if (videoRange)
            {
                static auto select = yuv2rgb_select(4, true, true, true, true);
                converter = select;
            }
            else
            {
                static auto select = yuv2rgb_select(4, true, true, true, false);
                converter = select;
            }
        }
        else
        {
            if (videoRange)
            {
                static auto select = yuv2rgb_select(4, false, true, true, true);
                converter = select;
            }
            else
            {
                static auto select = yuv2rgb_select(4, false, true, true, false);
                converter = select;
            }
        }
    }
    else
    {
        return;
    }

    converter(width, height, y, u, v, strideY, strideY, strideY, rgb, strideRGB);
}
//------------------------------------------------------------------------------
void yuv2rgb_nv21(const yuv2rgb_parameter* parameter)
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
    bool videoRange = parameter->videoRange;

    void* rgb = parameter->rgb;
    int componentRGB = parameter->componentRGB;
    int strideRGB = parameter->strideRGB ? parameter->strideRGB : componentRGB * width;
    bool swizzleRGB = parameter->swizzleRGB;
    if (strideRGB < 0)
    {
        rgb = (char*)rgb - (strideRGB * (height - 1));
    }

    u = u ? u : (char*)y + sizeY + 1;
    v = v ? v : (char*)y + sizeY;

    void (*converter)(int width, int height, const void* y, const void* u, const void* v, int strideY, int strideU, int strideV, void* rgb, int strideRGB);

    if (componentRGB == 3)
    {
        if (swizzleRGB)
        {
            if (videoRange)
            {
                static auto select = yuv2rgb_select(3, true, true, false, true);
                converter = select;
            }
            else
            {
                static auto select = yuv2rgb_select(3, true, true, false, false);
                converter = select;
            }
        }
        else
        {
            if (videoRange)
            {
                static auto select = yuv2rgb_select(3, false, true, false, true);
                converter = select;
            }
            else
            {
                static auto select = yuv2rgb_select(3, false, true, false, false);
                converter = select;
            }
        }
    }
    else if (componentRGB == 4)
    {
        if (swizzleRGB)
        {
            if (videoRange)
            {
                static auto select = yuv2rgb_select(4, true, true, false, true);
                converter = select;
            }
            else
            {
                static auto select = yuv2rgb_select(4, true, true, false, false);
                converter = select;
            }
        }
        else
        {
            if (videoRange)
            {
                static auto select = yuv2rgb_select(4, false, true, false, true);
                converter = select;
            }
            else
            {
                static auto select = yuv2rgb_select(4, false, true, false, false);
                converter = select;
            }
        }
    }
    else
    {
        return;
    }

    converter(width, height, y, u, v, strideY, strideY, strideY, rgb, strideRGB);
}
//------------------------------------------------------------------------------

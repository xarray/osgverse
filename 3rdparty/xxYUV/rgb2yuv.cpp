//==============================================================================
// xxYUV : rgb2yuv Source
//
// Copyright (c) 2020-2021 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================
#if defined(__llvm__)
#   pragma clang diagnostic ignored "-Wunused-variable"
#endif
#include "cpu.h"
#include "rgb2yuv.inl"
#include "rgb2yuv.h"

#define align(v, a) ((v) + ((a) - 1) & ~((a) - 1))

//------------------------------------------------------------------------------
void rgb2yuv_yu12(const rgb2yuv_parameter* parameter)
{
    int width = parameter->width;
    int height = parameter->height;

    const void* rgb = parameter->rgb;
    int componentRGB = parameter->componentRGB;
    int strideRGB = parameter->strideRGB ? parameter->strideRGB : componentRGB * width;
    bool swizzleRGB = parameter->swizzleRGB;
    if (strideRGB < 0)
    {
        rgb = (char*)rgb - (strideRGB * (height - 1));
    }

    void* y = parameter->y;
    void* u = parameter->u;
    void* v = parameter->v;
    int alignWidth = parameter->alignWidth ? parameter->alignWidth : 16;
    int alignHeight = parameter->alignHeight ? parameter->alignHeight : 1;
    int alignSize = parameter->alignSize ? parameter->alignSize : 1;
    int strideY = parameter->strideY ? parameter->strideY : align(width, alignWidth);
    int strideU = parameter->strideU ? parameter->strideU : align(width, alignWidth) / 2;
    int strideV = parameter->strideV ? parameter->strideV : align(width, alignWidth) / 2;
    int sizeY = align(strideY * align(height, alignHeight), alignSize);
    int sizeU = align(strideU * align(height, alignHeight) / 2, alignSize);
    bool videoRange = parameter->videoRange;

    u = u ? u : (char*)y + sizeY;
    v = v ? v : (char*)y + sizeY + sizeU;

    void (*converter)(int width, int height, const void* rgb, int strideRGB, void* y, void* u, void* v, int strideY, int strideU, int strideV);

    if (componentRGB == 3)
    {
        if (swizzleRGB)
        {
            if (videoRange)
            {
                static auto select = rgb2yuv_select(3, true, false, false, true);
                converter = select;
            }
            else
            {
                static auto select = rgb2yuv_select(3, true, false, false, false);
                converter = select;
            }
        }
        else
        {
            if (videoRange)
            {
                static auto select = rgb2yuv_select(3, false, false, false, true);
                converter = select;
            }
            else
            {
                static auto select = rgb2yuv_select(3, false, false, false, false);
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
                static auto select = rgb2yuv_select(4, true, false, false, true);
                converter = select;
            }
            else
            {
                static auto select = rgb2yuv_select(4, true, false, false, false);
                converter = select;
            }
        }
        else
        {
            if (videoRange)
            {
                static auto select = rgb2yuv_select(4, false, false, false, true);
                converter = select;
            }
            else
            {
                static auto select = rgb2yuv_select(4, false, false, false, false);
                converter = select;
            }
        }
    }
    else
    {
        return;
    }

    converter(width, height, rgb, strideRGB, y, u, v, strideY, strideU, strideU);
}
//------------------------------------------------------------------------------
void rgb2yuv_yv12(const rgb2yuv_parameter* parameter)
{
    int width = parameter->width;
    int height = parameter->height;

    const void* rgb = parameter->rgb;
    int componentRGB = parameter->componentRGB;
    int strideRGB = parameter->strideRGB ? parameter->strideRGB : componentRGB * width;
    bool swizzleRGB = parameter->swizzleRGB;
    if (strideRGB < 0)
    {
        rgb = (char*)rgb - (strideRGB * (height - 1));
    }

    void* y = parameter->y;
    void* u = parameter->u;
    void* v = parameter->v;
    int alignWidth = parameter->alignWidth ? parameter->alignWidth : 16;
    int alignHeight = parameter->alignHeight ? parameter->alignHeight : 1;
    int alignSize = parameter->alignSize ? parameter->alignSize : 1;
    int strideY = parameter->strideY ? parameter->strideY : align(width, alignWidth);
    int strideU = parameter->strideU ? parameter->strideU : align(width, alignWidth) / 2;
    int strideV = parameter->strideV ? parameter->strideV : align(width, alignWidth) / 2;
    int sizeY = align(strideY * align(height, alignHeight), alignSize);
    int sizeU = align(strideU * align(height, alignHeight) / 2, alignSize);
    bool videoRange = parameter->videoRange;

    u = u ? u : (char*)y + sizeY + sizeU;
    v = v ? v : (char*)y + sizeY;

    void (*converter)(int width, int height, const void* rgb, int strideRGB, void* y, void* u, void* v, int strideY, int strideU, int strideV);

    if (componentRGB == 3)
    {
        if (swizzleRGB)
        {
            if (videoRange)
            {
                static auto select = rgb2yuv_select(3, true, false, false, true);
                converter = select;
            }
            else
            {
                static auto select = rgb2yuv_select(3, true, false, false, false);
                converter = select;
            }
        }
        else
        {
            if (videoRange)
            {
                static auto select = rgb2yuv_select(3, false, false, false, true);
                converter = select;
            }
            else
            {
                static auto select = rgb2yuv_select(3, false, false, false, false);
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
                static auto select = rgb2yuv_select(4, true, false, false, true);
                converter = select;
            }
            else
            {
                static auto select = rgb2yuv_select(4, true, false, false, false);
                converter = select;
            }
        }
        else
        {
            if (videoRange)
            {
                static auto select = rgb2yuv_select(4, false, false, false, true);
                converter = select;
            }
            else
            {
                static auto select = rgb2yuv_select(4, false, false, false, false);
                converter = select;
            }
        }
    }
    else
    {
        return;
    }

    converter(width, height, rgb, strideRGB, y, u, v, strideY, strideU, strideU);
}
//------------------------------------------------------------------------------
void rgb2yuv_nv12(const rgb2yuv_parameter* parameter)
{
    int width = parameter->width;
    int height = parameter->height;

    const void* rgb = parameter->rgb;
    int componentRGB = parameter->componentRGB;
    int strideRGB = parameter->strideRGB ? parameter->strideRGB : componentRGB * width;
    bool swizzleRGB = parameter->swizzleRGB;
    if (strideRGB < 0)
    {
        rgb = (char*)rgb - (strideRGB * (height - 1));
    }

    void* y = parameter->y;
    void* u = parameter->u;
    void* v = parameter->v;
    int alignWidth = parameter->alignWidth ? parameter->alignWidth : 16;
    int alignHeight = parameter->alignHeight ? parameter->alignHeight : 1;
    int alignSize = parameter->alignSize ? parameter->alignSize : 1;
    int strideY = parameter->strideY ? parameter->strideY : align(width, alignWidth);
    int sizeY = align(strideY * align(height, alignHeight), alignSize);
    int sizeUV = align(strideY * align(height, alignHeight) / 2, alignSize);
    bool videoRange = parameter->videoRange;

    u = u ? u : (char*)y + sizeY;
    v = v ? v : (char*)y + sizeY + 1;

    void (*converter)(int width, int height, const void* rgb, int strideRGB, void* y, void* u, void* v, int strideY, int strideU, int strideV);

    if (componentRGB == 3)
    {
        if (swizzleRGB)
        {
            if (videoRange)
            {
                static auto select = rgb2yuv_select(3, true, true, true, true);
                converter = select;
            }
            else
            {
                static auto select = rgb2yuv_select(3, true, true, true, false);
                converter = select;
            }
        }
        else
        {
            if (videoRange)
            {
                static auto select = rgb2yuv_select(3, false, true, true, true);
                converter = select;
            }
            else
            {
                static auto select = rgb2yuv_select(3, false, true, true, false);
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
                static auto select = rgb2yuv_select(4, true, true, true, true);
                converter = select;
            }
            else
            {
                static auto select = rgb2yuv_select(4, true, true, true, false);
                converter = select;
            }
        }
        else
        {
            if (videoRange)
            {
                static auto select = rgb2yuv_select(4, false, true, true, true);
                converter = select;
            }
            else
            {
                static auto select = rgb2yuv_select(4, false, true, true, false);
                converter = select;
            }
        }
    }
    else
    {
        return;
    }

    converter(width, height, rgb, strideRGB, y, u, v, strideY, strideY, strideY);
}
//------------------------------------------------------------------------------
void rgb2yuv_nv21(const rgb2yuv_parameter* parameter)
{
    int width = parameter->width;
    int height = parameter->height;

    const void* rgb = parameter->rgb;
    int componentRGB = parameter->componentRGB;
    int strideRGB = parameter->strideRGB ? parameter->strideRGB : componentRGB * width;
    bool swizzleRGB = parameter->swizzleRGB;
    if (strideRGB < 0)
    {
        rgb = (char*)rgb - (strideRGB * (height - 1));
    }

    void* y = parameter->y;
    void* u = parameter->u;
    void* v = parameter->v;
    int alignWidth = parameter->alignWidth ? parameter->alignWidth : 16;
    int alignHeight = parameter->alignHeight ? parameter->alignHeight : 1;
    int alignSize = parameter->alignSize ? parameter->alignSize : 1;
    int strideY = parameter->strideY ? parameter->strideY : align(width, alignWidth);
    int sizeY = align(strideY * align(height, alignHeight), alignSize);
    int sizeUV = align(strideY * align(height, alignHeight) / 2, alignSize);
    bool videoRange = parameter->videoRange;

    u = u ? u : (char*)y + sizeY + 1;
    v = v ? v : (char*)y + sizeY;

    void (*converter)(int width, int height, const void* rgb, int strideRGB, void* y, void* u, void* v, int strideY, int strideU, int strideV);

    if (componentRGB == 3)
    {
        if (swizzleRGB)
        {
            if (videoRange)
            {
                static auto select = rgb2yuv_select(3, true, true, false, true);
                converter = select;
            }
            else
            {
                static auto select = rgb2yuv_select(3, true, true, false, false);
                converter = select;
            }
        }
        else
        {
            if (videoRange)
            {
                static auto select = rgb2yuv_select(3, false, true, false, true);
                converter = select;
            }
            else
            {
                static auto select = rgb2yuv_select(3, false, true, false, false);
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
                static auto select = rgb2yuv_select(4, true, true, false, true);
                converter = select;
            }
            else
            {
                static auto select = rgb2yuv_select(4, true, true, false, false);
                converter = select;
            }
        }
        else
        {
            if (videoRange)
            {
                static auto select = rgb2yuv_select(4, false, true, false, true);
                converter = select;
            }
            else
            {
                static auto select = rgb2yuv_select(4, false, true, false, false);
                converter = select;
            }
        }
    }
    else
    {
        return;
    }

    converter(width, height, rgb, strideRGB, y, u, v, strideY, strideY, strideY);
}
//------------------------------------------------------------------------------

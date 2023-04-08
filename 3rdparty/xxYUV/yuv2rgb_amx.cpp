//==============================================================================
// xxYUV : yuv2rgb_amx Source
//
// Copyright (c) 2021 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================
#if defined(__llvm__)
#   pragma clang diagnostic ignored "-Wunused-variable"
#endif
#include "apple_amx.h"
#include "yuv2rgb.h"

#define align(v, a) ((v) + ((a) - 1) & ~((a) - 1))

// BT.709 - Video Range
//     Y         U         V
// R = 1.164384  0.000000  1.792741
// G = 1.164384 -0.213249 -0.532909
// B = 1.164384  2.112402  0.000000
//
// BT.709 - Full Range
//     Y         U         V
// R = 1.000000  0.000000  1.581000
// G = 1.000000 -0.188062 -0.469967
// B = 1.000000  1.862906  0.000000
#define vY   1.164384
#define vUG -0.213249
#define vUB  2.112402
#define vVR  1.792741
#define vVG -0.532909
#define fY   1.000000
#define fUG -0.188062
#define fUB  1.862906
#define fVR  1.581000
#define fVG -0.469967

//------------------------------------------------------------------------------
template<int rgbWidth, bool rgbSwizzle, bool interleaved, bool firstU, bool fullRange>
void yuv2rgb_amx(int width, int height, const void* y, const void* u, const void* v, int strideY, int strideU, int strideV, void* rgb, int strideRGB)
{
#if defined(__APPLE__) && defined(__aarch64__)
    if (strideRGB < 0)
    {
        rgb = (char*)rgb - (strideRGB * (height - 1));
    }

    int halfWidth = width >> 1;
    int halfHeight = height >> 1;

    int iR = rgbSwizzle ? 2 : 0;
    int iG = 1;
    int iB = rgbSwizzle ? 0 : 2;
    int iA = 3;

    static constexpr int16_t Y = fullRange ? (int)(fY * 256) : (int)(vY * 256);
    static constexpr int16_t UG = fullRange ? (int)(fUG * 255) : (int)(vUG * 255);
    static constexpr int16_t UB = fullRange ? (int)(fUB * 255) : (int)(vUB * 255);
    static constexpr int16_t VR = fullRange ? (int)(fVR * 255) : (int)(vVR * 255);
    static constexpr int16_t VG = fullRange ? (int)(fVG * 255) : (int)(vVG * 255);

    static constexpr int16_t vector256[32] = { [0 ... 31] = 256 };
    static constexpr int16_t vectorN128[32] = { [0 ... 31] = -128 };
    static constexpr int16_t vectorY[32] = { [0 ... 31] = (int16_t)(Y >> 1) };
    static constexpr int16_t vectorVR[32] = { [0 ... 31] = (int16_t)(VR >> 2) };
    static constexpr int16_t vectorUG[32] = { [0 ... 31] = (int16_t)(UG >> 1) };
    static constexpr int16_t vectorVG[32] = { [0 ... 31] = (int16_t)(VG >> 1) };
    static constexpr int16_t vectorUB[32] = { [0 ... 31] = (int16_t)(UB >> 3) };

    amx_set();
    amx_ldy( /*.memory_offset = */(uint64_t)vector256,  .register_index = 1 );
    amx_ldy( /*.memory_offset = */(uint64_t)vectorN128, .register_index = 2 );
    amx_ldy( /*.memory_offset = */(uint64_t)vectorY,    .register_index = 3 );
    amx_ldy( /*.memory_offset = */(uint64_t)vectorVR,   .register_index = 4 );
    amx_ldy( /*.memory_offset = */(uint64_t)vectorUG,   .register_index = 5 );
    amx_ldy( /*.memory_offset = */(uint64_t)vectorVG,   .register_index = 6 );
    amx_ldy( /*.memory_offset = */(uint64_t)vectorUB,   .register_index = 7 );
    for (int h = 0; h < halfHeight; ++h)
    {
        const unsigned char* y0 = (unsigned char*)y;
        const unsigned char* y1 = y0 + strideY;         y = y1 + strideY;
        const unsigned char* u0 = (unsigned char*)u;    u = u0 + strideU;
        const unsigned char* v0 = (unsigned char*)v;    v = v0 + strideV;
        unsigned char* rgb0 = (unsigned char*)rgb;
        unsigned char* rgb1 = rgb0 + strideRGB;         rgb = rgb1 + strideRGB;
        int halfWidth128 = width / 128;
        for (int w = 0; w < halfWidth128; ++w)
        {
            // Clear
            amx_mac16( .skip_x = 1, .skip_y = 1, .skip_z = 1, .mode_32 = 1 );

            // Load
            amx_ldx( /*.memory_offset = */(uint64_t)y0 +  0, .register_index = 0 );
            amx_ldx( /*.memory_offset = */(uint64_t)y0 + 64, .register_index = 1 ); y0 += 128;
            amx_ldx( /*.memory_offset = */(uint64_t)y1 +  0, .register_index = 2 );
            amx_ldx( /*.memory_offset = */(uint64_t)y1 + 64, .register_index = 3 ); y1 += 128;
            amx_ldx( /*.memory_offset = */(uint64_t)u0 +  0, .register_index = 4 ); u0 += 64;
            amx_ldx( /*.memory_offset = */(uint64_t)v0 +  0, .register_index = 5 ); v0 += 64;

            // Y
            amx_vecint( .offset_x = 0x000, .offset_y = 0x0C0, .offset_z = 32, .count_x = 1, .mask = 64, .extended = 11, .shift_right = 7 );
            amx_vecint( .offset_x = 0x000, .offset_y = 0x0C0, .offset_z = 40, .count_x = 1, .mask = 64, .extended = 11, .shift_right = 7 );
            amx_vecint( .offset_x = 0x000, .offset_y = 0x0C0, .offset_z = 48, .count_x = 1, .mask = 64, .extended = 11, .shift_right = 7 );
            amx_vecint( .offset_x = 0x040, .offset_y = 0x0C0, .offset_z = 34, .count_x = 1, .mask = 64, .extended = 11, .shift_right = 7 );
            amx_vecint( .offset_x = 0x040, .offset_y = 0x0C0, .offset_z = 42, .count_x = 1, .mask = 64, .extended = 11, .shift_right = 7 );
            amx_vecint( .offset_x = 0x040, .offset_y = 0x0C0, .offset_z = 50, .count_x = 1, .mask = 64, .extended = 11, .shift_right = 7 );
            amx_vecint( .offset_x = 0x080, .offset_y = 0x0C0, .offset_z = 36, .count_x = 1, .mask = 64, .extended = 11, .shift_right = 7 );
            amx_vecint( .offset_x = 0x080, .offset_y = 0x0C0, .offset_z = 44, .count_x = 1, .mask = 64, .extended = 11, .shift_right = 7 );
            amx_vecint( .offset_x = 0x080, .offset_y = 0x0C0, .offset_z = 52, .count_x = 1, .mask = 64, .extended = 11, .shift_right = 7 );
            amx_vecint( .offset_x = 0x0C0, .offset_y = 0x0C0, .offset_z = 38, .count_x = 1, .mask = 64, .extended = 11, .shift_right = 7 );
            amx_vecint( .offset_x = 0x0C0, .offset_y = 0x0C0, .offset_z = 46, .count_x = 1, .mask = 64, .extended = 11, .shift_right = 7 );
            amx_vecint( .offset_x = 0x0C0, .offset_y = 0x0C0, .offset_z = 54, .count_x = 1, .mask = 64, .extended = 11, .shift_right = 7 );

            // UV
            amx_vecint( .offset_x = 0x100, .offset_y = 0x080, .offset_z = 56, .count_x = 2, .extended = 12, .add = 1 );
            amx_vecint( .offset_x = 0x140, .offset_y = 0x080, .offset_z = 60, .count_x = 2, .extended = 12, .add = 1 );
            amx_extrx( .offset_x = 0x000, .offset_z = 56 );
            amx_extrx( .offset_x = 0x040, .offset_z = 57 );
            amx_extrx( .offset_x = 0x080, .offset_z = 58 );
            amx_extrx( .offset_x = 0x0C0, .offset_z = 59 );
            amx_extrx( .offset_x = 0x100, .offset_z = 60 );
            amx_extrx( .offset_x = 0x140, .offset_z = 61 );
            amx_extrx( .offset_x = 0x180, .offset_z = 62 );
            amx_extrx( .offset_x = 0x1C0, .offset_z = 63 );
            amx_vecint( .offset_x = 0x200 - 2, .offset_y = 0, .offset_z = 56, .add = 1 );
            amx_vecint( .offset_x = 0x040 - 2, .offset_y = 0, .offset_z = 57, .add = 1 );
            amx_vecint( .offset_x = 0x080 - 2, .offset_y = 0, .offset_z = 58, .add = 1 );
            amx_vecint( .offset_x = 0x0C0 - 2, .offset_y = 0, .offset_z = 59, .add = 1 );
            amx_vecint( .offset_x = 0x100 - 2, .offset_y = 0, .offset_z = 60, .add = 1 );
            amx_vecint( .offset_x = 0x140 - 2, .offset_y = 0, .offset_z = 61, .add = 1 );
            amx_vecint( .offset_x = 0x180 - 2, .offset_y = 0, .offset_z = 62, .add = 1 );
            amx_vecint( .offset_x = 0x1C0 - 2, .offset_y = 0, .offset_z = 63, .add = 1 );
            amx_extrx( .offset_x = 0x000, .offset_z = 56 );
            amx_extrx( .offset_x = 0x040, .offset_z = 57 );
            amx_extrx( .offset_x = 0x080, .offset_z = 58 );
            amx_extrx( .offset_x = 0x0C0, .offset_z = 59 );
            amx_extrx( .offset_x = 0x100, .offset_z = 60 );
            amx_extrx( .offset_x = 0x140, .offset_z = 61 );
            amx_extrx( .offset_x = 0x180, .offset_z = 62 );
            amx_extrx( .offset_x = 0x1C0, .offset_z = 63 );
            amx_vecint( .offset_x = 0x000, .offset_y = 0x140, .offset_z = 40, .shift_right = 7 );
            amx_vecint( .offset_x = 0x000, .offset_y = 0x140, .offset_z = 44, .shift_right = 7 );
            amx_vecint( .offset_x = 0x040, .offset_y = 0x140, .offset_z = 41, .shift_right = 7 );
            amx_vecint( .offset_x = 0x040, .offset_y = 0x140, .offset_z = 45, .shift_right = 7 );
            amx_vecint( .offset_x = 0x080, .offset_y = 0x140, .offset_z = 42, .shift_right = 7 );
            amx_vecint( .offset_x = 0x080, .offset_y = 0x140, .offset_z = 46, .shift_right = 7 );
            amx_vecint( .offset_x = 0x0C0, .offset_y = 0x140, .offset_z = 43, .shift_right = 7 );
            amx_vecint( .offset_x = 0x0C0, .offset_y = 0x140, .offset_z = 47, .shift_right = 7 );
            amx_vecint( .offset_x = 0x000, .offset_y = 0x1C0, .offset_z = 48, .shift_right = 5 );
            amx_vecint( .offset_x = 0x000, .offset_y = 0x1C0, .offset_z = 52, .shift_right = 5 );
            amx_vecint( .offset_x = 0x040, .offset_y = 0x1C0, .offset_z = 49, .shift_right = 5 );
            amx_vecint( .offset_x = 0x040, .offset_y = 0x1C0, .offset_z = 53, .shift_right = 5 );
            amx_vecint( .offset_x = 0x080, .offset_y = 0x1C0, .offset_z = 50, .shift_right = 5 );
            amx_vecint( .offset_x = 0x080, .offset_y = 0x1C0, .offset_z = 54, .shift_right = 5 );
            amx_vecint( .offset_x = 0x0C0, .offset_y = 0x1C0, .offset_z = 51, .shift_right = 5 );
            amx_vecint( .offset_x = 0x0C0, .offset_y = 0x1C0, .offset_z = 55, .shift_right = 5 );
            amx_vecint( .offset_x = 0x100, .offset_y = 0x100, .offset_z = 32, .shift_right = 6 );
            amx_vecint( .offset_x = 0x100, .offset_y = 0x100, .offset_z = 36, .shift_right = 6 );
            amx_vecint( .offset_x = 0x140, .offset_y = 0x100, .offset_z = 33, .shift_right = 6 );
            amx_vecint( .offset_x = 0x140, .offset_y = 0x100, .offset_z = 37, .shift_right = 6 );
            amx_vecint( .offset_x = 0x180, .offset_y = 0x100, .offset_z = 34, .shift_right = 6 );
            amx_vecint( .offset_x = 0x180, .offset_y = 0x100, .offset_z = 38, .shift_right = 6 );
            amx_vecint( .offset_x = 0x1C0, .offset_y = 0x100, .offset_z = 35, .shift_right = 6 );
            amx_vecint( .offset_x = 0x1C0, .offset_y = 0x100, .offset_z = 39, .shift_right = 6 );
            amx_vecint( .offset_x = 0x100, .offset_y = 0x180, .offset_z = 40, .shift_right = 7 );
            amx_vecint( .offset_x = 0x100, .offset_y = 0x180, .offset_z = 44, .shift_right = 7 );
            amx_vecint( .offset_x = 0x140, .offset_y = 0x180, .offset_z = 41, .shift_right = 7 );
            amx_vecint( .offset_x = 0x140, .offset_y = 0x180, .offset_z = 45, .shift_right = 7 );
            amx_vecint( .offset_x = 0x180, .offset_y = 0x180, .offset_z = 42, .shift_right = 7 );
            amx_vecint( .offset_x = 0x180, .offset_y = 0x180, .offset_z = 46, .shift_right = 7 );
            amx_vecint( .offset_x = 0x1C0, .offset_y = 0x180, .offset_z = 43, .shift_right = 7 );
            amx_vecint( .offset_x = 0x1C0, .offset_y = 0x180, .offset_z = 47, .shift_right = 7 );

            // RGBA
            if (iR == 0)
            {
                amx_extrx( .offset_x = 0x000, .offset_z = 32 );
                amx_extrx( .offset_x = 0x040, .offset_z = 33 );
                amx_extrx( .offset_x = 0x080, .offset_z = 34 );
                amx_extrx( .offset_x = 0x0C0, .offset_z = 35 );
                amx_extrx( .offset_x = 0x100, .offset_z = 36 );
                amx_extrx( .offset_x = 0x140, .offset_z = 37 );
                amx_extrx( .offset_x = 0x180, .offset_z = 38 );
                amx_extrx( .offset_x = 0x1C0, .offset_z = 39 );
            }
            else
            {
                amx_extrx( .offset_x = 0x000, .offset_z = 48 );
                amx_extrx( .offset_x = 0x040, .offset_z = 49 );
                amx_extrx( .offset_x = 0x080, .offset_z = 50 );
                amx_extrx( .offset_x = 0x0C0, .offset_z = 51 );
                amx_extrx( .offset_x = 0x100, .offset_z = 52 );
                amx_extrx( .offset_x = 0x140, .offset_z = 53 );
                amx_extrx( .offset_x = 0x180, .offset_z = 54 );
                amx_extrx( .offset_x = 0x1C0, .offset_z = 55 );
            }
            amx_vecint( .offset_x = 0x000, .offset_y = 0x000, .offset_z =  0, .count_x = 1, .count_y = 1, .extended = 10, .add = 1 );
            amx_vecint( .offset_x = 0x040, .offset_y = 0x000, .offset_z =  4, .count_x = 1, .count_y = 1, .extended = 10, .add = 1 );
            amx_vecint( .offset_x = 0x080, .offset_y = 0x000, .offset_z =  8, .count_x = 1, .count_y = 1, .extended = 10, .add = 1 );
            amx_vecint( .offset_x = 0x0C0, .offset_y = 0x000, .offset_z = 12, .count_x = 1, .count_y = 1, .extended = 10, .add = 1 );
            amx_vecint( .offset_x = 0x100, .offset_y = 0x000, .offset_z = 16, .count_x = 1, .count_y = 1, .extended = 10, .add = 1 );
            amx_vecint( .offset_x = 0x140, .offset_y = 0x000, .offset_z = 20, .count_x = 1, .count_y = 1, .extended = 10, .add = 1 );
            amx_vecint( .offset_x = 0x180, .offset_y = 0x000, .offset_z = 24, .count_x = 1, .count_y = 1, .extended = 10, .add = 1 );
            amx_vecint( .offset_x = 0x1C0, .offset_y = 0x000, .offset_z = 28, .count_x = 1, .count_y = 1, .extended = 10, .add = 1 );
            amx_extrx( .offset_x = 0x000, .offset_z = 40 );
            amx_extrx( .offset_x = 0x040, .offset_z = 41 );
            amx_extrx( .offset_x = 0x080, .offset_z = 42 );
            amx_extrx( .offset_x = 0x0C0, .offset_z = 43 );
            amx_extrx( .offset_x = 0x100, .offset_z = 44 );
            amx_extrx( .offset_x = 0x140, .offset_z = 45 );
            amx_extrx( .offset_x = 0x180, .offset_z = 46 );
            amx_extrx( .offset_x = 0x1C0, .offset_z = 47 );
            amx_vecint( .offset_x = 0x000, .offset_y = 0x040, .offset_z =  0, .count_x = 1, .extended = 12 );
            amx_vecint( .offset_x = 0x040, .offset_y = 0x040, .offset_z =  4, .count_x = 1, .extended = 12 );
            amx_vecint( .offset_x = 0x080, .offset_y = 0x040, .offset_z =  8, .count_x = 1, .extended = 12 );
            amx_vecint( .offset_x = 0x0C0, .offset_y = 0x040, .offset_z = 12, .count_x = 1, .extended = 12 );
            amx_vecint( .offset_x = 0x100, .offset_y = 0x040, .offset_z = 16, .count_x = 1, .extended = 12 );
            amx_vecint( .offset_x = 0x140, .offset_y = 0x040, .offset_z = 20, .count_x = 1, .extended = 12 );
            amx_vecint( .offset_x = 0x180, .offset_y = 0x040, .offset_z = 24, .count_x = 1, .extended = 12 );
            amx_vecint( .offset_x = 0x1C0, .offset_y = 0x040, .offset_z = 28, .count_x = 1, .extended = 12 );
            if (iB == 2)
            {
                amx_extrx( .offset_x = 0x000, .offset_z = 48 );
                amx_extrx( .offset_x = 0x040, .offset_z = 49 );
                amx_extrx( .offset_x = 0x080, .offset_z = 50 );
                amx_extrx( .offset_x = 0x0C0, .offset_z = 51 );
                amx_extrx( .offset_x = 0x100, .offset_z = 52 );
                amx_extrx( .offset_x = 0x140, .offset_z = 53 );
                amx_extrx( .offset_x = 0x180, .offset_z = 54 );
                amx_extrx( .offset_x = 0x1C0, .offset_z = 55 );
            }
            else
            {
                amx_extrx( .offset_x = 0x000, .offset_z = 32 );
                amx_extrx( .offset_x = 0x040, .offset_z = 33 );
                amx_extrx( .offset_x = 0x080, .offset_z = 34 );
                amx_extrx( .offset_x = 0x0C0, .offset_z = 35 );
                amx_extrx( .offset_x = 0x100, .offset_z = 36 );
                amx_extrx( .offset_x = 0x140, .offset_z = 37 );
                amx_extrx( .offset_x = 0x180, .offset_z = 38 );
                amx_extrx( .offset_x = 0x1C0, .offset_z = 39 );
            }
            amx_vecint( .offset_x = 0x1E0, .offset_y = 0x040, .offset_z =  0, .count_x = 1, .mask = 1, .extended = 6, .shift_right = 8 );
            amx_vecint( .offset_x = 0x000, .offset_y = 0x040, .offset_z =  1, .count_x = 1, .mask = 1, .extended = 6, .shift_right = 8 );
            amx_vecint( .offset_x = 0x020, .offset_y = 0x040, .offset_z =  4, .count_x = 1, .mask = 1, .extended = 6, .shift_right = 8 );
            amx_vecint( .offset_x = 0x040, .offset_y = 0x040, .offset_z =  5, .count_x = 1, .mask = 1, .extended = 6, .shift_right = 8 );
            amx_vecint( .offset_x = 0x060, .offset_y = 0x040, .offset_z =  8, .count_x = 1, .mask = 1, .extended = 6, .shift_right = 8 );
            amx_vecint( .offset_x = 0x080, .offset_y = 0x040, .offset_z =  9, .count_x = 1, .mask = 1, .extended = 6, .shift_right = 8 );
            amx_vecint( .offset_x = 0x0A0, .offset_y = 0x040, .offset_z = 12, .count_x = 1, .mask = 1, .extended = 6, .shift_right = 8 );
            amx_vecint( .offset_x = 0x0C0, .offset_y = 0x040, .offset_z = 13, .count_x = 1, .mask = 1, .extended = 6, .shift_right = 8 );
            amx_vecint( .offset_x = 0x0E0, .offset_y = 0x040, .offset_z = 16, .count_x = 1, .mask = 1, .extended = 6, .shift_right = 8 );
            amx_vecint( .offset_x = 0x100, .offset_y = 0x040, .offset_z = 17, .count_x = 1, .mask = 1, .extended = 6, .shift_right = 8 );
            amx_vecint( .offset_x = 0x120, .offset_y = 0x040, .offset_z = 20, .count_x = 1, .mask = 1, .extended = 6, .shift_right = 8 );
            amx_vecint( .offset_x = 0x140, .offset_y = 0x040, .offset_z = 21, .count_x = 1, .mask = 1, .extended = 6, .shift_right = 8 );
            amx_vecint( .offset_x = 0x160, .offset_y = 0x040, .offset_z = 24, .count_x = 1, .mask = 1, .extended = 6, .shift_right = 8 );
            amx_vecint( .offset_x = 0x180, .offset_y = 0x040, .offset_z = 25, .count_x = 1, .mask = 1, .extended = 6, .shift_right = 8 );
            amx_vecint( .offset_x = 0x1A0, .offset_y = 0x040, .offset_z = 28, .count_x = 1, .mask = 1, .extended = 6, .shift_right = 8 );
            amx_vecint( .offset_x = 0x1C0, .offset_y = 0x040, .offset_z = 29, .count_x = 1, .mask = 1, .extended = 6, .shift_right = 8 );

            // Store
            amx_stz( /*.memory_offset = */(uint64_t)rgb0 +   0, .register_index =  0 );
            amx_stz( /*.memory_offset = */(uint64_t)rgb0 +  64, .register_index =  1 );
            amx_stz( /*.memory_offset = */(uint64_t)rgb0 + 128, .register_index =  4 );
            amx_stz( /*.memory_offset = */(uint64_t)rgb0 + 192, .register_index =  5 );
            amx_stz( /*.memory_offset = */(uint64_t)rgb0 + 256, .register_index =  8 );
            amx_stz( /*.memory_offset = */(uint64_t)rgb0 + 320, .register_index =  9 );
            amx_stz( /*.memory_offset = */(uint64_t)rgb0 + 384, .register_index = 12 );
            amx_stz( /*.memory_offset = */(uint64_t)rgb0 + 448, .register_index = 13 ); rgb0 += 128 * 4;
            amx_stz( /*.memory_offset = */(uint64_t)rgb1 +   0, .register_index = 16 );
            amx_stz( /*.memory_offset = */(uint64_t)rgb1 +  64, .register_index = 17 );
            amx_stz( /*.memory_offset = */(uint64_t)rgb1 + 128, .register_index = 20 );
            amx_stz( /*.memory_offset = */(uint64_t)rgb1 + 192, .register_index = 21 );
            amx_stz( /*.memory_offset = */(uint64_t)rgb1 + 256, .register_index = 24 );
            amx_stz( /*.memory_offset = */(uint64_t)rgb1 + 320, .register_index = 25 );
            amx_stz( /*.memory_offset = */(uint64_t)rgb1 + 384, .register_index = 28 );
            amx_stz( /*.memory_offset = */(uint64_t)rgb1 + 448, .register_index = 29 ); rgb1 += 128 * 4;
        }
    }
    amx_clr();
#endif
}
//------------------------------------------------------------------------------
void yyy2rgb_amx(int width, int height, const void* y, int strideY, void* rgb, int strideRGB)
{
#if defined(__APPLE__) && defined(__aarch64__)
    int halfWidth = width >> 1;
    int halfHeight = height >> 1;

    static constexpr int16_t vector257[32] = { [0 ... 31] = 257 };

    amx_set();
    amx_ldy( .memory_offset = (uint64_t)vector257, .register_index = 1 );
    for (int h = 0; h < halfHeight; ++h)
    {
        const unsigned char* y0 = (unsigned char*)y;
        const unsigned char* y1 = y0 + strideY;         y = y1 + strideY;
        unsigned char* rgb0 = (unsigned char*)rgb;
        unsigned char* rgb1 = rgb0 + strideRGB;         rgb = rgb1 + strideRGB;
        int halfWidth128 = width / 128;
        for (int w = 0; w < halfWidth128; ++w)
        {
            // Clear
            amx_mac16( .skip_x = 1, .skip_y = 1, .skip_z = 1, .mode_32 = 1 );

            // Load
            amx_ldx( .memory_offset = (uint64_t)y0 +  0, .register_index = 0 );
            amx_ldx( .memory_offset = (uint64_t)y0 + 64, .register_index = 2 );   y0 += 128;
            amx_ldx( .memory_offset = (uint64_t)y1 +  0, .register_index = 4 );
            amx_ldx( .memory_offset = (uint64_t)y1 + 64, .register_index = 6 );   y1 += 128;

            // Y
            amx_vecint( .offset_x = 0x000, .offset_y = 0x040, .offset_z =  0, .count_x = 2, .extended = 12 );
            amx_vecint( .offset_x = 0x080, .offset_y = 0x040, .offset_z =  8, .count_x = 2, .extended = 12 );
            amx_vecint( .offset_x = 0x100, .offset_y = 0x040, .offset_z = 16, .count_x = 2, .extended = 12 );
            amx_vecint( .offset_x = 0x180, .offset_y = 0x040, .offset_z = 24, .count_x = 2, .extended = 12 );
            amx_vecint( .offset_x = 0x000, .offset_y = 0x000, .offset_z =  2, .count_x = 2, .extended = 11, .add = 1 );
            amx_vecint( .offset_x = 0x080, .offset_y = 0x000, .offset_z = 10, .count_x = 2, .extended = 11, .add = 1 );
            amx_vecint( .offset_x = 0x100, .offset_y = 0x000, .offset_z = 18, .count_x = 2, .extended = 11, .add = 1 );
            amx_vecint( .offset_x = 0x180, .offset_y = 0x000, .offset_z = 26, .count_x = 2, .extended = 11, .add = 1 );
            amx_vecint( .offset_x = 0x1E0, .offset_y = 0x000, .offset_z =  0, .count_x = 2, .extended = 11, .add = 1 );
            amx_vecint( .offset_x = 0x060, .offset_y = 0x000, .offset_z =  8, .count_x = 2, .extended = 11, .add = 1 );
            amx_vecint( .offset_x = 0x0E0, .offset_y = 0x000, .offset_z = 16, .count_x = 2, .extended = 11, .add = 1 );
            amx_vecint( .offset_x = 0x160, .offset_y = 0x000, .offset_z = 24, .count_x = 2, .extended = 11, .add = 1 );
            amx_vecint( .offset_x = 0x1E0, .offset_y = 0x000, .offset_z =  2, .count_x = 2, .extended = 12, .neg = 1, .add = 1 );
            amx_vecint( .offset_x = 0x060, .offset_y = 0x000, .offset_z = 10, .count_x = 2, .extended = 12, .neg = 1, .add = 1 );
            amx_vecint( .offset_x = 0x0E0, .offset_y = 0x000, .offset_z = 18, .count_x = 2, .extended = 12, .neg = 1, .add = 1 );
            amx_vecint( .offset_x = 0x160, .offset_y = 0x000, .offset_z = 26, .count_x = 2, .extended = 12, .neg = 1, .add = 1 );

            // Store
            amx_stz( .memory_offset = (uint64_t)rgb0 +   0, .register_index =  0 );
            amx_stz( .memory_offset = (uint64_t)rgb0 +  64, .register_index =  1 );
            amx_stz( .memory_offset = (uint64_t)rgb0 + 128, .register_index =  2 );
            amx_stz( .memory_offset = (uint64_t)rgb0 + 192, .register_index =  3 );
            amx_stz( .memory_offset = (uint64_t)rgb0 + 256, .register_index =  8 );
            amx_stz( .memory_offset = (uint64_t)rgb0 + 320, .register_index =  9 );
            amx_stz( .memory_offset = (uint64_t)rgb0 + 384, .register_index = 10 );
            amx_stz( .memory_offset = (uint64_t)rgb0 + 448, .register_index = 11 );   rgb0 += 128 * 4;
            amx_stz( .memory_offset = (uint64_t)rgb1 +   0, .register_index = 16 );
            amx_stz( .memory_offset = (uint64_t)rgb1 +  64, .register_index = 17 );
            amx_stz( .memory_offset = (uint64_t)rgb1 + 128, .register_index = 18 );
            amx_stz( .memory_offset = (uint64_t)rgb1 + 192, .register_index = 19 );
            amx_stz( .memory_offset = (uint64_t)rgb1 + 256, .register_index = 24 );
            amx_stz( .memory_offset = (uint64_t)rgb1 + 320, .register_index = 25 );
            amx_stz( .memory_offset = (uint64_t)rgb1 + 384, .register_index = 26 );
            amx_stz( .memory_offset = (uint64_t)rgb1 + 448, .register_index = 27 );   rgb1 += 128 * 4;
        }
    }
    amx_clr();
#endif
}
//------------------------------------------------------------------------------
void yuv2rgb_yu12_amx(int width, int height, const void* yuv, void* rgb, bool fullRange, int rgbWidth, bool rgbSwizzle, int strideRGB, int alignWidth, int alignHeight, int alignSize)
{
    int strideY = align(width, alignWidth);
    int strideU = align(width, alignWidth) / 2;
    int sizeY = align(strideY * align(height, alignHeight), alignSize);
    int sizeU = align(strideU * align(height, alignHeight) / 2, alignSize);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    auto converter = yuv2rgb_amx<3, false, false, false, false>;

    if (rgbWidth == 3)
    {
        if (rgbSwizzle)
        {
            if (fullRange)
                converter = yuv2rgb_amx<3, true, false, false, true>;
            else
                converter = yuv2rgb_amx<3, true, false, false, false>;
        }
        else
        {
            if (fullRange)
                converter = yuv2rgb_amx<3, false, false, false, true>;
            else
                converter = yuv2rgb_amx<3, false, false, false, false>;
        }
    }
    else if (rgbWidth == 4)
    {
        if (rgbSwizzle)
        {
            if (fullRange)
                converter = yuv2rgb_amx<4, true, false, false, true>;
            else
                converter = yuv2rgb_amx<4, true, false, false, false>;
        }
        else
        {
            if (fullRange)
                converter = yuv2rgb_amx<4, false, false, false, true>;
            else
                converter = yuv2rgb_amx<4, false, false, false, false>;
        }
    }

    converter(width, height, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + sizeU, strideY, strideU, strideU, rgb, strideRGB);
}
//------------------------------------------------------------------------------
void yuv2rgb_yv12_amx(int width, int height, const void* yuv, void* rgb, bool fullRange, int rgbWidth, bool rgbSwizzle, int strideRGB, int alignWidth, int alignHeight, int alignSize)
{
    int strideY = align(width, alignWidth);
    int strideU = align(width, alignWidth) / 2;
    int sizeY = align(strideY * align(height, alignHeight), alignSize);
    int sizeU = align(strideU * align(height, alignHeight) / 2, alignSize);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    auto converter = yuv2rgb_amx<3, false, false, false, false>;

    if (rgbWidth == 3)
    {
        if (rgbSwizzle)
        {
            if (fullRange)
                converter = yuv2rgb_amx<3, true, false, false, true>;
            else
                converter = yuv2rgb_amx<3, true, false, false, false>;
        }
        else
        {
            if (fullRange)
                converter = yuv2rgb_amx<3, false, false, false, true>;
            else
                converter = yuv2rgb_amx<3, false, false, false, false>;
        }
    }
    else if (rgbWidth == 4)
    {
        if (rgbSwizzle)
        {
            if (fullRange)
                converter = yuv2rgb_amx<4, true, false, false, true>;
            else
                converter = yuv2rgb_amx<4, true, false, false, false>;
        }
        else
        {
            if (fullRange)
                converter = yuv2rgb_amx<4, false, false, false, true>;
            else
                converter = yuv2rgb_amx<4, false, false, false, false>;
        }
    }

    converter(width, height, yuv, (char*)yuv + sizeY + sizeU, (char*)yuv + sizeY, strideY, strideU, strideU, rgb, strideRGB);
}
//------------------------------------------------------------------------------
void yuv2rgb_nv12_amx(int width, int height, const void* yuv, void* rgb, bool fullRange, int rgbWidth, bool rgbSwizzle, int strideRGB, int alignWidth, int alignHeight, int alignSize)
{
    int strideYUV = align(width, alignWidth);
    int sizeY = align(strideYUV * align(height, alignHeight), alignSize);
    int sizeUV = align(strideYUV * align(height, alignHeight) / 2, alignSize);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    auto converter = yuv2rgb_amx<3, false, false, false, false>;

    if (rgbWidth == 3)
    {
        if (rgbSwizzle)
        {
            if (fullRange)
                converter = yuv2rgb_amx<3, true, true, true, true>;
            else
                converter = yuv2rgb_amx<3, true, true, true, false>;
        }
        else
        {
            if (fullRange)
                converter = yuv2rgb_amx<3, false, true, true, true>;
            else
                converter = yuv2rgb_amx<3, false, true, true, false>;
        }
    }
    else if (rgbWidth == 4)
    {
        if (rgbSwizzle)
        {
            if (fullRange)
                converter = yuv2rgb_amx<4, true, true, true, true>;
            else
                converter = yuv2rgb_amx<4, true, true, true, false>;
        }
        else
        {
            if (fullRange)
                converter = yuv2rgb_amx<4, false, true, true, true>;
            else
                converter = yuv2rgb_amx<4, false, true, true, false>;
        }
    }

    converter(width, height, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + 1, strideYUV, strideYUV, strideYUV, rgb, strideRGB);
}
//------------------------------------------------------------------------------
void yuv2rgb_nv21_amx(int width, int height, const void* yuv, void* rgb, bool fullRange, int rgbWidth, bool rgbSwizzle, int strideRGB, int alignWidth, int alignHeight, int alignSize)
{
    int strideYUV = align(width, alignWidth);
    int sizeY = align(strideYUV * align(height, alignHeight), alignSize);
    int sizeUV = align(strideYUV * align(height, alignHeight) / 2, alignSize);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    auto converter = yuv2rgb_amx<3, false, false, false, false>;

    if (rgbWidth == 3)
    {
        if (rgbSwizzle)
        {
            if (fullRange)
                converter = yuv2rgb_amx<3, true, true, false, true>;
            else
                converter = yuv2rgb_amx<3, true, true, false, false>;
        }
        else
        {
            if (fullRange)
                converter = yuv2rgb_amx<3, false, true, false, true>;
            else
                converter = yuv2rgb_amx<3, false, true, false, false>;
        }
    }
    else if (rgbWidth == 4)
    {
        if (rgbSwizzle)
        {
            if (fullRange)
                converter = yuv2rgb_amx<4, true, true, false, true>;
            else
                converter = yuv2rgb_amx<4, true, true, false, false>;
        }
        else
        {
            if (fullRange)
                converter = yuv2rgb_amx<4, false, true, false, true>;
            else
                converter = yuv2rgb_amx<4, false, true, false, false>;
        }
    }

    converter(width, height, yuv, (char*)yuv + sizeY + 1, (char*)yuv + sizeY, strideYUV, strideYUV, strideYUV, rgb, strideRGB);
}
//------------------------------------------------------------------------------

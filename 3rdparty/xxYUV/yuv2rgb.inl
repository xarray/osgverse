//==============================================================================
// xxYUV : yuv2rgb Inline
//
// Copyright (c) 2020-2021 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================
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

#if defined(__ARM_NEON__) || defined(__ARM_NEON) || defined(_M_ARM) || defined(_M_ARM64) || defined(_M_HYBRID_X86_ARM64)
#   include <arm_neon.h>
#   define NEON_FAST 1
#elif defined(_M_IX86) || defined(_M_AMD64) || defined(__i386__) || defined(__amd64__)
#   include <immintrin.h>
#   if defined(__llvm__)
#       include <avxintrin.h>
#       include <avx2intrin.h>
#   endif
#   define _MM_TRANSPOSE4_EPI8(R0, R1, R2, R3) {    \
        __m128i T0, T1, T2, T3;                     \
        T0 = _mm_unpacklo_epi8(R0, R1);             \
        T1 = _mm_unpacklo_epi8(R2, R3);             \
        T2 = _mm_unpackhi_epi8(R0, R1);             \
        T3 = _mm_unpackhi_epi8(R2, R3);             \
        R0 = _mm_unpacklo_epi16(T0, T1);            \
        R1 = _mm_unpackhi_epi16(T0, T1);            \
        R2 = _mm_unpacklo_epi16(T2, T3);            \
        R3 = _mm_unpackhi_epi16(T2, T3);            \
    }
#   define _MM256_TRANSPOSE4_EPI8(R0, R1, R2, R3) { \
        __m256i T0, T1, T2, T3;                     \
        T0 = _mm256_unpacklo_epi8(R0, R1);          \
        T1 = _mm256_unpacklo_epi8(R2, R3);          \
        T2 = _mm256_unpackhi_epi8(R0, R1);          \
        T3 = _mm256_unpackhi_epi8(R2, R3);          \
        R0 = _mm256_unpacklo_epi16(T0, T1);         \
        R1 = _mm256_unpackhi_epi16(T0, T1);         \
        R2 = _mm256_unpacklo_epi16(T2, T3);         \
        R3 = _mm256_unpackhi_epi16(T2, T3);         \
    }
#   define _MM256_TRANSPOSE4_SI128(R0, R1, R2, R3) {\
        __m256i T0, T1, T2, T3;                     \
        T0 = _mm256_permute2x128_si256(R0, R1, 32); \
        T1 = _mm256_permute2x128_si256(R0, R1, 49); \
        T2 = _mm256_permute2x128_si256(R2, R3, 32); \
        T3 = _mm256_permute2x128_si256(R2, R3, 49); \
        R0 = T0;                                    \
        R2 = T1;                                    \
        R1 = T2;                                    \
        R3 = T3;                                    \
    }
#   define _MM512_TRANSPOSE4_EPI8(R0, R1, R2, R3) { \
        __m512i T0, T1, T2, T3;                     \
        T0 = _mm512_unpacklo_epi8(R0, R1);          \
        T1 = _mm512_unpacklo_epi8(R2, R3);          \
        T2 = _mm512_unpackhi_epi8(R0, R1);          \
        T3 = _mm512_unpackhi_epi8(R2, R3);          \
        R0 = _mm512_unpacklo_epi16(T0, T1);         \
        R1 = _mm512_unpackhi_epi16(T0, T1);         \
        R2 = _mm512_unpacklo_epi16(T2, T3);         \
        R3 = _mm512_unpackhi_epi16(T2, T3);         \
    }
#   define _MM512_TRANSPOSE4_SI128(R0, R1, R2, R3) {\
        __m512i T0, T1, T2, T3;                     \
        T0 = _mm512_shuffle_i32x4(R0, R1, 0x44);    \
        T1 = _mm512_shuffle_i32x4(R2, R3, 0x44);    \
        T2 = _mm512_shuffle_i32x4(R0, R1, 0xEE);    \
        T3 = _mm512_shuffle_i32x4(R2, R3, 0xEE);    \
        R0 = _mm512_shuffle_i32x4(T0, T1, 0x88);    \
        R1 = _mm512_shuffle_i32x4(T0, T1, 0xDD);    \
        R2 = _mm512_shuffle_i32x4(T2, T3, 0x88);    \
        R3 = _mm512_shuffle_i32x4(T2, T3, 0xDD);    \
    }
#endif

//------------------------------------------------------------------------------
template<int componentRGB, bool swizzleRGB, bool interleaved, bool firstU, bool videoRange>
void yuv2rgb(int width, int height, const void* y, const void* u, const void* v, int strideY, int strideU, int strideV, void* rgb, int strideRGB)
{
    int halfWidth = width >> 1;
    int halfHeight = height >> 1;

    int iR = swizzleRGB ? 2 : 0;
    int iG = 1;
    int iB = swizzleRGB ? 0 : 2;
    int iA = 3;

    int Y, UG, UB, VR, VG;
    if (videoRange)
    {
        Y = (int)(vY * 256);
        UG = (int)(vUG * 255); UB = (int)(vUB * 255);
        VR = (int)(vVR * 255); VG = (int)(vVG * 255);
    }
    else
    {
        Y = (int)(fY * 256);
        UG = (int)(fUG * 255); UB = (int)(fUB * 255);
        VR = (int)(fVR * 255); VG = (int)(fVG * 255);
    }

    for (int h = 0; h < halfHeight; ++h)
    {
        const unsigned char* y0 = (unsigned char*)y;
        const unsigned char* y1 = y0 + strideY;         y = y1 + strideY;
        const unsigned char* u0 = (unsigned char*)u;    u = u0 + strideU;
        const unsigned char* v0 = (unsigned char*)v;    v = v0 + strideV;
        unsigned char* rgb0 = (unsigned char*)rgb;
        unsigned char* rgb1 = rgb0 + strideRGB;         rgb = rgb1 + strideRGB;
#if HAVE_NEON
        int halfWidth8 = (componentRGB == 4) ? halfWidth / 8 : 0;
        for (int w = 0; w < halfWidth8; ++w)
        {
            uint8x16_t y00lh = vld1q_u8(y0); y0 += 16;
            uint8x16_t y10lh = vld1q_u8(y1); y1 += 16;
            uint8x8_t y00;
            uint8x8_t y01;
            uint8x8_t y10;
            uint8x8_t y11;
            if (videoRange)
            {
                y00lh = vqsubq_u8(y00lh, vdupq_n_u8(16));
                y10lh = vqsubq_u8(y10lh, vdupq_n_u8(16));
                y00 = vshrn_n_u16(vmull_u8(vget_low_u8(y00lh), vdup_n_u8(Y >> 1)), 7);
                y01 = vshrn_n_u16(vmull_u8(vget_high_u8(y00lh), vdup_n_u8(Y >> 1)), 7);
                y10 = vshrn_n_u16(vmull_u8(vget_low_u8(y10lh), vdup_n_u8(Y >> 1)), 7);
                y11 = vshrn_n_u16(vmull_u8(vget_high_u8(y10lh), vdup_n_u8(Y >> 1)), 7);
            }
            else
            {
                y00 = vget_low_u8(y00lh);
                y01 = vget_high_u8(y00lh);
                y10 = vget_low_u8(y10lh);
                y11 = vget_high_u8(y10lh);
            }

            int8x8_t u000;
            int8x8_t v000;
            if (interleaved)
            {
                if (firstU)
                {
                    int8x16_t uv00 = vld1q_u8(u0); u0 += 16;
                    int8x8x2_t uv00lh = vuzp_s8(vget_low_s8(uv00), vget_high_s8(uv00));
                    int8x16_t uv000 = vaddq_s8(vcombine_s8(uv00lh.val[0], uv00lh.val[1]), vdupq_n_s8(-128));
                    u000 = vget_low_s8(uv000);
                    v000 = vget_high_s8(uv000);
                }
                else
                {
                    int8x16_t uv00 = vld1q_u8(v0); v0 += 16;
                    int8x8x2_t uv00lh = vuzp_s8(vget_low_s8(uv00), vget_high_s8(uv00));
                    int8x16_t uv000 = vaddq_s8(vcombine_s8(uv00lh.val[1], uv00lh.val[0]), vdupq_n_s8(-128));
                    u000 = vget_low_s8(uv000);
                    v000 = vget_high_s8(uv000);
                }
            }
            else
            {
                int8x16_t uv000 = vaddq_s8(vcombine_s8(vld1_u8(u0), vld1_u8(v0)), vdupq_n_s8(-128)); u0 += 8; v0 += 8;
                u000 = vget_low_s8(uv000);
                v000 = vget_high_s8(uv000);
            }

#if NEON_FAST
            int16x8_t dR = vshrq_n_s16(                                    vmull_s8(v000, vdup_n_s8(VR >> 2)), 6);
            int16x8_t dG = vshrq_n_s16(vmlal_s8(vmull_s8(u000, vdup_n_s8(UG >> 1)), v000, vdup_n_s8(VG >> 1)), 7);
            int16x8_t dB = vshrq_n_s16(         vmull_s8(u000, vdup_n_s8(UB >> 3)),                            5);
#else
            int16x8_t u00 = vshll_n_s8(u000, 7);
            int16x8_t v00 = vshll_n_s8(v000, 7);

            int16x8_t dR =                                               vqdmulhq_s16(v00, vdupq_n_s16(VR));
            int16x8_t dG = vaddq_s16(vqdmulhq_s16(u00, vdupq_n_s16(UG)), vqdmulhq_s16(v00, vdupq_n_s16(VG)));
            int16x8_t dB =           vqdmulhq_s16(u00, vdupq_n_s16(UB));
#endif

            uint16x8x2_t xR = vzipq_u16(vreinterpretq_u16_s16(dR), vreinterpretq_u16_s16(dR));
            uint16x8x2_t xG = vzipq_u16(vreinterpretq_u16_s16(dG), vreinterpretq_u16_s16(dG));
            uint16x8x2_t xB = vzipq_u16(vreinterpretq_u16_s16(dB), vreinterpretq_u16_s16(dB));

            uint8x16x4_t t;
            uint8x16x4_t b;

            t.val[iR] = vcombine_u8(vqmovun_s16(vaddw_u8(xR.val[0], y00)), vqmovun_s16(vaddw_u8(xR.val[1], y01)));
            t.val[iG] = vcombine_u8(vqmovun_s16(vaddw_u8(xG.val[0], y00)), vqmovun_s16(vaddw_u8(xG.val[1], y01)));
            t.val[iB] = vcombine_u8(vqmovun_s16(vaddw_u8(xB.val[0], y00)), vqmovun_s16(vaddw_u8(xB.val[1], y01)));
            t.val[iA] = vdupq_n_u8(255);
            b.val[iR] = vcombine_u8(vqmovun_s16(vaddw_u8(xR.val[0], y10)), vqmovun_s16(vaddw_u8(xR.val[1], y11)));
            b.val[iG] = vcombine_u8(vqmovun_s16(vaddw_u8(xG.val[0], y10)), vqmovun_s16(vaddw_u8(xG.val[1], y11)));
            b.val[iB] = vcombine_u8(vqmovun_s16(vaddw_u8(xB.val[0], y10)), vqmovun_s16(vaddw_u8(xB.val[1], y11)));
            b.val[iA] = vdupq_n_u8(255);

            vst4q_u8(rgb0, t);  rgb0 += 16 * 4;
            vst4q_u8(rgb1, b);  rgb1 += 16 * 4;
        }
        if (componentRGB == 4)
            continue;
#elif HAVE_AVX512
        int halfWidth16 = (componentRGB == 4) ? halfWidth / 32 : 0;
        for (int w = 0; w < halfWidth16; ++w)
        {
            __m512i y00lh = _mm512_loadu_si512((__m512i*)y0); y0 += 64;
            __m512i y10lh = _mm512_loadu_si512((__m512i*)y1); y1 += 64;
            __m512i y00;
            __m512i y01;
            __m512i y10;
            __m512i y11;
            if (videoRange)
            {
                y00lh = _mm512_subs_epu8(y00lh, _mm512_set1_epi8(16));
                y10lh = _mm512_subs_epu8(y10lh, _mm512_set1_epi8(16));
                y00 = _mm512_mulhi_epu16(_mm512_unpacklo_epi8(__m512i(), y00lh), _mm512_set1_epi16(Y));
                y01 = _mm512_mulhi_epu16(_mm512_unpackhi_epi8(__m512i(), y00lh), _mm512_set1_epi16(Y));
                y10 = _mm512_mulhi_epu16(_mm512_unpacklo_epi8(__m512i(), y10lh), _mm512_set1_epi16(Y));
                y11 = _mm512_mulhi_epu16(_mm512_unpackhi_epi8(__m512i(), y10lh), _mm512_set1_epi16(Y));
            }
            else
            {
                y00 = _mm512_unpacklo_epi8(y00lh, __m512i());
                y01 = _mm512_unpackhi_epi8(y00lh, __m512i());
                y10 = _mm512_unpacklo_epi8(y10lh, __m512i());
                y11 = _mm512_unpackhi_epi8(y10lh, __m512i());
            }

            __m512i u00;
            __m512i v00;
            if (interleaved)
            {
                if (firstU)
                {
                    __m512i uv00 = _mm512_loadu_si512((__m512i*)u0); u0 += 64;
                    uv00 = _mm512_sub_epi8(uv00, _mm512_set1_epi8(-128));
                    u00 = _mm512_slli_epi16(uv00, 8);
                    v00 = uv00;
                }
                else
                {
                    __m512i uv00 = _mm512_loadu_si512((__m512i*)v0); v0 += 64;
                    uv00 = _mm512_sub_epi8(uv00, _mm512_set1_epi8(-128));
                    u00 = uv00;
                    v00 = _mm512_slli_epi16(uv00, 8);
                }
            }
            else
            {
                __m256i u000 = _mm256_loadu_si256((__m256i*)u0); u0 += 32;
                __m256i v000 = _mm256_loadu_si256((__m256i*)v0); v0 += 32;
                u000 = _mm256_sub_epi8(u000, _mm256_set1_epi8(-128));
                v000 = _mm256_sub_epi8(v000, _mm256_set1_epi8(-128));
                u00 = _mm512_slli_epi16(_mm512_cvtepi8_epi16(u000), 8);
                v00 = _mm512_slli_epi16(_mm512_cvtepi8_epi16(v000), 8);
            }

            __m512i dR =                                                                  _mm512_mulhi_epi16(v00, _mm512_set1_epi16(VR));
            __m512i dG = _mm512_add_epi16(_mm512_mulhi_epi16(u00, _mm512_set1_epi16(UG)), _mm512_mulhi_epi16(v00, _mm512_set1_epi16(VG)));
            __m512i dB =                  _mm512_mulhi_epi16(u00, _mm512_set1_epi16(UB));

            __m512i xR[2] = { _mm512_unpacklo_epi16(dR, dR), _mm512_unpackhi_epi16(dR, dR) };
            __m512i xG[2] = { _mm512_unpacklo_epi16(dG, dG), _mm512_unpackhi_epi16(dG, dG) };
            __m512i xB[2] = { _mm512_unpacklo_epi16(dB, dB), _mm512_unpackhi_epi16(dB, dB) };

            __m512i t[4];
            __m512i b[4];

            t[iR] = _mm512_packus_epi16(_mm512_add_epi16(y00, xR[0]), _mm512_add_epi16(y01, xR[1]));
            t[iG] = _mm512_packus_epi16(_mm512_add_epi16(y00, xG[0]), _mm512_add_epi16(y01, xG[1]));
            t[iB] = _mm512_packus_epi16(_mm512_add_epi16(y00, xB[0]), _mm512_add_epi16(y01, xB[1]));
            t[iA] = _mm512_set1_epi8(-1);
            b[iR] = _mm512_packus_epi16(_mm512_add_epi16(y10, xR[0]), _mm512_add_epi16(y11, xR[1]));
            b[iG] = _mm512_packus_epi16(_mm512_add_epi16(y10, xG[0]), _mm512_add_epi16(y11, xG[1]));
            b[iB] = _mm512_packus_epi16(_mm512_add_epi16(y10, xB[0]), _mm512_add_epi16(y11, xB[1]));
            b[iA] = _mm512_set1_epi8(-1);

            _MM512_TRANSPOSE4_EPI8(t[0], t[1], t[2], t[3]);
            _MM512_TRANSPOSE4_EPI8(b[0], b[1], b[2], b[3]);
            _MM512_TRANSPOSE4_SI128(t[0], t[1], t[2], t[3]);
            _MM512_TRANSPOSE4_SI128(b[0], b[1], b[2], b[3]);

            _mm512_storeu_si512((__m512i*)rgb0 + 0, t[0]);
            _mm512_storeu_si512((__m512i*)rgb0 + 1, t[1]);
            _mm512_storeu_si512((__m512i*)rgb0 + 2, t[2]);
            _mm512_storeu_si512((__m512i*)rgb0 + 3, t[3]); rgb0 += 16 * 16;
            _mm512_storeu_si512((__m512i*)rgb1 + 0, b[0]);
            _mm512_storeu_si512((__m512i*)rgb1 + 1, b[1]);
            _mm512_storeu_si512((__m512i*)rgb1 + 2, b[2]);
            _mm512_storeu_si512((__m512i*)rgb1 + 3, b[3]); rgb1 += 16 * 16;
        }
        if (componentRGB == 4)
            continue;
#elif HAVE_AVX2
        int halfWidth16 = (componentRGB == 4) ? halfWidth / 16 : 0;
        for (int w = 0; w < halfWidth16; ++w)
        {
            __m256i y00lh = _mm256_loadu_si256((__m256i*)y0); y0 += 32;
            __m256i y10lh = _mm256_loadu_si256((__m256i*)y1); y1 += 32;
            __m256i y00;
            __m256i y01;
            __m256i y10;
            __m256i y11;
            if (videoRange)
            {
                y00lh = _mm256_subs_epu8(y00lh, _mm256_set1_epi8(16));
                y10lh = _mm256_subs_epu8(y10lh, _mm256_set1_epi8(16));
                y00 = _mm256_mulhi_epu16(_mm256_unpacklo_epi8(__m256i(), y00lh), _mm256_set1_epi16(Y));
                y01 = _mm256_mulhi_epu16(_mm256_unpackhi_epi8(__m256i(), y00lh), _mm256_set1_epi16(Y));
                y10 = _mm256_mulhi_epu16(_mm256_unpacklo_epi8(__m256i(), y10lh), _mm256_set1_epi16(Y));
                y11 = _mm256_mulhi_epu16(_mm256_unpackhi_epi8(__m256i(), y10lh), _mm256_set1_epi16(Y));
            }
            else
            {
                y00 = _mm256_unpacklo_epi8(y00lh, __m256i());
                y01 = _mm256_unpackhi_epi8(y00lh, __m256i());
                y10 = _mm256_unpacklo_epi8(y10lh, __m256i());
                y11 = _mm256_unpackhi_epi8(y10lh, __m256i());
            }

            __m256i u00;
            __m256i v00;
            if (interleaved)
            {
                if (firstU)
                {
                    __m256i uv00 = _mm256_loadu_si256((__m256i*)u0); u0 += 32;
                    uv00 = _mm256_sub_epi8(uv00, _mm256_set1_epi8(-128));
                    u00 = _mm256_slli_epi16(uv00, 8);
                    v00 = uv00;
                }
                else
                {
                    __m256i uv00 = _mm256_loadu_si256((__m256i*)v0); v0 += 32;
                    uv00 = _mm256_sub_epi8(uv00, _mm256_set1_epi8(-128));
                    u00 = uv00;
                    v00 = _mm256_slli_epi16(uv00, 8);
                }
            }
            else
            {
                __m128i u000 = _mm_loadu_si128((__m128i*)u0); u0 += 16;
                __m128i v000 = _mm_loadu_si128((__m128i*)v0); v0 += 16;
                u000 = _mm_sub_epi8(u000, _mm_set1_epi8(-128));
                v000 = _mm_sub_epi8(v000, _mm_set1_epi8(-128));
                u00 = _mm256_slli_epi16(_mm256_cvtepi8_epi16(u000), 8);
                v00 = _mm256_slli_epi16(_mm256_cvtepi8_epi16(v000), 8);
            }

            __m256i dR =                                                                  _mm256_mulhi_epi16(v00, _mm256_set1_epi16(VR));
            __m256i dG = _mm256_add_epi16(_mm256_mulhi_epi16(u00, _mm256_set1_epi16(UG)), _mm256_mulhi_epi16(v00, _mm256_set1_epi16(VG)));
            __m256i dB =                  _mm256_mulhi_epi16(u00, _mm256_set1_epi16(UB));

            __m256i xR[2] = { _mm256_unpacklo_epi16(dR, dR), _mm256_unpackhi_epi16(dR, dR) };
            __m256i xG[2] = { _mm256_unpacklo_epi16(dG, dG), _mm256_unpackhi_epi16(dG, dG) };
            __m256i xB[2] = { _mm256_unpacklo_epi16(dB, dB), _mm256_unpackhi_epi16(dB, dB) };

            __m256i t[4];
            __m256i b[4];

            t[iR] = _mm256_packus_epi16(_mm256_add_epi16(y00, xR[0]), _mm256_add_epi16(y01, xR[1]));
            t[iG] = _mm256_packus_epi16(_mm256_add_epi16(y00, xG[0]), _mm256_add_epi16(y01, xG[1]));
            t[iB] = _mm256_packus_epi16(_mm256_add_epi16(y00, xB[0]), _mm256_add_epi16(y01, xB[1]));
            t[iA] = _mm256_set1_epi8(-1);
            b[iR] = _mm256_packus_epi16(_mm256_add_epi16(y10, xR[0]), _mm256_add_epi16(y11, xR[1]));
            b[iG] = _mm256_packus_epi16(_mm256_add_epi16(y10, xG[0]), _mm256_add_epi16(y11, xG[1]));
            b[iB] = _mm256_packus_epi16(_mm256_add_epi16(y10, xB[0]), _mm256_add_epi16(y11, xB[1]));
            b[iA] = _mm256_set1_epi8(-1);

            _MM256_TRANSPOSE4_EPI8(t[0], t[1], t[2], t[3]);
            _MM256_TRANSPOSE4_EPI8(b[0], b[1], b[2], b[3]);
            _MM256_TRANSPOSE4_SI128(t[0], t[1], t[2], t[3]);
            _MM256_TRANSPOSE4_SI128(b[0], b[1], b[2], b[3]);

            _mm256_storeu_si256((__m256i*)rgb0 + 0, t[0]);
            _mm256_storeu_si256((__m256i*)rgb0 + 1, t[1]);
            _mm256_storeu_si256((__m256i*)rgb0 + 2, t[2]);
            _mm256_storeu_si256((__m256i*)rgb0 + 3, t[3]); rgb0 += 16 * 8;
            _mm256_storeu_si256((__m256i*)rgb1 + 0, b[0]);
            _mm256_storeu_si256((__m256i*)rgb1 + 1, b[1]);
            _mm256_storeu_si256((__m256i*)rgb1 + 2, b[2]);
            _mm256_storeu_si256((__m256i*)rgb1 + 3, b[3]); rgb1 += 16 * 8;
        }
        if (componentRGB == 4)
            continue;
#elif HAVE_SSE2
        int halfWidth8 = (componentRGB == 4) ? halfWidth / 8 : 0;
        for (int w = 0; w < halfWidth8; ++w)
        {
            __m128i y00lh = _mm_loadu_si128((__m128i*)y0); y0 += 16;
            __m128i y10lh = _mm_loadu_si128((__m128i*)y1); y1 += 16;
            __m128i y00;
            __m128i y01;
            __m128i y10;
            __m128i y11;
            if (videoRange)
            {
                y00lh = _mm_subs_epu8(y00lh, _mm_set1_epi8(16));
                y10lh = _mm_subs_epu8(y10lh, _mm_set1_epi8(16));
                y00 = _mm_mulhi_epu16(_mm_unpacklo_epi8(__m128i(), y00lh), _mm_set1_epi16(Y));
                y01 = _mm_mulhi_epu16(_mm_unpackhi_epi8(__m128i(), y00lh), _mm_set1_epi16(Y));
                y10 = _mm_mulhi_epu16(_mm_unpacklo_epi8(__m128i(), y10lh), _mm_set1_epi16(Y));
                y11 = _mm_mulhi_epu16(_mm_unpackhi_epi8(__m128i(), y10lh), _mm_set1_epi16(Y));
            }
            else
            {
                y00 = _mm_unpacklo_epi8(y00lh, __m128i());
                y01 = _mm_unpackhi_epi8(y00lh, __m128i());
                y10 = _mm_unpacklo_epi8(y10lh, __m128i());
                y11 = _mm_unpackhi_epi8(y10lh, __m128i());
            }

            __m128i u00;
            __m128i v00;
            if (interleaved)
            {
                if (firstU)
                {
                    __m128i uv00 = _mm_loadu_si128((__m128i*)u0); u0 += 16;
                    uv00 = _mm_sub_epi8(uv00, _mm_set1_epi8(-128));
                    u00 = _mm_slli_epi16(uv00, 8);
                    v00 = uv00;
                }
                else
                {
                    __m128i uv00 = _mm_loadu_si128((__m128i*)v0); v0 += 16;
                    uv00 = _mm_sub_epi8(uv00, _mm_set1_epi8(-128));
                    u00 = uv00;
                    v00 = _mm_slli_epi16(uv00, 8);
                }
            }
            else
            {
                __m128i u000 = _mm_loadl_epi64((__m128i*)u0); u0 += 8;
                __m128i v000 = _mm_loadl_epi64((__m128i*)v0); v0 += 8;
                u000 = _mm_sub_epi8(u000, _mm_set1_epi8(-128));
                v000 = _mm_sub_epi8(v000, _mm_set1_epi8(-128));
                u00 = _mm_unpacklo_epi8(__m128i(), u000);
                v00 = _mm_unpacklo_epi8(__m128i(), v000);
            }

            __m128i dR =                                                         _mm_mulhi_epi16(v00, _mm_set1_epi16(VR));
            __m128i dG = _mm_add_epi16(_mm_mulhi_epi16(u00, _mm_set1_epi16(UG)), _mm_mulhi_epi16(v00, _mm_set1_epi16(VG)));
            __m128i dB =               _mm_mulhi_epi16(u00, _mm_set1_epi16(UB));

            __m128i xR[2] = { _mm_unpacklo_epi16(dR, dR), _mm_unpackhi_epi16(dR, dR) };
            __m128i xG[2] = { _mm_unpacklo_epi16(dG, dG), _mm_unpackhi_epi16(dG, dG) };
            __m128i xB[2] = { _mm_unpacklo_epi16(dB, dB), _mm_unpackhi_epi16(dB, dB) };

            __m128i t[4];
            __m128i b[4];

            t[iR] = _mm_packus_epi16(_mm_add_epi16(y00, xR[0]), _mm_add_epi16(y01, xR[1]));
            t[iG] = _mm_packus_epi16(_mm_add_epi16(y00, xG[0]), _mm_add_epi16(y01, xG[1]));
            t[iB] = _mm_packus_epi16(_mm_add_epi16(y00, xB[0]), _mm_add_epi16(y01, xB[1]));
            t[iA] = _mm_set1_epi8(-1);
            b[iR] = _mm_packus_epi16(_mm_add_epi16(y10, xR[0]), _mm_add_epi16(y11, xR[1]));
            b[iG] = _mm_packus_epi16(_mm_add_epi16(y10, xG[0]), _mm_add_epi16(y11, xG[1]));
            b[iB] = _mm_packus_epi16(_mm_add_epi16(y10, xB[0]), _mm_add_epi16(y11, xB[1]));
            b[iA] = _mm_set1_epi8(-1);

            _MM_TRANSPOSE4_EPI8(t[0], t[1], t[2], t[3]);
            _MM_TRANSPOSE4_EPI8(b[0], b[1], b[2], b[3]);

            _mm_storeu_si128((__m128i*)rgb0 + 0, t[0]);
            _mm_storeu_si128((__m128i*)rgb0 + 1, t[1]);
            _mm_storeu_si128((__m128i*)rgb0 + 2, t[2]);
            _mm_storeu_si128((__m128i*)rgb0 + 3, t[3]); rgb0 += 16 * 4;
            _mm_storeu_si128((__m128i*)rgb1 + 0, b[0]);
            _mm_storeu_si128((__m128i*)rgb1 + 1, b[1]);
            _mm_storeu_si128((__m128i*)rgb1 + 2, b[2]);
            _mm_storeu_si128((__m128i*)rgb1 + 3, b[3]); rgb1 += 16 * 4;
        }
        if (componentRGB == 4)
            continue;
#endif
        for (int w = 0; w < halfWidth; ++w)
        {
            int y00 = (*y0++);
            int y01 = (*y0++);
            int y10 = (*y1++);
            int y11 = (*y1++);
            if (videoRange)
            {
                y00 = ((y00 - 16) * Y) >> 8;
                y01 = ((y01 - 16) * Y) >> 8;
                y10 = ((y10 - 16) * Y) >> 8;
                y11 = ((y11 - 16) * Y) >> 8;
            }

            int u00 = (*u0++) - 128;
            int v00 = (*v0++) - 128;
            if (interleaved)
            {
                u0++;
                v0++;
            }

            int dR = (           v00 * VR) >> 8;
            int dG = (u00 * UG + v00 * VG) >> 8;
            int dB = (u00 * UB           ) >> 8;

            auto clamp = [](int value) -> unsigned char
            {
                return (unsigned char)(value < 255 ? value < 0 ? 0 : value : 255);
            };

            if (componentRGB >= 1) rgb0[iR] = clamp(y00 + dR);
            if (componentRGB >= 2) rgb0[iG] = clamp(y00 + dG);
            if (componentRGB >= 3) rgb0[iB] = clamp(y00 + dB);
            if (componentRGB >= 4) rgb0[iA] = 255;
            rgb0 += componentRGB;

            if (componentRGB >= 1) rgb0[iR] = clamp(y01 + dR);
            if (componentRGB >= 2) rgb0[iG] = clamp(y01 + dG);
            if (componentRGB >= 3) rgb0[iB] = clamp(y01 + dB);
            if (componentRGB >= 4) rgb0[iA] = 255;
            rgb0 += componentRGB;

            if (componentRGB >= 1) rgb1[iR] = clamp(y10 + dR);
            if (componentRGB >= 2) rgb1[iG] = clamp(y10 + dG);
            if (componentRGB >= 3) rgb1[iB] = clamp(y10 + dB);
            if (componentRGB >= 4) rgb1[iA] = 255;
            rgb1 += componentRGB;

            if (componentRGB >= 1) rgb1[iR] = clamp(y11 + dR);
            if (componentRGB >= 2) rgb1[iG] = clamp(y11 + dG);
            if (componentRGB >= 3) rgb1[iB] = clamp(y11 + dB);
            if (componentRGB >= 4) rgb1[iA] = 255;
            rgb1 += componentRGB;
        }
    }
}
//------------------------------------------------------------------------------
#ifndef yuv2rgb_select
#define yuv2rgb_select(componentRGB, swizzleRGB, interleaved, firstU, videoRange) \
    yuv2rgb<componentRGB, swizzleRGB, interleaved, firstU, videoRange>
#endif
//------------------------------------------------------------------------------
#ifndef yuv2rgb
//------------------------------------------------------------------------------
#if defined(__llvm__)
#define rgb2yuv_attribute(value) __attribute__((target(value)))
#else
#define rgb2yuv_attribute(value)
#endif
//------------------------------------------------------------------------------
#if defined(__ARM_NEON__) || defined(__ARM_NEON) || defined(_M_ARM) || defined(_M_ARM64) || defined(_M_HYBRID_X86_ARM64)
#define HAVE_NEON 1
#define yuv2rgb rgb2yuv_attribute("neon") yuv2rgb_neon
#include "yuv2rgb.inl"
#undef yuv2rgb
#undef HAVE_NEON
#undef yuv2rgb_select
#define yuv2rgb_select(componentRGB, swizzleRGB, interleaved, firstU, videoRange) \
    neon() ? yuv2rgb_neon<componentRGB, swizzleRGB, interleaved, firstU, videoRange> : \
    yuv2rgb<componentRGB, swizzleRGB, interleaved, firstU, videoRange>
#endif
//------------------------------------------------------------------------------
#if defined(_M_IX86) || defined(_M_AMD64) || defined(__i386__) || defined(__amd64__)
#define HAVE_SSE2 1
#define yuv2rgb rgb2yuv_attribute("sse2") yuv2rgb_sse2
#include "yuv2rgb.inl"
#undef yuv2rgb
#undef HAVE_SSE2
#undef yuv2rgb_select
#define yuv2rgb_select(componentRGB, swizzleRGB, interleaved, firstU, videoRange) \
    sse2() ? yuv2rgb_sse2<componentRGB, swizzleRGB, interleaved, firstU, videoRange> : \
    yuv2rgb<componentRGB, swizzleRGB, interleaved, firstU, videoRange>
#endif
//------------------------------------------------------------------------------
#if defined(_M_IX86) || defined(_M_AMD64) || defined(__i386__) || defined(__amd64__)
#define HAVE_AVX2 1
#define yuv2rgb rgb2yuv_attribute("avx2") yuv2rgb_avx2
#include "yuv2rgb.inl"
#undef yuv2rgb
#undef HAVE_AVX2
#undef yuv2rgb_select
#define yuv2rgb_select(componentRGB, swizzleRGB, interleaved, firstU, videoRange) \
    avx2() ? yuv2rgb_avx2<componentRGB, swizzleRGB, interleaved, firstU, videoRange> : \
    sse2() ? yuv2rgb_sse2<componentRGB, swizzleRGB, interleaved, firstU, videoRange> : \
    yuv2rgb<componentRGB, swizzleRGB, interleaved, firstU, videoRange>
#endif
//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------

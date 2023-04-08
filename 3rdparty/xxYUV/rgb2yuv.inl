//==============================================================================
// xxYUV : rgb2yuv Inline
//
// Copyright (c) 2020-2021 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================
// BT.709 - Video Range
//      R        G        B
// Y =  0.18275  0.61477  0.06200
// U = -0.10072 -0.33882  0.43931
// V =  0.43867 -0.40048 -0.04038
//
// BT.709 - Full Range
//      R        G        B
// Y =  0.21260  0.71520  0.07220
// U = -0.11412 -0.38392  0.49804
// V =  0.49804 -0.45237 -0.04567
#define fRY  0.21260
#define fGY  0.71520
#define fBY  0.07220
#define fRU -0.11412
#define fGU -0.38392
#define fBU  0.49804
#define fRV  0.49804
#define fGV -0.45237
#define fBV -0.04567
#define vRY  0.18275
#define vGY  0.61477
#define vBY  0.06200
#define vRU -0.10072
#define vGU -0.33882
#define vBU  0.43931
#define vRV  0.43867
#define vGV -0.40048
#define vBV -0.04038

#if defined(__ARM_NEON__) || defined(__ARM_NEON) || defined(_M_ARM) || defined(_M_ARM64) || defined(_M_HYBRID_X86_ARM64)
#   include <arm_neon.h>
#elif defined(_M_IX86) || defined(_M_AMD64) || defined(__i386__) || defined(__amd64__)
#   include <immintrin.h>
#   include <tmmintrin.h>
#   if defined(__llvm__)
#       include <avxintrin.h>
#       include <avx2intrin.h>
#   endif
#   if defined(_MSC_VER) && !defined(__llvm__)
#       define _mm_shuffle_ps(a, b, c)      (__m128i&)_mm_shuffle_ps((__m128&)a, (__m128&)b, c)
#       define _mm256_shuffle_ps(a, b, c)   (__m256i&)_mm256_shuffle_ps((__m256&)a, (__m256&)b, c)
#       define _mm_movehl_ps(a, b)          (__m128i&)_mm_movehl_ps((__m128&)a, (__m128&)b)
#       define _mm_storel_pi(a, b)          _mm_storel_pi(a, (__m128&)b)
#       define _mm_storeh_pi(a, b)          _mm_storeh_pi(a, (__m128&)b)
#   endif
#endif

//------------------------------------------------------------------------------
template<int componentRGB, bool swizzleRGB, bool interleaved, bool firstU, bool videoRange>
void rgb2yuv(int width, int height, const void* rgb, int strideRGB, void* y, void* u, void* v, int strideY, int strideU, int strideV)
{
    int halfWidth = width >> 1;
    int halfHeight = height >> 1;

    int iR = swizzleRGB ? 2 : 0;
    int iG = 1;
    int iB = swizzleRGB ? 0 : 2;
    int iA = 3;

    int Y[3], U[3], V[3];
    if (videoRange)
    {
        Y[iR] = (int)(vRY * 256); U[iR] = (int)(vRU * 255); V[iR] = (int)(vRV * 255);
        Y[iG] = (int)(vGY * 256); U[iG] = (int)(vGU * 255); V[iG] = (int)(vGV * 255);
        Y[iB] = (int)(vBY * 256); U[iB] = (int)(vBU * 255); V[iB] = (int)(vBV * 255);
    }
    else
    {
        Y[iR] = (int)(fRY * 256); U[iR] = (int)(fRU * 255); V[iR] = (int)(fRV * 255);
        Y[iG] = (int)(fGY * 256); U[iG] = (int)(fGU * 255); V[iG] = (int)(fGV * 255);
        Y[iB] = (int)(fBY * 256); U[iB] = (int)(fBU * 255); V[iB] = (int)(fBV * 255);
    }

    for (int h = 0; h < halfHeight; ++h)
    {
        const unsigned char* rgb0 = (unsigned char*)rgb;
        const unsigned char* rgb1 = rgb0 + strideRGB;       rgb = rgb1 + strideRGB;
        unsigned char* y0 = (unsigned char*)y;
        unsigned char* y1 = y0 + strideY;                   y = y1 + strideY;
        unsigned char* u0 = (unsigned char*)u;              u = u0 + strideU;
        unsigned char* v0 = (unsigned char*)v;              v = v0 + strideV;
#if HAVE_NEON
        int halfWidth8 = (componentRGB == 4) ? halfWidth / 8 : 0;
        for (int w = 0; w < halfWidth8; ++w)
        {
            uint8x16x4_t rgb00 = vld4q_u8(rgb0);  rgb0 += 16 * 4;
            uint8x16x4_t rgb10 = vld4q_u8(rgb1);  rgb1 += 16 * 4;

            uint8x8_t r00 = vget_low_u8(rgb00.val[0]);
            uint8x8_t g00 = vget_low_u8(rgb00.val[1]);
            uint8x8_t b00 = vget_low_u8(rgb00.val[2]);
            uint8x8_t r01 = vget_high_u8(rgb00.val[0]);
            uint8x8_t g01 = vget_high_u8(rgb00.val[1]);
            uint8x8_t b01 = vget_high_u8(rgb00.val[2]);
            uint8x8_t r10 = vget_low_u8(rgb10.val[0]);
            uint8x8_t g10 = vget_low_u8(rgb10.val[1]);
            uint8x8_t b10 = vget_low_u8(rgb10.val[2]);
            uint8x8_t r11 = vget_high_u8(rgb10.val[0]);
            uint8x8_t g11 = vget_high_u8(rgb10.val[1]);
            uint8x8_t b11 = vget_high_u8(rgb10.val[2]);

            uint8x8_t y00 = vqshrn_n_u16(vmlal_u8(vmlal_u8(vmull_u8(r00, vdup_n_u8(Y[0])), g00, vdup_n_u8(Y[1])), b00, vdup_n_u8(Y[2])), 8);
            uint8x8_t y01 = vqshrn_n_u16(vmlal_u8(vmlal_u8(vmull_u8(r01, vdup_n_u8(Y[0])), g01, vdup_n_u8(Y[1])), b01, vdup_n_u8(Y[2])), 8);
            uint8x8_t y10 = vqshrn_n_u16(vmlal_u8(vmlal_u8(vmull_u8(r10, vdup_n_u8(Y[0])), g10, vdup_n_u8(Y[1])), b10, vdup_n_u8(Y[2])), 8);
            uint8x8_t y11 = vqshrn_n_u16(vmlal_u8(vmlal_u8(vmull_u8(r11, vdup_n_u8(Y[0])), g11, vdup_n_u8(Y[1])), b11, vdup_n_u8(Y[2])), 8);
            uint8x16_t y000 = vcombine_u8(y00, y01);
            uint8x16_t y100 = vcombine_u8(y10, y11);
            if (videoRange)
            {
                y000 = vqaddq_u8(vcombine_u8(y00, y01), vdupq_n_u8(16));
                y100 = vqaddq_u8(vcombine_u8(y10, y11), vdupq_n_u8(16));
            }
            else
            {
                y000 = vcombine_u8(y00, y01);
                y100 = vcombine_u8(y10, y11);
            }

            int16x8_t r000 = vpadalq_u8(vpaddlq_u8(rgb00.val[0]), rgb10.val[0]);
            int16x8_t g000 = vpadalq_u8(vpaddlq_u8(rgb00.val[1]), rgb10.val[1]);
            int16x8_t b000 = vpadalq_u8(vpaddlq_u8(rgb00.val[2]), rgb10.val[2]);

            uint8x8_t u00 = vrshrn_n_s16(vmlaq_s16(vmlaq_s16(vmulq_s16(r000, vdupq_n_s16(U[0] >> 2)), g000, vdupq_n_s16(U[1] >> 2)), b000, vdupq_n_s16(U[2] >> 2)), 8);
            uint8x8_t v00 = vrshrn_n_s16(vmlaq_s16(vmlaq_s16(vmulq_s16(r000, vdupq_n_s16(V[0] >> 2)), g000, vdupq_n_s16(V[1] >> 2)), b000, vdupq_n_s16(V[2] >> 2)), 8);
            u00 = vadd_u8(u00, vdup_n_u8(128));
            v00 = vadd_u8(v00, vdup_n_u8(128));

            vst1q_u8(y0, y000); y0 += 16;
            vst1q_u8(y1, y100); y1 += 16;
            if (interleaved)
            {
                if (firstU)
                {
                    uint8x8x2_t uv00 = vzip_u8(u00, v00);
                    vst1q_u8(u0, vcombine_u8(uv00.val[0], uv00.val[1])); u0 += 16;
                }
                else
                {
                    uint8x8x2_t uv00 = vzip_u8(v00, u00);
                    vst1q_u8(v0, vcombine_u8(uv00.val[0], uv00.val[1])); v0 += 16;
                }
            }
            else
            {
                vst1_u8(u0, u00); u0 += 8;
                vst1_u8(v0, v00); v0 += 8;
            }
        }
        if (componentRGB == 4)
            continue;
#elif HAVE_AVX2
        int halfWidth16 = (componentRGB == 4) ? halfWidth / 16 : 0;
        for (int w = 0; w < halfWidth16; ++w)
        {
            __m256i rgb00[4] = { _mm256_loadu_si256((__m256i*)rgb0), _mm256_loadu_si256((__m256i*)rgb0 + 1), _mm256_loadu_si256((__m256i*)rgb0 + 2), _mm256_loadu_si256((__m256i*)rgb0 + 3) };  rgb0 += 32 * 4;
            __m256i rgb10[4] = { _mm256_loadu_si256((__m256i*)rgb1), _mm256_loadu_si256((__m256i*)rgb1 + 1), _mm256_loadu_si256((__m256i*)rgb1 + 2), _mm256_loadu_si256((__m256i*)rgb1 + 3) };  rgb1 += 32 * 4;

            __m256i yy = _mm256_setr_epi8(Y[0] >> 1, Y[1] >> 1, Y[2] >> 1, 0,
                                          Y[0] >> 1, Y[1] >> 1, Y[2] >> 1, 0,
                                          Y[0] >> 1, Y[1] >> 1, Y[2] >> 1, 0,
                                          Y[0] >> 1, Y[1] >> 1, Y[2] >> 1, 0,
                                          Y[0] >> 1, Y[1] >> 1, Y[2] >> 1, 0,
                                          Y[0] >> 1, Y[1] >> 1, Y[2] >> 1, 0,
                                          Y[0] >> 1, Y[1] >> 1, Y[2] >> 1, 0,
                                          Y[0] >> 1, Y[1] >> 1, Y[2] >> 1, 0);
            __m256i yy000 = _mm256_maddubs_epi16(rgb00[0], yy);
            __m256i yy001 = _mm256_maddubs_epi16(rgb00[1], yy);
            __m256i yy010 = _mm256_maddubs_epi16(rgb00[2], yy);
            __m256i yy011 = _mm256_maddubs_epi16(rgb00[3], yy);
            __m256i yy100 = _mm256_maddubs_epi16(rgb10[0], yy);
            __m256i yy101 = _mm256_maddubs_epi16(rgb10[1], yy);
            __m256i yy110 = _mm256_maddubs_epi16(rgb10[2], yy);
            __m256i yy111 = _mm256_maddubs_epi16(rgb10[3], yy);
            __m256i y00 = _mm256_hadd_epi16(yy000, yy001);
            __m256i y01 = _mm256_hadd_epi16(yy010, yy011);
            __m256i y10 = _mm256_hadd_epi16(yy100, yy101);
            __m256i y11 = _mm256_hadd_epi16(yy110, yy111);
            y00 = _mm256_srli_epi16(y00, 7);
            y01 = _mm256_srli_epi16(y01, 7);
            y10 = _mm256_srli_epi16(y10, 7);
            y11 = _mm256_srli_epi16(y11, 7);
            __m256i y000 = _mm256_permutevar8x32_epi32(_mm256_packus_epi16(y00, y01), _mm256_setr_epi32(0,4,1,5,2,6,3,7));
            __m256i y100 = _mm256_permutevar8x32_epi32(_mm256_packus_epi16(y10, y11), _mm256_setr_epi32(0,4,1,5,2,6,3,7));
            if (videoRange)
            {
                y000 = _mm256_adds_epu8(y000, _mm256_set1_epi8(16));
                y100 = _mm256_adds_epu8(y100, _mm256_set1_epi8(16));
            }

            __m256i uv00 = _mm256_avg_epu8(rgb00[0], rgb10[0]);
            __m256i uv01 = _mm256_avg_epu8(rgb00[1], rgb10[1]);
            __m256i uv10 = _mm256_avg_epu8(rgb00[2], rgb10[2]);
            __m256i uv11 = _mm256_avg_epu8(rgb00[3], rgb10[3]);
            __m256i uv0 = _mm256_avg_epu8(_mm256_shuffle_ps(uv00, uv01, _MM_SHUFFLE(2,0,2,0)), _mm256_shuffle_ps(uv00, uv01, _MM_SHUFFLE(3,1,3,1)));
            __m256i uv1 = _mm256_avg_epu8(_mm256_shuffle_ps(uv10, uv11, _MM_SHUFFLE(2,0,2,0)), _mm256_shuffle_ps(uv10, uv11, _MM_SHUFFLE(3,1,3,1)));
            __m256i uu = _mm256_setr_epi8(U[0], U[1], U[2], 0,
                                          U[0], U[1], U[2], 0,
                                          U[0], U[1], U[2], 0,
                                          U[0], U[1], U[2], 0,
                                          U[0], U[1], U[2], 0,
                                          U[0], U[1], U[2], 0,
                                          U[0], U[1], U[2], 0,
                                          U[0], U[1], U[2], 0);
            __m256i vv = _mm256_setr_epi8(V[0], V[1], V[2], 0,
                                          V[0], V[1], V[2], 0,
                                          V[0], V[1], V[2], 0,
                                          V[0], V[1], V[2], 0,
                                          V[0], V[1], V[2], 0,
                                          V[0], V[1], V[2], 0,
                                          V[0], V[1], V[2], 0,
                                          V[0], V[1], V[2], 0);
            __m256i uu00 = _mm256_maddubs_epi16(uv0, uu);
            __m256i uu01 = _mm256_maddubs_epi16(uv1, uu);
            __m256i vv00 = _mm256_maddubs_epi16(uv0, vv);
            __m256i vv01 = _mm256_maddubs_epi16(uv1, vv);
            __m256i uu02 = _mm256_hadd_epi16(uu00, uu01);
            __m256i vv02 = _mm256_hadd_epi16(vv00, vv01);
            uu02 = _mm256_srai_epi16(uu02, 8);
            vv02 = _mm256_srai_epi16(vv02, 8);
            __m256i mask = _mm256_setr_epi8(0, 1, 8, 9, 2, 3, 10, 11, 4, 5, 12, 13, 6, 7, 14, 15,
                                            0, 1, 8, 9, 2, 3, 10, 11, 4, 5, 12, 13, 6, 7, 14, 15);
            __m256i uv02 = _mm256_shuffle_epi8(_mm256_permute4x64_epi64(_mm256_packs_epi16(uu02, vv02), _MM_SHUFFLE(3,1,2,0)), mask);
            uv02 = _mm256_sub_epi8(uv02, _mm256_set1_epi8(-128));

            _mm256_storeu_si256((__m256i*)y0, y000); y0 += 32;
            _mm256_storeu_si256((__m256i*)y1, y100); y1 += 32;
            if (interleaved)
            {
                __m128i u00 = _mm256_extractf128_si256(uv02, 0);
                __m128i v00 = _mm256_extractf128_si256(uv02, 1);
                if (firstU)
                {
                    __m256i uv00 = _mm256_setr_m128i(_mm_unpacklo_epi8(u00, v00), _mm_unpackhi_epi8(u00, v00));
                    _mm256_storeu_si256((__m256i*)u0, uv00); u0 += 32;
                }
                else
                {
                    __m256i uv00 = _mm256_setr_m128i(_mm_unpacklo_epi8(v00, u00), _mm_unpackhi_epi8(v00, u00));
                    _mm256_storeu_si256((__m256i*)v0, uv00); v0 += 32;
                }
            }
            else
            {
                _mm256_storeu2_m128i((__m128i*)v0, (__m128i*)u0, uv02);  u0 += 16; v0 += 16;
            }
        }
        if (componentRGB == 4)
            continue;
#elif HAVE_SSE2 || HAVE_SSSE3
        int halfWidth8 = (componentRGB == 4) ? halfWidth / 8 : 0;
        for (int w = 0; w < halfWidth8; ++w)
        {
            __m128i rgb00[4] = { _mm_loadu_si128((__m128i*)rgb0), _mm_loadu_si128((__m128i*)rgb0 + 1), _mm_loadu_si128((__m128i*)rgb0 + 2), _mm_loadu_si128((__m128i*)rgb0 + 3) };  rgb0 += 16 * 4;
            __m128i rgb10[4] = { _mm_loadu_si128((__m128i*)rgb1), _mm_loadu_si128((__m128i*)rgb1 + 1), _mm_loadu_si128((__m128i*)rgb1 + 2), _mm_loadu_si128((__m128i*)rgb1 + 3) };  rgb1 += 16 * 4;

#if HAVE_SSSE3
            __m128i yy = _mm_setr_epi8(Y[0] >> 1, Y[1] >> 1, Y[2] >> 1, 0,
                                       Y[0] >> 1, Y[1] >> 1, Y[2] >> 1, 0,
                                       Y[0] >> 1, Y[1] >> 1, Y[2] >> 1, 0,
                                       Y[0] >> 1, Y[1] >> 1, Y[2] >> 1, 0);
            __m128i yy000 = _mm_maddubs_epi16(rgb00[0], yy);
            __m128i yy001 = _mm_maddubs_epi16(rgb00[1], yy);
            __m128i yy010 = _mm_maddubs_epi16(rgb00[2], yy);
            __m128i yy011 = _mm_maddubs_epi16(rgb00[3], yy);
            __m128i yy100 = _mm_maddubs_epi16(rgb10[0], yy);
            __m128i yy101 = _mm_maddubs_epi16(rgb10[1], yy);
            __m128i yy110 = _mm_maddubs_epi16(rgb10[2], yy);
            __m128i yy111 = _mm_maddubs_epi16(rgb10[3], yy);
            __m128i y00 = _mm_hadd_epi16(yy000, yy001);
            __m128i y01 = _mm_hadd_epi16(yy010, yy011);
            __m128i y10 = _mm_hadd_epi16(yy100, yy101);
            __m128i y11 = _mm_hadd_epi16(yy110, yy111);
#else
            __m128i yy = _mm_setr_epi16(Y[0] >> 1, Y[1] >> 1, Y[2] >> 1, 0,
                                        Y[0] >> 1, Y[1] >> 1, Y[2] >> 1, 0);
            __m128i yy000 = _mm_packs_epi32(_mm_madd_epi16(_mm_unpacklo_epi8(rgb00[0], __m128i()), yy), _mm_madd_epi16(_mm_unpackhi_epi8(rgb00[0], __m128i()), yy));
            __m128i yy001 = _mm_packs_epi32(_mm_madd_epi16(_mm_unpacklo_epi8(rgb00[1], __m128i()), yy), _mm_madd_epi16(_mm_unpackhi_epi8(rgb00[1], __m128i()), yy));
            __m128i yy010 = _mm_packs_epi32(_mm_madd_epi16(_mm_unpacklo_epi8(rgb00[2], __m128i()), yy), _mm_madd_epi16(_mm_unpackhi_epi8(rgb00[2], __m128i()), yy));
            __m128i yy011 = _mm_packs_epi32(_mm_madd_epi16(_mm_unpacklo_epi8(rgb00[3], __m128i()), yy), _mm_madd_epi16(_mm_unpackhi_epi8(rgb00[3], __m128i()), yy));
            __m128i yy100 = _mm_packs_epi32(_mm_madd_epi16(_mm_unpacklo_epi8(rgb10[0], __m128i()), yy), _mm_madd_epi16(_mm_unpackhi_epi8(rgb10[0], __m128i()), yy));
            __m128i yy101 = _mm_packs_epi32(_mm_madd_epi16(_mm_unpacklo_epi8(rgb10[1], __m128i()), yy), _mm_madd_epi16(_mm_unpackhi_epi8(rgb10[1], __m128i()), yy));
            __m128i yy110 = _mm_packs_epi32(_mm_madd_epi16(_mm_unpacklo_epi8(rgb10[2], __m128i()), yy), _mm_madd_epi16(_mm_unpackhi_epi8(rgb10[2], __m128i()), yy));
            __m128i yy111 = _mm_packs_epi32(_mm_madd_epi16(_mm_unpacklo_epi8(rgb10[3], __m128i()), yy), _mm_madd_epi16(_mm_unpackhi_epi8(rgb10[3], __m128i()), yy));
            __m128i y00 = _mm_packs_epi32(_mm_madd_epi16(yy000, _mm_set1_epi16(1)), _mm_madd_epi16(yy001, _mm_set1_epi16(1)));
            __m128i y01 = _mm_packs_epi32(_mm_madd_epi16(yy010, _mm_set1_epi16(1)), _mm_madd_epi16(yy011, _mm_set1_epi16(1)));
            __m128i y10 = _mm_packs_epi32(_mm_madd_epi16(yy100, _mm_set1_epi16(1)), _mm_madd_epi16(yy101, _mm_set1_epi16(1)));
            __m128i y11 = _mm_packs_epi32(_mm_madd_epi16(yy110, _mm_set1_epi16(1)), _mm_madd_epi16(yy111, _mm_set1_epi16(1)));
#endif
            y00 = _mm_srli_epi16(y00, 7);
            y01 = _mm_srli_epi16(y01, 7);
            y10 = _mm_srli_epi16(y10, 7);
            y11 = _mm_srli_epi16(y11, 7);
            __m128i y000 = _mm_packus_epi16(y00, y01);
            __m128i y100 = _mm_packus_epi16(y10, y11);
            if (videoRange)
            {
                y000 = _mm_adds_epu8(y000, _mm_set1_epi8(16));
                y100 = _mm_adds_epu8(y100, _mm_set1_epi8(16));
            }

            __m128i uv00 = _mm_avg_epu8(rgb00[0], rgb10[0]);
            __m128i uv01 = _mm_avg_epu8(rgb00[1], rgb10[1]);
            __m128i uv10 = _mm_avg_epu8(rgb00[2], rgb10[2]);
            __m128i uv11 = _mm_avg_epu8(rgb00[3], rgb10[3]);
            __m128i uv0 = _mm_avg_epu8(_mm_shuffle_ps(uv00, uv01, _MM_SHUFFLE(2,0,2,0)), _mm_shuffle_ps(uv00, uv01, _MM_SHUFFLE(3,1,3,1)));
            __m128i uv1 = _mm_avg_epu8(_mm_shuffle_ps(uv10, uv11, _MM_SHUFFLE(2,0,2,0)), _mm_shuffle_ps(uv10, uv11, _MM_SHUFFLE(3,1,3,1)));
#if HAVE_SSSE3
            __m128i uu = _mm_setr_epi8(U[0], U[1], U[2], 0,
                                       U[0], U[1], U[2], 0,
                                       U[0], U[1], U[2], 0,
                                       U[0], U[1], U[2], 0);
            __m128i vv = _mm_setr_epi8(V[0], V[1], V[2], 0,
                                       V[0], V[1], V[2], 0,
                                       V[0], V[1], V[2], 0,
                                       V[0], V[1], V[2], 0);
            __m128i uu00 = _mm_maddubs_epi16(uv0, uu);
            __m128i uu01 = _mm_maddubs_epi16(uv1, uu);
            __m128i vv00 = _mm_maddubs_epi16(uv0, vv);
            __m128i vv01 = _mm_maddubs_epi16(uv1, vv);
            __m128i u00 = _mm_hadd_epi16(uu00, uu01);
            __m128i v00 = _mm_hadd_epi16(vv00, vv01);
#else
            __m128i uu = _mm_setr_epi16(U[0], U[1], U[2], 0,
                                        U[0], U[1], U[2], 0);
            __m128i vv = _mm_setr_epi16(V[0], V[1], V[2], 0,
                                        V[0], V[1], V[2], 0);
            __m128i uu00 = _mm_packs_epi32(_mm_madd_epi16(_mm_unpacklo_epi8(uv0, __m128i()), uu), _mm_madd_epi16(_mm_unpackhi_epi8(uv0, __m128i()), uu));
            __m128i uu01 = _mm_packs_epi32(_mm_madd_epi16(_mm_unpacklo_epi8(uv1, __m128i()), uu), _mm_madd_epi16(_mm_unpackhi_epi8(uv1, __m128i()), uu));
            __m128i vv00 = _mm_packs_epi32(_mm_madd_epi16(_mm_unpacklo_epi8(uv0, __m128i()), vv), _mm_madd_epi16(_mm_unpackhi_epi8(uv0, __m128i()), vv));
            __m128i vv01 = _mm_packs_epi32(_mm_madd_epi16(_mm_unpacklo_epi8(uv1, __m128i()), vv), _mm_madd_epi16(_mm_unpackhi_epi8(uv1, __m128i()), vv));
            __m128i u00 = _mm_packs_epi32(_mm_madd_epi16(uu00, _mm_set1_epi16(1)), _mm_madd_epi16(uu01, _mm_set1_epi16(1)));
            __m128i v00 = _mm_packs_epi32(_mm_madd_epi16(vv00, _mm_set1_epi16(1)), _mm_madd_epi16(vv01, _mm_set1_epi16(1)));
#endif
            u00 = _mm_srai_epi16(u00, 8);
            v00 = _mm_srai_epi16(v00, 8);
            __m128i uv02 = _mm_packs_epi16(u00, v00);
            uv02 = _mm_sub_epi8(uv02, _mm_set1_epi8(-128));

            _mm_storeu_si128((__m128i*)y0, y000); y0 += 16;
            _mm_storeu_si128((__m128i*)y1, y100); y1 += 16;
            if (interleaved)
            {
                u00 = uv02;
                v00 = _mm_movehl_ps(uv02, uv02);
                if (firstU)
                {
                    __m128i uv00 = _mm_unpacklo_epi8(u00, v00);
                    _mm_storeu_si128((__m128i*)u0, uv00); u0 += 16;
                }
                else
                {
                    __m128i uv00 = _mm_unpacklo_epi8(v00, u00);
                    _mm_storeu_si128((__m128i*)v0, uv00); v0 += 16;
                }
            }
            else
            {
                _mm_storel_pi((__m64*)u0, uv02);    u0 += 8;
                _mm_storeh_pi((__m64*)v0, uv02);    v0 += 8;
            }
        }
        if (componentRGB == 4)
            continue;
#endif
        for (int w = 0; w < halfWidth; ++w)
        {
            int b00 = (componentRGB >= 1) ? rgb0[0] : 255;
            int g00 = (componentRGB >= 2) ? rgb0[1] : 255;
            int r00 = (componentRGB >= 3) ? rgb0[2] : 255;
            int a00 = (componentRGB >= 4) ? rgb0[3] : 255; rgb0 += componentRGB;
            int b01 = (componentRGB >= 1) ? rgb0[0] : 255;
            int g01 = (componentRGB >= 2) ? rgb0[1] : 255;
            int r01 = (componentRGB >= 3) ? rgb0[2] : 255;
            int a01 = (componentRGB >= 4) ? rgb0[3] : 255; rgb0 += componentRGB;
            int b10 = (componentRGB >= 1) ? rgb1[0] : 255;
            int g10 = (componentRGB >= 2) ? rgb1[1] : 255;
            int r10 = (componentRGB >= 3) ? rgb1[2] : 255;
            int a10 = (componentRGB >= 4) ? rgb1[3] : 255; rgb1 += componentRGB;
            int b11 = (componentRGB >= 1) ? rgb1[0] : 255;
            int g11 = (componentRGB >= 2) ? rgb1[1] : 255;
            int r11 = (componentRGB >= 3) ? rgb1[2] : 255;
            int a11 = (componentRGB >= 4) ? rgb1[3] : 255; rgb1 += componentRGB;

            int r000 = (r00 + r01 + r10 + r11) / 4;
            int g000 = (g00 + g01 + g10 + g11) / 4;
            int b000 = (b00 + b01 + b10 + b11) / 4;

            int y00 = r00  * Y[0] + g00  * Y[1] + b00  * Y[2];
            int y01 = r01  * Y[0] + g01  * Y[1] + b01  * Y[2];
            int y10 = r10  * Y[0] + g10  * Y[1] + b10  * Y[2];
            int y11 = r11  * Y[0] + g11  * Y[1] + b11  * Y[2];
            int u00 = r000 * U[0] + g000 * U[1] + b000 * U[2];
            int v00 = r000 * V[0] + g000 * V[1] + b000 * V[2];

            auto clamp = [](int value) -> unsigned char
            {
                return (unsigned char)(value < 255 ? value < 0 ? 0 : value : 255);
            };

            if (videoRange)
            {
                (*y0++) = clamp((y00 >> 8) + 16);
                (*y0++) = clamp((y01 >> 8) + 16);
                (*y1++) = clamp((y10 >> 8) + 16);
                (*y1++) = clamp((y11 >> 8) + 16);
            }
            else
            {
                (*y0++) = clamp(y00 >> 8);
                (*y0++) = clamp(y01 >> 8);
                (*y1++) = clamp(y10 >> 8);
                (*y1++) = clamp(y11 >> 8);
            }
            (*u0++) = clamp((u00 >> 8) + 128);
            (*v0++) = clamp((v00 >> 8) + 128);
            if (interleaved)
            {
                u0++;
                v0++;
            }
        }
    }
}
//------------------------------------------------------------------------------
#ifndef rgb2yuv_select
#define rgb2yuv_select(componentRGB, swizzleRGB, interleaved, firstU, videoRange) \
    rgb2yuv<componentRGB, swizzleRGB, interleaved, firstU, videoRange>
#endif
//------------------------------------------------------------------------------
#ifndef rgb2yuv
//------------------------------------------------------------------------------
#if defined(__llvm__)
#define rgb2yuv_attribute(value) __attribute__((target(value)))
#else
#define rgb2yuv_attribute(value)
#endif
//------------------------------------------------------------------------------
#if defined(__ARM_NEON__) || defined(__ARM_NEON) || defined(_M_ARM) || defined(_M_ARM64) || defined(_M_HYBRID_X86_ARM64)
#define HAVE_NEON 1
#define rgb2yuv rgb2yuv_attribute("neon") rgb2yuv_neon
#include "rgb2yuv.inl"
#undef rgb2yuv
#undef HAVE_NEON
#undef rgb2yuv_select
#define rgb2yuv_select(componentRGB, swizzleRGB, interleaved, firstU, videoRange) \
    neon() ? rgb2yuv_neon<componentRGB, swizzleRGB, interleaved, firstU, videoRange> : \
    rgb2yuv<componentRGB, swizzleRGB, interleaved, firstU, videoRange>
#endif
//------------------------------------------------------------------------------
#if defined(_M_IX86) || defined(_M_AMD64) || defined(__i386__) || defined(__amd64__)
#define HAVE_SSE2 1
#define rgb2yuv rgb2yuv_attribute("sse2") rgb2yuv_sse2
#include "rgb2yuv.inl"
#undef rgb2yuv
#undef HAVE_SSE2
#undef rgb2yuv_select
#define rgb2yuv_select(componentRGB, swizzleRGB, interleaved, firstU, videoRange) \
    sse2() ? rgb2yuv_sse2<componentRGB, swizzleRGB, interleaved, firstU, videoRange> : \
    rgb2yuv<componentRGB, swizzleRGB, interleaved, firstU, videoRange>
#endif
//------------------------------------------------------------------------------
#if defined(_M_IX86) || defined(_M_AMD64) || defined(__i386__) || defined(__amd64__)
#define HAVE_SSSE3 1
#define rgb2yuv rgb2yuv_attribute("ssse3") rgb2yuv_ssse3
#include "rgb2yuv.inl"
#undef rgb2yuv
#undef HAVE_SSSE3
#undef rgb2yuv_select
#define rgb2yuv_select(componentRGB, swizzleRGB, interleaved, firstU, videoRange) \
    ssse3() ? rgb2yuv_ssse3<componentRGB, swizzleRGB, interleaved, firstU, videoRange> : \
    sse2() ? rgb2yuv_sse2<componentRGB, swizzleRGB, interleaved, firstU, videoRange> : \
    rgb2yuv<componentRGB, swizzleRGB, interleaved, firstU, videoRange>
#endif
//------------------------------------------------------------------------------
#if defined(_M_IX86) || defined(_M_AMD64) || defined(__i386__) || defined(__amd64__)
#define HAVE_AVX2 1
#define rgb2yuv rgb2yuv_attribute("avx2") rgb2yuv_avx2
#include "rgb2yuv.inl"
#undef rgb2yuv
#undef HAVE_AVX2
#undef rgb2yuv_select
#define rgb2yuv_select(componentRGB, swizzleRGB, interleaved, firstU, videoRange) \
    avx2() ? rgb2yuv_avx2<componentRGB, swizzleRGB, interleaved, firstU, videoRange> : \
    ssse3() ? rgb2yuv_ssse3<componentRGB, swizzleRGB, interleaved, firstU, videoRange> : \
    sse2() ? rgb2yuv_sse2<componentRGB, swizzleRGB, interleaved, firstU, videoRange> : \
    rgb2yuv<componentRGB, swizzleRGB, interleaved, firstU, videoRange>
#endif
//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------

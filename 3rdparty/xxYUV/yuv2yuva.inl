//==============================================================================
// xxYUV : yuv2yuva Inline
//
// Copyright (c) 2020-2021 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================
#if defined(__ARM_NEON__) || defined(__ARM_NEON) || defined(_M_ARM) || defined(_M_ARM64) || defined(_M_HYBRID_X86_ARM64)
#   include <arm_neon.h>
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
#endif

//------------------------------------------------------------------------------
template<bool interleaved, bool firstU, int iY, int iU, int iV, int iA>
void yuv2yuva(int width, int height, const void* y, const void* u, const void* v, int strideY, int strideU, int strideV, void* output, int strideOutput)
{
    int halfWidth = width >> 1;
    int halfHeight = height >> 1;

    for (int h = 0; h < halfHeight; ++h)
    {
        const unsigned char* y0 = (unsigned char*)y;
        const unsigned char* y1 = y0 + strideY;             y = y1 + strideY;
        const unsigned char* u0 = (unsigned char*)u;        u = u0 + strideU;
        const unsigned char* v0 = (unsigned char*)v;        v = v0 + strideV;
        unsigned char* output0 = (unsigned char*)output;
        unsigned char* output1 = output0 + strideOutput;    output = output1 + strideOutput;
#if HAVE_NEON
        int halfWidth8 = halfWidth / 8;
        for (int w = 0; w < halfWidth8; ++w)
        {
            uint8x16_t y00 = vld1q_u8(y0); y0 += 16;
            uint8x16_t y10 = vld1q_u8(y1); y1 += 16;

            int8x8x2_t u000;
            int8x8x2_t v000;
            int8x16_t u00;
            int8x16_t v00;
            if (interleaved)
            {
                if (firstU)
                {
                    int8x16_t uv00 = vld1q_u8(u0); u0 += 16;
                    int8x8x2_t uv00lh = vuzp_s8(vget_low_s8(uv00), vget_high_s8(uv00));
                    u000 = vzip_s8(uv00lh.val[0], uv00lh.val[0]);
                    v000 = vzip_s8(uv00lh.val[1], uv00lh.val[1]);
                }
                else
                {
                    int8x16_t uv00 = vld1q_u8(v0); v0 += 16;
                    int8x8x2_t uv00lh = vuzp_s8(vget_low_s8(uv00), vget_high_s8(uv00));
                    u000 = vzip_s8(uv00lh.val[1], uv00lh.val[1]);
                    v000 = vzip_s8(uv00lh.val[0], uv00lh.val[0]);
                }
            }
            else
            {
                int8x8_t u0000 = vld1_u8(u0); u0 += 8;
                int8x8_t v0000 = vld1_u8(v0); v0 += 8;
                u000 = vzip_s8(u0000, u0000);
                v000 = vzip_s8(v0000, v0000);
            }
            u00 = vcombine_s8(u000.val[0], u000.val[1]);
            v00 = vcombine_s8(v000.val[0], v000.val[1]);

            uint8x16x4_t t;
            uint8x16x4_t b;

            t.val[iY] = y00;
            t.val[iU] = u00;
            t.val[iV] = v00;
            t.val[iA] = vdupq_n_u8(255);
            b.val[iY] = y10;
            b.val[iU] = u00;
            b.val[iV] = v00;
            b.val[iA] = vdupq_n_u8(255);

            vst4q_u8(output0, t); output0 += 16 * 4;
            vst4q_u8(output1, b); output1 += 16 * 4;
        }
        continue;
#elif HAVE_AVX2
        int halfWidth16 = halfWidth / 16;
        for (int w = 0; w < halfWidth16; ++w)
        {
            __m256i y00 = _mm256_loadu_si256((__m256i*)y0); y0 += 32;
            __m256i y10 = _mm256_loadu_si256((__m256i*)y1); y1 += 32;

            __m256i u00;
            __m256i v00;
            if (interleaved)
            {
                if (firstU)
                {
                    __m256i uv00 = _mm256_loadu_si256((__m256i*)u0); u0 += 32;
                    u00 = _mm256_and_si256(uv00, _mm256_set1_epi16(0xFF));
                    v00 = _mm256_srli_epi16(uv00, 8);
                }
                else
                {
                    __m256i uv00 = _mm256_loadu_si256((__m256i*)v0); v0 += 32;
                    u00 = _mm256_srli_epi16(uv00, 8);
                    v00 = _mm256_and_si256(uv00, _mm256_set1_epi16(0xFF));
                }
                u00 = _mm256_packus_epi16(u00, u00);
                v00 = _mm256_packus_epi16(v00, v00);
            }
            else
            {
                __m128i u000 = _mm_loadu_si128((__m128i*)u0); u0 += 16;
                __m128i v000 = _mm_loadu_si128((__m128i*)v0); v0 += 16;
                u00 = _mm256_castsi128_si256(u000);
                v00 = _mm256_castsi128_si256(v000);
            }
            u00 = _mm256_unpacklo_epi8(u00, u00);
            v00 = _mm256_unpacklo_epi8(v00, v00);

            __m256i t[4];
            __m256i b[4];

            t[iY] = y00;
            t[iU] = u00;
            t[iV] = v00;
            t[iA] = _mm256_set1_epi8(-1);
            b[iY] = y10;
            b[iU] = u00;
            b[iV] = v00;
            b[iA] = _mm256_set1_epi8(-1);

            _MM256_TRANSPOSE4_EPI8(t[0], t[1], t[2], t[3]);
            _MM256_TRANSPOSE4_EPI8(b[0], b[1], b[2], b[3]);
            _MM256_TRANSPOSE4_SI128(t[0], t[1], t[2], t[3]);
            _MM256_TRANSPOSE4_SI128(b[0], b[1], b[2], b[3]);

            _mm256_storeu_si256((__m256i*)output0 + 0, t[0]);
            _mm256_storeu_si256((__m256i*)output0 + 1, t[1]);
            _mm256_storeu_si256((__m256i*)output0 + 2, t[2]);
            _mm256_storeu_si256((__m256i*)output0 + 3, t[3]);
            _mm256_storeu_si256((__m256i*)output1 + 0, b[0]); output0 += 16 * 8;
            _mm256_storeu_si256((__m256i*)output1 + 1, b[1]);
            _mm256_storeu_si256((__m256i*)output1 + 2, b[2]);
            _mm256_storeu_si256((__m256i*)output1 + 3, b[3]); output1 += 16 * 8;
        }
        continue;
#elif HAVE_SSE2
        int halfWidth8 = halfWidth / 8;
        for (int w = 0; w < halfWidth8; ++w)
        {
            __m128i y00 = _mm_loadu_si128((__m128i*)y0); y0 += 16;
            __m128i y10 = _mm_loadu_si128((__m128i*)y1); y1 += 16;

            __m128i u00;
            __m128i v00;
            if (interleaved)
            {
                if (firstU)
                {
                    __m128i uv00 = _mm_loadu_si128((__m128i*)u0); u0 += 16;
                    u00 = _mm_and_si128(uv00, _mm_set1_epi16(0xFF));
                    v00 = _mm_srli_epi16(uv00, 8);
                }
                else
                {
                    __m128i uv00 = _mm_loadu_si128((__m128i*)v0); v0 += 16;
                    u00 = _mm_srli_epi16(uv00, 8);
                    v00 = _mm_and_si128(uv00, _mm_set1_epi16(0xFF));
                }
                u00 = _mm_packus_epi16(u00, u00);
                v00 = _mm_packus_epi16(v00, v00);
            }
            else
            {
                u00 = _mm_loadl_epi64((__m128i*)u0); u0 += 8;
                v00 = _mm_loadl_epi64((__m128i*)v0); v0 += 8;
            }
            u00 = _mm_unpacklo_epi8(u00, u00);
            v00 = _mm_unpacklo_epi8(v00, v00);

            __m128i t[4];
            __m128i b[4];

            t[iY] = y00;
            t[iU] = u00;
            t[iV] = v00;
            t[iA] = _mm_set1_epi8(-1);
            b[iY] = y10;
            b[iU] = u00;
            b[iV] = v00;
            b[iA] = _mm_set1_epi8(-1);

            _MM_TRANSPOSE4_EPI8(t[0], t[1], t[2], t[3]);
            _MM_TRANSPOSE4_EPI8(b[0], b[1], b[2], b[3]);

            _mm_storeu_si128((__m128i*)output0 + 0, t[0]);
            _mm_storeu_si128((__m128i*)output0 + 1, t[1]);
            _mm_storeu_si128((__m128i*)output0 + 2, t[2]);
            _mm_storeu_si128((__m128i*)output0 + 3, t[3]); output0 += 16 * 4;
            _mm_storeu_si128((__m128i*)output1 + 0, b[0]);
            _mm_storeu_si128((__m128i*)output1 + 1, b[1]);
            _mm_storeu_si128((__m128i*)output1 + 2, b[2]);
            _mm_storeu_si128((__m128i*)output1 + 3, b[3]); output1 += 16 * 4;
        }
        continue;
#endif
        for (int w = 0; w < halfWidth; ++w)
        {
            auto y00 = (*y0++);
            auto y01 = (*y0++);
            auto y10 = (*y1++);
            auto y11 = (*y1++);

            auto u00 = (*u0++);
            auto v00 = (*v0++);
            if (interleaved)
            {
                u0++;
                v0++;
            }

            output0[iY] = y00;
            output0[iU] = u00;
            output0[iV] = v00;
            output0[iA] = 255;
            output0 += 4;

            output0[iY] = y01;
            output0[iU] = u00;
            output0[iV] = v00;
            output0[iA] = 255;
            output0 += 4;

            output1[iY] = y10;
            output1[iU] = u00;
            output1[iV] = v00;
            output1[iA] = 255;
            output1 += 4;

            output1[iY] = y11;
            output1[iU] = u00;
            output1[iV] = v00;
            output1[iA] = 255;
            output1 += 4;
        }
    }
}
//------------------------------------------------------------------------------
#ifndef yuv2yuva_select
#define yuv2yuva_select(interleaved, firstU, iY, iU, iV, iA) \
    yuv2yuva<interleaved, firstU, iY, iU, iV, iA>
#endif
//------------------------------------------------------------------------------
#ifndef yuv2yuva
//------------------------------------------------------------------------------
#if defined(__llvm__)
#define yuv2yuva_attribute(value) __attribute__((target(value)))
#else
#define yuv2yuva_attribute(value)
#endif
//------------------------------------------------------------------------------
#if defined(__ARM_NEON__) || defined(__ARM_NEON) || defined(_M_ARM) || defined(_M_ARM64) || defined(_M_HYBRID_X86_ARM64)
#define HAVE_NEON 1
#define yuv2yuva yuv2yuva_attribute("neon") yuv2yuva_neon
#include "yuv2yuva.inl"
#undef yuv2yuva
#undef HAVE_NEON
#undef yuv2yuva_select
#define yuv2yuva_select(interleaved, firstU, iY, iU, iV, iA) \
    neon() ? yuv2yuva_neon<interleaved, firstU, iY, iU, iV, iA> : \
    yuv2yuva<interleaved, firstU, iY, iU, iV, iA>
#endif
//------------------------------------------------------------------------------
#if defined(_M_IX86) || defined(_M_AMD64) || defined(__i386__) || defined(__amd64__)
#define HAVE_SSE2 1
#define yuv2yuva yuv2yuva_attribute("sse2") yuv2yuva_sse2
#include "yuv2yuva.inl"
#undef yuv2yuva
#undef HAVE_SSE2
#undef yuv2yuva_select
#define yuv2yuva_select(interleaved, firstU, iY, iU, iV, iA) \
    sse2() ? yuv2yuva_sse2<interleaved, firstU, iY, iU, iV, iA> : \
    yuv2yuva<interleaved, firstU, iY, iU, iV, iAe>
#endif
//------------------------------------------------------------------------------
#if defined(_M_IX86) || defined(_M_AMD64) || defined(__i386__) || defined(__amd64__)
#define HAVE_AVX2 1
#define yuv2yuva yuv2yuva_attribute("avx2") yuv2yuva_avx2
#include "yuv2yuva.inl"
#undef yuv2yuva
#undef HAVE_AVX2
#undef yuv2yuva_select
#define yuv2yuva_select(interleaved, firstU, iY, iU, iV, iA) \
    avx2() ? yuv2yuva_avx2<interleaved, firstU, iY, iU, iV, iA> : \
    sse2() ? yuv2yuva_sse2<interleaved, firstU, iY, iU, iV, iA> : \
    yuv2yuva<interleaved, firstU, iY, iU, iV, iA>
#endif
//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------

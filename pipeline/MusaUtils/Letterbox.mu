// ============================================================================
// CUDA Preprocessing Kernels for YOLO Inference
// ============================================================================
// Single-kernel letterbox + BGR-to-RGB + normalize pipeline.
// Writes NCHW float directly into the TensorRT input buffer on the GPU,
// eliminating all intermediate cv::Mat copies.
//
// Original author: YOLOs-TRT Team
// Rewritten by Wang Rui
// ============================================================================
#include <musa_runtime.h>
#include <stdint.h>
#include <stdio.h>

// ============================================================================
// Letterbox + HWC-to-CHW + Normalize Kernel
// ============================================================================
// Each thread computes one pixel in the destination (dstH x dstW) image.
// If the pixel falls within the resized source region, it reads from the
// source image and writes normalised RGB into 3 separate channel planes
// (NCHW layout). Otherwise it writes the padding value.
__global__ void letterboxNormalizeKernel(
    const uint8_t* __restrict__ src,   // HWC RGB/BGR source image (device)
    float* __restrict__ dst,           // NCHW RGB float output
    int srcW, int srcH,                // source dimensions
    int dstW, int dstH,                // destination (letterbox) dimensions
    int newW, int newH,                // resized source dimensions
    int padLeft, int padTop,           // padding offsets
    float normScale,                   // 1 / 255
    float padValue,                    // normalised pad colour (114/255)
    bool isRGB, bool hasAlpha)
{
    const int x = blockIdx.x * blockDim.x + threadIdx.x; // dst column
    const int y = blockIdx.y * blockDim.y + threadIdx.y; // dst row
    if (x >= dstW || y >= dstH) return;

    const int planeSize = dstH * dstW;
    const int idx = y * dstW + x;
    const int srcChannels = hasAlpha ? 4 : 3;

    // Check if this pixel is inside the resized source region
    const int srcX = x - padLeft, srcY = y - padTop;
    if (srcX >= 0 && srcX < newW && srcY >= 0 && srcY < newH)
    {
        // Bilinear coordinate mapping back to original source
        // src coords (float) for bilinear interpolation
        const float fx = static_cast<float>(srcX) * srcW / static_cast<float>(newW);
        const float fy = static_cast<float>(srcY) * srcH / static_cast<float>(newH);
        const int x0 = static_cast<int>(fx), y0 = static_cast<int>(fy);
        const int x1 = min(x0 + 1, srcW - 1), y1 = min(y0 + 1, srcH - 1);
        const float ax = fx - x0, ay = fy - y0;

        // Read 4 corners (RGB/BGR, HWC layout)
        const int stride = srcW * srcChannels;
        const uint8_t* p00 = src + y0 * stride + x0 * srcChannels;
        const uint8_t* p01 = src + y0 * stride + x1 * srcChannels;
        const uint8_t* p10 = src + y1 * stride + x0 * srcChannels;
        const uint8_t* p11 = src + y1 * stride + x1 * srcChannels;

        // Bilinear interpolation per channel, BGR to RGB + normalize
#pragma unroll
        for (int c = 0; c < 3; ++c)
        {
            // BGR channel index: 0=B, 1=G, 2=R
            // RGB output plane:  0=R, 1=G, 2=B  to  mapping: out_c = 2-c
            const int srcC = isRGB ? c : (2 - c); // BGR to RGB swap
            float val = (1.0f - ax) * (1.0f - ay) * p00[srcC]
                      + ax * (1.0f - ay) * p01[srcC]
                      + (1.0f - ax) * ay * p10[srcC]
                      + ax * ay * p11[srcC];
            dst[c * planeSize + idx] = val * normScale;
        }
    }
    else
    {
        // Padding pixel
        dst[0 * planeSize + idx] = padValue; // R
        dst[1 * planeSize + idx] = padValue; // G
        dst[2 * planeSize + idx] = padValue; // B
    }
}

// ============================================================================
// CHW-to-HWC + Normalize Kernel
// ============================================================================
__global__ void chwToRgbResizeKernel(
    const float* __restrict__ src,       // NCHW float input (3 channels, letterboxed)
    uint8_t* __restrict__ dst,           // HWC RGB/BGR output (original size)
    int srcW, int srcH,                  // source dimensions (letterbox size, e.g., 640x640)
    int dstW, int dstH,                  // destination dimensions (original size, e.g., 1920x1080)
    int newW, int newH,                  // valid image region in src (resized content)
    int padLeft, int padTop,             // padding offsets in src
    float denormScale, bool isRGB)
{
    const int x = blockIdx.x * blockDim.x + threadIdx.x;  // dst column (original width)
    const int y = blockIdx.y * blockDim.y + threadIdx.y;  // dst row (original height)
    if (x >= dstW || y >= dstH) return;

    // Map destination coordinate back to source (letterboxed) coordinate
    // Inverse of: newW = round(srcW * scale), we need to find where (x,y) in original
    // maps to in the letterboxed src image

    // Scale factor from original to letterbox content
    const float scaleX = static_cast<float>(newW) / dstW;
    const float scaleY = static_cast<float>(newH) / dstH;

    // Position in the valid content region of src
    const float srcXf = x * scaleX + padLeft;
    const float srcYf = y * scaleY + padTop;

    // Bilinear interpolation coordinates
    const int x0 = static_cast<int>(floorf(srcXf));
    const int y0 = static_cast<int>(floorf(srcYf));
    const int x1 = min(x0 + 1, srcW - 1);
    const int y1 = min(y0 + 1, srcH - 1);
    const float ax = srcXf - x0, ay = srcYf - y0;
    const int planeSize = srcH * srcW;

    // Clamp helper
    auto clamp01 = [](float v) -> uint8_t {
        v = fmaxf(0.0f, fminf(1.0f, v));
        return static_cast<uint8_t>(v * 255.0f + 0.5f);
    };

    // Bilinear sample each channel
#pragma unroll
    for (int c = 0; c < 3; ++c)
    {
        const float* plane = src + c * planeSize;
        float v00 = plane[y0 * srcW + x0];
        float v01 = plane[y0 * srcW + x1];
        float v10 = plane[y1 * srcW + x0];
        float v11 = plane[y1 * srcW + x1];

        float val = (1.0f - ax) * (1.0f - ay) * v00
                  + ax * (1.0f - ay) * v01
                  + (1.0f - ax) * ay * v10
                  + ax * ay * v11;

        // Determine output channel index based on isRGB
        int dstC = isRGB ? c : (2 - c);  // RGB: 0,1,2; BGR: 2,1,0
        dst[(y * dstW + x) * 3 + dstC] = clamp01(val);
    }
}

// ============================================================================
// Host-Side Launcher
// ============================================================================
void convertToCHW(const uint8_t* d_src, int srcW, int srcH, bool isRGB, bool hasAlpha,
                  float* d_dst, int dstW, int dstH, musaStream_t stream)
{
    // Compute resize dimensions (maintain aspect ratio)
    const float scale = fminf(static_cast<float>(dstH) / srcH,
                              static_cast<float>(dstW) / srcW);
    const int newW = static_cast<int>(roundf(srcW * scale));
    const int newH = static_cast<int>(roundf(srcH * scale));

    // Ultralytics-compatible asymmetric padding
    const float dw = (dstW - newW) / 2.0f, dh = (dstH - newH) / 2.0f;
    const int padLeft = static_cast<int>(roundf(dw - 0.1f));
    const int padTop = static_cast<int>(roundf(dh - 0.1f));

    constexpr float normScale = 1.0f / 255.0f;
    constexpr float padValue = 114.0f / 255.0f;

    // Launch kernel: one thread per destination pixel
    dim3 block(32, 8);
    dim3 grid((dstW + block.x - 1) / block.x, (dstH + block.y - 1) / block.y);
    letterboxNormalizeKernel<<<grid, block, 0, stream>>> (
        d_src, d_dst, srcW, srcH, dstW, dstH, newW, newH,
        padLeft, padTop, normScale, padValue, isRGB, hasAlpha);
}

void convertFromCHW(const float* d_src, int srcW, int srcH,
                    uint8_t* d_dst, int dstW, int dstH, bool isRGB, musaStream_t stream)
{
    const float scale = fminf(static_cast<float>(srcH) / dstH,
                              static_cast<float>(srcW) / dstW);
    const int newW = static_cast<int>(roundf(dstW * scale));
    const int newH = static_cast<int>(roundf(dstH * scale));

    const float dw = (srcW - newW) / 2.0f, dh = (srcH - newH) / 2.0f;
    const int padLeft = static_cast<int>(roundf(dw - 0.1f));
    const int padTop = static_cast<int>(roundf(dh - 0.1f));
    constexpr float denormScale = 255.0f;

    // Launch kernel: one thread per destination pixel
    dim3 block(32, 8);
    dim3 grid((dstW + block.x - 1) / block.x, (dstH + block.y - 1) / block.y);
    chwToRgbResizeKernel<<<grid, block, 0, stream>>> (
        d_src, d_dst, srcW, srcH, dstW, dstH, newW, newH,
        padLeft, padTop, denormScale, isRGB);
}

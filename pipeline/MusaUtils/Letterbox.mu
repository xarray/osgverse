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
    bool isRGB)
{
    const int x = blockIdx.x * blockDim.x + threadIdx.x; // dst column
    const int y = blockIdx.y * blockDim.y + threadIdx.y; // dst row

    if (x >= dstW || y >= dstH) return;
    const int planeSize = dstH * dstW;
    const int idx = y * dstW + x;

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
        const int stride = srcW * 3;
        const uint8_t* p00 = src + y0 * stride + x0 * 3;
        const uint8_t* p01 = src + y0 * stride + x1 * 3;
        const uint8_t* p10 = src + y1 * stride + x0 * 3;
        const uint8_t* p11 = src + y1 * stride + x1 * 3;

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
// Host-Side Launcher
// ============================================================================
void letterbox(const uint8_t* d_src, int srcW, int srcH, bool isRGB,
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
    letterboxNormalizeKernel << <grid, block, 0, stream>> > (
        d_src, d_dst, srcW, srcH, dstW, dstH, newW, newH,
        padLeft, padTop, normScale, padValue, isRGB);
}

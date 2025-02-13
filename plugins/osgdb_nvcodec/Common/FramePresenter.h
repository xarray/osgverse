/*
* Copyright 2017-2024 NVIDIA Corporation.  All rights reserved.
*
* Please refer to the NVIDIA end user license agreement (EULA) associated
* with this source code for terms and conditions that govern your use of
* this software. Any use, reproduction, disclosure, or distribution of
* this software and related documentation outside the terms of the EULA
* is strictly prohibited.
*
*/
#include <atomic>

#pragma once

// Base class for GLX and GLUT code paths.
// Declares essintial function as pure virtual.
class FramePresenter {

    public:

    FramePresenter(): nFrame(0), width(0), height(0) {
    }

    virtual ~FramePresenter(){
    }

    std::atomic<bool> endOfDecoding ; /*!< Indicates end of decoding for rendering thread to exit */
    std::atomic<bool> endOfRendering; /*!< Indicates end of rendering to ensure main thread does not
                              release graphics resources when they are being used by rendering thread */

    int nFrame;          /*!< Variable to indicate the current count of decoded frames */

    int width, height;   /*!< Denotes dimensions of the decoded frame */

    virtual void initWindowSystem() = 0;
    virtual void initOpenGLResources() = 0;
    virtual void releaseWindowSystem() = 0;

    virtual void Render() = 0;
    virtual bool isVendorNvidia() = 0;
    virtual int getWidth() = 0;
    virtual bool GetDeviceFrameBuffer(CUdeviceptr*, int *) = 0;
    virtual void ReleaseDeviceFrameBuffer() = 0;
};

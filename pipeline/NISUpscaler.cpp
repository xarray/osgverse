// NISUpscaler.cpp - Implementation of NVIDIA Image Scaling for osgVerse
//
// This file implements NISUpscaler which wraps NVIDIA Image Scaling SDK
// compute shaders for use in osgVerse pipeline stages.
//
// Design principles:
//   - NIS_Config.h and NIS_Scaler.h are NOT modified (used as-is from SDK)
//   - All GL 4.5 compute dispatch is done through osg::GLExtensions
//   - Coefficient textures are generated from constexpr arrays in NIS_Config.h
//   - Uniform buffer matches std140 layout of GLSL uniform block

#include "NISUpscaler.h"
#include <osg/Version>
#include <osg/FrameBufferObject>
#include <osg/GLExtensions>
#include <osg/Shader>
#include <osg/DeleteHandler>
#include <osgDB/FileUtils>
#include <sstream>
#include <cstring>
#include "NIS_Config.h"

// GL defines for compatibility
#ifndef GL_COMPUTE_SHADER
#define GL_COMPUTE_SHADER 0x91B9
#endif

#ifndef GL_RGBA32F
#define GL_RGBA32F 0x8814
#endif

#ifndef GL_WRITE_ONLY
#define GL_WRITE_ONLY 0x88B9
#endif

namespace osgVerse
{
    // ========================================================================
    // Embedded GLSL compute shader source
    // Uses #include "NIS_Scaler.h" then overrides texture macros for OSG.
    // Alternatively, load from external file via OSG shader path.
    // ========================================================================
    static const char* s_nisComputeGlsl = R"GLSL(
#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define NIS_GLSL 1
#define NIS_SCALER %NIS_SCALER%
#define NIS_HDR_MODE %NIS_HDR_MODE%
#define NIS_BLOCK_WIDTH %NIS_BLOCK_WIDTH%
#define NIS_BLOCK_HEIGHT %NIS_BLOCK_HEIGHT%
#define NIS_THREAD_GROUP_SIZE %NIS_THREAD_GROUP_SIZE%
#define NIS_VIEWPORT_SUPPORT 0
#define NIS_CLAMP_OUTPUT 1

#include "NIS_Scaler.h"

// Override NIS texture macros for OpenGL sampler2D direct binding.
// NIS_Scaler.h originally defines these for Vulkan separate image/sampler.
#undef NVTEX_LOAD
#undef NVTEX_SAMPLE
#undef NVTEX_SAMPLE_RED
#undef NVTEX_SAMPLE_GREEN
#undef NVTEX_SAMPLE_BLUE
#undef NVTEX_STORE
#define NVTEX_LOAD(x, pos) texelFetch(x, pos, 0)
#define NVTEX_SAMPLE(x, sampler, pos) textureLod(x, pos, 0)
#define NVTEX_SAMPLE_RED(x, sampler, pos) textureGather(x, pos, 0)
#define NVTEX_SAMPLE_GREEN(x, sampler, pos) textureGather(x, pos, 1)
#define NVTEX_SAMPLE_BLUE(x, sampler, pos) textureGather(x, pos, 2)
#define NVTEX_STORE(x, pos, v) imageStore(x, NVI2(pos), v)

// Uniform block - std140 layout matches NISConfig struct from NIS_Config.h
layout(std140, binding=0) uniform NISConfigBlock
{
    float kDetectRatio;
    float kDetectThres;
    float kMinContrastRatio;
    float kRatioNorm;
    float kContrastBoost;
    float kEps;
    float kSharpStartY;
    float kSharpScaleY;
    float kSharpStrengthMin;
    float kSharpStrengthScale;
    float kSharpLimitMin;
    float kSharpLimitScale;
    float kScaleX;
    float kScaleY;
    float kDstNormX;
    float kDstNormY;
    float kSrcNormX;
    float kSrcNormY;
    uint  kInputViewportOriginX;
    uint  kInputViewportOriginY;
    uint  kInputViewportWidth;
    uint  kInputViewportHeight;
    uint  kOutputViewportOriginX;
    uint  kOutputViewportOriginY;
    uint  kOutputViewportWidth;
    uint  kOutputViewportHeight;
    float reserved0;
    float reserved1;
};

layout(binding=1) uniform sampler2D in_texture;
layout(binding=2, rgba8) uniform writeonly image2D out_texture;

#if NIS_SCALER
layout(binding=3) uniform sampler2D coef_scaler;
layout(binding=4) uniform sampler2D coef_usm;
#endif

layout(local_size_x=%NIS_THREAD_GROUP_SIZE%) in;
void main()
{
#if NIS_SCALER
    NVScaler(gl_WorkGroupID.xy, gl_LocalInvocationID.x);
#else
    NVSharpen(gl_WorkGroupID.xy, gl_LocalInvocationID.x);
#endif
}
)GLSL";

    // ========================================================================
    NISUpscaler::NISUpscaler(Mode mode, GPUArchitecture gpuArch)
    :   _mode(mode), _inputWidth(0), _inputHeight(0), _outputWidth(0), _outputHeight(0),
        _sharpness(0.5f), _hdrMode(HDR_NONE), _uboId(0), _initialized(false)
    {
        _optimizer = new NISOptimizer(mode == SCALER, (NISGPUArchitecture)gpuArch);
        _blockWidth = _optimizer->GetOptimalBlockWidth();
        _blockHeight = _optimizer->GetOptimalBlockHeight();
        _threadGroupSize = _optimizer->GetOptimalThreadGroupSize();

        _config = new NISConfig;
        OSG_INFO << "[NISUpscaler] Created " << getModeName()
                 << " (block=" << _blockWidth << "x" << _blockHeight
                 << ", threads=" << _threadGroupSize << ")" << std::endl;
    }

    NISUpscaler::~NISUpscaler()
    {
        destroyGLObjects();
        delete _config; delete _optimizer;
    }

    void NISUpscaler::destroyGLObjects()
    {
        if (_uboId != 0)
        {
            // FIXME: actual GL deletion should happen in GL context
            ////osg::ref_ptr<osg::DeleteHandler> dh;
            _uboId = 0;
        }

        _program = NULL;
        _coefScalerTexture = NULL;
        _coefUSMTexture = NULL;
        _inputTexture = NULL;
        _outputTexture = NULL;
        _initialized = false;
    }

    bool NISUpscaler::initialize(int inputWidth, int inputHeight, int outputWidth, int outputHeight,
                                 float sharpness, HDRMode hdr, const std::string& computeGLSL)
    {
        if (inputWidth <= 0 || inputHeight <= 0 || outputWidth <= 0 || outputHeight <= 0)
        {
            OSG_WARN << "[NISUpscaler] Invalid dimensions: input=" << inputWidth << "x" << inputHeight
                     << ", output=" << outputWidth << "x" << outputHeight << std::endl;
            return false;
        }
        _inputWidth = inputWidth; _inputHeight = inputHeight;
        _outputWidth = outputWidth; _outputHeight = outputHeight;
        _sharpness = sharpness; _hdrMode = hdr;

        // Validate scaler constraints: 0.5 <= scale <= 1.0
        if (_mode == SCALER)
        {
            float sx = float(inputWidth) / float(outputWidth);
            float sy = float(inputHeight) / float(outputHeight);
            if (sx < 0.5f || sx > 1.0f || sy < 0.5f || sy > 1.0f)
            {
                OSG_WARN << "[NISUpscaler] SCALER requires 0.5 <= scale <= 1.0, got "
                         << sx << "x" << sy << std::endl;
                return false;
            }
        }

        destroyGLObjects();
        if (!createCoefficientTextures()) return false;  // Create coefficient textures (scaler needs both, sharpen none)
        if (!createShaderProgram(computeGLSL)) return false;  // Compile compute shader program

        // Generate UBO handle (upload deferred until first dispatch with valid context)
        _uboId = 99999; // marker; actual GL object created on first dispatch

        // Update NISConfig and prepare UBO data
        if (!updateConfig(sharpness, hdr))
        { OSG_WARN << "[NISUpscaler] Config update failed" << std::endl; return false; }

        OSG_NOTICE << "[NISUpscaler] Initialized " << getModeName()
                   << ": " << inputWidth << "x" << inputHeight
                   << " -> " << outputWidth << "x" << outputHeight << std::endl;
        _initialized = true; return true;
    }

    bool NISUpscaler::updateConfig(float sharpness, HDRMode hdr)
    {
        NISHDRMode nisHdr = NISHDRMode::None;
        switch (hdr)
        {
            case HDR_LINEAR: nisHdr = NISHDRMode::Linear; break;
            case HDR_PQ:     nisHdr = NISHDRMode::PQ; break;
            default:         nisHdr = NISHDRMode::None; break;
        }

        bool ok = false; _sharpness = sharpness; _hdrMode = hdr;
        if (_mode == SCALER)
        {
            ok = NVScalerUpdateConfig(*_config, sharpness,
                0, 0, _inputWidth, _inputHeight, _inputWidth, _inputHeight,
                0, 0, _outputWidth, _outputHeight, _outputWidth, _outputHeight, nisHdr);
        }
        else
        {
            ok = NVSharpenUpdateConfig(*_config, sharpness,
                0, 0, _inputWidth, _inputHeight, _inputWidth, _inputHeight, 0, 0, nisHdr);
        }
        if (ok) updateUniformBuffer();
        return ok;
    }

    bool NISUpscaler::createCoefficientTextures()
    {
        // NIS uses 64 phases x 8 taps (scaler uses first 6, sharpen uses all 8)
        const int texW = 2;   // 8 floats / 4 (rgba) = 2 texels per phase row
        const int texH = 64;  // kPhaseCount

        // Scaler coefficients (coef_scale from NIS_Config.h anonymous namespace)
        {
            osg::ref_ptr<osg::Image> img = new osg::Image;
            img->allocateImage(texW, texH, 1, GL_RGBA, GL_FLOAT);
            std::memset(img->data(), 0, img->getTotalSizeInBytes());

            float* dst = (float*)img->data();
            for (int p = 0; p < 64; ++p)
                for (int t = 0; t < 8; ++t)
                    dst[p * texW * 4 + t] = coef_scale[p][t];

            _coefScalerTexture = new osg::Texture2D(img.get());
            _coefScalerTexture->setInternalFormat(GL_RGBA32F);
            _coefScalerTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
            _coefScalerTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
            _coefScalerTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            _coefScalerTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        }

        // USM coefficients (coef_usm from NIS_Config.h anonymous namespace)
        {
            osg::ref_ptr<osg::Image> img = new osg::Image;
            img->allocateImage(texW, texH, 1, GL_RGBA, GL_FLOAT);
            std::memset(img->data(), 0, img->getTotalSizeInBytes());

            float* dst = (float*)img->data();
            for (int p = 0; p < 64; ++p)
                for (int t = 0; t < 8; ++t)
                    dst[p * texW * 4 + t] = coef_usm[p][t];

            _coefUSMTexture = new osg::Texture2D(img.get());
            _coefUSMTexture->setInternalFormat(GL_RGBA32F);
            _coefUSMTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
            _coefUSMTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
            _coefUSMTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            _coefUSMTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        }
        return true;
    }

    bool NISUpscaler::createShaderProgram(const std::string& computeGLSL)
    {
        // Build shader source by substituting template parameters
        std::string src = computeGLSL.empty() ? s_nisComputeGlsl: computeGLSL;
        std::string scalerVal = (_mode == SCALER) ? "1" : "0";
        std::string hdrVal = (_hdrMode == HDR_LINEAR) ? "1" : ((_hdrMode == HDR_PQ) ? "2" : "0");
        std::string bw = std::to_string(_blockWidth);
        std::string bh = std::to_string(_blockHeight);
        std::string tgs = std::to_string(_threadGroupSize);

        auto replace = [&src](const std::string& key, const std::string& val)
        {
            size_t pos = 0;
            while ((pos = src.find(key)) != std::string::npos)
                src.replace(pos, key.length(), val);
        };
        replace("%NIS_SCALER%", scalerVal);
        replace("%NIS_HDR_MODE%", hdrVal);
        replace("%NIS_BLOCK_WIDTH%", bw);
        replace("%NIS_BLOCK_HEIGHT%", bh);
        replace("%NIS_THREAD_GROUP_SIZE%", tgs);

#if OSG_VERSION_GREATER_THAN(3, 1, 5)
        osg::ref_ptr<osg::Shader> cs = new osg::Shader(osg::Shader::COMPUTE, src);
        cs->setName(std::string("NIS_") + getModeName());
        _program = new osg::Program; _program->setName(getModeName());
        _program->addShader(cs.get()); return true;
#else
        OSG_WARN << "[NISUpscaler] Compute shader not available" << std::endl;
        return false;
#endif
    }

    void NISUpscaler::updateUniformBuffer()
    {
        // UBO data is kept in _config; uploaded to GPU in dispatch()
        // No action needed here since _config is already updated
    }

    void NISUpscaler::dispatch(osg::State* state)
    {
        unsigned int ctxID = state->getContextID();
        if (!_initialized || !_program.valid()) return;
#if OSG_VERSION_GREATER_THAN(3, 5, 0)
        osg::GLExtensions* ext = state->get<osg::GLExtensions>();
        if (!ext || !ext->glDispatchCompute)
        {
            OSG_WARN << "[NISUpscaler] glDispatchCompute not available (needs GL 4.3+)" << std::endl;
            return;
        }

        // Compile and bind shader program
        _program->compileGLObjects(*state);
        _program->apply(*state);

        // Ensure textures have GL objects
        if (_inputTexture.valid()) _inputTexture->apply(*state);
        if (_outputTexture.valid()) _outputTexture->apply(*state);
        if (_coefScalerTexture.valid()) _coefScalerTexture->apply(*state);
        if (_coefUSMTexture.valid()) _coefUSMTexture->apply(*state);

        // --- Create UBO on first dispatch (need active GL context) ---
        if (_uboId == 99999)
        {
            ext->glGenBuffers(1, &_uboId);
            if (_uboId == 0) { OSG_WARN << "[NISUpscaler] Failed to generate UBO" << std::endl; return; }
            ext->glBindBuffer(GL_UNIFORM_BUFFER, _uboId);
            ext->glBufferData(GL_UNIFORM_BUFFER, sizeof(NISConfig), &_config, GL_DYNAMIC_DRAW);
        }
        else
        {   // Update existing UBO
            ext->glBindBuffer(GL_UNIFORM_BUFFER, _uboId);
            ext->glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(NISConfig), &_config);
        }
        ext->glBindBufferBase(GL_UNIFORM_BUFFER, 0, _uboId);

        // --- Bind textures to units ---
        if (_inputTexture.valid() && _inputTexture->getTextureObject(ctxID))
        {   // Unit 1: input texture
            ext->glActiveTexture(GL_TEXTURE0 + 1);
            _inputTexture->apply(*state);
        }

        // Unit 2: output as image (for imageStore)
        if (_outputTexture.valid() && _outputTexture->getTextureObject(ctxID))
        {
            GLuint texId = _outputTexture->getTextureObject(ctxID)->id();
            ext->glBindImageTexture(2, texId, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
        }

        // Units 3,4: coefficient textures (scaler only)
        if (_mode == SCALER)
        {
            if (_coefScalerTexture.valid() && _coefScalerTexture->getTextureObject(ctxID))
            {
                ext->glActiveTexture(GL_TEXTURE0 + 3);
                _coefScalerTexture->apply(*state);
            }
            if (_coefUSMTexture.valid() && _coefUSMTexture->getTextureObject(ctxID))
            {
                ext->glActiveTexture(GL_TEXTURE0 + 4);
                _coefUSMTexture->apply(*state);
            }
        }
        ext->glActiveTexture(GL_TEXTURE0);

        // --- Dispatch compute ---
        unsigned int groupsX = (_outputWidth  + _blockWidth  - 1) / _blockWidth;
        unsigned int groupsY = (_outputHeight + _blockHeight - 1) / _blockHeight;
        ext->glDispatchCompute(groupsX, groupsY, 1);

        // Ensure image writes complete before subsequent reads
        ext->glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // FIXME: Restore default program (let OSG manage state)?
        // state->apply(static_cast<osg::Program*>(NULL));
#else
        OSG_WARN << "[NISUpscaler] glDispatchCompute not available (needs OSG 3.5+)" << std::endl;
#endif
    }

    void NISUpscaler::applyShader(osg::StateSet* ss) const
    { if (_program.valid()) ss->setAttribute(_program.get()); }
}

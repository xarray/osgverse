#include "Pipeline.h"
#include "ShadowModule.h"
#include "LightModule.h"
#include "Utilities.h"

#include <osg/DisplaySettings>
#include <osgDB/ReadFile>
#include <osgDB/FileNameUtils>
#include <osgViewer/Viewer>
#define VERT osg::Shader::VERTEX
#define FRAG osg::Shader::FRAGMENT

class GLExtensionTester : public osg::Camera::DrawCallback
{
public:
    GLExtensionTester(osgVerse::Pipeline* p)
    {
        _data = new osgVerse::GLVersionData;
        _data->glVersion = 0.0f; _data->glslVersion = 0.0f;
        _data->glslSupported = false;
        if (p) p->setVersionData(_data.get());
    }

    virtual void operator()(osg::RenderInfo& renderInfo) const
    {
        osgVerse::GLVersionData* d = const_cast<osgVerse::GLVersionData*>(_data.get());
#if OSG_VERSION_GREATER_THAN(3, 3, 2)
        osg::GLExtensions* ext = renderInfo.getState()->get<osg::GLExtensions>();
        d->glVersion = ext->glVersion * 100;
        d->glslVersion = ext->glslLanguageVersion * 100;
        d->glslSupported = ext->isGlslSupported;
#else
        osg::GL2Extensions* ext = osg::GL2Extensions::Get(renderInfo.getContextID(), true);
        d->glVersion = ext->getGlVersion() * 100;
        d->glslVersion = ext->getLanguageVersion() * 100;
        d->glslSupported = ext->isGlslSupported();
#endif
        
        const char* versionString = (const char*)glGetString(GL_VERSION);
        const char* rendererString = (const char*)glGetString(GL_RENDERER);
        if (versionString != NULL) d->version = versionString;
        if (rendererString != NULL) d->renderer = rendererString;
    }

protected:
    osg::ref_ptr<osgVerse::GLVersionData> _data;
};

#if defined(VERSE_WINDOWS)
#include <windows.h>
static void obtainScreenResolution(unsigned int& w, unsigned int& h)
{
    HWND hd = ::GetDesktopWindow(); RECT rect; ::GetWindowRect(hd, &rect);
    w = (rect.right - rect.left); h = (rect.bottom - rect.top);
    OSG_NOTICE << "[obtainScreenResolution] Get screen size " << w << " x " << h << "\n";
}
#elif defined(VERSE_X11)
#include <X11/Xlib.h>
static void obtainScreenResolution(unsigned int& w, unsigned int& h)
{
    Display* disp = XOpenDisplay(NULL);
    Screen* screen = DefaultScreenOfDisplay(disp);
    w = screen->width; h = screen->height;
    OSG_NOTICE << "[obtainScreenResolution] Get screen size " << w << " x " << h << "\n";
}
#else
static void obtainScreenResolution(unsigned int& w, unsigned int& h)
{
    w = osg::DisplaySettings::instance()->getScreenWidth();
    h = osg::DisplaySettings::instance()->getScreenHeight();
    OSG_NOTICE << "[obtainScreenResolution] Get screen size " << w << " x " << h << "\n";
}
#endif

namespace osgVerse
{
    StandardPipelineParameters::StandardPipelineParameters()
    :   deferredMask(DEFERRED_SCENE_MASK), forwardMask(FORWARD_SCENE_MASK), shadowCastMask(SHADOW_CASTER_MASK),
        shadowNumber(0), shadowResolution(2048), debugShadowModule(false)
    {
        obtainScreenResolution(originWidth, originHeight);
        if (!originWidth) originWidth = 1920; if (!originHeight) originHeight = 1080;
    }

    StandardPipelineParameters::StandardPipelineParameters(const std::string& dir, const std::string& sky)
    :   deferredMask(DEFERRED_SCENE_MASK), forwardMask(FORWARD_SCENE_MASK), shadowCastMask(SHADOW_CASTER_MASK),
        shadowNumber(3), shadowResolution(2048), debugShadowModule(false)
    {
        obtainScreenResolution(originWidth, originHeight);
        if (!originWidth) originWidth = 1920; if (!originHeight) originHeight = 1080;

        shaders.gbufferVS = osgDB::readShaderFile(VERT, dir + "std_gbuffer.vert.glsl");
        shaders.shadowCastVS = osgDB::readShaderFile(VERT, dir + "std_shadow_cast.vert.glsl");
        shaders.quadVS = osgDB::readShaderFile(VERT, dir + "std_common_quad.vert.glsl");

        shaders.gbufferFS = osgDB::readShaderFile(FRAG, dir + "std_gbuffer.frag.glsl");
        shaders.shadowCastFS = osgDB::readShaderFile(FRAG, dir + "std_shadow_cast.frag.glsl");
        shaders.ssaoFS = osgDB::readShaderFile(FRAG, dir + "std_ssao.frag.glsl");
        shaders.ssaoBlurFS = osgDB::readShaderFile(FRAG, dir + "std_ssao_blur.frag.glsl");
        shaders.pbrLightingFS = osgDB::readShaderFile(FRAG, dir + "std_pbr_lighting.frag.glsl");
        shaders.shadowCombineFS = osgDB::readShaderFile(FRAG, dir + "std_shadow_combine.frag.glsl");
        shaders.downsampleFS = osgDB::readShaderFile(FRAG, dir + "std_luminance_downsample.frag.glsl");
        shaders.brightnessFS = osgDB::readShaderFile(FRAG, dir + "std_brightness_extraction.frag.glsl");
        shaders.brightnessCombineFS = osgDB::readShaderFile(FRAG, dir + "std_brightness_combine.frag.glsl");
        shaders.bloomFS = osgDB::readShaderFile(FRAG, dir + "std_brightness_bloom.frag.glsl");
        shaders.tonemappingFS = osgDB::readShaderFile(FRAG, dir + "std_tonemapping.frag.glsl");
        shaders.antiAliasingFS = osgDB::readShaderFile(FRAG, dir + "std_antialiasing.frag.glsl");
        shaders.brdfLutFS = osgDB::readShaderFile(FRAG, dir + "std_brdf_lut.frag.glsl");
        shaders.envPrefilterFS = osgDB::readShaderFile(FRAG, dir + "std_environment_prefiltering.frag.glsl");
        shaders.irrConvolutionFS = osgDB::readShaderFile(FRAG, dir + "std_irradiance_convolution.frag.glsl");
        shaders.quadFS = osgDB::readShaderFile(FRAG, dir + "std_common_quad.frag.glsl");
        shaders.displayFS = osgDB::readShaderFile(FRAG, dir + "std_display.frag.glsl");

        std::string iblFile = osgDB::getNameLessExtension(sky) + ".ibl.osgb";
        skyboxIBL = dynamic_cast<osg::StateSet*>(osgDB::readObjectFile(iblFile));
        skyboxMap = osgVerse::createTexture2D(osgDB::readImageFile(sky), osg::Texture::MIRROR);
        if (!skyboxMap || !skyboxIBL)
        {
            OSG_NOTICE << "[StandardPipelineParameters] Skybox " << sky
                       << " or its IBL data is invalid. Will try generating IBL data at runtime.";
        }
        else if (skyboxMap.valid())
        {
            skyboxMap->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
            skyboxMap->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        }
    }

    GLVersionData* queryOpenGLVersion(Pipeline* p)
    {
        osgViewer::Viewer tempViewer;
        GLExtensionTester* tester = new GLExtensionTester(p);
        tempViewer.getCamera()->setPreDrawCallback(tester);

        tempViewer.setSceneData(new osg::Node);
        tempViewer.setUpViewInWindow(0, 0, 1, 1);
        tempViewer.realize();
        for (int i = 0; i < 5; ++i) tempViewer.frame();

        tempViewer.setDone(true);
        return p ? p->getVersionData() : NULL;
    }

    void setupStandardPipeline(osgVerse::Pipeline* p, osgViewer::View* view,
                               const StandardPipelineParameters& spp)
    {
        if (!view)
        {
            OSG_WARN << "[StandardPipeline] No view provided." << std::endl;
            return;
        }
#if OSG_VERSION_GREATER_THAN(3, 2, 0)
        else if (!view->getLastAppliedViewConfig() && !view->getCamera()->getGraphicsContext())
#else
        else if (!view->getCamera()->getGraphicsContext())
#endif
        {
            OSG_NOTICE << "[StandardPipeline] No view config applied. The pipeline will be constructed "
                       << "with provided parameters. Please DO NOT apply any view config like "
                       << "setUpViewInWindow() or setUpViewAcrossAllScreens() AFTER you called "
                       << "setupStandardPipeline(). It may cause problems!!" << std::endl;
        }

        GLVersionData* data = p->getVersionData() ? NULL : queryOpenGLVersion(p);
        p->startStages(spp.originWidth, spp.originHeight, view->getCamera()->getGraphicsContext());
        if (data)
        {
            OSG_NOTICE << "[StandardPipeline] OpenGL Driver: " << data->version << "; GLSL: "
                       << data->glslVersion << "; Renderer: " << data->renderer << std::endl;
            OSG_NOTICE << "[StandardPipeline] Using OpenGL Context: " << p->getTargetVersion()
                       << "; Using GLSL Version: "<< p->getGlslTargetVersion() << std::endl;
            if (data->renderer.find("MTT") != std::string::npos)
            {
#ifndef VERSE_ENABLE_MTT
                OSG_WARN << "[StandardPipeline] It seems you are using MooreThreads graphics "
                         << "driver but not setting VERSE_USE_MTT_DRIVER in CMake. It may cause "
                         << "unexpected problems at present." << std::endl;
#endif
            }
        }

        // GBuffer should always be first because it also computes the scene near/far planes
        // for following stages to use
        int msaa = 0;  // FIXME: seems to cause some more flickers
        osgVerse::Pipeline::Stage* gbuffer = p->addInputStage("GBuffer", spp.deferredMask, msaa,
            spp.shaders.gbufferVS, spp.shaders.gbufferFS, 5,
            "NormalBuffer", osgVerse::Pipeline::RGBA_INT8,
            "DiffuseMetallicBuffer", osgVerse::Pipeline::RGBA_INT8,
            "SpecularRoughnessBuffer", osgVerse::Pipeline::RGBA_INT8,
            "EmissionOcclusionBuffer", osgVerse::Pipeline::RGBA_FLOAT16,
#ifndef VERSE_ENABLE_MTT
            "DepthBuffer", osgVerse::Pipeline::DEPTH24_STENCIL8);
#else
            "DepthBuffer", osgVerse::Pipeline::DEPTH32);
#endif

        // Shadow module initialization
        osg::ref_ptr<osgVerse::ShadowModule> shadowModule =
            new osgVerse::ShadowModule("Shadow", p, spp.debugShadowModule);
        shadowModule->createStages(spp.shadowResolution, spp.shadowNumber,
            spp.shaders.shadowCastVS, spp.shaders.shadowCastFS, spp.shadowCastMask);

        // Update shadow matrices at the end of g-buffer (when near/far planes are sync-ed)
        osg::ref_ptr<osgVerse::ShadowDrawCallback> shadowCallback =
                new osgVerse::ShadowDrawCallback(shadowModule.get());
        shadowCallback->setup(gbuffer->camera.get(), FINAL_DRAW);
        view->getCamera()->addUpdateCallback(shadowModule.get());

        // Light module only needs to be added to main camera
        osg::ref_ptr<osgVerse::LightModule> lightModule = new osgVerse::LightModule("Light", p);
        view->getCamera()->addUpdateCallback(lightModule.get());

        // IBL related textures can be read from files or from run-once stages
        osgVerse::Pipeline::Stage *brdfLut = NULL, *prefiltering = NULL, *convolution = NULL;
        osg::Texture *brdfLutTex = NULL, *prefilteringTex = NULL, *convolutionTex = NULL;
        if (!spp.skyboxIBL)
        {
            brdfLut = p->addDeferredStage("BrdfLut", 1.0f, true,
                    spp.shaders.quadVS, spp.shaders.brdfLutFS, 1,
                    "BrdfLutBuffer", osgVerse::Pipeline::RG_FLOAT16);

            prefiltering = p->addDeferredStage("Prefilter", 1.0f, true,
                    spp.shaders.quadVS, spp.shaders.envPrefilterFS, 1,
                    "PrefilterBuffer", osgVerse::Pipeline::RGB_INT8);
            prefiltering->applyTexture(spp.skyboxMap.get(), "EnvironmentMap", 0);
            prefiltering->applyUniform(new osg::Uniform("GlobalRoughness", 4.0f));

            convolution = p->addDeferredStage("IrrConvolution", 1.0f, true,
                    spp.shaders.quadVS, spp.shaders.irrConvolutionFS, 1,
                    "IrradianceBuffer", osgVerse::Pipeline::RGB_INT8);
            convolution->applyTexture(spp.skyboxMap.get(), "EnvironmentMap", 0);
        }
        else
        {
            brdfLutTex = static_cast<osg::Texture*>(
                spp.skyboxIBL->getTextureAttribute(0, osg::StateAttribute::TEXTURE));
            brdfLutTex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
            brdfLutTex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

            prefilteringTex = static_cast<osg::Texture*>(
                spp.skyboxIBL->getTextureAttribute(1, osg::StateAttribute::TEXTURE));
            prefilteringTex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
            prefilteringTex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

            convolutionTex = static_cast<osg::Texture*>(
                spp.skyboxIBL->getTextureAttribute(2, osg::StateAttribute::TEXTURE));
            convolutionTex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
            convolutionTex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        }

        // SSAO stages: AO -> BlurH -> BlurV
        osgVerse::Pipeline::Stage* ssao = p->addWorkStage("Ssao", 1.0f,
                spp.shaders.quadVS, spp.shaders.ssaoFS, 1,
                "SsaoBuffer", osgVerse::Pipeline::R_INT8);
        ssao->applyBuffer(*gbuffer, "NormalBuffer", 0);
        ssao->applyBuffer(*gbuffer, "DepthBuffer", 1);
        ssao->applyTexture(generateNoises2D(4, 4), "RandomTexture", 2);
        ssao->applyUniform(new osg::Uniform("AORadius", 6.0f));
        ssao->applyUniform(new osg::Uniform("AOBias", 0.1f));
        ssao->applyUniform(new osg::Uniform("AOPowExponent", 12.0f));

        osgVerse::Pipeline::Stage* ssaoBlur1 = p->addWorkStage("SsaoBlur1", 1.0f,
                spp.shaders.quadVS, spp.shaders.ssaoBlurFS, 1,
                "SsaoBlurredBuffer0", osgVerse::Pipeline::R_INT8);
        ssaoBlur1->applyBuffer(*ssao, "SsaoBuffer", 0);
        ssaoBlur1->applyUniform(new osg::Uniform("BlurDirection", osg::Vec2(1.0f, 0.0f)));
        ssaoBlur1->applyUniform(new osg::Uniform("BlurSharpness", 40.0f));

        osgVerse::Pipeline::Stage* ssaoBlur2 = p->addWorkStage("SsaoBlur2", 1.0f,
                spp.shaders.quadVS, spp.shaders.ssaoBlurFS, 1,
                "SsaoBlurredBuffer", osgVerse::Pipeline::R_INT8);
        ssaoBlur2->applyBuffer(*ssaoBlur1, "SsaoBlurredBuffer0", "SsaoBuffer", 0);
        ssaoBlur2->applyUniform(new osg::Uniform("BlurDirection", osg::Vec2(0.0f, 1.0f)));
        ssaoBlur2->applyUniform(new osg::Uniform("BlurSharpness", 40.0f));

        // Deferred lighting stage
        osgVerse::Pipeline::Stage* lighting = p->addWorkStage("Lighting", 1.0f,
                spp.shaders.quadVS, spp.shaders.pbrLightingFS, 2,
                "ColorBuffer", osgVerse::Pipeline::RGB_FLOAT16,
                "IblAmbientBuffer", osgVerse::Pipeline::RGB_INT8);
        lighting->applyBuffer(*gbuffer, "NormalBuffer", 0);
        lighting->applyBuffer(*gbuffer, "DiffuseMetallicBuffer", 1);
        lighting->applyBuffer(*gbuffer, "SpecularRoughnessBuffer", 2);
        lighting->applyBuffer(*gbuffer, "EmissionOcclusionBuffer", 3);
        lighting->applyBuffer(*gbuffer, "DepthBuffer", 4);
        if (!spp.skyboxIBL)
        {
            lighting->applyBuffer(*brdfLut, "BrdfLutBuffer", 5, osg::Texture::MIRROR);
            lighting->applyBuffer(*prefiltering, "PrefilterBuffer", 6, osg::Texture::MIRROR);
            lighting->applyBuffer(*convolution, "IrradianceBuffer", 7, osg::Texture::MIRROR);
        }
        else
        {
            lighting->applyTexture(brdfLutTex, "BrdfLutBuffer", 5);
            lighting->applyTexture(prefilteringTex, "PrefilterBuffer", 6);
            lighting->applyTexture(convolutionTex, "IrradianceBuffer", 7);
        }
        lightModule->applyTextureAndUniforms(lighting, "LightParameterMap", 8);

        // Shadow & AO combining stage
        osgVerse::Pipeline::Stage* shadowing = p->addWorkStage("Shadowing", 1.0f,
                spp.shaders.quadVS, spp.shaders.shadowCombineFS, 1,
                "CombinedBuffer", osgVerse::Pipeline::RGB_FLOAT16);
        shadowing->applyBuffer(*lighting, "ColorBuffer", 0);
        shadowing->applyBuffer(*ssaoBlur2, "SsaoBlurredBuffer", 1);
        shadowing->applyBuffer(*gbuffer, "NormalBuffer", 2);
        shadowing->applyBuffer(*gbuffer, "DepthBuffer", 3);
        shadowing->applyTexture(generatePoissonDiscDistribution(16), "RandomTexture0", 4);
        shadowing->applyTexture(generatePoissonDiscDistribution(16), "RandomTexture1", 5);
        shadowModule->applyTextureAndUniforms(shadowing, "ShadowMap", 6);

        // Bloom stages: Brightness -> Downscaling x N -> Combine -> Bloom
        osgVerse::Pipeline::Stage* brighting = p->addDeferredStage("Brighting", 1.0f, false,
            spp.shaders.quadVS, spp.shaders.brightnessFS, 1,
            "BrightnessBuffer0", osgVerse::Pipeline::RGB_INT8);
        brighting->applyBuffer(*shadowing, "CombinedBuffer", "ColorBuffer", 0);
        brighting->applyUniform(new osg::Uniform("BrightnessThreshold", 0.7f));

        std::vector<osgVerse::Pipeline::Stage*> downsamples;
        osg::Vec2s stageSize = p->getStageSize();
        int downsampleIndex = 1; float downsampleValue = 1080.0f;
        downsamples.push_back(brighting);
        for (; downsampleValue > 2.0f; ++downsampleIndex)
        {
            float sizeScale = 1.0f / float(1 << downsampleIndex);
            downsampleValue = osg::maximum((int)stageSize[1], 1080) * sizeScale;
            osg::Vec2 invRes(1.0f / osg::maximum((int)stageSize[0], 1920) * sizeScale,
                             1.0f / downsampleValue);

            std::string id = std::to_string(downsampleIndex), lastId = std::to_string(downsampleIndex - 1);
            osgVerse::Pipeline::Stage* brightDownsampling = p->addDeferredStage(
                "Downsampling" + id, sizeScale, false, spp.shaders.quadVS, spp.shaders.downsampleFS, 1,
                ("BrightnessBuffer" + id).c_str(), osgVerse::Pipeline::RGB_INT8);
            brightDownsampling->applyBuffer(
                *downsamples.back(), "BrightnessBuffer" + lastId, "ColorBuffer", 0);
            brightDownsampling->applyUniform(new osg::Uniform("InvBufferResolution", invRes));
            downsamples.push_back(brightDownsampling);
        }

        osgVerse::Pipeline::Stage* brightCombining = p->addDeferredStage("BrightCombining", 1.0f, false,
            spp.shaders.quadVS, spp.shaders.brightnessCombineFS, 1,
            "BrightnessCombinedBuffer", osgVerse::Pipeline::RGB_INT8);
        for (size_t i = 1; i <= 4; ++i)
        {
            std::string id = std::to_string(i);
            brightCombining->applyBuffer(*downsamples[i], "BrightnessBuffer" + id, i - 1);
        }

        osgVerse::Pipeline::Stage* blooming = p->addDeferredStage("Blooming", 1.0f, false,
            spp.shaders.quadVS, spp.shaders.bloomFS, 1,
            "BloomBuffer", osgVerse::Pipeline::RGB_INT8);
        blooming->applyBuffer(*brightCombining, "BrightnessCombinedBuffer", 0);
        blooming->applyUniform(new osg::Uniform("BloomFactor", 1.0f));

        // Lensflare stages
        // TODO

        // Eye-adaption & Tonemapping stage
        std::string lastDs = std::to_string(downsamples.size() - 1);
        osgVerse::Pipeline::Stage* tonemapping = p->addDeferredStage("ToneMapping", 1.0f, false,
            spp.shaders.quadVS, spp.shaders.tonemappingFS, 1,
            "ToneMappedBuffer", osgVerse::Pipeline::RGB_FLOAT16);
        tonemapping->applyBuffer(*shadowing, "CombinedBuffer", "ColorBuffer", 0);
        tonemapping->applyBuffer(*downsamples.back(), "BrightnessBuffer" + lastDs, "LuminanceBuffer", 1);
        tonemapping->applyBuffer(*blooming, "BloomBuffer", 2);
        tonemapping->applyBuffer(*lighting, "IblAmbientBuffer", 3);
        tonemapping->applyUniform(new osg::Uniform("LuminanceFactor", osg::Vec2(1.0f, 10.0f)));

        // Anti-aliasing
        osgVerse::Pipeline::Stage* antiAliasing = p->addDeferredStage("AntiAliasing", 1.0f, false,
            spp.shaders.quadVS, spp.shaders.antiAliasingFS, 1,
            "AntiAliasedBuffer", osgVerse::Pipeline::RGB_FLOAT16);
        antiAliasing->applyBuffer(*tonemapping, "ToneMappedBuffer", "ColorBuffer", 0);

        // Final stage (color grading)
        osgVerse::Pipeline::Stage* output = p->addDisplayStage("Final",
                spp.shaders.quadVS, spp.shaders.displayFS, osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
        output->applyBuffer(*antiAliasing, "AntiAliasedBuffer", "ColorBuffer", 0);
        output->applyBuffer(*gbuffer, "DepthBuffer", 1);
        output->applyUniform(new osg::Uniform("FogDistance", osg::Vec2(0.0f, 0.0f)));
        output->applyUniform(new osg::Uniform("FogColor", osg::Vec3(0.5f, 0.5f, 0.5f)));
        output->applyUniform(new osg::Uniform("ColorAttribute", osg::Vec3(1.0f, 1.0f, 1.0f)));
        output->applyUniform(new osg::Uniform("ColorBalance", osg::Vec3(0.0f, 0.0f, 0.0f)));  // [-1, 1]
        output->applyUniform(new osg::Uniform("ColorBalanceMode", (int)0));
        output->applyUniform(new osg::Uniform("VignetteRadius", 1.0f));
        output->applyUniform(new osg::Uniform("VignetteDarkness", 0.0f));

        p->applyStagesToView(view, spp.forwardMask);
        p->requireDepthBlit(gbuffer, true);
    }
}

#undef VERT
#undef FRAG

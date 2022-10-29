#include "Pipeline.h"
#include "ShadowModule.h"
#include "LightModule.h"
#include "Utilities.h"
#include <osg/DisplaySettings>
#include <osgDB/ReadFile>
#include <osgDB/FileNameUtils>
#define VERT osg::Shader::VERTEX
#define FRAG osg::Shader::FRAGMENT

namespace osgVerse
{
    StandardPipelineParameters::StandardPipelineParameters()
    :   deferredMask(DEFERRED_SCENE_MASK), forwardMask(FORWARD_SCENE_MASK), shadowCastMask(SHADOW_CASTER_MASK),
        shadowNumber(0), shadowResolution(2048), debugShadowModule(false)
    {
        originWidth = osg::DisplaySettings::instance()->getScreenWidth();
        originHeight = osg::DisplaySettings::instance()->getScreenHeight();
        if (!originWidth) originWidth = 1920; if (!originHeight) originHeight = 1080;
    }

    StandardPipelineParameters::StandardPipelineParameters(const std::string& dir, const std::string& sky)
    :   deferredMask(DEFERRED_SCENE_MASK), forwardMask(FORWARD_SCENE_MASK), shadowCastMask(SHADOW_CASTER_MASK),
        shadowNumber(3), shadowResolution(2048), debugShadowModule(false)
    {
        originWidth = osg::DisplaySettings::instance()->getScreenWidth();
        originHeight = osg::DisplaySettings::instance()->getScreenHeight();
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
        shaders.displayFS = osgDB::readShaderFile(FRAG, dir + "std_display.frag.glsl");
        shaders.brdfLutFS = osgDB::readShaderFile(FRAG, dir + "std_brdf_lut.frag.glsl");
        shaders.envPrefilterFS = osgDB::readShaderFile(FRAG, dir + "std_environment_prefiltering.frag.glsl");
        shaders.irrConvolutionFS = osgDB::readShaderFile(FRAG, dir + "std_irradiance_convolution.frag.glsl");

        std::string iblFile = osgDB::getNameLessExtension(sky) + ".ibl.osgb";
        skyboxIBL = dynamic_cast<osg::StateSet*>(osgDB::readObjectFile(iblFile));
        skyboxMap = osgVerse::createTexture2D(osgDB::readImageFile(sky), osg::Texture::MIRROR);
        if (!skyboxMap || !skyboxIBL)
        {
            OSG_NOTICE << "[StandardPipelineParameters] Skybox " << sky
                       << " or its IBL data is invalid. Will try generating IBL data at runtime.";
        }
    }

    void setupStandardPipeline(osgVerse::Pipeline* p, osgViewer::View* view,
                               const StandardPipelineParameters& spp)
    {
        int msaa = 0;  // FIXME: seems to cause some more flickers
        p->startStages(spp.originWidth, spp.originHeight, view->getCamera()->getGraphicsContext());

        // GBuffer should always be first because it also computes the scene near/far planes
        // for following stages to use
        osgVerse::Pipeline::Stage* gbuffer = p->addInputStage("GBuffer", spp.deferredMask, msaa,
            spp.shaders.gbufferVS, spp.shaders.gbufferFS, 5,
            "NormalBuffer", osgVerse::Pipeline::RGBA_INT10_2,
            "DiffuseMetallicBuffer", osgVerse::Pipeline::RGBA_INT8,
            "SpecularRoughnessBuffer", osgVerse::Pipeline::RGBA_INT8,
            "EmissionOcclusionBuffer", osgVerse::Pipeline::RGBA_FLOAT16,
            "DepthBuffer", osgVerse::Pipeline::DEPTH24_STENCIL8);

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

        osgVerse::Pipeline::Stage *brdfLut = NULL, *prefiltering = NULL, *convolution = NULL;
        osg::Texture *brdfLutTex = NULL, *prefilteringTex = NULL, *convolutionTex = NULL;
        if (!spp.skyboxIBL)
        {
            brdfLut = p->addDeferredStage("BrdfLut", true,
                    spp.shaders.quadVS, spp.shaders.brdfLutFS, 1,
                    "BrdfLutBuffer", osgVerse::Pipeline::RG_FLOAT16);

            prefiltering = p->addDeferredStage("Prefilter", true,
                    spp.shaders.quadVS, spp.shaders.envPrefilterFS, 1,
                    "PrefilterBuffer", osgVerse::Pipeline::RGB_INT8);
            prefiltering->applyTexture(spp.skyboxMap.get(), "EnvironmentMap", 0);
            prefiltering->applyUniform(new osg::Uniform("GlobalRoughness", 4.0f));

            convolution = p->addDeferredStage("IrrConvolution", true,
                    spp.shaders.quadVS, spp.shaders.irrConvolutionFS, 1,
                    "IrradianceBuffer", osgVerse::Pipeline::RGB_INT8);
            convolution->applyTexture(spp.skyboxMap.get(), "EnvironmentMap", 0);
        }
        else
        {
            brdfLutTex = static_cast<osg::Texture*>(
                spp.skyboxIBL->getTextureAttribute(0, osg::StateAttribute::TEXTURE));
            prefilteringTex = static_cast<osg::Texture*>(
                spp.skyboxIBL->getTextureAttribute(1, osg::StateAttribute::TEXTURE));
            convolutionTex = static_cast<osg::Texture*>(
                spp.skyboxIBL->getTextureAttribute(2, osg::StateAttribute::TEXTURE));
        }

        osgVerse::Pipeline::Stage* ssao = p->addWorkStage("Ssao",
                spp.shaders.quadVS, spp.shaders.ssaoFS, 1,
                "SsaoBuffer", osgVerse::Pipeline::R_INT8);
        ssao->applyBuffer(*gbuffer, "NormalBuffer", 0);
        ssao->applyBuffer(*gbuffer, "DepthBuffer", 1);
        ssao->applyTexture(generateNoises2D(4, 4), "RandomTexture", 2);
        ssao->applyUniform(new osg::Uniform("AORadius", 4.0f));
        ssao->applyUniform(new osg::Uniform("AOBias", 0.1f));
        ssao->applyUniform(new osg::Uniform("AOPowExponent", 1.5f));

        osgVerse::Pipeline::Stage* ssaoBlur1 = p->addWorkStage("SsaoBlur1",
                spp.shaders.quadVS, spp.shaders.ssaoBlurFS, 1,
                "SsaoBlurredBuffer0", osgVerse::Pipeline::R_INT8);
        ssaoBlur1->applyBuffer(*ssao, "SsaoBuffer", 0);
        ssaoBlur1->applyUniform(new osg::Uniform("BlurDirection", osg::Vec2(1.0f, 0.0f)));
        ssaoBlur1->applyUniform(new osg::Uniform("BlurSharpness", 40.0f));

        osgVerse::Pipeline::Stage* ssaoBlur2 = p->addWorkStage("SsaoBlur2",
                spp.shaders.quadVS, spp.shaders.ssaoBlurFS, 1,
                "SsaoBlurredBuffer", osgVerse::Pipeline::R_INT8);
        ssaoBlur2->applyBuffer(*ssaoBlur1, "SsaoBlurredBuffer0", "SsaoBuffer", 0);
        ssaoBlur2->applyUniform(new osg::Uniform("BlurDirection", osg::Vec2(0.0f, 1.0f)));
        ssaoBlur2->applyUniform(new osg::Uniform("BlurSharpness", 40.0f));

        osgVerse::Pipeline::Stage* lighting = p->addWorkStage("Lighting",
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

        osgVerse::Pipeline::Stage* shadowing = p->addWorkStage("Shadowing",
                spp.shaders.quadVS, spp.shaders.shadowCombineFS, 1,
                "CombinedBuffer", osgVerse::Pipeline::RGB_FLOAT16);
        shadowing->applyBuffer(*lighting, "ColorBuffer", 0);
        shadowing->applyBuffer(*lighting, "IblAmbientBuffer", 1);
        shadowing->applyBuffer(*gbuffer, "NormalBuffer", 2);
        shadowing->applyBuffer(*gbuffer, "DepthBuffer", 3);
        shadowing->applyTexture(generatePoissonDiscDistribution(16), "RandomTexture0", 4);
        shadowing->applyTexture(generatePoissonDiscDistribution(16), "RandomTexture1", 5);
        shadowModule->applyTextureAndUniforms(shadowing, "ShadowMap", 6);

        osgVerse::Pipeline::Stage* output = p->addDisplayStage("Final",
                spp.shaders.quadVS, spp.shaders.displayFS, osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
        output->applyBuffer(*shadowing, "CombinedBuffer", 0);
        output->applyBuffer(*ssaoBlur2, "SsaoBlurredBuffer", 1);

        p->applyStagesToView(view, spp.forwardMask);
        p->requireDepthBlit(gbuffer, true);
    }
}

#undef VERT
#undef FRAG

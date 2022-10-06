#include "Pipeline.h"
#include "ShadowModule.h"
#include "Utilities.h"
#include <osgDB/ReadFile>
#include <random>

#define DEBUG_SHADOW_MODULE 0
static std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
static std::default_random_engine generator;

static osg::Uniform* generateSsaoSamples(int numberOfSamples)
{
    auto lerp = [](float a, float b, float f) -> float { return a + f * (b - a); };
    osg::ref_ptr<osg::Uniform> uniform = new osg::Uniform(
        osg::Uniform::FLOAT_VEC3, "ssaoSamples", numberOfSamples);
    for (int i = 0; i < numberOfSamples; ++i)
    {
        osg::Vec3 sample(randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator));
        sample.normalize();

        float rand = randomFloats(generator);
        sample[0] *= rand; sample[1] *= rand; sample[2] *= rand;

        float scale = (float)i / (float)numberOfSamples;
        scale = lerp(0.1, 1.0, scale * scale);
        sample[0] *= scale; sample[1] *= scale; sample[2] *= scale;
        uniform->setElement(i, sample);
    }
    return uniform.release();
}

static osg::Uniform* generateSsaoNoises(int numberOfNoise)
{
    osg::ref_ptr<osg::Uniform> uniform = new osg::Uniform(
        osg::Uniform::FLOAT_VEC3, "ssaoNoises", numberOfNoise);
    for (int i = 0; i < numberOfNoise; ++i)
    {
        osg::Vec3 noise(randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f, 0.0f);
        uniform->setElement(i, noise);
    }
    return uniform.release();
}

namespace osgVerse
{
    void setupStandardPipeline(osgVerse::Pipeline* p, osgViewer::View* view, osg::Group* root,
                               const std::string& shaderDir, unsigned int originW, unsigned int originH)
    {
        osg::ref_ptr<osg::Texture2D> hdrMap = osgVerse::createTexture2D(
            osgDB::readImageFile("../skyboxes/barcelona/barcelona.hdr"), osg::Texture::MIRROR);
        p->startStages(originW, originH, NULL);

        // GBuffer should always be first because it also computes the scene near/far planes
        // for following stages to use
        osgVerse::Pipeline::Stage* gbuffer = p->addInputStage("GBuffer", DEFERRED_SCENE_MASK,
            osgDB::readShaderFile(shaderDir + "std_gbuffer.vert"),
            osgDB::readShaderFile(shaderDir + "std_gbuffer.frag"), 5,
            "NormalBuffer", osgVerse::Pipeline::RGBA_INT10_2,
            "DiffuseMetallicBuffer", osgVerse::Pipeline::RGBA_INT8,
            "SpecularRoughnessBuffer", osgVerse::Pipeline::RGBA_INT8,
            "EmissionOcclusionBuffer", osgVerse::Pipeline::RGBA_FLOAT16,
            "DepthBuffer", osgVerse::Pipeline::DEPTH32);

#if DEBUG_SHADOW_MODULE
        osg::ref_ptr<osgVerse::ShadowModule> shadow = new osgVerse::ShadowModule("Shadow", p, true);
#else
        osg::ref_ptr<osgVerse::ShadowModule> shadow = new osgVerse::ShadowModule("Shadow", p, false);
#endif
        shadow->createStages(2048, 3,
            osgDB::readShaderFile(shaderDir + "std_shadow_cast.vert"),
            osgDB::readShaderFile(shaderDir + "std_shadow_cast.frag"), SHADOW_CASTER_MASK);

        // Update shadow matrices at the end of g-buffer (when near/far planes are sync-ed
        osg::ref_ptr<osgVerse::ShadowDrawCallback> shadowCallback =
                new osgVerse::ShadowDrawCallback(shadow.get());
        shadowCallback->setup(gbuffer->camera.get(), FINAL_DRAW);
        view->getCamera()->addUpdateCallback(shadow.get());

        osgVerse::Pipeline::Stage* brdfLut = p->addDeferredStage("BrdfLut", true,
            osgDB::readShaderFile(shaderDir + "std_common_quad.vert"),
            osgDB::readShaderFile(shaderDir + "std_brdf_lut.frag"), 1,
            "BrdfLutBuffer", osgVerse::Pipeline::RG_FLOAT16);

        osgVerse::Pipeline::Stage* prefiltering = p->addDeferredStage("Prefilter", true,
            osgDB::readShaderFile(shaderDir + "std_common_quad.vert"),
            osgDB::readShaderFile(shaderDir + "std_environment_prefiltering.frag"), 1,
            "PrefilterBuffer", osgVerse::Pipeline::RGB_INT8);
        prefiltering->applyTexture(hdrMap.get(), "EnvironmentMap", 0);
        prefiltering->applyUniform(new osg::Uniform("roughness", 4.0f));

        osgVerse::Pipeline::Stage* convolution = p->addDeferredStage("IrrConvolution", true,
            osgDB::readShaderFile(shaderDir + "std_common_quad.vert"),
            osgDB::readShaderFile(shaderDir + "std_irradiance_convolution.frag"), 1,
            "IrradianceBuffer", osgVerse::Pipeline::RGB_INT8);
        convolution->applyTexture(hdrMap.get(), "EnvironmentMap", 0);

        osgVerse::Pipeline::Stage* ssao = p->addWorkStage("Ssao",
            osgDB::readShaderFile(shaderDir + "std_common_quad.vert"),
            osgDB::readShaderFile(shaderDir + "std_ssao.frag"), 1,
            "SsaoBuffer", osgVerse::Pipeline::R_INT8);
        ssao->applyBuffer(*gbuffer, "NormalBuffer", 0);
        ssao->applyBuffer(*gbuffer, "DepthBuffer", 1);
        ssao->applyUniform(generateSsaoSamples(8));
        ssao->applyUniform(generateSsaoNoises(4));

        osgVerse::Pipeline::Stage* ssaoBlur = p->addWorkStage("SsaoBlur",
            osgDB::readShaderFile(shaderDir + "std_common_quad.vert"),
            osgDB::readShaderFile(shaderDir + "std_ssao_blur.frag"), 1,
            "SsaoBlurredBuffer", osgVerse::Pipeline::R_INT8);
        ssaoBlur->applyBuffer(*ssao, "SsaoBuffer", 0);

        osgVerse::Pipeline::Stage* lighting = p->addWorkStage("Lighting",
            osgDB::readShaderFile(shaderDir + "std_common_quad.vert"),
            osgDB::readShaderFile(shaderDir + "std_pbr_lighting.frag"), 2,
            "ColorBuffer", osgVerse::Pipeline::RGB_FLOAT16,
            "IblAmbientBuffer", osgVerse::Pipeline::RGB_INT8);
        lighting->applyBuffer(*gbuffer, "NormalBuffer", 0);
        lighting->applyBuffer(*gbuffer, "DiffuseMetallicBuffer", 1);
        lighting->applyBuffer(*gbuffer, "SpecularRoughnessBuffer", 2);
        lighting->applyBuffer(*gbuffer, "EmissionOcclusionBuffer", 3);
        lighting->applyBuffer(*gbuffer, "DepthBuffer", 4);
        lighting->applyBuffer(*brdfLut, "BrdfLutBuffer", 5);
        lighting->applyBuffer(*prefiltering, "PrefilterBuffer", 6);
        lighting->applyBuffer(*convolution, "IrradianceBuffer", 7);

        osgVerse::Pipeline::Stage* shadowing = p->addWorkStage("Shadowing",
            osgDB::readShaderFile(shaderDir + "std_common_quad.vert"),
            osgDB::readShaderFile(shaderDir + "std_shadow_combine.frag"), 1,
            "CombinedBuffer", osgVerse::Pipeline::RGB_FLOAT16);
        shadowing->applyBuffer(*lighting, "ColorBuffer", 0);
        shadowing->applyBuffer(*lighting, "IblAmbientBuffer", 1);
        shadowing->applyBuffer(*gbuffer, "NormalBuffer", 2);
        shadowing->applyBuffer(*gbuffer, "DepthBuffer", 3);
        shadowing->applyTexture(shadow->getTextureArray(), "ShadowMapArray", 4);
        shadowing->applyUniform(shadow->getLightMatrices());

        osgVerse::Pipeline::Stage* output = p->addDisplayStage("Final",
            osgDB::readShaderFile(shaderDir + "std_common_quad.vert"),
            osgDB::readShaderFile(shaderDir + "std_display.frag"), osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
        output->applyBuffer(*shadowing, "CombinedBuffer", 0);
        output->applyBuffer(*ssaoBlur, "SsaoBlurredBuffer", 1);

        p->applyStagesToView(view, FORWARD_SCENE_MASK);
        p->requireDepthBlit(gbuffer, true);

#if DEBUG_SHADOW_MODULE
        shadow->getFrustumGeode()->setNodeMask(FORWARD_SCENE_MASK);
        root->addChild(shadow->getFrustumGeode());
#endif
    }
}

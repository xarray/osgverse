#include "Pipeline.h"
#include "ShadowModule.h"
#include "Utilities.h"
#include <osgDB/ReadFile>
#include <random>

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
        p->startStages(originW, originH);

        osg::ref_ptr<osgVerse::ShadowModule> shadow = new osgVerse::ShadowModule(p, true);
        shadow->setLightState(osg::Vec3(), osg::Vec3(1.0f, 0.0f, -1.0f), 1500.0f);
        shadow->createStages(2048, 1,
            osgDB::readShaderFile(shaderDir + "std_shadow_cast.vert"),
            osgDB::readShaderFile(shaderDir + "std_shadow_cast.frag"), SHADOW_CASTER_MASK);
        view->getCamera()->addUpdateCallback(shadow.get());

        osgVerse::Pipeline::Stage* gbuffer = p->addInputStage("GBuffer", DEFERRED_SCENE_MASK,
            osgDB::readShaderFile(shaderDir + "std_gbuffer.vert"),
            osgDB::readShaderFile(shaderDir + "std_gbuffer.frag"), 5,
            "NormalBuffer", osgVerse::Pipeline::RGBA_INT10_2,
            "DiffuseMetallicBuffer", osgVerse::Pipeline::RGBA_INT8,
            "SpecularRoughnessBuffer", osgVerse::Pipeline::RGBA_INT8,
            "EmissionOcclusionBuffer", osgVerse::Pipeline::RGBA_FLOAT16,
            "DepthBuffer", osgVerse::Pipeline::DEPTH32);

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
            osgDB::readShaderFile(shaderDir + "std_pbr_lighting.frag"), 1,
            "ColorBuffer", osgVerse::Pipeline::RGB_FLOAT16);
        lighting->applyBuffer(*gbuffer, "NormalBuffer", 0);
        lighting->applyBuffer(*gbuffer, "DiffuseMetallicBuffer", 1);
        lighting->applyBuffer(*gbuffer, "SpecularRoughnessBuffer", 2);
        lighting->applyBuffer(*gbuffer, "EmissionOcclusionBuffer", 3);
        lighting->applyBuffer(*gbuffer, "DepthBuffer", 4);
        lighting->applyBuffer(*brdfLut, "BrdfLutBuffer", 5);
        lighting->applyBuffer(*prefiltering, "PrefilterBuffer", 6);
        lighting->applyBuffer(*convolution, "IrradianceBuffer", 7);
        //lighting->applyTexture(shadow->getTextureArray(), "ShadowMapArray", 5);
        //lighting->applyUniform(shadow->getLightMatrices());

        osgVerse::Pipeline::Stage* output = p->addDisplayStage("Final",
            osgDB::readShaderFile(shaderDir + "std_common_quad.vert"),
            osgDB::readShaderFile(shaderDir + "std_display.frag"), osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
        output->applyBuffer(*lighting, "ColorBuffer", 0);
        output->applyBuffer(*ssaoBlur, "SsaoBlurredBuffer", 1);

        p->applyStagesToView(view, FORWARD_SCENE_MASK);
        p->requireDepthBlit(gbuffer, true);

        //shadow->getFrustumGeode()->setNodeMask(FORWARD_SCENE_MASK);
        //root->addChild(shadow->getFrustumGeode());
    }
}

#include "Pipeline.h"
#include "ShadowModule.h"
#include "Utilities.h"
#include <osgDB/ReadFile>
#include <random>

#define DEBUG_SHADOW_MODULE 0
static std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
static std::default_random_engine generator;

static osg::Texture* generateSsaoNoises(int numRows)
{
    std::vector<osg::Vec3f> noises;
    for (int i = 0; i < numRows; ++i)
        for (int j = 0; j < numRows; ++j)
        {
            osg::Vec3 noise(randomFloats(generator) * 2.0f - 1.0f,
                            randomFloats(generator) * 2.0f - 1.0f, 0.0f);
            noises.push_back(noise);
        }

    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->setImage(numRows, numRows, 1, GL_RGB32F_ARB, GL_RGB, GL_FLOAT,
                    (unsigned char*)&noises[0], osg::Image::NO_DELETE);

    osg::ref_ptr<osg::Texture2D> noiseTex = new osg::Texture2D;
    noiseTex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    noiseTex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    noiseTex->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    noiseTex->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    noiseTex->setImage(image.get());
    return noiseTex.release();
}

namespace osgVerse
{
    void setupStandardPipeline(osgVerse::Pipeline* p, osgViewer::View* view, osg::Group* root,
                               const std::string& shaderDir, unsigned int originW, unsigned int originH)
    {
        osg::ref_ptr<osg::Texture2D> hdrMap = osgVerse::createTexture2D(
            osgDB::readImageFile(BASE_DIR "/skyboxes/barcelona/barcelona.hdr"), osg::Texture::MIRROR);  // FIXME
        osg::ref_ptr<osg::Shader> commonVert =
            osgDB::readShaderFile(osg::Shader::VERTEX, shaderDir + "std_common_quad.vert.glsl");
        p->startStages(originW, originH, NULL);

        // GBuffer should always be first because it also computes the scene near/far planes
        // for following stages to use
        osgVerse::Pipeline::Stage* gbuffer = p->addInputStage("GBuffer", DEFERRED_SCENE_MASK,
            osgDB::readShaderFile(osg::Shader::VERTEX, shaderDir + "std_gbuffer.vert.glsl"),
            osgDB::readShaderFile(osg::Shader::FRAGMENT, shaderDir + "std_gbuffer.frag.glsl"), 5,
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
            osgDB::readShaderFile(osg::Shader::VERTEX, shaderDir + "std_shadow_cast.vert.glsl"),
            osgDB::readShaderFile(osg::Shader::FRAGMENT, shaderDir + "std_shadow_cast.frag.glsl"),
            SHADOW_CASTER_MASK);

        // Update shadow matrices at the end of g-buffer (when near/far planes are sync-ed
        osg::ref_ptr<osgVerse::ShadowDrawCallback> shadowCallback =
                new osgVerse::ShadowDrawCallback(shadow.get());
        shadowCallback->setup(gbuffer->camera.get(), FINAL_DRAW);
        view->getCamera()->addUpdateCallback(shadow.get());

        osgVerse::Pipeline::Stage* brdfLut = p->addDeferredStage("BrdfLut", true, commonVert,
            osgDB::readShaderFile(osg::Shader::FRAGMENT, shaderDir + "std_brdf_lut.frag.glsl"),
            1, "BrdfLutBuffer", osgVerse::Pipeline::RG_FLOAT16);

        osgVerse::Pipeline::Stage* prefiltering = p->addDeferredStage("Prefilter", true, commonVert,
            osgDB::readShaderFile(osg::Shader::FRAGMENT, shaderDir + "std_environment_prefiltering.frag.glsl"),
            1, "PrefilterBuffer", osgVerse::Pipeline::RGB_INT8);
        prefiltering->applyTexture(hdrMap.get(), "EnvironmentMap", 0);
        prefiltering->applyUniform(new osg::Uniform("roughness", 4.0f));

        osgVerse::Pipeline::Stage* convolution = p->addDeferredStage("IrrConvolution", true, commonVert,
            osgDB::readShaderFile(osg::Shader::FRAGMENT, shaderDir + "std_irradiance_convolution.frag.glsl"),
            1, "IrradianceBuffer", osgVerse::Pipeline::RGB_INT8);
        convolution->applyTexture(hdrMap.get(), "EnvironmentMap", 0);

        osgVerse::Pipeline::Stage* ssao = p->addWorkStage("Ssao", commonVert,
            osgDB::readShaderFile(osg::Shader::FRAGMENT, shaderDir + "std_ssao.frag.glsl"),
            1, "SsaoBuffer", osgVerse::Pipeline::R_INT8);
        ssao->applyBuffer(*gbuffer, "NormalBuffer", 0);
        ssao->applyBuffer(*gbuffer, "DepthBuffer", 1);
        ssao->applyTexture(generateSsaoNoises(8), "RandomTexture", 2);

        osgVerse::Pipeline::Stage* ssaoBlur = p->addWorkStage("SsaoBlur", commonVert,
            osgDB::readShaderFile(osg::Shader::FRAGMENT, shaderDir + "std_ssao_blur.frag.glsl"),
            1, "SsaoBlurredBuffer", osgVerse::Pipeline::R_INT8);
        ssaoBlur->applyBuffer(*ssao, "SsaoBuffer", 0);

        osgVerse::Pipeline::Stage* lighting = p->addWorkStage("Lighting", commonVert,
            osgDB::readShaderFile(osg::Shader::FRAGMENT, shaderDir + "std_pbr_lighting.frag.glsl"), 2,
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

        osgVerse::Pipeline::Stage* shadowing = p->addWorkStage("Shadowing", commonVert,
            osgDB::readShaderFile(osg::Shader::FRAGMENT, shaderDir + "std_shadow_combine.frag.glsl"),
            1, "CombinedBuffer", osgVerse::Pipeline::RGB_FLOAT16);
        shadowing->applyBuffer(*lighting, "ColorBuffer", 0);
        shadowing->applyBuffer(*lighting, "IblAmbientBuffer", 1);
        shadowing->applyBuffer(*gbuffer, "NormalBuffer", 2);
        shadowing->applyBuffer(*gbuffer, "DepthBuffer", 3);
        shadowing->applyTexture(shadow->getTextureArray(), "ShadowMapArray", 4);
        shadowing->applyUniform(shadow->getLightMatrices());

        osgVerse::Pipeline::Stage* output = p->addDisplayStage("Final", commonVert,
            osgDB::readShaderFile(osg::Shader::FRAGMENT, shaderDir + "std_display.frag.glsl"),
            osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
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

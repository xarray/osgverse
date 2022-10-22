#include "Pipeline.h"
#include "ShadowModule.h"
#include "Utilities.h"
#include <osgDB/ReadFile>
#include <osgDB/FileNameUtils>

#define DEBUG_SHADOW_MODULE 0
#define GENERATE_IBL_TEXTURES 0

namespace osgVerse
{
    void setupStandardPipeline(osgVerse::Pipeline* p, osgViewer::View* view, const std::string& shaderDir,
                               const std::string& skyboxFile, unsigned int w, unsigned int h)
    {
        std::string iblFile = osgDB::getNameLessExtension(skyboxFile) + ".ibl.osgb";
        osg::ref_ptr<osg::Texture2D> hdrMap = osgVerse::createTexture2D(
                osgDB::readImageFile(skyboxFile), osg::Texture::MIRROR);
        osg::ref_ptr<osg::StateSet> iblData = dynamic_cast<osg::StateSet*>(osgDB::readObjectFile(iblFile));
        int msaa = 0;  // FIXME: seems to cause some more flickers

        osg::ref_ptr<osg::Shader> commonVert =
            osgDB::readShaderFile(osg::Shader::VERTEX, shaderDir + "std_common_quad.vert.glsl");
        p->startStages(w, h, NULL);

        // GBuffer should always be first because it also computes the scene near/far planes
        // for following stages to use
        osgVerse::Pipeline::Stage* gbuffer = p->addInputStage("GBuffer", DEFERRED_SCENE_MASK, msaa,
            osgDB::readShaderFile(osg::Shader::VERTEX, shaderDir + "std_gbuffer.vert.glsl"),
            osgDB::readShaderFile(osg::Shader::FRAGMENT, shaderDir + "std_gbuffer.frag.glsl"), 5,
            "NormalBuffer", osgVerse::Pipeline::RGBA_INT10_2,
            "DiffuseMetallicBuffer", osgVerse::Pipeline::RGBA_INT8,
            "SpecularRoughnessBuffer", osgVerse::Pipeline::RGBA_INT8,
            "EmissionOcclusionBuffer", osgVerse::Pipeline::RGBA_FLOAT16,
            "DepthBuffer", osgVerse::Pipeline::DEPTH24_STENCIL8);

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

#if GENERATE_IBL_TEXTURES
        osgVerse::Pipeline::Stage* brdfLut = p->addDeferredStage("BrdfLut", true, commonVert,
            osgDB::readShaderFile(osg::Shader::FRAGMENT, shaderDir + "std_brdf_lut.frag.glsl"),
            1, "BrdfLutBuffer", osgVerse::Pipeline::RG_FLOAT16);

        osgVerse::Pipeline::Stage* prefiltering = p->addDeferredStage("Prefilter", true, commonVert,
            osgDB::readShaderFile(osg::Shader::FRAGMENT, shaderDir + "std_environment_prefiltering.frag.glsl"),
            1, "PrefilterBuffer", osgVerse::Pipeline::RGB_INT8);
        prefiltering->applyTexture(hdrMap.get(), "EnvironmentMap", 0);
        prefiltering->applyUniform(new osg::Uniform("GlobalRoughness", 4.0f));

        osgVerse::Pipeline::Stage* convolution = p->addDeferredStage("IrrConvolution", true, commonVert,
            osgDB::readShaderFile(osg::Shader::FRAGMENT, shaderDir + "std_irradiance_convolution.frag.glsl"),
            1, "IrradianceBuffer", osgVerse::Pipeline::RGB_INT8);
        convolution->applyTexture(hdrMap.get(), "EnvironmentMap", 0);
#else
        osg::Texture *brdfLutTex = NULL, *prefilteringTex = NULL, *convolutionTex = NULL;
        if (iblData.valid())
        {
            brdfLutTex = static_cast<osg::Texture*>(
                iblData->getTextureAttribute(0, osg::StateAttribute::TEXTURE));
            prefilteringTex = static_cast<osg::Texture*>(
                iblData->getTextureAttribute(1, osg::StateAttribute::TEXTURE));
            convolutionTex = static_cast<osg::Texture*>(
                iblData->getTextureAttribute(2, osg::StateAttribute::TEXTURE));
        }
#endif

        osgVerse::Pipeline::Stage* ssao = p->addWorkStage("Ssao", commonVert,
            osgDB::readShaderFile(osg::Shader::FRAGMENT, shaderDir + "std_ssao.frag.glsl"),
            1, "SsaoBuffer", osgVerse::Pipeline::R_INT8);
        ssao->applyBuffer(*gbuffer, "NormalBuffer", 0);
        ssao->applyBuffer(*gbuffer, "DepthBuffer", 1);
        ssao->applyTexture(generateNoises2D(4, 4), "RandomTexture", 2);
        ssao->applyUniform(new osg::Uniform("AORadius", 4.0f));
        ssao->applyUniform(new osg::Uniform("AOBias", 0.1f));
        ssao->applyUniform(new osg::Uniform("AOPowExponent", 1.5f));

        osgVerse::Pipeline::Stage* ssaoBlur1 = p->addWorkStage("SsaoBlur1", commonVert,
            osgDB::readShaderFile(osg::Shader::FRAGMENT, shaderDir + "std_ssao_blur.frag.glsl"),
            1, "SsaoBlurredBuffer0", osgVerse::Pipeline::R_INT8);
        ssaoBlur1->applyBuffer(*ssao, "SsaoBuffer", 0);
        ssaoBlur1->applyUniform(new osg::Uniform("BlurDirection", osg::Vec2(1.0f, 0.0f)));
        ssaoBlur1->applyUniform(new osg::Uniform("BlurSharpness", 40.0f));

        osgVerse::Pipeline::Stage* ssaoBlur2 = p->addWorkStage("SsaoBlur2", commonVert,
            osgDB::readShaderFile(osg::Shader::FRAGMENT, shaderDir + "std_ssao_blur.frag.glsl"),
            1, "SsaoBlurredBuffer", osgVerse::Pipeline::R_INT8);
        ssaoBlur2->applyBuffer(*ssaoBlur1, "SsaoBlurredBuffer0", "SsaoBuffer", 0);
        ssaoBlur2->applyUniform(new osg::Uniform("BlurDirection", osg::Vec2(0.0f, 1.0f)));
        ssaoBlur2->applyUniform(new osg::Uniform("BlurSharpness", 40.0f));

        osgVerse::Pipeline::Stage* lighting = p->addWorkStage("Lighting", commonVert,
            osgDB::readShaderFile(osg::Shader::FRAGMENT, shaderDir + "std_pbr_lighting.frag.glsl"), 2,
            "ColorBuffer", osgVerse::Pipeline::RGB_FLOAT16,
            "IblAmbientBuffer", osgVerse::Pipeline::RGB_INT8);
        lighting->applyBuffer(*gbuffer, "NormalBuffer", 0);
        lighting->applyBuffer(*gbuffer, "DiffuseMetallicBuffer", 1);
        lighting->applyBuffer(*gbuffer, "SpecularRoughnessBuffer", 2);
        lighting->applyBuffer(*gbuffer, "EmissionOcclusionBuffer", 3);
        lighting->applyBuffer(*gbuffer, "DepthBuffer", 4);
#if GENERATE_IBL_TEXTURES
        lighting->applyBuffer(*brdfLut, "BrdfLutBuffer", 5);
        lighting->applyBuffer(*prefiltering, "PrefilterBuffer", 6);
        lighting->applyBuffer(*convolution, "IrradianceBuffer", 7);
#else
        lighting->applyTexture(brdfLutTex, "BrdfLutBuffer", 5);
        lighting->applyTexture(prefilteringTex, "PrefilterBuffer", 6);
        lighting->applyTexture(convolutionTex, "IrradianceBuffer", 7);
#endif

        osgVerse::Pipeline::Stage* shadowing = p->addWorkStage("Shadowing", commonVert,
            osgDB::readShaderFile(osg::Shader::FRAGMENT, shaderDir + "std_shadow_combine.frag.glsl"),
            1, "CombinedBuffer", osgVerse::Pipeline::RGB_FLOAT16);
        shadowing->applyBuffer(*lighting, "ColorBuffer", 0);
        shadowing->applyBuffer(*lighting, "IblAmbientBuffer", 1);
        shadowing->applyBuffer(*gbuffer, "NormalBuffer", 2);
        shadowing->applyBuffer(*gbuffer, "DepthBuffer", 3);
        shadowing->applyTexture(generatePoissonDiscDistribution(16), "RandomTexture0", 4);
        shadowing->applyTexture(generatePoissonDiscDistribution(16), "RandomTexture1", 5);
        shadow->applyTextureAndUniforms(shadowing, "ShadowMap", 6);

        osgVerse::Pipeline::Stage* output = p->addDisplayStage("Final", commonVert,
            osgDB::readShaderFile(osg::Shader::FRAGMENT, shaderDir + "std_display.frag.glsl"),
            osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
        output->applyBuffer(*shadowing, "CombinedBuffer", 0);
        output->applyBuffer(*ssaoBlur2, "SsaoBlurredBuffer", 1);

        p->applyStagesToView(view, FORWARD_SCENE_MASK);
        p->requireDepthBlit(gbuffer, true);
    }
}

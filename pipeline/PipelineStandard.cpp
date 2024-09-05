#include "Pipeline.h"
#include "SkyBox.h"
#include "UserInputModule.h"
#include "ShadowModule.h"
#include "LightModule.h"
#include "IntersectionManager.h"
#include "NodeSelector.h"
#include "Utilities.h"

#include <osg/GLExtensions>
#include <osg/DisplaySettings>
#include <osgDB/ReadFile>
#include <osgDB/FileNameUtils>
#include <osgDB/ConvertUTF>
#include <osgText/Font>
#include <osgText/Text>
#include <osgViewer/Viewer>

#define VERT osg::Shader::VERTEX
#define FRAG osg::Shader::FRAGMENT
#define GEOM osg::Shader::GEOMETRY

class GLExtensionTester : public osg::Camera::DrawCallback
{
public:
    GLExtensionTester(osgVerse::Pipeline* p)
    {
        _data = new osgVerse::GLVersionData;
        _data->glVersion = 0.0f; _data->glslVersion = 0.0f;
        _data->glslSupported = false; _data->fboSupported = false;
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
        d->fboSupported = ext->isFrameBufferObjectSupported;
        d->drawBuffersSupported = (ext->glDrawBuffers != NULL);
#else
        osg::GL2Extensions* ext = osg::GL2Extensions::Get(renderInfo.getContextID(), true);
        d->glVersion = ext->getGlVersion() * 100;
        d->glslVersion = ext->getLanguageVersion() * 100;
        d->glslSupported = ext->isGlslSupported();

        typedef void (GL_APIENTRY * DrawBuffersProc)(GLsizei n, const GLenum *bufs);
        DrawBuffersProc glDrawBuffersTemp = NULL;
        osg::setGLExtensionFuncPtr(glDrawBuffersTemp, "glDrawBuffers", "glDrawBuffersARB");
        d->drawBuffersSupported = (glDrawBuffersTemp != NULL);

        osg::FBOExtensions* ext2 = osg::FBOExtensions::instance(renderInfo.getContextID(), true);
        d->fboSupported = ext2->isSupported();
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
void obtainScreenResolution(unsigned int& w, unsigned int& h)
{
    HWND hd = ::GetDesktopWindow(); RECT rect; ::GetWindowRect(hd, &rect);
    w = (rect.right - rect.left); h = (rect.bottom - rect.top);
    OSG_NOTICE << "[obtainScreenResolution] Get screen size " << w << " x " << h << "\n";
}
#elif defined(VERSE_X11)
#include <X11/Xlib.h>
void obtainScreenResolution(unsigned int& w, unsigned int& h)
{
    Display* disp = XOpenDisplay(NULL);
    Screen* screen = DefaultScreenOfDisplay(disp);
    w = screen->width; h = screen->height;
    OSG_NOTICE << "[obtainScreenResolution] Get screen size " << w << " x " << h << "\n";
}
#else
void obtainScreenResolution(unsigned int& w, unsigned int& h)
{
    w = osg::DisplaySettings::instance()->getScreenWidth();
    h = osg::DisplaySettings::instance()->getScreenHeight();
    OSG_NOTICE << "[obtainScreenResolution] Get screen size " << w << " x " << h << "\n";
}
#endif

namespace osgVerse
{
    StandardPipelineParameters::StandardPipelineParameters()
    :   deferredMask(DEFERRED_SCENE_MASK), forwardMask(FORWARD_SCENE_MASK),
        shadowCastMask(SHADOW_CASTER_MASK), shadowNumber(0), shadowResolution(4096),
        withEmbeddedViewer(false), debugShadowModule(false), enableVSync(true), enableMRT(true),
        enableAO(true), enablePostEffects(true), enableUserInput(false)
    {
        obtainScreenResolution(originWidth, originHeight);
        if (!originWidth) originWidth = 1920; if (!originHeight) originHeight = 1080;
    }

    StandardPipelineParameters::StandardPipelineParameters(const std::string& dir, const std::string& sky)
    :   deferredMask(DEFERRED_SCENE_MASK), forwardMask(FORWARD_SCENE_MASK),
        shadowCastMask(SHADOW_CASTER_MASK), shadowNumber(3), shadowResolution(4096),
        withEmbeddedViewer(false), debugShadowModule(false), enableVSync(true), enableMRT(true),
        enableAO(true), enablePostEffects(true), enableUserInput(false)
    {
        obtainScreenResolution(originWidth, originHeight);
        if (!originWidth) originWidth = 1920; if (!originHeight) originHeight = 1080;

        osgDB::ReaderWriter* rw = osgDB::Registry::instance()->getReaderWriterForExtension("glsl");
        if (rw != NULL)
        {
#define READ_SHADER(var, type, name) \
    var = rw->readShader(name).getShader(); \
    if (var != NULL) var->setType(type);

            READ_SHADER(shaders.gbufferVS, VERT, dir + "std_gbuffer.vert.glsl");
            READ_SHADER(shaders.shadowCastVS, VERT, dir + "std_shadow_cast.vert.glsl");
            READ_SHADER(shaders.quadVS, VERT, dir + "std_common_quad.vert.glsl");

            READ_SHADER(shaders.gbufferFS, FRAG, dir + "std_gbuffer.frag.glsl");
            READ_SHADER(shaders.shadowCastFS, FRAG, dir + "std_shadow_cast.frag.glsl");
            READ_SHADER(shaders.ssaoFS, FRAG, dir + "std_ssao.frag.glsl");
            READ_SHADER(shaders.ssaoBlurFS, FRAG, dir + "std_ssao_blur.frag.glsl");
            READ_SHADER(shaders.pbrLightingFS, FRAG, dir + "std_pbr_lighting.frag.glsl");
            READ_SHADER(shaders.shadowCombineFS, FRAG, dir + "std_shadow_combine.frag.glsl");
            READ_SHADER(shaders.downsampleFS, FRAG, dir + "std_luminance_downsample.frag.glsl");
            READ_SHADER(shaders.brightnessFS, FRAG, dir + "std_brightness_extraction.frag.glsl");
            READ_SHADER(shaders.brightnessCombineFS, FRAG, dir + "std_brightness_combine.frag.glsl");
            READ_SHADER(shaders.bloomFS, FRAG, dir + "std_brightness_bloom.frag.glsl");
            READ_SHADER(shaders.tonemappingFS, FRAG, dir + "std_tonemapping.frag.glsl");
            READ_SHADER(shaders.antiAliasingFS, FRAG, dir + "std_antialiasing.frag.glsl");
            READ_SHADER(shaders.brdfLutFS, FRAG, dir + "std_brdf_lut.frag.glsl");
            READ_SHADER(shaders.envPrefilterFS, FRAG, dir + "std_environment_prefiltering.frag.glsl");
            READ_SHADER(shaders.irrConvolutionFS, FRAG, dir + "std_irradiance_convolution.frag.glsl");
            READ_SHADER(shaders.quadFS, FRAG, dir + "std_common_quad.frag.glsl");
            READ_SHADER(shaders.displayFS, FRAG, dir + "std_display.frag.glsl");

            READ_SHADER(shaders.forwardVS, VERT, dir + "std_forward_render.vert.glsl");
            READ_SHADER(shaders.forwardFS, FRAG, dir + "std_forward_render.frag.glsl");
        }

        std::string iblFile = osgDB::getNameLessExtension(sky) + ".ibl.rseq";
        skyboxIBL = dynamic_cast<osg::ImageSequence*>(osgDB::readImageFile(iblFile));
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

    void StandardPipelineParameters::addUserInputStage(const std::string& name, unsigned int mask,
                                                       UserInputOccasion occasion, UserInputType t)
    { userInputs[occasion].push_back(UserInputStageData(name, mask, t)); }

    void StandardPipelineParameters::applyUserInputStages(
            osg::Camera* mainCam, Pipeline* p, UserInputOccasion occasion,
            osg::Texture* colorBuffer, osg::Texture* depthBuffer) const
    {
        std::map<UserInputOccasion, UserInputStageList>::const_iterator itr = userInputs.find(occasion);
        const StandardPipelineParameters::UserInputStageList& userStages = itr->second;
        for (size_t u = 0; u < userStages.size(); ++u)
        {
            const StandardPipelineParameters::UserInputStageData usd = userStages[u];
            osgVerse::UserInputModule* inModule = new osgVerse::UserInputModule(usd.stageName, p);
            inModule->createStages(
                usd.mask, NULL, NULL, "ColorBuffer", colorBuffer, "DepthBuffer", depthBuffer);
            mainCam->addUpdateCallback(inModule);  // TODO: UserInputType?
        }
    }

    GLVersionData* queryOpenGLVersion(Pipeline* p, bool asEmbedded, osg::GraphicsContext* embeddedGC)
    {
        osgViewer::Viewer tempViewer;
        GLExtensionTester* tester = new GLExtensionTester(p);
        tempViewer.getCamera()->setPreDrawCallback(tester);

        tempViewer.setSceneData(new osg::Node);
        if (asEmbedded && embeddedGC) embeddedGC->makeCurrent();
        if (asEmbedded) tempViewer.setUpViewerAsEmbeddedInWindow(0, 0, 1, 1);
        else tempViewer.setUpViewInWindow(0, 0, 1, 1);
        for (int i = 0; i < 5; ++i) tempViewer.frame();

        tempViewer.setDone(true);
        if (asEmbedded && embeddedGC) embeddedGC->releaseContext();
        return p ? p->getVersionData() : NULL;
    }

    bool setupStandardPipeline(osgVerse::Pipeline* p, osgViewer::View* view,
                               const StandardPipelineParameters& spp)
    {
        if (view) return setupStandardPipelineEx(p, view, view->getCamera(), spp);
        else { OSG_WARN << "[StandardPipeline] No view provided." << std::endl; return false; }
    }

    bool setupStandardPipelineEx(Pipeline* p, osgViewer::View* view, osg::Camera* mainCam,
                                 const StandardPipelineParameters& spp)
    {
        bool supportDrawBuffersMRT = spp.enableMRT;
        StandardPipelineParameters::UserInputOccasion occasion;
        if (!mainCam) mainCam = view->getCamera();

        if (!view)
        {
            OSG_WARN << "[StandardPipeline] No view provided." << std::endl;
            return false;
        }
#if OSG_VERSION_GREATER_THAN(3, 2, 3)
        else if (!view->getLastAppliedViewConfig() && !mainCam->getGraphicsContext())
#else
        else if (!mainCam->getGraphicsContext())
#endif
        {
            OSG_NOTICE << "[StandardPipeline] No view config applied. The pipeline will be constructed "
                       << "with provided parameters. Please DO NOT apply any view config like "
                       << "setUpViewInWindow() or setUpViewAcrossAllScreens() AFTER you called "
                       << "setupStandardPipeline(). It may cause problems!!" << std::endl;
        }

        GLVersionData* data = p->getVersionData();
        if (!data) data = queryOpenGLVersion(p, spp.withEmbeddedViewer, mainCam->getGraphicsContext());

        p->startStages(spp.originWidth, spp.originHeight, mainCam->getGraphicsContext());
        if (data)
        {
            if (!data->glslSupported || !data->fboSupported)
            {
                OSG_FATAL << "[StandardPipeline] Necessary OpenGL features missing. The pipeline "
                          << "can not work on your machine at present." << std::endl;
                return false;
            }
            else if (data->renderer.find("MTT") != std::string::npos)
            {
#ifndef VERSE_ENABLE_MTT
                OSG_WARN << "[StandardPipeline] It seems you are using MooreThreads graphics "
                         << "driver but not setting VERSE_USE_MTT_DRIVER in CMake. It may cause "
                         << "unexpected problems at present." << std::endl;
#endif
            }
            supportDrawBuffersMRT &= data->drawBuffersSupported;
        }

        // GBuffer should always be first because it also computes the scene near/far planes
        // for following stages to use
        int msaa = 0;  // FIXME: seems to cause some more flickers
        osgVerse::Pipeline::Stage* gbuffer = NULL;
        if (supportDrawBuffersMRT)
        {
            gbuffer = p->addInputStage("GBuffer", spp.deferredMask, msaa,
                spp.shaders.gbufferVS, spp.shaders.gbufferFS, 5,
#ifdef VERSE_WEBGL1
                "NormalBuffer", osgVerse::Pipeline::RGBA_INT8,
                "DiffuseMetallicBuffer", osgVerse::Pipeline::RGBA_INT8,
                "SpecularRoughnessBuffer", osgVerse::Pipeline::RGBA_INT8,
                "EmissionOcclusionBuffer", osgVerse::Pipeline::RGBA_INT8,
                "DepthBuffer", osgVerse::Pipeline::DEPTH32);
#else
                "NormalBuffer", osgVerse::Pipeline::RGBA_FLOAT16,
                "DiffuseMetallicBuffer", osgVerse::Pipeline::RGBA_INT8,
                "SpecularRoughnessBuffer", osgVerse::Pipeline::RGBA_INT8,
                "EmissionOcclusionBuffer", osgVerse::Pipeline::RGBA_FLOAT16,
#   ifdef VERSE_WASM
                "DepthBuffer", osgVerse::Pipeline::DEPTH32);
#   else
                "DepthBuffer", osgVerse::Pipeline::DEPTH24_STENCIL8);
#   endif
#endif
        }
        else
        {
            gbuffer = p->addInputStage("GBuffer", spp.deferredMask, msaa,
                spp.shaders.gbufferVS, spp.shaders.gbufferFS, 2,
                "NormalBuffer", osgVerse::Pipeline::RGBA_INT8,
#ifdef VERSE_WASM
                "DepthBuffer", osgVerse::Pipeline::DEPTH32);
#else
                "DepthBuffer", osgVerse::Pipeline::DEPTH24_STENCIL8);
#endif

            // TODO: more gbuffers
            OSG_WARN << "[StandardPipeline] DrawBuffersMRT doesn't work here. It may cause "
                     << "unexpected problems at present." << std::endl;
        }

        // Shadow module initialization
        osg::ref_ptr<osgVerse::ShadowModule> shadowModule =
            new osgVerse::ShadowModule("Shadow", p, spp.debugShadowModule);
        shadowModule->createStages(spp.shadowResolution, spp.shadowNumber,
            spp.shaders.shadowCastVS, spp.shaders.shadowCastFS, spp.shadowCastMask);

        // Update shadow matrices at the end of g-buffer (when near/far planes are sync-ed)
        osg::ref_ptr<osgVerse::ShadowDrawCallback> shadowCallback =
                new osgVerse::ShadowDrawCallback(shadowModule.get());
        shadowCallback->setup(gbuffer->camera.get(), FINAL_DRAW);
        mainCam->addUpdateCallback(shadowModule.get());

        // Light module only needs to be added to main camera
        osg::ref_ptr<osgVerse::LightModule> lightModule = new osgVerse::LightModule("Light", p);
        mainCam->addUpdateCallback(lightModule.get());

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
#if OSG_VERSION_GREATER_THAN(3, 2, 0)
            size_t imgCount = spp.skyboxIBL->getNumImageData();
#else
            size_t imgCount = spp.skyboxIBL->getNumImages();
#endif
            if (imgCount > 0)
            {
#if defined(VERSE_WEBGL1)
                osg::ref_ptr<osg::Image> brdfImg = spp.skyboxIBL->getImage(0);
                unsigned char* data = new unsigned char[brdfImg->getTotalSizeInBytes()];
                memcpy(data, brdfImg->data(), brdfImg->getTotalSizeInBytes());

                osg::ref_ptr<osg::Image> newBrdfImg = new osg::Image;
                newBrdfImg->setImage(brdfImg->s(), brdfImg->t(), brdfImg->r(), GL_RGB,
                    GL_RGB, GL_HALF_FLOAT_OES, data, osg::Image::USE_NEW_DELETE);
                brdfLutTex = createTexture2D(newBrdfImg.get(), osg::Texture::MIRROR);
#else
                brdfLutTex = createTexture2D(spp.skyboxIBL->getImage(0), osg::Texture::MIRROR);
#endif
                brdfLutTex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
                brdfLutTex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
            }

            if (imgCount > 1)
            {
#if defined(VERSE_WEBGL1)
                spp.skyboxIBL->getImage(1)->setInternalTextureFormat(GL_RGB);
#endif
                prefilteringTex = createTexture2D(spp.skyboxIBL->getImage(1), osg::Texture::MIRROR);
                prefilteringTex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
                prefilteringTex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
            }

            if (imgCount > 2)
            {
#if defined(VERSE_WEBGL1)
                spp.skyboxIBL->getImage(2)->setInternalTextureFormat(GL_RGB);
#endif
                convolutionTex = createTexture2D(spp.skyboxIBL->getImage(2), osg::Texture::MIRROR);
                convolutionTex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
                convolutionTex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
            }
        }

        // Deferred lighting stage
        osgVerse::Pipeline::Stage* lighting = p->addWorkStage("Lighting", 1.0f,
            spp.shaders.quadVS, spp.shaders.pbrLightingFS, 2,
#if defined(VERSE_WASM)
            "ColorBuffer", osgVerse::Pipeline::RGB_INT8,
#else
            "ColorBuffer", osgVerse::Pipeline::RGB_FLOAT16,
#endif
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
#   if defined(VERSE_WEBGL1)  // FIXME???
            lighting->applyTexture(createDefaultTexture(
                osg::Vec4(0.3f, 0.3f, 0.3f, 1.0f)), "BrdfLutBuffer", 5);
            lighting->applyTexture(createDefaultTexture(
                osg::Vec4(0.3f, 0.3f, 0.3f, 1.0f)), "PrefilterBuffer", 6);
            lighting->applyTexture(createDefaultTexture(
                osg::Vec4(0.3f, 0.3f, 0.3f, 1.0f)), "IrradianceBuffer", 7);
#   else
            lighting->applyTexture(brdfLutTex, "BrdfLutBuffer", 5);
            lighting->applyTexture(prefilteringTex, "PrefilterBuffer", 6);
            lighting->applyTexture(convolutionTex, "IrradianceBuffer", 7);
#   endif
        }
        lightModule->applyTextureAndUniforms(lighting, "LightParameterMap", 8);

        osgVerse::Pipeline::Stage* lastAoStage = NULL;
        if (spp.enableAO)
        {
            // SSAO stages: AO -> BlurH -> BlurV
            osgVerse::Pipeline::Stage* ssao = p->addWorkStage("Ssao", 1.0f,
                spp.shaders.quadVS, spp.shaders.ssaoFS, 1,
#if defined(VERSE_WEBGL1)
                "SsaoBuffer", osgVerse::Pipeline::RGB_INT8);
#else
                "SsaoBuffer", osgVerse::Pipeline::R_INT8);
#endif
            ssao->applyBuffer(*gbuffer, "NormalBuffer", 0);
            ssao->applyBuffer(*gbuffer, "DepthBuffer", 1);
            ssao->applyTexture(generatePoissonDiscDistribution(4, 4), "RandomTexture", 2);
            ssao->applyUniform(new osg::Uniform("AORadius", 12.0f));
            ssao->applyUniform(new osg::Uniform("AOBias", 0.1f));
            ssao->applyUniform(new osg::Uniform("AOPowExponent", 12.0f));

            osgVerse::Pipeline::Stage* ssaoBlur1 = p->addWorkStage("SsaoBlur1", 1.0f,
                spp.shaders.quadVS, spp.shaders.ssaoBlurFS, 1,
#if defined(VERSE_WEBGL1)
                "SsaoBlurredBuffer0", osgVerse::Pipeline::RGB_INT8);
#else
                "SsaoBlurredBuffer0", osgVerse::Pipeline::R_INT8);
#endif
            ssaoBlur1->applyBuffer(*ssao, "SsaoBuffer", 0);
            ssaoBlur1->applyUniform(new osg::Uniform("BlurDirection", osg::Vec2(1.0f, 0.0f)));
            ssaoBlur1->applyUniform(new osg::Uniform("BlurSharpness", 40.0f));

            osgVerse::Pipeline::Stage* ssaoBlur2 = p->addWorkStage("SsaoBlur2", 1.0f,
                spp.shaders.quadVS, spp.shaders.ssaoBlurFS, 1,
#if defined(VERSE_WEBGL1)
                "SsaoBlurredBuffer", osgVerse::Pipeline::RGB_INT8);
#else
                "SsaoBlurredBuffer", osgVerse::Pipeline::R_INT8);
#endif
            ssaoBlur2->applyBuffer(*ssaoBlur1, "SsaoBlurredBuffer0", "SsaoBuffer", 0);
            ssaoBlur2->applyUniform(new osg::Uniform("BlurDirection", osg::Vec2(0.0f, 1.0f)));
            ssaoBlur2->applyUniform(new osg::Uniform("BlurSharpness", 40.0f));
            lastAoStage = ssaoBlur2;
        }

        // Shadow & AO combining stage
        osgVerse::Pipeline::Stage* shadowing = p->addWorkStage("Shadowing", 1.0f,
                spp.shaders.quadVS, spp.shaders.shadowCombineFS, 1,
                "CombinedBuffer", osgVerse::Pipeline::RGB_INT8);
        shadowing->applyBuffer(*lighting, "ColorBuffer", 0);
        if (lastAoStage != NULL)
            shadowing->applyBuffer(*lastAoStage, "SsaoBlurredBuffer", 1);
        else
            shadowing->applyTexture(createDefaultTexture(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f)),
                                    "SsaoBlurredBuffer", 1);
        shadowing->applyBuffer(*gbuffer, "NormalBuffer", 2);
        shadowing->applyBuffer(*gbuffer, "DepthBuffer", 3);
        shadowing->applyTexture(generatePoissonDiscDistribution(16, 2), "RandomTexture", 4);
        shadowModule->applyTextureAndUniforms(shadowing, "ShadowMap", 5);

        // User input module before post-effects
        occasion = StandardPipelineParameters::BEFORE_POSTEFFECTS;
        if (spp.enableUserInput && spp.userInputs.find(occasion) != spp.userInputs.end())
        {
            spp.applyUserInputStages(mainCam, p, occasion,
                                     shadowing->getBufferTexture("CombinedBuffer"),
                                     gbuffer->getBufferTexture(osg::Camera::DEPTH_BUFFER));
        }

        if (spp.enablePostEffects)
        {
            // Bloom stages: Brightness -> Downscaling x N -> Combine -> Bloom
            osgVerse::Pipeline::Stage* brighting = p->addDeferredStage("Brighting", 1.0f, false,
                spp.shaders.quadVS, spp.shaders.brightnessFS, 1,
                "BrightnessBuffer0", osgVerse::Pipeline::RGB_INT8);
            brighting->applyBuffer("ColorBuffer", 0, p);
            //brighting->applyBuffer(*shadowing, "CombinedBuffer", "ColorBuffer", 0);
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
            osgVerse::Pipeline::Stage* tonemapping = p->addWorkStage("ToneMapping", 1.0f,
                spp.shaders.quadVS, spp.shaders.tonemappingFS, 1,
                "ToneMappedBuffer", osgVerse::Pipeline::RGB_INT8);  // RGB_FLOAT16
            tonemapping->applyBuffer(*shadowing, "CombinedBuffer", "ColorBuffer", 0);
            tonemapping->applyBuffer(*downsamples.back(), "BrightnessBuffer" + lastDs, "LuminanceBuffer", 1);
            tonemapping->applyBuffer(*blooming, "BloomBuffer", 2);
            tonemapping->applyBuffer(*lighting, "IblAmbientBuffer", 3);
            tonemapping->applyUniform(new osg::Uniform("LuminanceFactor", osg::Vec2(1.0f, 10.0f)));

            // Anti-aliasing
            osgVerse::Pipeline::Stage* antiAliasing = p->addWorkStage("AntiAliasing", 1.0f,
                spp.shaders.quadVS, spp.shaders.antiAliasingFS, 1,
                "AntiAliasedBuffer", osgVerse::Pipeline::RGB_INT8);
            antiAliasing->applyBuffer(*tonemapping, "ToneMappedBuffer", "ColorBuffer", 0);
        }

        // Final stage (color grading)
        osgVerse::Pipeline::Stage* output = p->addDisplayStage("Final",
                spp.shaders.quadVS, spp.shaders.displayFS, osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
        output->applyBuffer("ColorBuffer", 0, p);
        //output->applyBuffer(*antiAliasing, "AntiAliasedBuffer", "ColorBuffer", 0);
        output->applyBuffer(*gbuffer, "DepthBuffer", 1);
        output->applyUniform(new osg::Uniform("FogDistance", osg::Vec2(0.0f, 0.0f)));
        output->applyUniform(new osg::Uniform("FogColor", osg::Vec3(0.5f, 0.5f, 0.5f)));
        output->applyUniform(new osg::Uniform("ColorAttribute", osg::Vec3(1.0f, 1.0f, 1.0f)));
        output->applyUniform(new osg::Uniform("ColorBalance", osg::Vec3(0.0f, 0.0f, 0.0f)));  // [-1, 1]
        output->applyUniform(new osg::Uniform("ColorBalanceMode", (int)0));
        output->applyUniform(new osg::Uniform("VignetteRadius", 1.0f));
        output->applyUniform(new osg::Uniform("VignetteDarkness", 0.0f));

        // Post operations
        if (p->getContext())
        {
            osgViewer::GraphicsWindow* gw =
                dynamic_cast<osgViewer::GraphicsWindow*>(p->getContext());
            if (gw != NULL) gw->setSyncToVBlank(spp.enableVSync);
        }
        p->applyStagesToView(view, mainCam, spp.forwardMask);
        p->requireDepthBlit(gbuffer, true);

        /*osg::StateSet* forwardSS = p->createForwardStateSet(
            spp.shaders.forwardVS.get(), spp.shaders.forwardFS.get());
        if (forwardSS && lightModule)
        {
            forwardSS->setTextureAttributeAndModes(7, lightModule->getParameterTable());
            forwardSS->addUniform(new osg::Uniform("LightParameterMap", 7));
            forwardSS->addUniform(lightModule->getLightNumber());
        }*/
        return true;
    }

    ////////////////// StandardPipelineViewer
    class SelectSceneHandler : public osgGA::GUIEventHandler
    {
    public:
        SelectSceneHandler(NodeSelector* sel, osg::Geode* tNode)
            : _selector(sel), _textGeode(tNode)
        {
            _condition.nodesToIgnore.insert(_selector->getAuxiliaryRoot());
            _condition.nodesToIgnore.insert(_textGeode.get());
        }

        virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
        {
            osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
            if (ea.getEventType() == osgGA::GUIEventAdapter::RELEASE &&
                (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL))
            {
                IntersectionResult result = findNearestIntersection(
                    view->getCamera(), ea.getXnormalized(), ea.getYnormalized(), &_condition);
                if (!result.drawable) return false;

                osg::Object* selectedObj = NULL; _selector->clearAllSelectedNodes();
#if OSG_VERSION_LESS_OR_EQUAL(3, 4, 1)
                selectedObj = result.nodePath.back();
                _selector->addSelectedNode(result.nodePath.back());
#else
                selectedObj = result.drawable.get();
                _selector->addSelectedNode(result.drawable.get());
#endif

                osgText::Text* text = NULL;
                if (_textGeode->getNumDrawables() == 0)
                {
                    text = new osgText::Text;
                    text->setPosition(osg::Vec3(10.0f, 1060.0f, 0.0f));
                    text->setCharacterSize(20.0f, 1.0f);
                    text->setFont(MISC_DIR "LXGWFasmartGothic.ttf");
                    _textGeode->addDrawable(text);
                }
                else
                    text = static_cast<osgText::Text*>(_textGeode->getDrawable(0));

                std::wstring t = osgDB::convertUTF8toUTF16(getNodePathID(*selectedObj, view->getSceneData()));
                text->setText((t.length() > 80) ? (L"..." + t.substr(t.length() - 80)).c_str() : t.c_str());
            }
            return false;
        }

    protected:
        osg::ref_ptr<NodeSelector> _selector;
        osg::observer_ptr<osg::Geode> _textGeode;
        IntersectionCondition _condition;
    };

    StandardPipelineViewer::StandardPipelineViewer(bool withSky, bool withSelector, bool withDebugShadow)
        : osgViewer::Viewer(), _withSky(withSky), _withSelector(withSelector)
    {
        _parameters = osgVerse::StandardPipelineParameters(SHADER_DIR, SKYBOX_DIR "barcelona.hdr");
        _parameters.enablePostEffects = true; _parameters.enableAO = true;
        _parameters.debugShadowModule = withDebugShadow;
        _lightGeode = new osg::Geode; _textGeode = new osg::Geode; _root = new osg::Group;
    }

    StandardPipelineViewer::StandardPipelineViewer(const StandardPipelineParameters& spp,
                                                   bool withSky, bool withSelector)
        : osgViewer::Viewer(), _parameters(spp), _withSky(withSky), _withSelector(withSelector)
    { _lightGeode = new osg::Geode; _textGeode = new osg::Geode; _root = new osg::Group; }

    void StandardPipelineViewer::setSceneData(osg::Node* node)
    {
        if (!_root->containsNode(node)) _root->addChild(node);
        osgViewer::Viewer::setSceneData(_root.get()); _scene = node;
    }

    void StandardPipelineViewer::realize()
    {
        if (isRealized()) return;
        initialize(_parameters, _withSky, _withSelector);
        setMainLight(osg::Vec3(1.5f, 1.5f, 1.2f), osg::Vec3(0.02f, 0.1f, -1.0f));
        setThreadingModel(osgViewer::Viewer::SingleThreaded);
        osgViewer::Viewer::realize();
    }

    void StandardPipelineViewer::setMainLight(const osg::Vec3& color, const osg::Vec3& dir)
    {
        osg::ref_ptr<osgVerse::LightDrawable> light0 = new osgVerse::LightDrawable;
        light0->setColor(color); light0->setDirection(dir);
        light0->setDirectional(true);
        if (_lightGeode->getNumDrawables() > 0) _lightGeode->setDrawable(0, light0.get());
        else _lightGeode->addDrawable(light0.get());
        if (!_pipeline) return;

        osgVerse::LightModule* light = static_cast<osgVerse::LightModule*>(_pipeline->getModule("Light"));
        if (light) light->setMainLight(light0.get(), "Shadow");
    }

    osg::GraphicsOperation* StandardPipelineViewer::createRenderer(osg::Camera* camera)
    {
        if (_pipeline.valid()) return _pipeline->createRenderer(camera);
        else return osgViewer::Viewer::createRenderer(camera);
    }

    void StandardPipelineViewer::initialize(const StandardPipelineParameters& spp,
                                            bool withSky, bool withSelector)
    {
        _pipeline = new osgVerse::Pipeline;
#if true
        setupStandardPipeline(_pipeline.get(), this, spp);
#else
        std::ifstream ppConfig(SHADER_DIR "/standard_pipeline.json");
        _pipeline->load(ppConfig, &viewer);
#endif

        osgVerse::LightModule* light = static_cast<osgVerse::LightModule*>(_pipeline->getModule("Light"));
        if (light && _lightGeode->getNumDrawables() > 0)
            light->setMainLight(static_cast<LightDrawable*>(_lightGeode->getDrawable(0)), "Shadow");
        _root->addChild(_lightGeode.get());

        if (withSky)
        {
            osg::ref_ptr<osg::Camera> postCamera = SkyBox::createSkyCamera();
            _root->addChild(postCamera.get());

            osg::ref_ptr<SkyBox> skybox = new SkyBox(_pipeline.get());
            skybox->setSkyShaders(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "skybox.vert.glsl"),
                osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "skybox.frag.glsl"));
            skybox->setEnvironmentMap(spp.skyboxMap.get(), false);
            Pipeline::setPipelineMask(*skybox, FORWARD_SCENE_MASK);
            postCamera->addChild(skybox.get());
        }

        if (withSelector)
        {
            osg::ref_ptr<osg::Camera> postCamera = new osg::Camera;
            postCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
            postCamera->setRenderOrder(osg::Camera::POST_RENDER, 10000);
            postCamera->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
            _root->addChild(postCamera.get());

            osg::ref_ptr<osg::Camera> postCamera2 = createHUDCamera(NULL, 1920, 1080);
            postCamera2->addChild(_textGeode.get());
            _root->addChild(postCamera2.get());

            osg::ref_ptr<NodeSelector> selector = new NodeSelector;
            selector->setMainCamera(_pipeline->getForwardCamera());
            postCamera->addChild(selector->getAuxiliaryRoot());
            addEventHandler(new SelectSceneHandler(selector.get(), _textGeode.get()));
        }
    }
}

#undef VERT
#undef FRAG
#undef GEOM

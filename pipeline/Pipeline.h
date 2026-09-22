#ifndef MANA_PP_PIPELINE_HPP
#define MANA_PP_PIPELINE_HPP

#include <osg/Vec2s>
#include <osg/ImageSequence>
#include <osg/Depth>
#include <osg/FrameBufferObject>
#include <osg/Program>
#include <osg/Texture2D>
#include <osg/Group>
#include <osg/Geode>
#include <osgViewer/Viewer>
#include <string>
#include "DeferredCallback.h"

/** Pipeline mask range:
    - Deferred attributes: Bit 24-32
    - Shadow & misc:       Bit 16-31
    - User reserved:       Bit 8-15
    - Forward reserved:    Bit 0-7
*/
#define DEFERRED_SCENE_MASK   0xff000000
#define FORWARD_SCENE_MASK    0x000000ff
#define SHADOW_CASTER_MASK    0x00100000
#define CUSTOM_INPUT_MASK     0x00010000

#ifndef GL_HALF_FLOAT
    #define GL_HALF_FLOAT                     0x140B
#endif

#ifndef GL_HALF_FLOAT_OES
    #define GL_HALF_FLOAT_OES                 0x8D61
#endif

#ifndef GL_ARB_texture_rg
    #define GL_RG                             0x8227
    #define GL_R8                             0x8229
    #define GL_R16                            0x822A
    #define GL_RG8                            0x822B
    #define GL_RG16                           0x822C
    #define GL_R16F                           0x822D
    #define GL_R32F                           0x822E
    #define GL_RG16F                          0x822F
    #define GL_RG32F                          0x8230
#endif

namespace osgVerse
{
    class LightModule;
    class ShadowModule;
    class UserInputModule;
    class ScriptableProgram;
    class RenderCallbackXR;
    class ExposureController;

    /** OpenGL version data for graphics hardware adpation.
        OpenGL Version: GLSL Version
        - 2.0: #version 110
        - 2.1: #version 120
        - 3.0: #version 130    // use in/out instead of attribute/varying, supports 'int'
        - 3.1: #version 140
        - 3.2: #version 150    // use generic 'texture()'
        - 3.3: #version 330    // use layout qualifiers
        - 4.0: #version 400...
        - ES 2.0: #version 100 es
        - ES 3.0: #version 300 es
    */
    struct GLVersionData : public osg::Referenced
    {
        std::map<std::string, int> capabilities;
        std::vector<std::string> extensions;
        std::string version, renderer;
        int glVersion, glslVersion;
        bool glslSupported, fboSupported;
        bool drawBuffersSupported, depthStencilSupported;

        GLVersionData() : glVersion(0), glslVersion(0), glslSupported(false), fboSupported(false),
                          drawBuffersSupported(false), depthStencilSupported(false) {}
        void copy(GLVersionData* data);  // copy from another version checker
        int score();  // compute a score of current graphics card for checking performance
    };

    /** Module to be used in pipeline */
    class RenderingModuleBase : public osg::NodeCallback
    {
    public:
        virtual LightModule* asLightModule() { return NULL; }
        virtual ShadowModule* asShadowModule() { return NULL; }
        virtual UserInputModule* asUserInputModule() { return NULL; }
    };

    /** Effect pipeline using a list of slave cameras, without invading main scene graph
        Some uniforms will be set automatically for internal stages:
        - sampler2d DiffuseMap: diffuse/albedo RGB texture of input scene
        - sampler2d NormalMap: tangent-space normal texture of input scene
        - sampler2d SpecularMap: specular RGB texture of input scene
        - sampler2d ShininessMap: occlusion/roughness/metallic (RGB) texture of input scene
        - sampler2d AmbientMap: ambient texture of input scene (FIXME: NOT USED)
        - sampler2d EmissiveMap: emissive RGB texture of input scene
        - sampler2d ReflectionMap: reflection RGB texture of input scene
        - mat4 <StageName>Matrices: matrices of specified input stage for rebuilding vertex attributes
                                    Including: world-to-view, view-to-world, view-to-proj, proj-to-view
        - vec2 NearFarPlanes: calculated near/far values of entire scene
        - vec2 InvScreenResolution: (1.0 / view-width, 1.0 / view-height)

        Vertex attribute mapping suggestions:
        - osg_Vertex: 0
        - osg_BoneWeight (Custom): 1
        - osg_Normal: 2
        - osg_Color: 3
        - osg_SecondaryColor: 4
        - osg_FogCoord: 5
        - osg_Tangent (Custom): 6
        - osg_Binormal (Custom): 7
        - osg_MultiTexCoord0 - osg_MultiTexCoord7: 8-15
    */
    class Pipeline : public osg::Referenced
    {
    public:
        enum BufferType
        {
            RGB_INT8 = 0/*24bit*/, RGB_INT5/*16bit*/, RGB_INT10/*32bit*/,
            RGB_FLOAT16/*48bit*/, RGB_FLOAT32/*96bit*/, SRGB_INT8/*24bit*/,
            RGBA_INT8/*32bit*/, RGBA_INT5_1/*16bit*/, RGBA_INT10_2/*32bit*/,
            RGBA_FLOAT16/*64bit*/, RGBA_FLOAT32/*128bit*/, SRGBA_INT8/*24bit*/,
            R_INT8/*8bit*/, R_FLOAT16/*16bit*/, R_FLOAT32/*32bit*/,
            RG_INT8/*16bit*/, RG_FLOAT16/*32bit*/, RG_FLOAT32/*64bit*/,
            DEPTH16/*16bit*/, DEPTH24_STENCIL8/*32bit*/, DEPTH32/*32bit*/,
            DEPTH32_STENCIL8/*32bit*/
        };

        enum InputFlag
        {
            NO_DEFAULT_TEXTURES  = 0x0010,
            USE_COVERAGE_SAMPLES = 0x0020,
            COVERAGE_SAMPLES_2X  = 0x0022,  // low 4 bits hold the wanted sample number
            COVERAGE_SAMPLES_4X  = 0x0024,
            COVERAGE_SAMPLES_8X  = 0x0028,
            COVERAGE_SAMPLES_16X = 0x0060   // 16 can't be stored in the low 4 bits, use 0x0040
        };
        
        struct Stage : public osg::Referenced
        {
            std::map<std::string, osg::observer_ptr<osg::Texture>> outputs;
            std::map<std::string, osg::observer_ptr<osg::Uniform>> uniforms;
            osg::observer_ptr<RenderingModuleBase> parentModule;
            osg::ref_ptr<osgVerse::DeferredRenderCallback::RttGeometryRunner> runner;
            osg::ref_ptr<osg::Camera> camera; std::string name;
            osg::Matrix projectionOffset, viewOffset;
            osg::Vec2d depthPartition;  // x: 0=none, 1=front, 2=back; y: global near
            bool inputStage, deferred, overridedPrograms, jitterProjection;

            void applyBuffer(Stage& s, const std::string& buffer, int unit,
                             osg::Texture::WrapMode wp = (osg::Texture::WrapMode)0);
            void applyBuffer(Stage& s, const std::string& buffer, const std::string& name, int unit,
                             osg::Texture::WrapMode wp = (osg::Texture::WrapMode)0);
            void applyBuffer(const std::string& name, int unit, Pipeline* p,
                             int stageID = -1, const std::string& buffer = "",
                             osg::Texture::WrapMode wp = (osg::Texture::WrapMode)0);

            void applyUniform(osg::Uniform* u);
            void applyTexture(osg::Texture* tex, const std::string& buffer, int u);
            void applyDefaultTexture(const osg::Vec4& color, const std::string& buffer, int u);

            ScriptableProgram* getProgram();
            const ScriptableProgram* getProgram() const;

            osg::StateSet* getOrCreateStateSet()
            { return deferred ? runner->geometry->getOrCreateStateSet() : camera->getOrCreateStateSet(); }

            osg::StateSet::UniformList getUniforms() const;
            osg::Uniform* getUniform(const std::string& name) const;
            osg::Texture* getTexture(const std::string& name) const;
            osg::Texture* getBufferTexture(osg::Camera::BufferComponent bc);

            osg::Texture* getBufferTexture(const std::string& name)
            { return (outputs.find(name) != outputs.end()) ? outputs[name].get() : NULL; }

            Stage() : name("Undefined"), inputStage(false), deferred(false), overridedPrograms(false),
                      jitterProjection(false) {}
            Stage(const Stage& s)
                : outputs(s.outputs), uniforms(s.uniforms), runner(s.runner),
                  camera(s.camera), name(s.name), depthPartition(s.depthPartition),
                  inputStage(s.inputStage), deferred(s.deferred), overridedPrograms(s.overridedPrograms),
                  jitterProjection(s.jitterProjection) {}
        };

        Pipeline(int glContextVer = 100, int glslVer = 120);
        static osg::GraphicsContext* createGraphicsContext(int w, int h, const std::string& glContext,
                                                           osg::GraphicsContext* shared = NULL, int flags = 0);

        /** Create RTT texture of specific buffer type */
        static osg::Texture* createTexture(BufferType type, int w, int h, int glVer = 0);
        static void setTextureBuffer(osg::Texture* tex, BufferType type, int glVer = 0);

        /** Add necessaray definitions for each Pipeline related shader
            See ShaderLibrary::createShaderDefinitions() for details */
        static void createShaderDefinitions(osg::Shader* s, int glVer, int glslVer,
                                            const std::vector<std::string>& defs = std::vector<std::string>());
        void createShaderDefinitionsFromPipeline(
            osg::Shader* s, const std::vector<std::string>& defs = std::vector<std::string>());

        /** Set pipeline mask of scene graph nodes */
        static void setPipelineMask(osg::Object& node, unsigned int mask,
                                    unsigned int flags = osg::StateAttribute::ON);
        static unsigned int getPipelineMask(osg::Object& node);
        static unsigned int getPipelineMaskFlags(osg::Object& node);

        void addStage(Stage* s) { _stages.push_back(s); }
        void removeStage(unsigned int index) { _stages.erase(_stages.begin() + index); }

        unsigned int getNumStages() const { return _stages.size(); }
        Stage* getStage(unsigned int index) { return _stages[index].get(); }
        Stage* getStage(const std::string& name);

        Stage* getStage(osg::Camera* camera);
        const Stage* getStage(osg::Camera* camera) const;

        /** Start adding pipeline stages after this function */
        void startStages(int w, int h, osg::GraphicsContext* gc = NULL);

        /** Finish all pipeline stages in this function. It will automatically add
            a forward pass for normal scene object rendering */
        void applyStagesToView(osgViewer::View* view, osg::Camera* mainCam, unsigned int defForwardMask);
        
        /** Convenient method to finish pipeline stages */
        void applyStagesToView(osgViewer::View* view, unsigned int forwardMask)
        { applyStagesToView(view, view->getCamera(), forwardMask); }

        /** Remove all stages and reset the viewer to default (clear all slaves) */
        void clearStagesFromView(osgViewer::View* view, osg::Camera* mainCam = NULL);

        /** Apply initial attributes (single-pass stereo geometry shader) for VR mode */
        bool updateStageForStereoVR(Stage* s, osg::Shader* geomShader, bool useClip);

        /** Apply HMD matrices from RenderCallbackXR and begin new XR frame for VR mode */
        bool updateMatricesForStereoVR(Stage* s, RenderCallbackXR* callbackXR, osg::Matrix& view, osg::Matrix& proj,
                                       double presetZnear = 0.0, double presetZfar = 0.0);

        /** Require depth buffer of specific stage to blit to default forward pass */
        void requireDepthBlit(Stage* s, bool addToList)
        { _deferredCallback->requireDepthBlit(s->camera, addToList); }

        /** Use it in a cusom osgViewer::View class! */
        osg::GraphicsOperation* createRenderer(osg::Camera* camera);

        /** The buffer description */
        struct BufferDescription
        {
            osg::Texture* bufferToShare;
            std::string bufferName; BufferType type;

            BufferDescription() : bufferToShare(NULL), type(RGBA_INT8) {}
            BufferDescription(const std::string& s, BufferType t, osg::Texture* tex = NULL)
                : bufferToShare(tex), bufferName(s), type(t) {}
        };
        typedef std::vector<BufferDescription> BufferDescriptions;

        /** Add input stage which uses main scene graph for initial shading and rendering-to-texture */
        Stage* addInputStage(const std::string& n, unsigned int cullMask, int flags,
                             osg::Shader* vs, osg::Shader* fs, int buffers, ...);
        Stage* addInputStage(const std::string& n, unsigned int cullMask, int flags,
                             osg::Shader* vs, osg::Shader* fs, const BufferDescriptions& buffers);

        /** Add textures and use an internal screen-sized buffer for shading */
        Stage* addWorkStage(const std::string& n, float sizeScale,
                            osg::Shader* vs, osg::Shader* fs, int buffers, ...);
        Stage* addWorkStage(const std::string& n, float sizeScale,
                            osg::Shader* vs, osg::Shader* fs, const BufferDescriptions& buffers);

        /** Similar to WorkStage, but will use DeferredRenderCallback::Runner instead of a camera
            Note: it doesn't support <name>ProjectionToWorld which helps rebuild world vertex */
        Stage* addDeferredStage(const std::string& n, float sizeScale, bool runOnce,
                                osg::Shader* vs, osg::Shader* fs, int buffers, ...);
        Stage* addDeferredStage(const std::string& n, float sizeScale, bool runOnce,
                                osg::Shader* vs, osg::Shader* fs, const BufferDescriptions& buffers);

        /** Display shading results on a screen-sized quad */
        Stage* addDisplayStage(const std::string& n, osg::Shader* vs, osg::Shader* fs,
                               const osg::Vec4& screenGeom);

        /** Make deferred stage active/inactive (one-time stage will re-run only once) */
        void activateDeferredStage(const std::string& n, bool active);

        osgVerse::DeferredRenderCallback* getDeferredCallback() { return _deferredCallback.get(); }
        const osgVerse::DeferredRenderCallback* getDeferredCallback() const { return _deferredCallback.get(); }

        /** Invalidate the history of the temporal anti-aliasing pass for one frame, so that it
            is resolved from the current frame only. The application should call it whenever the
            content of the scene changes while the camera stays still, as the pipeline can not
            detect such a case by itself: switching to another scene, loading or removing a
            model, resetting the view... Camera cuts (manipulator jumps), skipped frames, window
            resizing and the very first frame are detected automatically. It does nothing if
            temporal anti-aliasing is not enabled */
        void resetTAAHistory();

        osg::GraphicsContext* getContext() { return _stageContext.get(); }
        const osg::GraphicsContext* getContext() const { return _stageContext.get(); }

        osg::Camera* getForwardCamera() { return _forwardCamera.get(); }
        const osg::Camera* getForwardCamera() const { return _forwardCamera.get(); }

        osg::Uniform* getInvScreenResolution() { return _invScreenResolution.get(); }
        osg::Vec2s getStageSize() const { return _stageSize; }

        void setVersionData(GLVersionData* d);
        GLVersionData* getVersionData() { return _glVersionData.get(); }
        int getContextTargetVersion() const { return _glContextVersion; }
        int getGlslTargetVersion() const { return _glslTargetVersion; }
        int getGlCurrentVersion() const { return _glVersion; }

        /** Eye adaptation controller of the standard pipeline (NULL if the pipeline was created
            without post effects, or without using setupStandardPipeline()) */
        void setExposureController(ExposureController* c);
        ExposureController* getExposureController() { return _exposureController.get(); }
        const ExposureController* getExposureController() const { return _exposureController.get(); }

        /** Check if a camera is created by this pipeline (stage or forward) */
        bool isValidCamera(osg::Camera* cam) const
        { return getStage(cam) != NULL || (_forwardCamera == cam); }

        /** Add user modules to this pipeline */
        void addModule(const std::string& n, RenderingModuleBase* cb) { _modules[n] = cb; }
        void removeModule(RenderingModuleBase* cb);
        RenderingModuleBase* getModule(const std::string& n) { return _modules[n].get(); }
        const std::map<std::string, osg::ref_ptr<RenderingModuleBase>>& getModules() const { return _modules; }

        /** Create forward shading stateset which can make use of PBR and lighting functonalities */
        osg::StateSet* createForwardStateSet(osg::Shader* vs, osg::Shader* fs);

        /** Load pipeline preset from stream */
        bool load(std::istream& in, osgViewer::View* view,
                  osg::Camera* mainCam = NULL, bool asEmbedded = false);

    protected:
        void applyDefaultStageData(Stage& s, const std::string& name, osg::Shader* vs, osg::Shader* fs);
        int applyDefaultInputStateSet(osg::StateSet& ss, bool applyDefTextures, bool blendOff);
        int getNumNonDepthBuffers(const BufferDescriptions& buffers);
        
        std::vector<osg::ref_ptr<Stage>> _stages;
        std::map<std::string, osg::ref_ptr<RenderingModuleBase>> _modules;
        osg::ref_ptr<osgVerse::DeferredRenderCallback> _deferredCallback;
        osg::ref_ptr<osg::GraphicsContext> _stageContext;
        osg::ref_ptr<osg::Depth> _deferredDepth;
        osg::ref_ptr<osg::Uniform> _invScreenResolution;
        osg::ref_ptr<GLVersionData> _glVersionData;
        osg::ref_ptr<ExposureController> _exposureController;
        osg::observer_ptr<osg::Camera> _forwardCamera;
        osg::Vec2s _stageSize;
        int _glContextVersion, _glVersion, _glslTargetVersion;
    };

    /** Eye adaptation (auto exposure) controller of the standard pipeline. The adaptation itself
        runs on the GPU: a pair of 1x1 stages (see the "EyeAdaptation0/1" stages which
        setupStandardPipeline creates, and std_exposure_adaptation.frag.glsl) ping-pong the state
        of the adaptation, one of them reading the state which the other one wrote during the
        previous frame. This controller only drives them from the CPU side, once per frame: it
        uploads the frame delta time and the tuning values, swaps which stage of the pair is
        active, and points the tone mapping stage to the exposure of the frame. Nothing is ever
        read back from the GPU, so there is neither a synchronization point nor a stall here.

        Everything is tunable at runtime: target middle gray (key value), manual compensation in
        stops (EV), the adaptation speeds used when the exposure has to increase or decrease, and
        the exposure limits. Note it is declared after Pipeline because it names Pipeline::Stage */
    class ExposureController : public osg::Camera::DrawCallback
    {
    public:
        /** - stage0, stage1: the two 1x1 stages which hold the adaptation state. Their output
                              buffers hold the exposure encoded in log2 space, see the shader
            - tonemapping: the stage which has to sample the exposure of the current frame */
        ExposureController(Pipeline::Stage* stage0, Pipeline::Stage* stage1,
                           Pipeline::Stage* tonemapping);

        /** Target middle gray of the adapted image: exposure = keyValue / averageLuminance */
        void setKeyValue(float k) { _keyValue = k; }
        float getKeyValue() const { return _keyValue; }

        /** Manual exposure compensation, in stops (EV), can be changed at any time */
        void setCompensation(float stops) { _compensation = stops; }
        float getCompensation() const { return _compensation; }

        /** Speeds, in 1/second, of the exponential approach used when the exposure has to
            increase (the image is too dark) or to decrease (the image is too bright) */
        void setAdaptationSpeeds(float increase, float decrease)
        { _speedIncrease = increase; _speedDecrease = decrease; }

        /** Limits of the exposure, to avoid extreme values on unusual frames */
        void setExposureLimits(float mn, float mx) { _minExposure = mn; _maxExposure = mx; }
        float getMinExposure() const { return _minExposure; }
        float getMaxExposure() const { return _maxExposure; }

        /** The 1x1 buffer which holds the exposure adapted by the last frame, encoded over the
            log2 range documented in std_exposure_adaptation.frag.glsl. Sample it (in a HUD or a
            debug pass for instance) to display the exposure: as the whole adaptation runs on the
            GPU, there is no CPU side value to read here */
        osg::Texture* getExposureBuffer() const;

        virtual void operator()(osg::RenderInfo& renderInfo) const;

    protected:
        /** The two stages of the ping-pong pair, and the stage which samples their result */
        osg::observer_ptr<Pipeline::Stage> _stages[2];
        osg::observer_ptr<Pipeline::Stage> _tonemapping;
        float _keyValue, _compensation, _speedIncrease, _speedDecrease;
        float _minExposure, _maxExposure;
        mutable double _lastTime; mutable unsigned int _frames;
        mutable int _active;
    };

    /** Standard pipeline parameters */
    struct StandardPipelineParameters
    {
        enum UserInputOccasion
        {
            BEFORE_DEFERRED_PASSES,  // After GBuffer, will be affected by PBR lighting
            BEFORE_POSTEFFECTS,      // After shadow composition, will have light reflections
            BEFORE_FINAL_STAGE       // After post-effects, before final display pass
        };

        enum UserInputType
        {
            DEFAULT_INPUT,
            DEPTH_PARTITION_FRONT,
            DEPTH_PARTITION_BACK
        };

        /** User input stage information */
        struct UserInputStageData
        {
            std::string stageName;
            unsigned int mask;
            UserInputType type;

            UserInputStageData() : mask(0), type(DEFAULT_INPUT) { stageName = "Forward"; }
            UserInputStageData(const std::string& n, unsigned int m,
                               UserInputType t = DEFAULT_INPUT) : stageName(n), mask(m), type(t) {}
        };

        /** Shader configuations */
        struct ShaderParameters
        {
            osg::ref_ptr<osg::Shader> gbufferVS, gbufferGS, shadowCastVS, shadowCastGS;
            osg::ref_ptr<osg::Shader> gbufferFS, shadowCastFS, ssaoFS, ssaoBlurFS;
            osg::ref_ptr<osg::Shader> pbrLightingFS, shadowCombineFS, shadowDebugCombineFS;
            osg::ref_ptr<osg::Shader> downsampleFS, luminanceFS, brightnessFS, brightnessCombineFS, bloomFS;
            osg::ref_ptr<osg::Shader> exposureAdaptationFS;
            osg::ref_ptr<osg::Shader> tonemappingFS, antiAliasingFS, taaFS, displayFS, quadFS;
            osg::ref_ptr<osg::Shader> skyboxFS;
            osg::ref_ptr<osg::Shader> brdfLutFS, envPrefilterFS, irrConvolutionFS;
            osg::ref_ptr<osg::Shader> forwardVS, forwardFS, quadVS;
        };

        typedef std::vector<UserInputStageData> UserInputStageList;
        std::map<UserInputOccasion, UserInputStageList> userInputs;
        ShaderParameters shaders;

        osg::ref_ptr<osg::ImageSequence> skyboxIBL;
        osg::ref_ptr<osg::Texture2D> skyboxMap;
        unsigned int originWidth, originHeight, deferredMask, forwardMask;
        unsigned int shadowCastMask, shadowNumber, shadowResolution, shadowTechnique, coverageSamples;
        /** Receiver-side bias of the shadow lookup, forwarded to ShadowModule::setShadowBias().
            All three values are multiples of one shadow-map texel, so they stay valid when
            shadowResolution, shadowNumber or the scene size change:
            - shadowConstantBias: bias applied everywhere, in shadow-map texels
            - shadowSlopeScale: extra bias per unit of depth slope (grazing surfaces)
            - shadowNormalOffsetScale: offset of the lookup position along the receiver normal
            All three default to 0 (the plain hard comparison). Enable them only when one shadow
            texel is small in world space, otherwise the bias removes valid shadows */
        float shadowConstantBias, shadowSlopeScale, shadowNormalOffsetScale;
        /** Limit the shadow distance in world units in front of the camera, forwarded to
            LightModule::setShadowMaxDistance(). Casters and receivers farther than it are left
            unshadowed, which is what a scene with a camera far plane far wider than the
            interesting area needs (a city on a globe, for instance). A negative value lets the
            shadow module derive the range from the scene bounds instead */
        double shadowMaxDistance;
        /** Screen-space contact shadows, forwarded to ShadowModule::setContactShadow().
            contactShadowLength is in world units and 0 (the default) leaves the effect off: it
            has to be sized to the details the shadow map misses, which depends on the scene */
        float contactShadowLength, contactShadowStrength;
        double depthPartitionNearValue;
        bool withEmbeddedViewer, debugShadowModule, debugShadowCombination, enableVSync, enableMRT;
        bool enableAO, enablePostEffects, enableUserInput, enableDepthPartition, enableVR, enable3DGS;
        /** Temporal anti-aliasing. It needs the jittered projection & history buffer of the
            G-Buffer, and is switched off automatically on low-performance devices */
        bool enableTAA;
        /** Bloom and eye adaptation (auto exposure). Both are switched off together: the tone
            mapping then reads a black bloom and a fixed neutral exposure, so the rest of the
            rendering and of the post effects is left untouched */
        bool enableBloom;
        /** Draw the sky as a full-screen stage inside the deferred pipeline (using skyboxMap
            and skyboxFS), so that it takes part in bloom, tone mapping and TAA instead of
            being painted by a sky box camera after the deferred stages */
        bool useDeferredSky;

        StandardPipelineParameters();
        StandardPipelineParameters(const std::string& shaderDir, const std::string& skyboxFile);
        void addUserInputStage(const std::string& name, unsigned int mask,
                               UserInputOccasion occasion, UserInputType t = DEFAULT_INPUT);
        void applyUserInputStages(osg::Camera* mainCam, Pipeline* pipeline, UserInputOccasion occ,
                                  bool sameStage, osg::Texture* colorBuffer, osg::Texture* depthBuffer) const;
    };

    /** Create standard pipeline */
    extern bool setupStandardPipeline(Pipeline* p, osgViewer::View* view,
                                      const StandardPipelineParameters& spp);
    extern bool setupStandardPipelineEx(Pipeline* p, osgViewer::View* view, osg::Camera* mainCam,
                                        const StandardPipelineParameters& spp);

    /** Setup a OpenGL version tester and save the result to user-data of the pipeline */
    extern GLVersionData* queryOpenGLVersion(Pipeline* p, bool asEmbedded,
                                             osg::GraphicsContext* embeddedGC = NULL);

    /** Create a quick PBR+deferred pipeline viewer */
    class StandardPipelineViewer : public osgViewer::Viewer
    {
    public:
        StandardPipelineViewer(bool withSky = true, bool withSelector = true, bool withDebugShadow = false);
        StandardPipelineViewer(const StandardPipelineParameters& spp,
                               bool withSky = true, bool withSelector = true);

        void setMainLight(const osg::Vec3& color, const osg::Vec3& dir);
        osg::Geode* getLightRoot() { return _lightGeode.get(); }
        StandardPipelineParameters& getParameters() { return _parameters; }
        Pipeline* getPipeline() { return _pipeline.get(); }

        virtual void setSceneData(osg::Node* node);
        virtual void realize();

    protected:
        virtual osg::GraphicsOperation* createRenderer(osg::Camera* camera);
        void initialize(const StandardPipelineParameters& spp, bool withSky, bool withSelector);

        osg::ref_ptr<Pipeline> _pipeline;
        osg::ref_ptr<osg::Group> _root;
        osg::ref_ptr<osg::Geode> _lightGeode, _textGeode;
        osg::observer_ptr<osg::Node> _scene;
        StandardPipelineParameters _parameters;
        bool _withSky, _withSelector;
    };

    /** Realize operation for viewer to check OpenGL states before rendering */
    class RealizeOperation : public osg::Operation
    {
    public:
        RealizeOperation(bool checkGL = true) { if (checkGL) _glVersionData = new GLVersionData; }
        GLVersionData* getVersionData() { return _glVersionData.get(); }
        virtual void operator()(osg::Object* object);

    protected:
        osg::ref_ptr<GLVersionData> _glVersionData;
    };
}

#endif

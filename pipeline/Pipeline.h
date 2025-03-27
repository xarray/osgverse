#ifndef MANA_PP_PIPELINE_HPP
#define MANA_PP_PIPELINE_HPP

#include <osg/Vec2s>
#include <osg/ImageSequence>
#include <osg/Depth>
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
        std::string version, renderer;
        int glVersion, glslVersion;
        bool glslSupported, fboSupported;
        bool drawBuffersSupported;
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
            DEPTH16/*16bit*/, DEPTH24_STENCIL8/*32bit*/, DEPTH32/*32bit*/
        };

        enum InputFlag
        {
            NO_DEFAULT_TEXTURES  = 0x0010,
            USE_COVERAGE_SAMPLES = 0x0020,
            COVERAGE_SAMPLES_2X  = 0x0021,
            COVERAGE_SAMPLES_4X  = 0x0024,
            COVERAGE_SAMPLES_8X  = 0x0028,
            COVERAGE_SAMPLES_16X = 0x002F
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
            bool inputStage, deferred;

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

            osg::StateSet::UniformList getUniforms() const;
            osg::Uniform* getUniform(const std::string& name) const;
            osg::Texture* getTexture(const std::string& name) const;
            osg::Texture* getBufferTexture(osg::Camera::BufferComponent bc);

            osg::Texture* getBufferTexture(const std::string& name)
            { return (outputs.find(name) != outputs.end()) ? outputs[name].get() : NULL; }

            Stage() : name("Undefined"), inputStage(false), deferred(false) {}
            Stage(const Stage& s)
                : outputs(s.outputs), uniforms(s.uniforms), runner(s.runner),
                  camera(s.camera), name(s.name), depthPartition(s.depthPartition),
                  inputStage(s.inputStage), deferred(s.deferred) {}
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

        /** Apply attributes for VR mode geometry shader */
        void updateStageForStereoVR(Stage* s, osg::Shader* geomShader, double eyeSep, bool useClip);

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
        osg::observer_ptr<osg::Camera> _forwardCamera;
        osg::Vec2s _stageSize;
        int _glContextVersion, _glVersion, _glslTargetVersion;
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
            osg::ref_ptr<osg::Shader> pbrLightingFS, shadowCombineFS, downsampleFS;
            osg::ref_ptr<osg::Shader> brightnessFS, brightnessCombineFS, bloomFS;
            osg::ref_ptr<osg::Shader> tonemappingFS, antiAliasingFS, displayFS, quadFS;
            osg::ref_ptr<osg::Shader> brdfLutFS, envPrefilterFS, irrConvolutionFS;
            osg::ref_ptr<osg::Shader> forwardVS, forwardFS, quadVS;
        };

        typedef std::vector<UserInputStageData> UserInputStageList;
        std::map<UserInputOccasion, UserInputStageList> userInputs;
        ShaderParameters shaders;

        osg::ref_ptr<osg::ImageSequence> skyboxIBL;
        osg::ref_ptr<osg::Texture2D> skyboxMap;
        unsigned int originWidth, originHeight, deferredMask, forwardMask;
        unsigned int shadowCastMask, shadowNumber, shadowResolution, coverageSamples;
        double depthPartitionNearValue, eyeSeparationVR;
        bool withEmbeddedViewer, debugShadowModule, enableVSync, enableMRT;
        bool enableAO, enablePostEffects, enableUserInput, enableDepthPartition, enableVR;

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
}

#endif

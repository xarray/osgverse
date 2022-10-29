#ifndef MANA_PP_PIPELINE_HPP
#define MANA_PP_PIPELINE_HPP

#include <osg/Vec2s>
#include <osg/Depth>
#include <osg/Program>
#include <osg/Texture2D>
#include <osg/Group>
#include <osgViewer/View>
#include <string>
#include "DeferredCallback.h"

#define FORWARD_SCENE_MASK  0x00ffffff
#define DEFERRED_SCENE_MASK 0xff000000
#define SHADOW_CASTER_MASK  0x10000000

#ifndef GL_HALF_FLOAT
    #define GL_HALF_FLOAT                     0x140B
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
    /** Effect pipeline using a list of slave cameras, without invading main scene graph
        Some uniforms will be set automatically for internal stages:
        - sampler2d DiffuseMap: diffuse/albedo RGB texture of input scene
        - sampler2d NormalMap: tangent-space normal texture of input scene
        - sampler2d SpecularMap: specular RGB texture of input scene
        - sampler2d ShininessMap: roughness+metallic (R+G) texture of input scene
        - sampler2d AmbientMap: ambient occlusion R texture of input scene
        - sampler2d EmissiveMap: emissive RGB texture of input scene
        - sampler2d ReflectionMap: reflection RGB texture of input scene
        - mat4 <StageName>Matrices: matrices of specified input stage for rebuilding vertex attributes
                                    Including: world-to-view, view-to-world, view-to-proj, proj-to-view
        - vec2 NearFarPlanes: calculated near/far values of entire scene
        - vec2 InvScreenResolution: (1.0 / view-width, 1.0 / view-height)
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
        
        struct Stage : public osg::Referenced
        {
            std::map<std::string, osg::observer_ptr<osg::Texture>> outputs;
            std::map<std::string, osg::observer_ptr<osg::Uniform>> uniforms;
            osg::ref_ptr<osgVerse::DeferredRenderCallback::RttGeometryRunner> runner;
            osg::ref_ptr<osg::Camera> camera; std::string name;
            bool inputStage, deferred;

            void applyUniform(osg::Uniform* u);
            void applyBuffer(Stage& s, const std::string& buffer, int unit,
                             osg::Texture::WrapMode wp = (osg::Texture::WrapMode)0);
            void applyBuffer(Stage& s, const std::string& buffer, const std::string& name, int unit,
                             osg::Texture::WrapMode wp = (osg::Texture::WrapMode)0);
            void applyTexture(osg::Texture* tex, const std::string& buffer, int u);
            void applyDefaultTexture(const osg::Vec4& color, const std::string& buffer, int u);

            osg::StateSet::UniformList getUniforms() const;
            osg::Uniform* getUniform(const std::string& name) const;
            osg::Texture* getTexture(const std::string& name) const;

            osg::Texture* getBufferTexture(const std::string& name)
            { return (outputs.find(name) != outputs.end()) ? outputs[name].get() : NULL; }

            Stage() : name("Undefined"), inputStage(false), deferred(false) {}
            Stage(const Stage& s)
                : outputs(s.outputs), uniforms(s.uniforms), runner(s.runner), camera(s.camera),
                  name(s.name), inputStage(s.inputStage), deferred(s.deferred) {}
        };

        Pipeline();
        static osg::Texture* createTexture(BufferType type, int w, int h);

        void addStage(Stage* s) { _stages.push_back(s); }
        void removeStage(unsigned int index) { _stages.erase(_stages.begin() + index); }
        void clearStages() { _stages.clear(); }

        unsigned int getNumStages() const { return _stages.size(); }
        Stage* getStage(unsigned int index) { return _stages[index].get(); }
        Stage* getStage(const std::string& name);
        Stage* getStage(osg::Camera* camera);

        /** Start adding pipeline stages after this function */
        void startStages(int w, int h, osg::GraphicsContext* gc = NULL);

        /** Finish all pipeline stages in this function. It will automatically add
            a forward pass for normal scene object rendering */
        void applyStagesToView(osgViewer::View* view, unsigned int forwardMask);

        /** Require depth buffer of specific stage to blit to default forward pass */
        void requireDepthBlit(Stage* s, bool addToList)
        { _deferredCallback->requireDepthBlit(s->camera, addToList); }

        /** Use it in a cusom osgViewer::View class! */
        osg::GraphicsOperation* createRenderer(osg::Camera* camera);

        /** Add input stage which uses main scene graph for initial shading and rendering-to-texture */
        Stage* addInputStage(const std::string& n, unsigned int cullMask, int samples,
                             osg::Shader* vs, osg::Shader* fs, int buffers, ...);

        /** Add textures and use an internal screen-sized buffer for shading */
        Stage* addWorkStage(const std::string& n, osg::Shader* vs, osg::Shader* fs, int buffers, ...);

        /** Similar to WorkStage, but will use DeferredRenderCallback::Runner instead of a camera
            Note: it doesn't support <name>ProjectionToWorld which helps rebuild world vertex */
        Stage* addDeferredStage(const std::string& n, bool runOnce,
                                osg::Shader* vs, osg::Shader* fs, int buffers, ...);

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

        void addModule(const std::string& n, osg::NodeCallback* cb) { _modules[n] = cb; }
        void removeModule(osg::NodeCallback* cb);
        osg::NodeCallback* getModule(const std::string& n) { return _modules[n].get(); }
        const std::map<std::string, osg::ref_ptr<osg::NodeCallback>>& getModules() const { return _modules; }

    protected:
        void applyDefaultStageData(Stage& s, const std::string& name, osg::Shader* vs, osg::Shader* fs);
        void applyDefaultInputStateSet(osg::StateSet* ss);
        
        std::vector<osg::ref_ptr<Stage>> _stages;
        std::map<std::string, osg::ref_ptr<osg::NodeCallback>> _modules;
        osg::ref_ptr<osgVerse::DeferredRenderCallback> _deferredCallback;
        osg::ref_ptr<osg::GraphicsContext> _stageContext;
        osg::ref_ptr<osg::Depth> _deferredDepth;
        osg::ref_ptr<osg::Uniform> _invScreenResolution;
        osg::observer_ptr<osg::Camera> _forwardCamera;
        osg::Vec2s _stageSize;
    };

    /** Standard pipeline parameters */
    struct StandardPipelineParameters
    {
        struct ShaderParameters
        {
            osg::ref_ptr<osg::Shader> gbufferVS, shadowCastVS, quadVS;
            osg::ref_ptr<osg::Shader> gbufferFS, shadowCastFS, ssaoFS, ssaoBlurFS;
            osg::ref_ptr<osg::Shader> pbrLightingFS, shadowCombineFS, displayFS;
            osg::ref_ptr<osg::Shader> brdfLutFS, envPrefilterFS, irrConvolutionFS;
        };

        ShaderParameters shaders;
        osg::ref_ptr<osg::StateSet> skyboxIBL;
        osg::ref_ptr<osg::Texture2D> skyboxMap;
        unsigned int originWidth, originHeight;
        unsigned int deferredMask, forwardMask, shadowCastMask;
        unsigned int shadowNumber, shadowResolution;
        bool debugShadowModule;

        StandardPipelineParameters();
        StandardPipelineParameters(const std::string& shaderDir, const std::string& skyboxFile);
    };

    /** Standard pipeline */
    extern void setupStandardPipeline(Pipeline* p, osgViewer::View* view, const StandardPipelineParameters& spp);
}

#endif

#ifndef MANA_PP_PIPELINE_HPP
#define MANA_PP_PIPELINE_HPP

#include <osg/Vec2s>
#include <osg/Program>
#include <osg/Texture2D>
#include <osg/Group>
#include <osgViewer/View>
#include <string>
#include "DeferredCallback.h"

#define FORWARD_SCENE_MASK  0x0000ff00
#define DEFERRED_SCENE_MASK 0x00ff0000
#define SHADOW_CASTER_MASK  0x01000000

namespace osgVerse
{
    /** Effect pipeline using a list of slave cameras, without invading main scene graph
        Some uniforms will be set automatically for internal stages:
        - sampler2d DiffuseMap: diffuse/albedo RGB texture of input scene
        - sampler2d NormalMap: tangent-space normal texture of input scene
        - sampler2d SpecularMap: specular RGB texture of input scene
        - sampler2d ShininessMap: metallic+roughness RG texture of input scene
        - sampler2d AmbientMap: ambient occlusion R texture of input scene
        - sampler2d EmissiveMap: emissive RGB texture of input scene
        - sampler2d ReflectionMap: reflection RGB texture of input scene
        - mat4 <StageName>Matrices: matrices of specified input stage for rebuilding vertex attributes
                                    Including: world-to-view, view-to-world, view-to-proj, proj-to-view
        - vec2 NearFarPlanes: calculated near/far values of entire scene
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
            void applyBuffer(Stage& s, const std::string& buffer, int unit);
            void applyTexture(osg::Texture* tex, const std::string& buffer, int u);
            void applyDefaultTexture(const osg::Vec4& color, const std::string& buffer, int u);

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
        Stage* addInputStage(const std::string& n, unsigned int cullMask,
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
        osg::Vec2s getStageSize() const { return _stageSize; }

        void addModule(const std::string& n, osg::NodeCallback* cb) { _modules[n] = cb; }
        osg::NodeCallback* getModule(const std::string& n) { return _modules[n].get(); }
        const std::map<std::string, osg::ref_ptr<osg::NodeCallback>>& getModules() const { return _modules; }

    protected:
        void applyDefaultStageData(Stage& s, const std::string& name, osg::Shader* vs, osg::Shader* fs);
        void applyDefaultInputStateSet(osg::StateSet* ss);
        
        std::vector<osg::ref_ptr<Stage>> _stages;
        std::map<std::string, osg::ref_ptr<osg::NodeCallback>> _modules;
        osg::ref_ptr<osgVerse::DeferredRenderCallback> _deferredCallback;
        osg::ref_ptr<osg::GraphicsContext> _stageContext;
        osg::observer_ptr<osg::Camera> _forwardCamera;
        osg::Vec2s _stageSize;
    };

    /** Standard pipeline */
    extern void setupStandardPipeline(Pipeline* p, osgViewer::View* view, osg::Group* auxRoot,
                                      const std::string& shaderDir, unsigned int originW, unsigned int originH);
}

#endif

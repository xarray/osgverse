#ifndef MANA_PIPELINE_HPP
#define MANA_PIPELINE_HPP

#include <osg/Program>
#include <osg/Texture2D>
#include <osg/Group>
#include <osgViewer/View>
#include "DeferredCallback.h"

namespace osgVerse
{
    /** Effect pipeline using a list of slave cameras, without invading main scene graph
        - InputStage: add scene graph for shading and rendering-to-texture
        - WorkStage: add textures and use an internal screen-sized buffer for shading
        - DeferredStage: same with WorkStage, but use DeferredRenderCallback::Runner instead a camera
        - DisplayStage: display shading results on a screen-sized quad
        
        Some uniforms will be set automatically for internal stages:
        - sampler2d DiffuseMap: diffuse/albedo RGB texture of input scene
        - sampler2d NormalMap: tangent-space normal texture of input scene
        - sampler2d SpecularMap: specular RGB texture of input scene
        - sampler2d ShininessMap: metallic+roughness RG texture of input scene
        - sampler2d AmbientMap: ambient occlusion R texture of input scene
        - sampler2d EmissiveMap: emissive RGB texture of input scene
        - sampler2d ReflectionMap: reflection RGB texture of input scene
        - mat4 <StageName>ProjectionToWorld: projection-to-world matrix of specified input stage
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
            std::map<std::string, osg::observer_ptr<osg::Texture2D>> outputs;
            std::map<std::string, osg::observer_ptr<osg::Uniform>> uniforms;
            osg::ref_ptr<osgVerse::DeferredRenderCallback::RttGeometryRunner> runner;
            osg::ref_ptr<osg::Camera> camera; std::string name;
            bool inputStage, deferred;

            void applyUniform(const std::string& name, osg::Uniform* u);
            void applyBuffer(Stage& s, const std::string& buffer, int unit);
            void applyTexture(osg::Texture* tex, const std::string& buffer, int u);
            void applyDefaultTexture(const osg::Vec4& color, const std::string& buffer, int u);

            osg::Texture2D* getBufferTexture(const std::string& name)
            { return (outputs.find(name) != outputs.end()) ? outputs[name].get() : NULL; }

            Stage() : name("Undefined"), inputStage(false), deferred(false) {}
            Stage(const Stage& s)
                : outputs(s.outputs), uniforms(s.uniforms), runner(s.runner), camera(s.camera),
                  name(s.name), inputStage(s.inputStage), deferred(s.deferred) {}
        };

        Pipeline();
        static osg::Texture2D* createTexture(BufferType type, int w, int h);

        void addStage(Stage* s) { _stages.push_back(s); }
        void removeStage(unsigned int index) { _stages.erase(_stages.begin() + index); }
        void clearStages() { _stages.clear(); }

        unsigned int getNumStages() const { return _stages.size(); }
        Stage* getStage(unsigned int index) { return _stages[index].get(); }
        Stage* getStage(const std::string& name);

        /** Start adding pipeline stages after this function */
        void startStages(int w, int h);

        /** Finish all pipeline stages in this function. It will automatically add
            a forward pass for normal scene object rendering */
        void applyStagesToView(osgViewer::View* view, unsigned int forwardMask);

        /** Require depth buffer of specific stage to blit to default forward pass */
        void requireDepthBlit(Stage* s, bool addToList)
        { _deferredCallback->requireDepthBlit(s->camera, addToList); }

        /** Use it in a cusom osgViewer::View class! */
        osg::GraphicsOperation* createRenderer(osg::Camera* camera);

        Stage* addInputStage(const std::string& n, unsigned int cullMask,
                             osg::Shader* vs, osg::Shader* fs, int buffers, ...);
        Stage* addWorkStage(const std::string& n, osg::Shader* vs, osg::Shader* fs, int buffers, ...);
        Stage* addDeferredStage(const std::string& n, osg::Shader* vs, osg::Shader* fs, int buffers, ...);
        Stage* addDisplayStage(const std::string& n, osg::Shader* vs, osg::Shader* fs,
                               const osg::Vec4& screenGeom);

        osgVerse::DeferredRenderCallback* getDeferredCallback() { return _deferredCallback.get(); }
        const osgVerse::DeferredRenderCallback* getDeferredCallback() const { return _deferredCallback.get(); }
        osg::GraphicsContext* getContext() { return _stageContext.get(); }
        const osg::GraphicsContext* getContext() const { return _stageContext.get(); }
        osg::Camera* getForwardCamera() { return _forwardCamera.get(); }
        const osg::Camera* getForwardCamera() const { return _forwardCamera.get(); }
        osg::Vec2i getStageSize() const { return _stageSize; }

    protected:
        void applyDefaultStageData(Stage& s, const std::string& name, osg::Shader* vs, osg::Shader* fs);
        void applyDefaultInputStateSet(osg::StateSet* ss);
        
        std::vector<osg::ref_ptr<Stage>> _stages;
        osg::ref_ptr<osgVerse::DeferredRenderCallback> _deferredCallback;
        osg::ref_ptr<osg::GraphicsContext> _stageContext;
        osg::observer_ptr<osg::Camera> _forwardCamera;
        osg::Vec2i _stageSize;
    };
}

#endif

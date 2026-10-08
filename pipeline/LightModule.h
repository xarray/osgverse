#ifndef MANA_PP_LIGHT_MODULE_HPP
#define MANA_PP_LIGHT_MODULE_HPP

#include <osg/CullFace>
#include <osg/PolygonOffset>
#include <osg/LightSource>
#include <osg/Geometry>
#include <osg/TextureBuffer>
#include "Pipeline.h"
#include "LightDrawable.h"
#include "LightCluster.h"

namespace osgVerse
{
    class LightModule : public RenderingModuleBase
    {
    public:
        LightModule(const std::string& name, Pipeline* pipeline, int maxLightsInPass = 24);
        virtual LightModule* asLightModule() { return this; }
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

        /** Feed light parameter data & uniforms to certain pipeline stage */
        int applyTextureAndUniforms(Pipeline::Stage* stage, const std::string& prefix, int startU);

        /** Whether the light parameter table is stored in a texture buffer object (TBO), which is
            the default on platforms providing samplerBuffer (desktop GL3 core, see
            VERSE_LIGHT_BUFFER in ShaderLibrary::createShaderDefinitions). Low-end devices
            (GLES2/WebGL1) and the legacy JSON pipeline keep the Texture2D update path */
        bool getUseTextureBuffer() const { return _useTextureBuffer; }
        void setUseTextureBuffer(bool b) { _useTextureBuffer = b; }

        /** Clustered light culling. It is enabled by default (only effective together with the
            TBO path, as the cluster index buffers are texture buffers themselves): the lighting
            shader then only walks the lights of the cluster a pixel falls in. Turn it off to
            compare with, or fall back to, the plain per-pixel loop over the whole table */
        bool getClusterEnabled() const;
        void setClusterEnabled(bool b);
        LightCluster* getLightCluster() { return _lightCluster.get(); }
        const LightCluster* getLightCluster() const { return _lightCluster.get(); }

        /** Maximum number of lights the flat (non-clustered) loop processes per pass */
        void setMaxLightsInPass(int n) { _maxLightsInPass = n; }
        int getMaxLightsInPass() const { return _maxLightsInPass; }

        /** The TBO copy of the parameter table (NULL when the Texture2D path is used) */
        osg::TextureBuffer* getParameterBuffer() { return _parameterBuffer.get(); }
        const osg::TextureBuffer* getParameterBuffer() const { return _parameterBuffer.get(); }

        /** Set main-light which can automatically update shadow as well */
        void setMainLight(LightDrawable* ld, const std::string& shadowModule)
        { _mainLight = ld; _shadowModuleName = shadowModule; }

        /** Limit the shadow distance, in world units in front of the camera: casters and
            receivers farther than this are left unshadowed. A scene whose camera far plane is
            far wider than the interesting area (a globe viewer, for instance) needs it,
            otherwise the cascades are spread over empty space and lose all their resolution.
            0 or a negative value leaves the range to the shadow module, which derives it from
            the scene bounds instead */
        void setShadowMaxDistance(double d) { _shadowMaxDistance = d; }
        double getShadowMaxDistance() const { return _shadowMaxDistance; }

        LightDrawable* getMainLight() { return _mainLight.get(); }
        const std::string& getShadowModuleName() const { return _shadowModuleName; }

        /** Get light parameter table data, 1024x4 RGBA32F with one column per light:
            - row0: light color & power (vec3), light type (float)
            - row1: eye-space position (vec3), attenuation range (float)
            - row2: eye-space direction (vec3), spot cutoff as cosine (float)
            - row3: reserved for future use and always zero
        */
        osg::Texture2D* getParameterTable() { return _parameterTex.get(); }
        const osg::Texture2D* getParameterTable() const { return _parameterTex.get(); }

        osg::Uniform* getLightNumber() { return _lightNumber.get(); }
        const osg::Uniform* getLightNumber() const { return _lightNumber.get(); }

    protected:
        virtual ~LightModule();

        /** Rebuild the light parameter table (and the light clusters) from the lights collected by
            the cull traversal, expressed in the view space of 'cam', which has to be the camera
            drawing the stage that consumes the table */
        void updateLightTable(osg::Camera* cam);

        /** Draw callback refreshing the light table right before the stage consuming it is drawn.
            Both the camera matrices of the frame being rendered and the lights culled for it are
            available at that moment, which is what makes the eye-space light data agree with the
            matrices the lighting shader uses (see LightModule::updateLightTable). */
        class LightTableCallback : public osg::Camera::DrawCallback
        {
        public:
            LightTableCallback(LightModule* m, osg::Camera* cam) : _module(m), _camera(cam) {}
            virtual void operator()(osg::RenderInfo& renderInfo) const;

        protected:
            osg::observer_ptr<LightModule> _module;
            osg::observer_ptr<osg::Camera> _camera;
        };

        osg::observer_ptr<Pipeline> _pipeline;
        osg::observer_ptr<osg::Camera> _tableCamera;  // camera whose pre-draw refreshes the table
        osg::ref_ptr<LightDrawable> _mainLight;
        osg::ref_ptr<osg::Texture2D> _parameterTex;      // low-end fallback & forward PBR
        osg::ref_ptr<osg::TextureBuffer> _parameterBuffer;  // TBO mode (default)
        osg::ref_ptr<osg::Image> _parameterImage;        // shared source data of the two above
        osg::ref_ptr<osg::Uniform> _lightNumber;         // vec2
        osg::ref_ptr<LightCluster> _lightCluster;        // clustered light culling
        osg::ref_ptr<osg::Uniform> _clusterEnabled;      // int
        osg::ref_ptr<osg::Uniform> _clusterGrid;         // ivec3
        osg::ref_ptr<osg::Uniform> _clusterTile;         // vec2
        osg::ref_ptr<osg::Uniform> _clusterDepth;        // vec2
        std::string _shadowModuleName;
        int _maxLightsInPass;
        bool _useTextureBuffer;
        double _shadowMaxDistance;
    };

    class LightGlobalManager : public osg::Referenced
    {
    public:
        static LightGlobalManager* instance();
        LightCullCallback* getCallback() { return _callback.get(); }
        bool checkDirty() { bool b = _dirty; _dirty = false; return b; }

        struct LightData
        {
            LightDrawable* light;
            unsigned int frameNo;
        };
        size_t getSortedResult(std::vector<LightData>& result);

        void add(const LightData& ld) { _lights[ld.light] = ld; _dirty = true; }
        void remove(LightDrawable* light);
        void prune(const osg::FrameStamp* fs, int outdatedFrames = 5);

    protected:
        LightGlobalManager();
        std::map<LightDrawable*, LightData> _lights;
        osg::ref_ptr<LightCullCallback> _callback;
        bool _dirty;
    };
}

#endif

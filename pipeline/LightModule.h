#ifndef MANA_PP_LIGHT_MODULE_HPP
#define MANA_PP_LIGHT_MODULE_HPP

#include <osg/CullFace>
#include <osg/PolygonOffset>
#include <osg/LightSource>
#include <osg/Geometry>
#include "Pipeline.h"
#include "LightDrawable.h"

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

        osg::observer_ptr<Pipeline> _pipeline;
        osg::ref_ptr<LightDrawable> _mainLight;
        osg::ref_ptr<osg::Texture2D> _parameterTex;
        osg::ref_ptr<osg::Image> _parameterImage;
        osg::ref_ptr<osg::Uniform> _lightNumber;  // vec2
        std::string _shadowModuleName;
        int _maxLightsInPass;
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
            osg::Matrix matrix;
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

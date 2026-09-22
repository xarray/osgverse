#ifndef MANA_PP_SHADOW_MODULE_HPP
#define MANA_PP_SHADOW_MODULE_HPP

#include <osg/CullFace>
#include <osg/PolygonOffset>
#include <osg/Texture2DArray>
#include <osg/Geometry>
#include "Pipeline.h"
#define MAX_SHADOWS 4

namespace osgVerse
{
    class ShadowModule : public RenderingModuleBase
    {
    public:
        struct ShadowData : public osg::Referenced  // will be set to camera's user-data
        {
            int index, smallPixels; osg::BoundingBoxd bound;
            osg::Matrix viewMatrix, projMatrix; osg::ref_ptr<osg::Viewport> _viewport;
            ShadowData() { index = -1; smallPixels = 0; }
        };

        ShadowModule(const std::string& name, Pipeline* pipeline, bool withDebugGeom);
        virtual ShadowModule* asShadowModule() { return this; }

        enum TechniqueItem
        {
            DefaultSM = 0, EyeSpaceDepthSM = 0x1, BandPCF = 0x2, PossionPCF = 0x4,
            VarianceSM = 0x8, ExponentialSM = 0x10, ExponentialVarianceSM = 0x20
        };
        typedef int Technique; // TechniqueItems

        /** Technique can only be set and applied before createStages() */
        void setTechnique(Technique t) { _technique = t; }
        Technique getTechnique() const { return _technique; }

        /** Apply definition macros to shadow related shaders */
        void applyTechniqueDefines(osg::StateSet* ss) const;

        /** Set small-pixels-culling-feature of shadow cameras after createStages() */
        void setSmallPixelsToCull(int cameraNum, int smallPixels);

        /** Create simplified caster geometries to improve shadow pass effectiveness */
        void createCasterGeometries(osg::Node* scene, unsigned int casterMask, float boundRatio = 0.1f,
                                    const std::set<std::string>& whitelist = std::set<std::string>());
        
        std::vector<Pipeline::Stage*> createStages(int shadowSize, int shadowNum, osg::Shader* vs, osg::Shader* fs,
                                                   unsigned int casterMask);
        void setLightState(const osg::Vec3& pos, const osg::Vec3& dir, double maxDistance = -1.0,
                           bool retainLightPos = false);

        void addReferenceBound(const osg::BoundingBoxd& bb, bool toReset);
        void addReferenceBound(const osg::BoundingBoxf& bb, bool toReset);
        void addReferencePoints(const std::vector<osg::Vec3d>& pt, bool toReset);
        void clearReferencePoints() { _referencePoints.clear(); }

        int applyTextureAndUniforms(Pipeline::Stage* stage, const std::string& prefix, int startU);
        double getShadowMaxDistance() const { return _shadowMaxDistance; }
        int getShadowNumber() const { return _shadowNumber; }

        /** Set the receiver-side bias of the shadow lookup. Values are multiples of one
            shadow-map texel, so they stay meaningful when the cascade resolution or the cascade
            depth range changes. A bias given in normalized depth would instead scale with the
            cascade depth range and erase the small shadows of the far cascades:
            - constantBias: bias applied everywhere, enough to cover the depth quantization of
              the shadow map storage
            - slopeScale: extra bias per unit of depth slope, which is what fails first on
              grazing surfaces
            - normalOffsetScale: offset of the lookup position along the receiver normal
            See getShadowDepthBias() in shadowing.module.glsl for the exact formulation.

            All three default to 0, which keeps the plain hard comparison the shadow module has
            always used. This is deliberate: the bias removes as much shadow as it adds, since
            one texel is a world-space length and it is the current cascade range which decides
            how long that is. Measure the texel size first (a bias of 1 means 1 texel, so the
            shadow of anything thinner than it will disappear), and only enable the bias once
            the cascades actually cover the viewed area - limiting the shadow distance with
            setLightState() is usually what makes a texel small enough to be negligible */
        void setShadowBias(float constantBias, float slopeScale, float normalOffsetScale);

        /** Set the polygon offset applied while rendering shadow casters (default 1.1, 4.0).
            It is deliberately small: the receiver-side bias and normal offset handle the
            acne, so a large caster-side offset would only add peter-panning */
        void setCasterPolygonOffset(float factor, float units);

        /** Enable screen-space contact shadows: a short ray marched towards the light, tested
            against the depth buffer, which recovers the occlusion a shadow map can not resolve
            where an object touches a surface (one of its texels is a length in world space, so
            the shadow always starts a few texels away from the contact point). rayLength is in
            world units and has to be sized to the details the shadow map misses - a few times
            the "units/texel" reported at startup is a good starting point. It is 0 by default,
            which keeps the effect off: the useful length is a property of the scene scale and
            not of the shadow map, so it can not be derived here. See
            getContactShadowValue() in shadowing.module.glsl */
        void setContactShadow(float rayLength, float strength = 1.0f);

        osg::Texture2D* getTexture(int i) { return _shadowMaps[i].get(); }
        const osg::Texture2D* getTexture(int i) const { return _shadowMaps[i].get(); }

        osg::Uniform* getLightMatrices() { return _lightMatrices.get(); }
        const osg::Uniform* getLightMatrices() const { return _lightMatrices.get(); }

        /** Set the relative size of the blending band between two cascades (0.0 ~ 1.0).
            Cascades are selected by view distance in shaders, and this band is used to
            smoothly fade from one cascade to the next one */
        void setCascadeBlendRatio(float r) { _cascadeBlendRatio = r; }
        float getCascadeBlendRatio() const { return _cascadeBlendRatio; }

        /** Uniforms used by shaders to select cascades:
            - CascadeInfo: (number of active cascades, blending ratio)
            - CascadeFarDepths: far view distance of each cascade */
        osg::Uniform* getCascadeInfo() { return _cascadeInfo.get(); }
        osg::Uniform* getCascadeDepths() { return _cascadeDepths.get(); }

        osg::Geode* getFrustumGeode() { return _shadowFrustum.get(); }
        const osg::Geode* getFrustumGeode() const { return _shadowFrustum.get(); }

        void updateInDraw(osg::RenderInfo& renderInfo);
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

    protected:
        virtual ~ShadowModule();
        Pipeline::Stage* createShadowCaster(int id, osg::Program* prog, unsigned int casterMask);
        void updateFrustumGeometry(int id, osg::Camera* shadowCam);
        
        osg::observer_ptr<Pipeline> _pipeline;
        osg::observer_ptr<osg::Camera> _updatedCamera;
        osg::ref_ptr<osg::Geode> _shadowFrustum;
        osg::ref_ptr<osg::CullFace> _cullFace;
        osg::ref_ptr<osg::PolygonOffset> _polygonOffset;
        osg::ref_ptr<osg::Texture2D> _shadowMaps[MAX_SHADOWS];
        osg::ref_ptr<osg::Uniform> _lightMatrices;  // matrixf[]
        osg::ref_ptr<osg::Uniform> _invTextureSize;  // vec2
        osg::ref_ptr<osg::Uniform> _cascadeInfo;  // vec2: (num, blendRatio)
        osg::ref_ptr<osg::Uniform> _cascadeDepths;  // vec4: far distance of each cascade
        osg::ref_ptr<osg::Uniform> _biasParams;  // vec4: (constNDC, slope, normalOffset, -)
        osg::ref_ptr<osg::Uniform> _biasScales;  // vec4: depth delta of one texel, per cascade
        osg::ref_ptr<osg::Uniform> _texelSizes;  // vec4: world size of one texel, per cascade
        osg::ref_ptr<osg::Uniform> _mainLightDir;  // vec3: world-space dir to the main light
        osg::ref_ptr<osg::Uniform> _contactShadow;  // vec4: (length, thickness, bias, strength)
        std::vector<osg::observer_ptr<osg::Camera>> _shadowCameras;

        osg::Matrix _lightMatrix, _lightInputMatrix;
        osg::Vec3 _lightDirectionWorld;  // unit, world space, from a surface to the light
        std::vector<osg::Vec3d> _referencePoints;
        Technique _technique;
        double _shadowMaxDistance; int _shadowNumber;
        float _cascadeBlendRatio;
        float _biasConstant, _biasSlopeScale, _biasNormalOffset;
        bool _retainLightPos, _dirtyReference;
        bool _infoPrinted;
    };

    class ShadowDrawCallback : public CameraDrawCallback
    {
    public:
        ShadowDrawCallback(ShadowModule* m) : _module(m) {}
        virtual void operator()(osg::RenderInfo& renderInfo) const
        {
            if (_module.valid()) _module->updateInDraw(renderInfo);
            if (_subCallback.valid()) _subCallback.get()->run(renderInfo);
        }

    protected:
        osg::observer_ptr<ShadowModule> _module;
    };
}

#endif

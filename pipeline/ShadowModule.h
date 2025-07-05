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

        osg::Texture2D* getTexture(int i) { return _shadowMaps[i].get(); }
        const osg::Texture2D* getTexture(int i) const { return _shadowMaps[i].get(); }

        osg::Uniform* getLightMatrices() { return _lightMatrices.get(); }
        const osg::Uniform* getLightMatrices() const { return _lightMatrices.get(); }

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
        std::vector<osg::observer_ptr<osg::Camera>> _shadowCameras;

        osg::Matrix _lightMatrix, _lightInputMatrix;
        std::vector<osg::Vec3d> _referencePoints;
        Technique _technique;
        double _shadowMaxDistance; int _shadowNumber;
        bool _retainLightPos, _dirtyReference;
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

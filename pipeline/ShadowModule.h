#ifndef MANA_PP_SHADOW_MODULE_HPP
#define MANA_PP_SHADOW_MODULE_HPP

#include <osg/CullFace>
#include <osg/PolygonOffset>
#include <osg/Texture2DArray>
#include <osg/Geometry>
#include "Pipeline.h"

namespace osgVerse
{
    class ShadowModule : public osg::NodeCallback
    {
    public:
        struct ShadowData : public osg::Referenced
        { int index; osg::BoundingBoxd bound; };  // will be set to camera's user-data

        ShadowModule(const std::string& name, Pipeline* pipeline, bool withDebugGeom);
        void createStages(int shadowSize, int shadowNum, osg::Shader* vs, osg::Shader* fs,
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
        osg::ref_ptr<osg::Texture2D> _shadowMaps[4];
        osg::ref_ptr<osg::Uniform> _lightMatrices;  // matrixf[]
        std::vector<osg::observer_ptr<osg::Camera>> _shadowCameras;

        osg::Matrix _lightMatrix, _lightInputMatrix;
        std::vector<osg::Vec3d> _referencePoints;
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

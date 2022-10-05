#ifndef MANA_PP_SHADOW_MODULE_HPP
#define MANA_PP_SHADOW_MODULE_HPP

#include <osg/Texture2DArray>
#include <osg/Geometry>
#include "Pipeline.h"

namespace osgVerse
{
    class ShadowModule : public osg::NodeCallback
    {
    public:
        ShadowModule(const std::string& name, Pipeline* pipeline, bool withDebugGeom);
        void createStages(int shadowSize, int shadowNum, osg::Shader* vs, osg::Shader* fs,
                          unsigned int casterMask);

        void setLightState(const osg::Vec3& pos, const osg::Vec3& dir, float maxDistance);
        void addReferenceBound(const osg::BoundingBox& bb, bool toReset);
        void addReferencePoints(const std::vector<osg::Vec3>& pt, bool toReset);
        void clearReferencePoints() { _referencePoints.clear(); }

        osg::Texture2DArray* getTextureArray() { return _shadowMaps.get(); }
        const osg::Texture2DArray* getTextureArray() const { return _shadowMaps.get(); }
        osg::Uniform* getLightMatrices() { return _lightMatrices.get(); }
        const osg::Uniform* getLightMatrices() const { return _lightMatrices.get(); }

        osg::Geode* getFrustumGeode() { return _shadowFrustum.get(); }
        const osg::Geode* getFrustumGeode() const { return _shadowFrustum.get(); }

        void updateInDraw(osg::RenderInfo& renderInfo);
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

    protected:
        Pipeline::Stage* createShadowCaster(int id, osg::Program* prog, unsigned int casterMask);
        void updateFrustumGeometry(int id, osg::Camera* shadowCam);
        
        osg::observer_ptr<Pipeline> _pipeline;
        osg::observer_ptr<osg::Camera> _updatedCamera;
        osg::ref_ptr<osg::Geode> _shadowFrustum;
        osg::ref_ptr<osg::Texture2DArray> _shadowMaps;
        osg::ref_ptr<osg::Uniform> _lightMatrices;
        std::vector<osg::observer_ptr<osg::Camera>> _shadowCameras;

        osg::Matrix _lightMatrix, _lightInvMatrix;
        std::vector<osg::Vec3> _referencePoints;
        float _shadowMaxDistance;
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

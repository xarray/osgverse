#ifndef MANA_SHADOW_MODULE_HPP
#define MANA_SHADOW_MODULE_HPP

#include <osg/Texture2DArray>
#include <osg/Geometry>
#include "Pipeline.h"

namespace osgVerse
{
    class ShadowModule : public osg::NodeCallback
    {
    public:
        ShadowModule(Pipeline* pipeline, bool withDebugGeom);
        void createStages(int shadowSize, int shadowNum, osg::Shader* vs, osg::Shader* fs,
                          unsigned int casterMask);

        void setLightState(const osg::Vec3& pos, const osg::Vec3& dir, float maxDistance);
        void setMainCameras(osg::Camera* mvCam, osg::Camera* projCam);
        void addReferencePoints(const std::vector<osg::Vec3>& pt);
        void clearReferencePoints() { _referencePoints.clear(); }

        osg::Texture2DArray* getTextureArray() { return _shadowMaps.get(); }
        const osg::Texture2DArray* getTextureArray() const { return _shadowMaps.get(); }
        osg::Uniform* getLightMatrices() { return _lightMatrices.get(); }
        const osg::Uniform* getLightMatrices() const { return _lightMatrices.get(); }

        osg::Geode* getFrustumGeode() { return _shadowFrustum.get(); }
        const osg::Geode* getFrustumGeode() const { return _shadowFrustum.get(); }
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

    protected:
        Pipeline::Stage* createShadowCaster(int id, osg::Program* prog, unsigned int casterMask);
        void updateFrustumGeometry(int id, osg::Camera* shadowCam);
        
        osg::ref_ptr<Pipeline> _pipeline;
        osg::ref_ptr<osg::Geode> _shadowFrustum;
        osg::ref_ptr<osg::Texture2DArray> _shadowMaps;
        osg::ref_ptr<osg::Uniform> _lightMatrices;
        osg::observer_ptr<osg::Camera> _cameraMV, _cameraProj;
        std::vector<osg::observer_ptr<osg::Camera>> _shadowCameras;

        osg::Matrix _lightMatrix, _lightInvMatrix;
        std::vector<osg::Vec3> _referencePoints;
        float _shadowMaxDistance;
    };
}

#endif

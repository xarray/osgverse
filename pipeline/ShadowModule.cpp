#include <osg/io_utils>
#include <osgDB/ReadFile>
#include <iostream>
#include "ShadowModule.h"
#include "Utilities.h"

namespace osgVerse
{
    ShadowModule::ShadowModule(Pipeline* pipeline, bool withDebugGeom)
        : _pipeline(pipeline), _shadowMaxDistance(-1.0f)
    {
        _shadowMaps = new osg::Texture2DArray;
        _shadowFrustum = withDebugGeom ? new osg::Geode : NULL;
        _lightMatrices = new osg::Uniform(osg::Uniform::FLOAT_MAT4, "LightMatrices", 4);
    }

    void ShadowModule::setLightState(const osg::Vec3& pos, const osg::Vec3& dir0, float maxDistance)
    {
        osg::Vec3 up = osg::Z_AXIS, dir = dir0;
        dir.normalize();
        if (dir.length2() == 1.0f)
        {
            if (dir.z() == 1.0f || dir.z() == -1.0f)
                up = osg::Y_AXIS;
        }

        osg::Vec3 side = up ^ dir; up = dir ^ side;
        _lightMatrix = osg::Matrix::lookAt(pos, pos + dir * maxDistance, up);
        _lightInvMatrix = osg::Matrix::inverse(_lightMatrix);
        _shadowMaxDistance = maxDistance;
    }

    void ShadowModule::createStages(int shadowSize, int shadowNum, osg::Shader* vs, osg::Shader* fs,
                                    unsigned int casterMask)
    {
        shadowNum = osg::minimum(shadowNum, 4);
        _shadowMaps->setTextureSize(shadowSize, shadowSize, shadowNum);
        _shadowMaps->setInternalFormat(GL_RGB16F_ARB);
        _shadowMaps->setSourceFormat(GL_RGB);
        _shadowMaps->setSourceType(GL_FLOAT);
        _shadowMaps->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        _shadowMaps->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        _shadowMaps->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
        _shadowMaps->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_BORDER);
        _shadowMaps->setBorderColor(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));

        if (_pipeline.valid())
        {
            osg::ref_ptr<osg::Program> prog = new osg::Program;
            prog->setName("ShadowCaster_PROGRAM");
            if (vs) { vs->setName("ShadowCaster_SHADER_VS"); prog->addShader(vs); }
            if (fs) { fs->setName("ShadowCaster_SHADER_FS"); prog->addShader(fs); }
            for (int i = 0; i < shadowNum; ++i)
                _pipeline->addStage(createShadowCaster(i, prog.get(), casterMask));
        }
    }

    void ShadowModule::setMainCameras(osg::Camera* mvCam, osg::Camera* projCam)
    { _cameraMV = mvCam; _cameraProj = projCam; }

    void ShadowModule::addReferencePoints(const std::vector<osg::Vec3>& pt)
    { _referencePoints.insert(_referencePoints.end(), pt.begin(), pt.end()); }

    void ShadowModule::operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        if (!_cameraMV) _cameraMV = dynamic_cast<osg::Camera*>(node);
        if (!_cameraProj) _cameraProj = _pipeline->getForwardCamera();

        if (_cameraMV.valid() && _cameraProj.valid())
        {
            Frustum frustum;
            frustum.create(_cameraMV->getViewMatrix(), _cameraProj->getProjectionMatrix(),
                           -1.0f, _shadowMaxDistance);

            osg::BoundingBox shadowBB = frustum.createShadowBound(_referencePoints, _lightInvMatrix);
            float halfX = (shadowBB.xMax() - shadowBB.xMin()) * 0.5;
            float halfY = (shadowBB.yMax() - shadowBB.yMin()) * 0.5;
            //std::cout << halfX << ", " << halfY << std::endl;
            for (size_t i = 0; i < _shadowCameras.size(); ++i)
            {
                // TODO: split...
                osg::Camera* shadowCam = _shadowCameras[i].get();
                //shadowCam->setViewMatrixAsLookAt(
                //    osg::Vec3(-100.0f, 0.0f, 100.0f), osg::Vec3(), osg::Vec3(0.707f, 0.f, 0.707f));
                //shadowCam->setProjectionMatrixAsOrtho(
                //    -100.0f, 100.0f, -100.0f, 100.0f, 0.0f, 300.0f);
                shadowCam->setViewMatrix(_lightMatrix);
                shadowCam->setProjectionMatrixAsOrtho(
                    -halfX, halfX, -halfY, halfY, shadowBB.zMin(), shadowBB.zMax());
                _lightMatrices->setElement(i, osg::Matrixf(
                    shadowCam->getViewMatrix() * shadowCam->getProjectionMatrix()));
                updateFrustumGeometry(i, shadowCam);
            }
            _lightMatrices->dirty();
        }
        traverse(node, nv);
    }

    Pipeline::Stage* ShadowModule::createShadowCaster(int id, osg::Program* prog, unsigned int casterMask)
    {
        osg::ref_ptr<osg::Camera> camera = new osg::Camera;
        camera->setDrawBuffer(GL_FRONT);
        camera->setReadBuffer(GL_FRONT);
        camera->setAllowEventFocus(false);
        camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        camera->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
        camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
        camera->setRenderOrder(osg::Camera::PRE_RENDER);
        camera->setGraphicsContext(_pipeline->getContext());
        camera->setViewport(0, 0, _shadowMaps->getTextureWidth(), _shadowMaps->getTextureHeight());
        camera->attach(osg::Camera::COLOR_BUFFER0, _shadowMaps.get(), id);
        camera->getOrCreateStateSet()->setAttributeAndModes(
            prog, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        _shadowCameras.push_back(camera.get());

        Pipeline::Stage* stage = new Pipeline::Stage;
        stage->name = "ShadowCaster" + std::to_string(id);
        stage->camera = camera; stage->camera->setName(stage->name);
        stage->deferred = false; stage->inputStage = true;
        stage->camera->setCullMask(casterMask);
        stage->camera->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
        return stage;
    }

    void ShadowModule::updateFrustumGeometry(int id, osg::Camera* shadowCam)
    {
        osg::Geometry* geom = NULL;
        if (!_shadowFrustum) return;
        if (id < (int)_shadowFrustum->getNumDrawables())
            geom = _shadowFrustum->getDrawable(id)->asGeometry();
        else
        {
            osg::DrawElementsUByte* de = new osg::DrawElementsUByte(GL_LINES);
            de->push_back(0); de->push_back(1); de->push_back(1); de->push_back(5);
            de->push_back(5); de->push_back(4);
            de->push_back(1); de->push_back(2); de->push_back(2); de->push_back(6);
            de->push_back(6); de->push_back(5);
            de->push_back(2); de->push_back(3); de->push_back(3); de->push_back(7);
            de->push_back(7); de->push_back(6);
            de->push_back(3); de->push_back(0); de->push_back(0); de->push_back(4);
            de->push_back(4); de->push_back(7);
            osg::DrawElementsUByte* de2 = new osg::DrawElementsUByte(GL_QUADS);
            de2->push_back(4); de2->push_back(5); de2->push_back(6); de2->push_back(7);

            geom = new osg::Geometry;
            geom->setUseDisplayList(false);
            geom->setUseVertexBufferObjects(true);
            geom->setVertexArray(new osg::Vec3Array(8));
            geom->addPrimitiveSet(de); geom->addPrimitiveSet(de2);
            _shadowFrustum->addDrawable(geom);
        }
        
        Frustum frustum; osg::Vec3Array* va = static_cast<osg::Vec3Array*>(geom->getVertexArray());
        frustum.create(shadowCam->getViewMatrix(), shadowCam->getProjectionMatrix());
        for (int i = 0; i < 8; ++i) (*va)[i] = frustum.corners[i];
        va->dirty(); geom->dirtyBound();
    }
}

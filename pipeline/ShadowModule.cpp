#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <osgDB/ReadFile>
#include <iostream>
#include "ShadowModule.h"
#include "Utilities.h"

namespace osgVerse
{
    ShadowModule::ShadowModule(const std::string& name, Pipeline* pipeline, bool withDebugGeom)
    :   _pipeline(pipeline), _shadowMaxDistance(-1.0), _shadowNumber(0),
        _retainLightPos(false), _dirtyReference(false)
    {
        for (int i = 0; i < 4; ++i) _shadowMaps[i] = new osg::Texture2D;
        _cullFace = new osg::CullFace(osg::CullFace::FRONT);
        _polygonOffset = new osg::PolygonOffset(1.1f, 4.0f);

        _shadowFrustum = withDebugGeom ? new osg::Geode : NULL;
        _lightMatrices = new osg::Uniform(osg::Uniform::FLOAT_MAT4, "ShadowSpaceMatrices", 4);
        if (pipeline) pipeline->addModule(name, this);
    }

    ShadowModule::~ShadowModule()
    {
        if (_pipeline.valid()) _pipeline->removeModule(this);
    }

    void ShadowModule::setLightState(const osg::Vec3& pos, const osg::Vec3& dir0,
                                     double maxDistance, bool retainLightPos)
    {
        osg::Vec3 up = osg::Z_AXIS, dir = dir0; dir.normalize();
        float cosine = (dir * osg::Z_AXIS);
        if (cosine < -0.9f || cosine > 0.9f) up = osg::Y_AXIS;
        else if (dir.length2() < 0.01) return;  // invalid direction

        osg::Vec3 side = up ^ dir; up = dir ^ side;
        osg::Matrix m = osg::Matrix::lookAt(
            pos, pos + dir * (maxDistance > 0.0 ? maxDistance : 100.0), up);
        if (m.compare(_lightInputMatrix) != 0)
        {
            _lightInputMatrix = m; _lightMatrix = m; _dirtyReference = true;
            _shadowMaxDistance = maxDistance; _retainLightPos = retainLightPos;
        }
    }

    void ShadowModule::createStages(int shadowSize, int shadowNum, osg::Shader* vs, osg::Shader* fs,
                                    unsigned int casterMask)
    {
        _shadowCameras.clear();
        _shadowNumber = osg::minimum(shadowNum, 4);
        for (int i = 0; i < _shadowNumber; ++i)
        {
            _shadowMaps[i]->setTextureSize(shadowSize, shadowSize);
            _shadowMaps[i]->setInternalFormat(GL_RGB16F_ARB);
            _shadowMaps[i]->setSourceFormat(GL_RGB);
            _shadowMaps[i]->setSourceType(GL_FLOAT);
            _shadowMaps[i]->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
            _shadowMaps[i]->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
            _shadowMaps[i]->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
            _shadowMaps[i]->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_BORDER);
            _shadowMaps[i]->setBorderColor(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
        }

        if (_pipeline.valid())
        {
            osg::ref_ptr<osg::Program> prog = new osg::Program;
            prog->setName("ShadowCaster_PROGRAM");
            for (int i = 0; i < _shadowNumber; ++i)
                _pipeline->addStage(createShadowCaster(i, prog.get(), casterMask));

            int gl = _pipeline->getGlslTargetVersion(), glsl = _pipeline->getGlslTargetVersion();
            if (vs)
            {
                vs->setName("ShadowCaster_SHADER_VS"); prog->addShader(vs);
                Pipeline::createShaderDefinitions(vs, gl, glsl);
            }
            
            if (fs)
            {
                fs->setName("ShadowCaster_SHADER_FS"); prog->addShader(fs);
                Pipeline::createShaderDefinitions(fs, gl, glsl);
            }
        }
    }

    void ShadowModule::addReferencePoints(const std::vector<osg::Vec3d>& pt, bool toReset)
    {
        if (toReset) _referencePoints.clear(); _dirtyReference = true;
        _referencePoints.insert(_referencePoints.end(), pt.begin(), pt.end());
    }
    
    void ShadowModule::addReferenceBound(const osg::BoundingBoxd& bb, bool toReset)
    {
        if (toReset) _referencePoints.clear(); _dirtyReference = true;
        for (int i = 0; i < 8; ++i) _referencePoints.push_back(bb.corner(i));
    }

    void ShadowModule::addReferenceBound(const osg::BoundingBoxf& bb, bool toReset)
    {
        if (toReset) _referencePoints.clear(); _dirtyReference = true;
        for (int i = 0; i < 8; ++i) _referencePoints.push_back(bb.corner(i));
    }

    int ShadowModule::applyTextureAndUniforms(Pipeline::Stage* stage, const std::string& prefix, int startU)
    {
        int unit = startU;
        for (int i = 0; i < _shadowNumber; ++i)
        {
            std::string name = prefix + std::to_string(i);
            stage->applyTexture(_shadowMaps[i].get(), name, unit++);
        }
        stage->applyUniform(getLightMatrices());
        return unit;
    }

    void ShadowModule::updateInDraw(osg::RenderInfo& renderInfo)
    {
        osg::Camera* cam = _updatedCamera.get();
        osg::State* state = renderInfo.getState();
        if (!cam || !state) return;

        osg::Matrix viewInv = cam->getInverseViewMatrix(), proj = state->getProjectionMatrix();
        double fov, ratio, zn, zf; proj.getPerspective(fov, ratio, zn, zf);
        if (_shadowMaxDistance > 0.0 && (zn + _shadowMaxDistance) < zf) zf = zn + _shadowMaxDistance;
        
        double shadowDistance = zf - zn;
        if (shadowDistance <= 0.01) return;  // state not prepared? we have to quit then
        if (_dirtyReference && !_retainLightPos)
        {
            // Recalculate light-space matrix
            osg::Vec3d eye, center, up, dir; osg::BoundingSphered bs;
            _lightInputMatrix.getLookAt(eye, center, up, shadowDistance);
            dir = center - eye; dir.normalize();

            for (size_t i = 0; i < _referencePoints.size(); ++i) bs.expandBy(_referencePoints[i]);
            center = bs.center(); eye = center - dir * shadowDistance;
            _lightMatrix = osg::Matrix::lookAt(eye, center, up); _dirtyReference = false;
        }

        // Split the main frustum
        static const float ratios[] = { 0.0f, 0.15f, 0.35f, 0.55f, 1.0f };
        size_t numCameras = _shadowCameras.size();
        double zStep = shadowDistance / ratios[numCameras], zMaxTotal = 0.0f;
        std::vector<osg::BoundingBoxd> shadowBBs(numCameras);
        for (size_t i = 0; i < numCameras; ++i)
        {
            double zMin = zn + zStep * ratios[i], zMax = zn + zStep * ratios[i + 1];
            Frustum frustum; frustum.create(cam->getViewMatrix(), proj, zMin, zMax);

            // Get light-space bounding box of the splitted frustum
            osg::BoundingBoxd shadowBB = frustum.createShadowBound(_referencePoints, _lightMatrix);
            double zNew = osg::maximum(osg::absolute(shadowBB.zMin()), osg::absolute(shadowBB.zMax()));
            shadowBBs[i] = shadowBB; if (zMaxTotal < zNew) zMaxTotal = zNew;
        }

        for (size_t i = 0; i < numCameras; ++i)
        {
            const osg::BoundingBoxd& shadowBB = shadowBBs[i];
            const osg::Vec3 center = shadowBB.center();
            double radius = osg::maximum(shadowBB.xMax() - shadowBB.xMin(),
                shadowBB.yMax() - shadowBB.yMin()) * 0.5;
            double xMin = center[0] - radius, xMax = center[0] + radius;
            double yMin = center[1] - radius, yMax = center[1] + radius;
            //xMin = shadowBB.xMin(), xMax = shadowBB.xMax();
            //yMin = shadowBB.yMin(), yMax = shadowBB.yMax();
            //std::cout << i << ": X = (" << xMin << ", " << xMax << "), Y = ("
            //          << yMin << ", " << yMax << "); Z = " << zMaxTotal << "\n";

            // Apply the shadow camera & uniform
            osg::Camera* shadowCam = _shadowCameras[i].get();
            shadowCam->setViewMatrix(_lightMatrix);
            shadowCam->setProjectionMatrixAsOrtho(xMin, xMax, yMin, yMax, 0.0, zMaxTotal);
            _lightMatrices->setElement(i, osg::Matrixf(viewInv *
                shadowCam->getViewMatrix() * shadowCam->getProjectionMatrix()));

            ShadowData* sData = static_cast<ShadowData*>(shadowCam->getUserData());
            if (sData != NULL) sData->bound = shadowBB;
        }
        _lightMatrices->dirty();
    }

    void ShadowModule::operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        if (node->asGroup())
        {
            osg::Group* group = node->asGroup();
            for (size_t i = 0; i < group->getNumChildren(); ++i)
            {
                osg::ComputeBoundsVisitor cbv; group->getChild(i)->accept(cbv);
                addReferenceBound(cbv.getBoundingBox(), i == 0);
            }
        }

        osg::Camera* cameraMV = static_cast<osg::Camera*>(node);
        if (cameraMV) _updatedCamera = cameraMV;
        for (size_t i = 0; i < _shadowCameras.size(); ++i)
        {
            osg::Camera* shadowCam = _shadowCameras[i].get();
            updateFrustumGeometry(i, shadowCam);
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
        camera->setClearColor(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
        camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
        camera->setRenderOrder(osg::Camera::PRE_RENDER);

        osg::ref_ptr<ShadowData> sData = new ShadowData; sData->index = id;
        camera->setUserData(sData.get());

        if (_pipeline.valid()) camera->setGraphicsContext(_pipeline->getContext());
        camera->setViewport(0, 0, _shadowMaps[id]->getTextureWidth(), _shadowMaps[id]->getTextureHeight());
        camera->attach(osg::Camera::COLOR_BUFFER0, _shadowMaps[id].get());

        int value = osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE;
        camera->getOrCreateStateSet()->setAttributeAndModes(prog, value);
        camera->getOrCreateStateSet()->setAttributeAndModes(_cullFace.get(), value);
        camera->getOrCreateStateSet()->setAttributeAndModes(_polygonOffset.get(), value);
        camera->getOrCreateStateSet()->setMode(GL_POLYGON_OFFSET_FILL, value);
        _shadowCameras.push_back(camera.get());

        Pipeline::Stage* stage = new Pipeline::Stage;
        stage->name = "ShadowCaster" + std::to_string(id);
        stage->camera = camera; stage->camera->setName(stage->name);
        stage->deferred = false; stage->inputStage = true;
        stage->camera->setUserValue("PipelineMask", casterMask);  // replacing setCullMask()
        stage->camera->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
        return stage;
    }

    void ShadowModule::updateFrustumGeometry(int id, osg::Camera* shadowCam)
    {
        osg::Geometry* geom = NULL;
        if (!_shadowFrustum) return;
        else _shadowFrustum->setCullingActive(false);

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
            osg::Vec4Array* ca = new osg::Vec4Array; ca->push_back(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));

            geom = new osg::Geometry;
            geom->setUseDisplayList(false);
            geom->setUseVertexBufferObjects(true);
            geom->setVertexArray(new osg::Vec3Array(8));
            geom->setColorArray(ca); geom->setColorBinding(osg::Geometry::BIND_OVERALL);
            geom->addPrimitiveSet(de); geom->addPrimitiveSet(de2);
            geom->setComputeBoundingBoxCallback(new DisableBoundingBoxCallback);
            geom->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
            _shadowFrustum->addDrawable(geom);
        }
        
        Frustum frustum; osg::Vec3Array* va = static_cast<osg::Vec3Array*>(geom->getVertexArray());
        frustum.create(shadowCam->getViewMatrix(), shadowCam->getProjectionMatrix());
        for (int i = 0; i < 8; ++i) (*va)[i] = frustum.corners[i];
        va->dirty(); geom->dirtyBound();
    }
}

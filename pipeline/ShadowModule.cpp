#include <osg/io_utils>
#include <osg/Version>
#include <osg/ComputeBoundsVisitor>
#include <osgDB/ReadFile>
#include <osgUtil/SmoothingVisitor>
#include <iostream>
#include "../modeling/Utilities.h"
#include "Utilities.h"
#include "ShaderLibrary.h"
#include "ShadowModule.h"

#ifndef GL_DEPTH_CLAMP
#define GL_DEPTH_CLAMP 0x864F
#endif

class CreateVHACDVisitor : public osg::NodeVisitor
{
public:
    CreateVHACDVisitor(const osg::BoundingBox& bb, const std::set<std::string>& whitelist, float bRatio)
    :   osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), _sceneBoundingBox(bb),
        _whitelist(whitelist), _numMinTriangleVertices(50)
    {
        osg::Vec3 extent = bb._max - bb._min;
        _sceneBoundThreshold = extent[0] * extent[1] * extent[2] * bRatio;
    }

    virtual void apply(osg::Geode& node)
    {
#if OSG_VERSION_LESS_OR_EQUAL(3, 4, 1)
        for (unsigned int i = 0; i < node.getNumDrawables(); ++i)
        {
            osg::Geometry* geom = node.getDrawable(i)->asGeometry();
            if (geom != NULL) apply(*geom);
        }
#endif
        traverse(node);
    }

    virtual void apply(osg::Geometry& geom)
    {
        bool found = false; const std::string& name = geom.getName();
        for (std::set<std::string>::iterator itr = _whitelist.begin(); itr != _whitelist.end(); ++itr)
        { if (name.find(*itr) != std::string::npos) { found = true; break; } }

        osgVerse::BoundingVolumeVisitor bvv; bvv.apply(geom);
        found |= _whitelist.empty();
        if (_sceneBoundThreshold > 0.0f)
        {
#if OSG_VERSION_GREATER_THAN(3, 2, 3)
            const osg::BoundingBox& bb = geom.getBoundingBox();
#else
            const osg::BoundingBox& bb = geom.getBound();
#endif
            osg::Vec3 extent = bb._max - bb._min; float volume = extent[0] * extent[1] * extent[2];
            if (volume > _sceneBoundThreshold) found = false;
        }

        unsigned int numTriangleIndices = bvv.getTriangles().size();
        if (found && numTriangleIndices > _numMinTriangleVertices)
        {
            osg::ref_ptr<osg::Geometry> newGeom = bvv.computeVHACD(false, true, 5000, 100);
            //osg::ref_ptr<osg::Geometry> newGeom = bvv.computeCoACD(0.1f);
            if (newGeom.valid() && newGeom->getNumPrimitiveSets() > 0)
            {
                unsigned int numNewTriangleIndices =
                    static_cast<osg::DrawElementsUShort*>(newGeom->getPrimitiveSet(0))->size();
                if (numNewTriangleIndices < numTriangleIndices) _vhacdMap[&geom] = newGeom;
            }
        }
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
        traverse(geom);
#endif
    }

    void updateGeometries(unsigned int rcvMask, unsigned int castMask)
    {
        for (std::map<osg::Geometry*, osg::ref_ptr<osg::Geometry>>::iterator itr = _vhacdMap.begin();
             itr != _vhacdMap.end(); ++itr)
        {
            osg::Geometry *geom = itr->first, *geom2 = itr->second.get();
            if (geom->getNumParents() == 0) continue; geom2->setName(geom->getName());
#if false
            for (unsigned int i = 0; i < geom->getNumParents(); ++i)
            {
                osg::Geode* geode = static_cast<osg::Geode*>(geom->getParent(i));
                osgUtil::SmoothingVisitor::smooth(*geom2);
                geode->replaceDrawable(geom, geom2);
            }
#else
            unsigned int mask = osgVerse::Pipeline::getPipelineMask(*geom);
            osgVerse::Pipeline::setPipelineMask(*geom, mask & rcvMask);
            osgVerse::Pipeline::setPipelineMask(*geom2, castMask);
            for (unsigned int i = 0; i < geom->getNumParents(); ++i)
            {
                osg::Geode* geode = static_cast<osg::Geode*>(geom->getParent(i));
                geode->addDrawable(geom2);
            }
#endif
        }
    }

protected:
    std::map<osg::Geometry*, osg::ref_ptr<osg::Geometry>> _vhacdMap;
    std::set<std::string> _whitelist;
    osg::BoundingBox _sceneBoundingBox;
    unsigned int _numMinTriangleVertices;
    float _sceneBoundThreshold;
};

namespace osgVerse
{
    ShadowModule::ShadowModule(const std::string& name, Pipeline* pipeline, bool withDebugGeom)
    :   _pipeline(pipeline), _technique(DefaultSM), _shadowMaxDistance(-1.0), _shadowNumber(0),
        _retainLightPos(false), _dirtyReference(false)
    {
        for (int i = 0; i < MAX_SHADOWS; ++i) _shadowMaps[i] = new osg::Texture2D;
        _cullFace = new osg::CullFace(osg::CullFace::FRONT);
        _polygonOffset = new osg::PolygonOffset(1.1f, 4.0f);

        _shadowFrustum = withDebugGeom ? new osg::Geode : NULL;
        _lightMatrices = new osg::Uniform(
            osg::Uniform::FLOAT_MAT4, "ShadowSpaceMatrices", MAX_SHADOWS);
        if (pipeline) pipeline->addModule(name, this);
    }

    ShadowModule::~ShadowModule()
    {
        if (_pipeline.valid()) _pipeline->removeModule(this);
    }

    void ShadowModule::applyTechniqueDefines(osg::StateSet* ss) const
    {
        switch (_technique)
        {
        case EyeSpaceDepthSM: ss->setDefine("VERSE_SHADOW_EYESPACE"); break;
        default: break;
        }
    }

    void ShadowModule::setSmallPixelsToCull(int cameraNum, int smallPixels)
    {
        if (cameraNum < _shadowCameras.size())
        {
            ShadowData* sData = static_cast<ShadowData*>(_shadowCameras[cameraNum]->getUserData());
            if (sData != NULL) sData->smallPixels = smallPixels;
        }
        else
            OSG_NOTICE << "[ShadowModule] No camera found for setSmallPixelsToCull()" << std::endl;
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

    void ShadowModule::createCasterGeometries(osg::Node* scene, unsigned int casterMask, float boundRatio,
                                              const std::set<std::string>& whitelist)
    {
        osg::ComputeBoundsVisitor cbv; scene->accept(cbv);
        CreateVHACDVisitor cvv(cbv.getBoundingBox(), whitelist, boundRatio);
        scene->accept(cvv); cvv.updateGeometries(~casterMask, casterMask);
    }

    std::vector<Pipeline::Stage*> ShadowModule::createStages(int shadowSize, int shadowNum,
                                                             osg::Shader* vs, osg::Shader* fs, unsigned int casterMask)
    {
        _shadowCameras.clear();
        _shadowNumber = osg::minimum(shadowNum, MAX_SHADOWS);
        for (int i = 0; i < _shadowNumber; ++i)
        {
            _shadowMaps[i]->setTextureSize(shadowSize, shadowSize);
#if defined(VERSE_EMBEDDED_GLES2)
            // As WebGL requires, shadow map value should be encoded from float to RGBA8
            // https://registry.khronos.org/webgl/specs/latest/1.0/#6.6
            _shadowMaps[i]->setInternalFormat(GL_RGBA);
            _shadowMaps[i]->setSourceFormat(GL_RGBA);
            _shadowMaps[i]->setSourceType(GL_UNSIGNED_BYTE);
#else
            _shadowMaps[i]->setInternalFormat(GL_RGBA16F_ARB);
            _shadowMaps[i]->setSourceFormat(GL_RGBA);
            _shadowMaps[i]->setSourceType(GL_HALF_FLOAT);
#endif
            _shadowMaps[i]->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
            _shadowMaps[i]->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
            _shadowMaps[i]->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
            _shadowMaps[i]->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_BORDER);
            _shadowMaps[i]->setBorderColor(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
        }

        std::vector<Pipeline::Stage*> stages;
        osg::ref_ptr<ScriptableProgram> prog = new ScriptableProgram;
        prog->setName("ShadowCaster_PROGRAM");

        int glVer = 100, glslVer = 130;  // FIXME
        if (_pipeline.valid())
        {
            glVer = _pipeline->getContextTargetVersion();
            glslVer = _pipeline->getGlslTargetVersion();
        }

        for (int i = 0; i < _shadowNumber; ++i)
        {
            Pipeline::Stage* stage = createShadowCaster(i, prog.get(), casterMask);
            applyTechniqueDefines(stage->getOrCreateStateSet());
            stages.push_back(stage); if (_pipeline.valid()) _pipeline->addStage(stage);
        }

        if (vs)
        {
            vs->setName("ShadowCaster_SHADER_VS"); prog->addShader(vs);
            Pipeline::createShaderDefinitions(vs, glVer, glslVer);
        }

        if (fs)
        {
            fs->setName("ShadowCaster_SHADER_FS"); prog->addShader(fs);
            Pipeline::createShaderDefinitions(fs, glVer, glslVer);
        }
        return stages;
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

        osg::Matrix viewMat = cam->getViewMatrix(), proj = state->getProjectionMatrix(),
                    viewInv = cam->getInverseViewMatrix();
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
        double step = 0.0, zMaxTotal = 0.0;
        size_t numCameras = _shadowCameras.size();
        std::vector<osg::BoundingBoxd> shadowBBs(numCameras);
#if false
        static const float ratios[] = { 0.0f, 0.15f, 0.35f, 0.55f, 1.0f };
        step = shadowDistance / ratios[numCameras];
        for (size_t i = 0; i < numCameras; ++i)
        {
            double zMin = zn + step * ratios[i], zMax = zn + step * ratios[i + 1];
            Frustum frustum; frustum.create(viewMat, proj, zMin, zMax);

            // Get light-space bounding box of the splitted frustum
            Frustum::AABB aabb = frustum.createShadowBound(_referencePoints, _lightMatrix);
            osg::BoundingBoxd shadowBB(aabb.first, aabb.second);
            double zNew = osg::maximum(osg::absolute(shadowBB.zMin()), osg::absolute(shadowBB.zMax()));
            shadowBBs[i] = shadowBB; if (zMaxTotal < zNew) zMaxTotal = zNew;
        }
#else
        // Get light-space bounding box of the entire frustum
        Frustum frustum; frustum.create(viewMat, proj, zn, zf);
        Frustum::AABB aabb = frustum.createShadowBound(_referencePoints, _lightMatrix);
        osg::BoundingBoxd entireShadowBB(aabb.first, aabb.second);
        zMaxTotal = osg::maximum(osg::absolute(entireShadowBB.zMin()),
                                 osg::absolute(entireShadowBB.zMax()));

        static double splitSchemeBias = 0.66;
        double far0 = entireShadowBB.xMax(), near0 = entireShadowBB.xMin();
        for (size_t i = 0; i < numCameras; ++i)
        {
            // exponential: Ci = (f - n) * (i / numsplits) ^ (bias + 1) + n;
            double IDM0 = (double)(i) / numCameras, IDM1 = (double)(i + 1) / numCameras;
            shadowBBs[i] = entireShadowBB;
            shadowBBs[i]._min.x() = (far0 - near0) * (pow(IDM0, splitSchemeBias + 1.0)) + near0;
            shadowBBs[i]._max.x() = (far0 - near0) * (pow(IDM1, splitSchemeBias + 1.0)) + near0;
        }
#endif

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
            if (_technique == EyeSpaceDepthSM)
            {
                osg::Matrix proj = shadowCam->getProjectionMatrix(), projKeepZ;
                //projKeepZ(2, 2) = proj(0, 0); projKeepZ(1, 1) = proj(1, 1);  // consider perspective
                projKeepZ = proj; projKeepZ(2, 2) = 1.0f; projKeepZ(3, 2) = 0.0f;  // consider ortho
                _lightMatrices->setElement(i, osg::Matrixf(viewInv * shadowCam->getViewMatrix() * projKeepZ));
            }
            else
            {
                _lightMatrices->setElement(i, osg::Matrixf(viewInv *
                    shadowCam->getViewMatrix() * shadowCam->getProjectionMatrix()));
            }

            ShadowData* sData = static_cast<ShadowData*>(shadowCam->getUserData());
            if (sData != NULL)
            {
                sData->viewMatrix = viewMat; sData->projMatrix = proj;
                sData->_viewport = cam->getViewport(); sData->bound = shadowBB;
            }
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
        camera->setDrawBuffer(GL_FRONT); camera->setReadBuffer(GL_FRONT);
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
#if defined(VERSE_EMBEDDED_GLES2)
        // FBO without depth attachment will not enable depth test
        // By default OSG use "ImplicitBufferAttachmentMask" to handle this (attached DEPTH_COMPONENT24 in RenderStage.cpp),
        // but the internal format should be reset directly for WebGL1 cases
        // https://developer.mozilla.org/en-US/docs/Web/API/WebGLRenderingContext/renderbufferStorage
        camera->attach(osg::Camera::DEPTH_BUFFER, GL_DEPTH_COMPONENT16);
        camera->setImplicitBufferAttachmentMask(0, 0);
#endif

        int value = osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE;
        camera->getOrCreateStateSet()->setAttributeAndModes(prog, value);
        camera->getOrCreateStateSet()->setAttributeAndModes(_cullFace.get(), value);
        camera->getOrCreateStateSet()->setAttribute(_polygonOffset.get(), value);
        camera->getOrCreateStateSet()->setMode(GL_POLYGON_OFFSET_FILL, value);
#if !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE)
        camera->getOrCreateStateSet()->setMode(GL_DEPTH_CLAMP, value);
#endif
        _shadowCameras.push_back(camera.get());

        Pipeline::Stage* stage = new Pipeline::Stage;
        stage->deferred = false; stage->inputStage = true;
        stage->name = "ShadowCaster" + std::to_string(id);
        stage->camera = camera; stage->camera->setName(stage->name);
        stage->camera->setUserValue("PipelineCullMask", casterMask);  // replacing setCullMask()
        stage->camera->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
        stage->outputs["ShadowOutput"] = _shadowMaps[id].get();
        stage->overridedPrograms = true;  // all child shaders must be disabled
        stage->parentModule = this; return stage;
    }

    void ShadowModule::updateFrustumGeometry(int id, osg::Camera* shadowCam)
    {
        osg::Geometry* geom = NULL;
        if (!_shadowFrustum) return;
        else _shadowFrustum->setCullingActive(false);

        static osg::Vec4 dbgColor[MAX_SHADOWS] = {
            osg::Vec4(1.0f, 0.0f, 0.0f, 0.6f), osg::Vec4(1.0f, 1.0f, 0.0f, 0.6f),
            osg::Vec4(0.0f, 0.0f, 1.0f, 0.6f), osg::Vec4(0.0f, 1.0f, 1.0f, 0.6f)
        };

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

            osg::DrawElementsUByte* de2 = new osg::DrawElementsUByte(GL_LINES);
            de2->push_back(4); de2->push_back(5); de2->push_back(5); de2->push_back(6);
            de2->push_back(6); de2->push_back(7); de2->push_back(7); de2->push_back(4);
            de2->push_back(4); de2->push_back(6); de2->push_back(5); de2->push_back(7);
            osg::Vec4Array* ca = new osg::Vec4Array; ca->push_back(dbgColor[id]);

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

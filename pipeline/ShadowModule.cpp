#include <osg/io_utils>
#include <osg/Version>
#include <osg/ComputeBoundsVisitor>
#include <osgDB/ReadFile>
#include <osgUtil/SmoothingVisitor>
#include <iostream>
#include <sstream>
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
    :   _pipeline(pipeline), _technique(PossionPCF), _shadowMaxDistance(-1.0), _shadowNumber(0),
        _cascadeBlendRatio(0.1f), _biasConstant(0.0f), _biasSlopeScale(0.0f),
        _biasNormalOffset(0.0f), _retainLightPos(false), _dirtyReference(false), _infoPrinted(false)
    {
        for (int i = 0; i < MAX_SHADOWS; ++i) _shadowMaps[i] = new osg::Texture2D;
        _cullFace = new osg::CullFace(osg::CullFace::FRONT);
        // Note: polygon offset units are multiples of the smallest resolvable depth value, so
        // with the 24-bit depth attachment of the shadow camera the slope factor is the part
        // which really shifts the caster depth. The values are the original ones; use
        // setCasterPolygonOffset() to tune them if the receiver-side bias below is enabled
        _polygonOffset = new osg::PolygonOffset(1.1f, 4.0f);

        _shadowFrustum = withDebugGeom ? new osg::Geode : NULL;
        _lightMatrices = new osg::Uniform(
            osg::Uniform::FLOAT_MAT4, "ShadowSpaceMatrices", MAX_SHADOWS);
        _cascadeInfo = new osg::Uniform("CascadeInfo", osg::Vec2(0.0f, _cascadeBlendRatio));
        _cascadeDepths = new osg::Uniform("CascadeFarDepths", osg::Vec4());
        _biasParams = new osg::Uniform("ShadowBiasParams", osg::Vec4(
            _biasConstant, _biasSlopeScale, _biasNormalOffset, 0.0f));
        _biasScales = new osg::Uniform("ShadowBiasScales", osg::Vec4());
        _texelSizes = new osg::Uniform("ShadowTexelSizes", osg::Vec4());
        _mainLightDir = new osg::Uniform("MainLightDirection", osg::Vec3(0.0f, 0.0f, 1.0f));
        _contactShadow = new osg::Uniform("ContactShadowParams", osg::Vec4());
        _lightDirectionWorld.set(0.0f, 0.0f, 1.0f);
        if (pipeline) pipeline->addModule(name, this);
    }

    ShadowModule::~ShadowModule()
    {
        if (_pipeline.valid()) _pipeline->removeModule(this);
    }

    void ShadowModule::applyTechniqueDefines(osg::StateSet* ss) const
    {
#if OSG_VERSION_GREATER_THAN(3, 3, 6)
        if (_technique & EyeSpaceDepthSM)
        {
            ss->setDefine("VERSE_SHADOW_EYESPACE");
            if ((_technique & VarianceSM) > 0 || (_technique & ExponentialSM) > 0 ||
                (_technique & ExponentialVarianceSM) > 0)
            {
                OSG_NOTICE << "[ShadowModule] Current shadow technique " << std::hex << _technique
                           << std::dec << " is unsupported with eye-space depth" << std::endl; return;
            }
        }

        if (_technique & BandPCF) ss->setDefine("VERSE_SHADOW_BAND_PCF");
        if (_technique & PossionPCF) ss->setDefine("VERSE_SHADOW_POSSION_PCF");
#   if defined(VERSE_EMBEDDED_GLES2)
        if ((_technique & VarianceSM) > 0 || (_technique & ExponentialSM) > 0 ||
            (_technique & ExponentialVarianceSM) > 0)
        {
            OSG_NOTICE << "[ShadowModule] Current shadow technique " << std::hex << _technique
                       << std::dec << " is unsupported in GLES2/WebGL1" << std::endl;
        }
#   else
        if (_technique & VarianceSM) ss->setDefine("VERSE_SHADOW_VSM");
        if (_technique & ExponentialSM) ss->setDefine("VERSE_SHADOW_ESM");
        if (_technique & ExponentialVarianceSM) ss->setDefine("VERSE_SHADOW_EVSM");
#   endif
#else
        OSG_NOTICE << "[ShadowModule] applyTechniqueDefines() unsupported before OSG 3.3.7" << std::endl;
#endif
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

    void ShadowModule::setShadowBias(float constantBias, float slopeScale, float normalOffsetScale)
    {
        _biasConstant = constantBias; _biasSlopeScale = slopeScale;
        _biasNormalOffset = normalOffsetScale;
        if (_biasParams.valid())
            _biasParams->set(osg::Vec4(_biasConstant, _biasSlopeScale, _biasNormalOffset, 0.0f));
    }

    void ShadowModule::setCasterPolygonOffset(float factor, float units)
    {
        if (!_polygonOffset.valid()) return;
        _polygonOffset->setFactor(factor); _polygonOffset->setUnits(units);
    }

    void ShadowModule::setContactShadow(float rayLength, float strength)
    {
        // The thickness and the bias are derived from the ray length so that one value is enough
        // to size the effect: the thickness only has to accept the occluders the ray is expected
        // to find (a quarter of the ray), while the bias has to be small enough not to let the
        // ray start above the details it looks for
        if (!_contactShadow.valid()) return;
        _contactShadow->set(osg::Vec4(
            rayLength, rayLength * 0.25f, rayLength * 0.05f, strength));
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
            _retainLightPos = retainLightPos;
        }

        // The shadow range is kept outside of the check above: it describes the wanted range and
        // not the light transform, so a value set after the first frame still has to take effect
        // even when the light itself never moves
        _shadowMaxDistance = maxDistance;

        // The receiver-side bias needs the direction *towards* the light (the input one is the
        // travel direction of the light), in world space. It is kept as it is and converted to
        // eye space inside the shader, using the same G-Buffer matrix convention as the rest of
        // the pipeline, so no CPU-side matrix or quaternion conversion can get it wrong
        _lightDirectionWorld = -dir;
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
        _shadowCameras.clear(); _shadowNumber = osg::minimum(shadowNum, MAX_SHADOWS);
        _invTextureSize = new osg::Uniform("InvShadowMapSize", osg::Vec2(1.0f / shadowSize, 1.0f / shadowSize));
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

        int cxtVer = 0, glslVer = 0; guessOpenGLVersions(cxtVer, glslVer);
        if (_pipeline.valid())
        {
            cxtVer = _pipeline->getContextTargetVersion();
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
            Pipeline::createShaderDefinitions(vs, cxtVer, glslVer);
        }

        if (fs)
        {
            fs->setName("ShadowCaster_SHADER_FS"); prog->addShader(fs);
            Pipeline::createShaderDefinitions(fs, cxtVer, glslVer);
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
        stage->applyUniform(_invTextureSize.get());
        stage->applyUniform(_cascadeInfo.get());
        stage->applyUniform(_cascadeDepths.get());
        stage->applyUniform(_biasParams.get());
        stage->applyUniform(_biasScales.get());
        stage->applyUniform(_texelSizes.get());
        stage->applyUniform(_mainLightDir.get());
        stage->applyUniform(_contactShadow.get());
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

        // The camera near/far are usually much wider than the scene, and every unit of empty
        // space inside the cascade range is shadow resolution thrown away: with a camera near
        // plane far in front of the scene, the first cascades cover nothing at all, while the
        // scene may even stick out beyond the far plane, where no cascade is selected and no
        // shadow can be found. The reference points bound the scene already, and their
        // view-space depth is the range the cascades should really partition. It is only applied
        // when it leaves a valid range, so a scene whose bounds do not overlap the camera range
        // (a terrain seen from very far, for instance) simply keeps the camera range
        if (_shadowMaxDistance <= 0.0 && !_referencePoints.empty())
        {
            double sceneNear = 1e30, sceneFar = 0.0;
            for (size_t i = 0; i < _referencePoints.size(); ++i)
            {
                double viewDepth = -(_referencePoints[i] * viewMat).z();
                sceneNear = osg::minimum(sceneNear, viewDepth);
                sceneFar = osg::maximum(sceneFar, viewDepth);
            }

            double nearScene = osg::maximum(zn, sceneNear), farScene = osg::minimum(zf, sceneFar);
            if (farScene > (nearScene + 0.01)) { zn = nearScene; zf = farScene; }
        }

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

        // World-space direction to the main light, used by the receiver-side slope bias. The
        // shader converts it to eye space itself (see getEyeSpaceLightDirection())
        _mainLightDir->set(_lightDirectionWorld);

        // Split the main frustum
        size_t numCameras = _shadowCameras.size();
        std::vector<osg::BoundingBoxd> shadowBBs(numCameras);

        // The ortho near plane only needs to start where the nearest possible shadow caster is,
        // instead of at the light position (0.0). Shadow maps are stored in half-float
        // textures, so a tighter depth range directly means better depth precision and less
        // shadow acne. Reference points bound the whole scene, so no caster gets clipped here
        double zNearTotal = 0.0; osg::BoundingBoxd refBB;
        for (size_t i = 0; i < _referencePoints.size(); ++i)
            refBB.expandBy(_referencePoints[i] * _lightMatrix);
        if (!_referencePoints.empty()) zNearTotal = osg::maximum(0.0, -refBB.zMax());

        // CSM split: practical split scheme, mixing logarithmic and uniform partitioning:
        // a pure logarithmic split gives too little resolution to distant cascades
        const double splitLambda = 0.5;
        std::vector<double> splitDepths(numCameras + 1); splitDepths[0] = zn;
        for (size_t i = 1; i <= numCameras; ++i)
        {
            double fi = (double)i / numCameras;
            double logValue = zn * pow(zf / zn, fi), uniValue = zn + (zf - zn) * fi;
            splitDepths[i] = logValue * splitLambda + uniValue * (1.0 - splitLambda);
        }
        splitDepths[numCameras] = zf;

        // Tell shaders the view distance of each cascade, so they can select one cascade
        // per pixel instead of multiplying the results of all overlapping cascades
        double cascadeDepths[4] = { 0.0, 0.0, 0.0, 0.0 };
        for (size_t i = 0; i < numCameras && i < 4; ++i) cascadeDepths[i] = splitDepths[i + 1];
        _cascadeDepths->set(osg::Vec4((float)cascadeDepths[0], (float)cascadeDepths[1],
                                      (float)cascadeDepths[2], (float)cascadeDepths[3]));
        _cascadeInfo->set(osg::Vec2((float)numCameras, _cascadeBlendRatio));

        // Compute light-space bounding box for each split frustum
        for (size_t i = 0; i < numCameras; ++i)
        {
            Frustum frustum; frustum.create(viewMat, proj, splitDepths[i], splitDepths[i + 1]);
            Frustum::AABB aabb = frustum.createShadowBound(_referencePoints, _lightMatrix);
            shadowBBs[i] = osg::BoundingBoxd(aabb.first, aabb.second);
        }

        // Receiver-side bias data of each cascade, gathered here and uploaded once after the
        // loop: the bias has to live in the same depth space as the values compared by
        // CompareDepth(), and that space is per cascade (each one has its own far plane) and
        // per technique (eye-space storage keeps a world-space distance instead of NDC depth)
        float biasScales[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        float texelSizes[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        for (size_t i = 0; i < numCameras; ++i)
        {
            const osg::BoundingBoxd& shadowBB = shadowBBs[i];
            const osg::Vec3 center = shadowBB.center();
            double radius = osg::maximum(shadowBB.xMax() - shadowBB.xMin(),
                                         shadowBB.yMax() - shadowBB.yMin()) * 0.5;
            double cascadeTexelSize = 0.0;  // world size of one texel of this cascade

#if false
            double xMin = center[0] - radius, xMax = center[0] + radius;
            double yMin = center[1] - radius, yMax = center[1] + radius;
            cascadeTexelSize = (2.0 * radius) / _shadowMaps[i]->getTextureWidth();
            //xMin = shadowBB.xMin(), xMax = shadowBB.xMax();
            //yMin = shadowBB.yMin(), yMax = shadowBB.yMax();
#else       // Texel snap
            // The world size of one texel has to be stable between frames: a radius that
            // changes every frame keeps re-scaling the sampling grid, which makes the texel
            // snapping below useless and leaves shadow edges shimmering while the camera
            // moves. Quantizing the radius to a power of 2 keeps the texel size constant for
            // a whole octave, in exchange for up to 2x larger extents of this cascade
            if (radius > 0.0) radius = pow(2.0, ceil(log2(radius)));
            double texelSize = (2.0 * radius) / _shadowMaps[i]->getTextureWidth();
            cascadeTexelSize = texelSize;
            
            // Snap the center to texel grid and recompute keeping the radius
            double snappedCenterX = floor(center.x() / texelSize) * texelSize;
            double snappedCenterY = floor(center.y() / texelSize) * texelSize;
            double xMin = snappedCenterX - radius, xMax = snappedCenterX + radius;
            double yMin = snappedCenterY - radius, yMax = snappedCenterY + radius;
#endif
            //std::cout << i << ": X = (" << xMin << ", " << xMax << "), Y = ("
            //          << yMin << ", " << yMax << "); Z = " << zMaxTotal << "\n";

            // Apply the shadow camera & uniform
            osg::Camera* shadowCam = _shadowCameras[i].get();
            // A caster farther than the farthest receiver of this cascade can not cast any
            // shadow on it, so the far plane can be limited to this cascade's own bound
            double zFar = osg::maximum(-shadowBB.zMin(), zNearTotal + 0.001);
            shadowCam->setViewMatrix(_lightMatrix);
            shadowCam->setProjectionMatrixAsOrtho(xMin, xMax, yMin, yMax, zNearTotal, zFar);
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

            // How much the light-space depth of one shadow-map texel changes, expressed in the
            // depth space used by CompareDepth(): this is the unit in which the slope-scaled
            // bias and the normal offset are configured, so they keep working when the cascade
            // resolution or the cascade depth range changes
            if (i < 4)
            {
                double depthRange = osg::maximum(zFar - zNearTotal, 1e-6);
                biasScales[i] = (float)((_technique == EyeSpaceDepthSM) ?
                    cascadeTexelSize : (2.0 * cascadeTexelSize / depthRange));
                texelSizes[i] = (float)cascadeTexelSize;
            }
        }
        _biasScales->set(osg::Vec4(biasScales[0], biasScales[1], biasScales[2], biasScales[3]));
        _texelSizes->set(osg::Vec4(texelSizes[0], texelSizes[1], texelSizes[2], texelSizes[3]));
        _lightMatrices->dirty();

        // One texel of a shadow map is a length in world space, and it is what decides both how
        // sharp the shadows can be and how much a receiver-side bias (see setShadowBias()) costs.
        // Reporting it once keeps that trade-off visible instead of hidden behind the two
        // constants involved
        if (!_infoPrinted)
        {
            _infoPrinted = true;
            std::stringstream ss; ss << "[ShadowModule] View range [" << zn << ", " << zf << "]:";
            for (size_t i = 0; i < numCameras; ++i)
                ss << " #" << i << " [" << splitDepths[i] << ", " << splitDepths[i + 1]
                   << "] = " << texelSizes[i] << " units/texel;";
            OSG_NOTICE << ss.str() << std::endl;
        }
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
#if !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE) && !defined(OSG_GL3_AVAILABLE)
            geom->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
#endif
            _shadowFrustum->addDrawable(geom);
        }

        Frustum frustum; osg::Vec3Array* va = static_cast<osg::Vec3Array*>(geom->getVertexArray());
        frustum.create(shadowCam->getViewMatrix(), shadowCam->getProjectionMatrix());
        for (int i = 0; i < 8; ++i) (*va)[i] = frustum.corners[i];
        va->dirty(); geom->dirtyBound();
    }
}

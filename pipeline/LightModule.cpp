#include <osg/io_utils>
#include <osgDB/ReadFile>
#include <osgUtil/UpdateVisitor>
#include <iostream>
#include <cmath>
#include "LightModule.h"
#include "ShadowModule.h"
#include "Utilities.h"

namespace osgVerse
{
    LightModule::LightModule(const std::string& name, Pipeline* pipeline, int maxLightsInPass)
        : _pipeline(pipeline), _maxLightsInPass(maxLightsInPass), _shadowMaxDistance(-1.0)
    {
        // The texture buffer object (TBO) is the default way to upload the light table. Whether
        // samplerBuffer can be used is decided from the runtime GL & GLSL versions, which is the
        // very same test the shaders use to define VERSE_LIGHT_BUFFER (see
        // ShaderLibrary::createShaderDefinitions), so the CPU and GPU sides always agree: a
        // compatibility profile context on a modern driver gets the TBO path as well, while
        // GLES2/WebGL1 keeps the Texture2D one. setUseTextureBuffer() can override the default
        if (pipeline)
            _useTextureBuffer = isTextureBufferSupported(
                pipeline->getContextTargetVersion(), pipeline->getGlslTargetVersion());
        else
            _useTextureBuffer = false;

        _parameterImage = new osg::Image;
        _parameterImage->allocateImage(1024, 4, 1, GL_RGBA, GL_FLOAT);
#if defined(VERSE_EMBEDDED_GLES2)
        _parameterImage->setInternalTextureFormat(GL_RGBA);
#else
        _parameterImage->setInternalTextureFormat(GL_RGBA32F_ARB);
#endif
        // Row 3 is reserved for future use, so keep it clean from the very beginning
        memset(_parameterImage->data(), 0, _parameterImage->getTotalSizeInBytes());

        _parameterTex = new osg::Texture2D;
        _parameterTex->setImage(_parameterImage.get());
        _parameterTex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
        _parameterTex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
        _parameterTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
        _parameterTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_BORDER);
        _parameterTex->setBorderColor(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));

        // The TBO shares the very same image data with the Texture2D above (the flat index of
        // element (i, row) is then row * 1024 + i), so only one of them ever has to be uploaded
        if (_useTextureBuffer)
        {
            _parameterBuffer = new osg::TextureBuffer;
            _parameterBuffer->setImage(_parameterImage.get());
            _parameterBuffer->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
            _parameterBuffer->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
        }

        _lightNumber = new osg::Uniform("LightNumber", osg::Vec2(0.0f, (float)maxLightsInPass));
        _lightCluster = new LightCluster;
        _lightCluster->setEnabled(_useTextureBuffer);
        _clusterEnabled = new osg::Uniform("LightClusterEnabled", 0);
        _clusterGrid = new osg::Uniform("LightClusterGrid", osg::Vec3(0.0f, 0.0f, 0.0f));
        _clusterTile = new osg::Uniform("LightClusterTile", osg::Vec2(64.0f, 64.0f));
        _clusterDepth = new osg::Uniform("LightClusterDepth", osg::Vec2(0.1f, 1000.0f));
        if (pipeline) pipeline->addModule(name, this);
    }

    bool LightModule::getClusterEnabled() const
    { return _useTextureBuffer && _lightCluster.valid() && _lightCluster->getEnabled(); }

    void LightModule::setClusterEnabled(bool b)
    { if (_lightCluster.valid()) _lightCluster->setEnabled(b); }

    LightModule::~LightModule()
    {
        if (_pipeline.valid()) _pipeline->removeModule(this);
    }

    void LightModule::operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        osgUtil::UpdateVisitor* uv = static_cast<osgUtil::UpdateVisitor*>(nv);
        if (!uv) { traverse(node, nv); return; }

        // Update shadow module if main-light exists
        const float dirLength = 1000.0f;
        if (_pipeline.valid() && _mainLight.valid())
        {
            ShadowModule* shadow = static_cast<ShadowModule*>(_pipeline->getModule(_shadowModuleName));
            if (shadow != NULL)
            {
                // FIXME: what if main-light is not directional?
                osg::Matrix worldM = _mainLight->getWorldMatrices()[0];
                osg::Vec3d pos0 = _mainLight->getPosition() * worldM;
                osg::Vec3d pos1 = (_mainLight->getPosition() +
                                   _mainLight->getDirection() * dirLength) * worldM;
                shadow->setLightState(pos0, pos1 - pos0, _shadowMaxDistance);
            }
        }

        // Prune global light manager
        if (uv->getFrameStamp() && !(uv->getFrameStamp()->getFrameNumber() % 10))
            LightGlobalManager::instance()->prune(uv->getFrameStamp());

        // When a pipeline stage consumes the table, it is rebuilt by that stage's pre-draw callback
        // instead (see applyTextureAndUniforms), where the camera matrices of the frame being
        // rendered are already available. Only a forward-only setup, which never feeds a stage,
        // has to keep updating the table here
        if (!_tableCamera.valid() && LightGlobalManager::instance()->checkDirty())
            updateLightTable(node->asCamera());
        traverse(node, nv);
    }

    void LightModule::LightTableCallback::operator()(osg::RenderInfo&) const
    {
        LightModule* m = _module.get();
        if (m != NULL) m->updateLightTable(_camera.get());
    }

    void LightModule::updateLightTable(osg::Camera* cam)
    {
        // Get and sort lights by its importance (e.g., last frame number, distance to eye)
        std::vector<LightGlobalManager::LightData> resultLights;
        size_t numData = LightGlobalManager::instance()->getSortedResult(resultLights);
        if (numData > 1024) numData = 1024;

        // The lights are stored in their own local space (see LightCullCallback::cull), and the
        // shader expects them in the view space used to render the scene into the G-Buffer, which
        // is also the space the fragment positions are rebuilt in (see GBufferMatrices in
        // std_pbr_lighting.frag.glsl). The camera given to this function can not be used for it:
        // a screen-spaced stage (the lighting pass is one) is an absolute-reference camera with an
        // identity view matrix. The G-Buffer stage camera is the one whose matrices become
        // GBufferMatrices (see Pipeline::requireDepthBlit), and its view matrix is the current
        // frame's one by the time the consuming stage is drawn. Building the table earlier, in the
        // update traversal, would be one frame late (both the camera manipulator and the slave
        // camera matrices are only updated after it): the lights would keep the previous view
        // matrix and swim over the surfaces as soon as the camera moves, while a still camera
        // would hide the error
        osg::Camera* viewCam = cam;
        if (_pipeline.valid())
        {
            Pipeline::Stage* gb = _pipeline->getStage("GBuffer");
            if (gb != NULL && gb->camera.valid()) viewCam = gb->camera.get();
        }
        osg::Matrix view = (viewCam != NULL) ? viewCam->getViewMatrix() : osg::Matrix::identity();
        const float dirLength = 1000.0f;

        // Save all lights to a parameter texture to use in deferred shader.
        // The layout must match getLightAttributes() in std_pbr_lighting.frag.glsl and
        // std_forward_render.frag.glsl:
        //   row0: light color (vec3) + type (float)
        //   row1: eye-space position (vec3) + attenuation range (float)
        //   row2: eye-space direction (vec3) + spot cutoff as cosine (float)
        osg::Vec4f* paramPtr = (osg::Vec4f*)_parameterImage->data();
        std::vector<osg::Matrix> localToEye(numData);
        for (size_t i = 0; i < numData; ++i)
        {
            LightGlobalManager::LightData& ld = resultLights[i];
            if (!ld.light) continue; bool unlimited = false;
            LightDrawable::Type t = ld.light->getType(unlimited);
            const osg::Vec3& color = ld.light->getColor();

            // An eye-space light (see LightDrawable::setEyeSpace) already follows the viewer, so
            // its position is used as is and no view transform is applied to it
            osg::Matrix& localToEyeM = localToEye[i];
            if (ld.light->getEyeSpace()) localToEyeM = osg::Matrix::identity();
            else
            {
                osg::MatrixList worldM = ld.light->getWorldMatrices();
                localToEyeM = worldM.empty() ? osg::Matrix::identity() : worldM.front();
                localToEyeM.postMult(view);
            }

            osg::Vec3 pos0 = ld.light->getPosition() * localToEyeM;
            osg::Vec3 pos1 = (ld.light->getPosition() +
                              ld.light->getDirection() * dirLength) * localToEyeM;
            osg::Vec3 dir = pos1 - pos0; dir.normalize();

            // The shader compares the spot cutoff with dot(spot_dir, light_dir), so a cosine
            // is expected here; LightDrawable::getSpotCutoff() stores the half-angle in radians
            float spotCosine = cosf(ld.light->getSpotCutoff());
            *(paramPtr + 1024 * 0 + i)/*light color, type*/ =
                osg::Vec4f(color[0], color[1], color[2], (float)t);
            *(paramPtr + 1024 * 1 + i)/*eye-space position, range*/ =
                osg::Vec4f(pos0[0], pos0[1], pos0[2], ld.light->getRange());
            *(paramPtr + 1024 * 2 + i)/*eye-space direction, spot cutoff*/ =
                osg::Vec4f(dir[0], dir[1], dir[2], spotCosine);
        }
        _lightNumber->set(osg::Vec2((float)numData, (float)_maxLightsInPass));
        _parameterImage->dirty();

        // Build the CPU-side light clusters and update the shader uniforms. The eye-space positions
        // used here are exactly the ones written to the parameter table above, so the cluster
        // assignment and the lighting result describe the same lights
        if (getClusterEnabled())
        {
            // The cluster grid has to be built with the same projection shape, viewport and depth
            // range the shader uses for getClusterIndex(), all of them taken from the G-Buffer
            // camera (see the view matrix above): its projection gives the frustum shape, while
            // the near/far actually applied to it is the one calculated in the G-Buffer pre-draw
            osg::Matrix proj; osg::Vec2s vp(1920, 1080); float zn = 0.1f, zf = 1000.0f;
            if (_pipeline.valid())
            {
                vp = _pipeline->getStageSize();
                if (_pipeline->getDeferredCallback() != NULL)
                {
                    osg::Vec2d nf = _pipeline->getDeferredCallback()->getCalculatedNearFar();
                    if (nf.x() > 0.0 && nf.y() > nf.x()) { zn = (float)nf.x(); zf = (float)nf.y(); }
                }
            }

            if (viewCam != NULL) proj = viewCam->getProjectionMatrix();
            if (vp.x() <= 0 || vp.y() <= 0)
            {
                const osg::Viewport* viewport = (cam != NULL) ? cam->getViewport() : NULL;
                if (viewport != NULL) vp.set((short)viewport->width(), (short)viewport->height());
            }

            std::vector<LightCluster::Light> clusterLights;
            clusterLights.reserve(numData);
            for (size_t i = 0; i < numData; ++i)
            {
                LightGlobalManager::LightData& ld = resultLights[i];
                if (!ld.light) continue;
                bool unlimited = false;
                LightDrawable::Type t = ld.light->getType(unlimited);
                osg::Vec3 eyePos = ld.light->getPosition() * localToEye[i];
                clusterLights.push_back(LightCluster::Light(
                    (int)i, eyePos, ld.light->getRange(), t == LightDrawable::Directional));
            }

            if (_lightCluster->build(proj, vp, zn, zf, clusterLights))
            {
                const osg::Vec3i& gridSize = _lightCluster->getGridSize();
                _clusterEnabled->set(1);
                _clusterGrid->set(osg::Vec3((float)gridSize.x(), (float)gridSize.y(),
                                            (float)gridSize.z()));
                _clusterTile->set(_lightCluster->getTilePixels());
                _clusterDepth->set(_lightCluster->getNearFar());
            }
            else
                _clusterEnabled->set(0);
        }
        else
            _clusterEnabled->set(0);
    }

    int LightModule::applyTextureAndUniforms(Pipeline::Stage* stage,
                                             const std::string& prefix, int startU)
    {
        // The stage fed here is the one consuming the table, so it is also the right place to keep
        // it up to date: refreshing it in its pre-draw callback (instead of in the update
        // traversal) is what makes the eye-space light data match the camera matrices the shader
        // is going to use (see LightModule::updateLightTable)
        osg::Camera* stageCam = (stage != NULL) ? stage->camera.get() : NULL;
        if (stageCam != NULL && !_tableCamera.valid())
        {
            _tableCamera = stageCam;
            stageCam->addPreDrawCallback(new LightTableCallback(this, stageCam));
        }

        if (_useTextureBuffer && _parameterBuffer.valid())
            stage->applyTexture(_parameterBuffer.get(), "LightParameterBuffer", startU);
        else
            stage->applyTexture(_parameterTex.get(), prefix, startU);

        stage->applyUniform(getLightNumber());
        stage->applyUniform(_clusterEnabled.get());
        if (getClusterEnabled() && _lightCluster->getIndexBuffer() != NULL &&
            _lightCluster->getClusterBuffer() != NULL)
        {
            stage->applyTexture(_lightCluster->getIndexBuffer(), "LightIndexBuffer", startU + 1);
            stage->applyTexture(_lightCluster->getClusterBuffer(), "LightClusterBuffer", startU + 2);
            stage->applyUniform(_clusterGrid.get());
            stage->applyUniform(_clusterTile.get());
            stage->applyUniform(_clusterDepth.get());
            return startU + 3;
        }
        return startU + 1;
    }

    LightGlobalManager* LightGlobalManager::instance()
    {
        static osg::ref_ptr<LightGlobalManager> s_instance = new LightGlobalManager;
        return s_instance.get();
    }

    LightGlobalManager::LightGlobalManager()
    { _callback = new LightCullCallback; _dirty = false; }

    size_t LightGlobalManager::getSortedResult(std::vector<LightData>& result)
    {
        for (std::map<LightDrawable*, LightData>::iterator itr = _lights.begin();
             itr != _lights.end(); ++itr)
        { result.push_back(itr->second); }

        std::sort(result.begin(), result.end(),
                  [](const LightData& l, const LightData& r) {
            // TODO: more sort comparers, like distance-to-eye?
            return l.frameNo > r.frameNo;
        });
        return result.size();
    }

    void LightGlobalManager::remove(LightDrawable* light)
    {
        if (_lights.find(light) != _lights.end())
        { _lights.erase(_lights.find(light)); _dirty = true; }
    }

    void LightGlobalManager::prune(const osg::FrameStamp* fs, int outdatedFrames)
    {
        unsigned int frameNo = fs->getFrameNumber();
        for (std::map<LightDrawable*, LightData>::iterator itr = _lights.begin();
             itr != _lights.end();)
        {
            if ((itr->second.frameNo + outdatedFrames) >= frameNo) itr++;
            else { itr = _lights.erase(itr); _dirty = true; }
        }
    }
}

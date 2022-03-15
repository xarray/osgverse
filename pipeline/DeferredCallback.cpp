#include <osg/io_utils>
#include <osg/FrameBufferObject>
#include <osg/RenderInfo>
#include <osg/GLExtensions>
#include <osg/ContextData>
#include <osg/PolygonMode>
#include <osg/Geode>
#include <osgUtil/SceneView>
#include <iostream>
#include "DeferredCallback.h"
#include "Utilities.h"

namespace osgVerse
{
    DeferredRenderCallback::DeferredRenderCallback(bool inPipeline)
    :   _drawBuffer(GL_NONE), _readBuffer(GL_NONE), _cullFrameNumber(0),
        _inPipeline(inPipeline), _drawBufferApplyMask(false), _readBufferApplyMask(false)
    {
        _nearFarUniform = new osg::Uniform("NearFarPlanes", osg::Vec2());
        _calculatedNearFar.set(-1.0, -1.0);
        _clearMask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
        _clearColor.set(0.0f, 0.0f, 0.0f, 0.0f);
        _clearAccum.set(0.0f, 0.0f, 0.0f, 0.0f);
        _clearDepth = 1.0; _clearStencil = 0.0;
    }

    void DeferredRenderCallback::requireDepthBlit(osg::Camera* cam, bool addToList)
    {
        if (_depthBlitList.find(cam) != _depthBlitList.end())
        { if (!addToList) _depthBlitList.erase(cam); }
        else if (addToList) _depthBlitList.insert(cam);

        if (_cameraMatrixMap.find(cam) != _cameraMatrixMap.end())
        { if (!addToList) _cameraMatrixMap.erase(cam); }
        else if (addToList && cam) _cameraMatrixMap[cam] =
            MatrixListPair(cam->getName(), std::vector<osg::Matrixf>());
    }

    void DeferredRenderCallback::applyAndUpdateCameraUniforms(osgUtil::SceneView* sv)
    {
        osg::Camera* cam = sv->getCamera();
        for (std::map<osg::Camera*, MatrixListPair>::iterator
             itr = _cameraMatrixMap.begin(); itr != _cameraMatrixMap.end(); ++itr)
        {
            std::string uName = std::get<0>(itr->second);
            if (itr->first == cam)
            {
                std::vector<osg::Matrixf> matrices;
                matrices.push_back(sv->getViewMatrix());
                matrices.push_back(osg::Matrix::inverse(matrices.back()));
                matrices.push_back(sv->getProjectionMatrix());
                matrices.push_back(osg::Matrix::inverse(matrices.back()));
                std::get<1>(itr->second) = matrices;
            }
            
            osg::Uniform* u1 = sv->getLocalStateSet()->getOrCreateUniform(
                (uName + "Matrices").c_str(), osg::Uniform::FLOAT_MAT4, 4);
            const std::vector<osg::Matrixf>& matrices = std::get<1>(itr->second);
            for (size_t i = 0; i < matrices.size(); ++i) u1->setElement(i, matrices[i]);
        }
    }

    osg::Vec2d DeferredRenderCallback::cullWithNearFarCalculation(osgUtil::SceneView* sv)
    {
        unsigned int frameNo = sv->getFrameStamp()->getFrameNumber();
        if (frameNo <= _cullFrameNumber) return _calculatedNearFar;
        else _cullFrameNumber = frameNo;

        // Update global near/far using entire scene, ignoring callback/cull-mask
        osg::ref_ptr<osg::CullSettings::ClampProjectionMatrixCallback> clamper =
            sv->getClampProjectionMatrixCallback();
        unsigned int cullMask = sv->getCullMask();
        sv->setClampProjectionMatrixCallback(_userClamperCallback.get());
        sv->setCullMask(0xffffffff);
        sv->osgUtil::SceneView::cull();
        sv->setCullMask(cullMask);
        sv->setClampProjectionMatrixCallback(clamper.get());

        // Apply near/far variable for future stages and forward pass to use
        double znear = 0.0, zfar = 0.0, epsilon = 1e-6;
        osg::Matrixd& proj = sv->getProjectionMatrix();
        if (fabs(proj(0, 3)) < epsilon  && fabs(proj(1, 3)) < epsilon  && fabs(proj(2, 3)) < epsilon)
        {
            double left = 0.0, right = 0.0, bottom = 0.0, top = 0.0;
            proj.getOrtho(left, right, bottom, top, znear, zfar);
        }
        else
        {
            double ratio = 0.0, fovy = 0.0;
            proj.getPerspective(fovy, ratio, znear, zfar);
        }

        _nearFarUniform->set(osg::Vec2(znear, zfar));
        _calculatedNearFar.set(znear, zfar);
        return _calculatedNearFar;
    }

    void DeferredRenderCallback::operator()(osg::RenderInfo& renderInfo) const
    {
        osg::State* state = renderInfo.getState();
        osg::GLExtensions* ext = state->get<osg::GLExtensions>();
        if (!ext->isFrameBufferObjectSupported)
        {
            OSG_WARN << "[DeferredRenderCallback] No FBO support" << std::endl;
            return;
        }

        DeferredRenderCallback* cb = const_cast<DeferredRenderCallback*>(this);
        for (size_t i = 0; i < _runners.size(); ++i)
        {
            RttRunner* r = _runners[i].get();
            if (r->attachments.empty() || !r->active) continue;

            // Initialize runner and internal FBO objects
            if (!r->created)
            {
                r->created = r->setup(cb, renderInfo);
                if (!r->created) OSG_WARN << "[RttRunner] Unable to setup FBO of " << r->name << std::endl;
            }

            // Apply FBO buffer for drawing and clear the viewport
            if (r->created)
            {
                r->start(cb, renderInfo);
                if (r->viewport.valid())
                {
                    state->applyAttribute(r->viewport.get());
                    glScissor(static_cast<int>(r->viewport->x()), static_cast<int>(r->viewport->y()),
                        static_cast<int>(r->viewport->width()), static_cast<int>(r->viewport->height()));
                    state->applyMode(GL_SCISSOR_TEST, true);
                }

                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                if (_clearMask != 0)
                {
                    if (_clearMask & GL_COLOR_BUFFER_BIT)
                        glClearColor(_clearColor[0], _clearColor[1], _clearColor[2], _clearColor[3]);
                    if (_clearMask & GL_DEPTH_BUFFER_BIT)
                    {
#if !defined(OSG_GLES1_AVAILABLE) && !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE)
                        glClearDepth(_clearDepth); glDepthMask(GL_TRUE);
#else
                        glClearDepthf(_clearDepth); glDepthMask(GL_TRUE);
#endif
                        state->haveAppliedAttribute(osg::StateAttribute::DEPTH);
                    }
                    if (_clearMask & GL_STENCIL_BUFFER_BIT)
                    {
                        glClearStencil((int)_clearStencil); glStencilMask(~0u);
                        state->haveAppliedAttribute(osg::StateAttribute::STENCIL);
                    }
#if !defined(OSG_GLES1_AVAILABLE) && !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE) && !defined(OSG_GL3_AVAILABLE)
                    if (_clearMask & GL_ACCUM_BUFFER_BIT)
                        glClearAccum(_clearAccum[0], _clearAccum[1], _clearAccum[2], _clearAccum[3]);
#endif
                    glClear(_clearMask);
                }

                // Draw user-defined geometry and finish the FBO rendering
                r->draw(cb, renderInfo);
                r->finish(cb, renderInfo);
            }
        }

        osg::Camera* forwardCam = renderInfo.getCurrentCamera();
        if (!_depthBlitList.empty())
        {
            typedef std::map<osg::Camera*, osg::observer_ptr<osg::FrameBufferObject>> CamFboMap;
            ext->glBindFramebuffer(GL_DRAW_FRAMEBUFFER_EXT, 0); // write to default framebuffer

            int sWidth = 1920, tWidth = 1920, sHeight = 1080, tHeight = 1080;
            osg::Viewport* viewport = forwardCam->getViewport();
            if (viewport != NULL) { tWidth = viewport->width(); tHeight = viewport->height(); }

            // Try to blit specified depth buffer in pipeline FBOs to the following forward pass
            for (std::set<osg::observer_ptr<osg::Camera>>::iterator itr = _depthBlitList.begin();
                itr != _depthBlitList.end(); ++itr)
            {
                osg::Camera* cam = itr->get();
                CamFboMap::const_iterator fboItr = _depthFboMap.find(cam);
                if (!cam || fboItr == _depthFboMap.end()) continue; else viewport = cam->getViewport();
                if (viewport != NULL) { sWidth = viewport->width(); sHeight = viewport->height(); }

                osg::FrameBufferObject* fbo = fboItr->second.get();
                if (fbo) fbo->apply(*state, osg::FrameBufferObject::READ_FRAMEBUFFER);
                ext->glBlitFramebuffer(0, 0, sWidth, sHeight, 0, 0, tWidth, tHeight,
                                       GL_DEPTH_BUFFER_BIT, GL_NEAREST);
#if 0
                OSG_NOTICE << "Blitting " << cam->getName() << ": " << sWidth << "x" << sHeight << " => "
                           << forwardCam->getName() << ": " << tWidth << "x" << tHeight << std::endl;
#endif
            }
            ext->glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
        }
        else if (_inPipeline)
        {
            OSG_NOTICE << "[DeferredRenderCallback] No previous depth buffer is going to blit with "
                       << "current camera. Should not happen in deferred rendering mode" << std::endl;
        }
    }

    bool DeferredRenderCallback::RttRunner::setup(DeferredRenderCallback* cb, osg::RenderInfo& renderInfo)
    {
        osg::State* state = renderInfo.getState();
        osg::GLExtensions* ext = state->get<osg::GLExtensions>();
        int width = 1, height = 1, usage = 0/*color: 1, depth: 2, stencil: 4*/;
        if (viewport.valid())
        {
            width = static_cast<int>(viewport->x() + viewport->width());
            height = static_cast<int>(viewport->y() + viewport->height());
        }

        for (osg::Camera::BufferAttachmentMap::iterator itr = attachments.begin();
            itr != attachments.end(); ++itr)
        {
            width = osg::maximum(width, itr->second.width());
            height = osg::maximum(height, itr->second.height());
        }
        if (!viewport) viewport = new osg::Viewport(0, 0, width, height);

        for (osg::Camera::BufferAttachmentMap::iterator itr = attachments.begin();
            itr != attachments.end(); ++itr)
        {
            osg::Texture* texture = itr->second._texture.get();
            if (texture->getTextureWidth() == 0 || texture->getTextureHeight() == 0)
            {
                osg::Texture2D* tex2D = dynamic_cast<osg::Texture2D*>(texture);
                if (tex2D != NULL)
                    tex2D->setTextureSize(width, height);
                else
                {
                    osg::TextureCubeMap* texCube = dynamic_cast<osg::TextureCubeMap*>(texture);
                    if (texCube != NULL) texCube->setTextureSize(width, height);
                }
                texture->dirtyTextureObject();
            }
        }

        fbo = new osg::FrameBufferObject;
        for (osg::Camera::BufferAttachmentMap::iterator itr = attachments.begin();
            itr != attachments.end(); ++itr)
        {
            switch (itr->first)
            {
            case osg::Camera::DEPTH_BUFFER: usage |= 2; break;
            case osg::Camera::STENCIL_BUFFER: usage |= 4; break;
            case osg::Camera::PACKED_DEPTH_STENCIL_BUFFER: usage |= 6; break;
            case osg::Camera::COLOR_BUFFER: usage |= 1; break;
            default: continue;  // not valid attachment
            }
            fbo->setAttachment(itr->first, osg::FrameBufferAttachment(itr->second));
        }

        fbo->apply(*state);
        if (!(usage & 1))
        {
#if !defined(OSG_GLES1_AVAILABLE) && !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE)
            cb->setDrawBuffer(GL_NONE, true); state->glDrawBuffer(GL_NONE);
            cb->setReadBuffer(GL_NONE, true); state->glReadBuffer(GL_NONE);
#endif
        }

        GLenum status = ext->glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
        if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
        {
            GLuint fboId = state->getGraphicsContext()
                ? state->getGraphicsContext()->getDefaultFboId() : 0;
            ext->glBindFramebuffer(GL_FRAMEBUFFER_EXT, fboId);
            osg::get<osg::GLRenderBufferManager>(state->getContextID())->flushAllDeletedGLObjects();
            osg::get<osg::GLFrameBufferObjectManager>(state->getContextID())->flushAllDeletedGLObjects();

            OSG_WARN << "[Runner] FBO setup failed: 0x" << std::hex << status
                << std::dec << ", name: " << name << std::endl;
            fbo = NULL; return false;
        }

        cb->setDrawBuffer(GL_NONE, false);
        cb->setReadBuffer(GL_NONE, false);
        return true;
    }

    void DeferredRenderCallback::RttRunner::start(DeferredRenderCallback* cb, osg::RenderInfo& renderInfo)
    {
        osg::State* state = renderInfo.getState();
        bool useMRT = fbo->hasMultipleRenderingTargets();
        if (!useMRT)
        {
#if !defined(OSG_GLES1_AVAILABLE) && !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE)
            if (cb->_drawBufferApplyMask) state->glDrawBuffer(cb->_drawBuffer);
            if (cb->_readBufferApplyMask) state->glReadBuffer(cb->_readBuffer);
#endif
        }
        fbo->apply(*state);
    }

    void DeferredRenderCallback::RttRunner::finish(DeferredRenderCallback* cb, osg::RenderInfo& renderInfo)
    {
        osg::State* state = renderInfo.getState();
        osg::GLExtensions* ext = state->get<osg::GLExtensions>();
        GLuint fboId = state->getGraphicsContext()
            ? state->getGraphicsContext()->getDefaultFboId() : 0;
        ext->glBindFramebuffer(GL_FRAMEBUFFER_EXT, fboId);

        for (osg::Camera::BufferAttachmentMap::iterator itr = attachments.begin();
            itr != attachments.end(); ++itr)
        {
            if (itr->second._texture.valid() && itr->second._mipMapGeneration)
            {
                state->setActiveTextureUnit(0);
                state->applyTextureAttribute(0, itr->second._texture.get());
                ext->glGenerateMipmap(itr->second._texture->getTextureTarget());
            }
        }
        if (runOnce) active = false;  // In runOnce mode, inactivate runner now
    }

    bool DeferredRenderCallback::RttRunner::draw(DeferredRenderCallback* cb, osg::RenderInfo& renderInfo)
    {
        osg::State* state = renderInfo.getState();
        osg::GLExtensions* ext = state->get<osg::GLExtensions>();

#ifdef OSG_GL_MATRICES_AVAILABLE
        glMatrixMode(GL_MODELVIEW);
#endif
        state->applyProjectionMatrix(new osg::RefMatrix(projection));
        state->applyModelViewMatrix(modelView);
        if (state->getUseModelViewAndProjectionUniforms())
            state->applyModelViewAndProjectionUniformsIfRequired();
        drawInner(cb, renderInfo);
        state->apply();

        GLenum status = ext->glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
        if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
        {
            OSG_WARN << "[Runner] FBO drawing failed: 0x" << std::hex << status
                << std::dec << ", name: " << name << std::endl;
            return false;
        }

        fbo->apply(*state, osg::FrameBufferObject::READ_FRAMEBUFFER);
        if (cb->_readBufferApplyMask)
        {
#if !defined(OSG_GLES1_AVAILABLE) && !defined(OSG_GLES2_AVAILABLE)
            glReadBuffer(cb->_readBuffer);
#endif
        }
        return true;
    }

    void DeferredRenderCallback::RttGeometryRunner::setUseScreenQuad(unsigned int unit, osg::Texture* texIn)
    {
        geometry = osg::createTexturedQuadGeometry(
            osg::Vec3(), osg::Vec3(1.0f, 0.0f, 0.0f), osg::Vec3(0.0f, 1.0, 0.0f));
        if (texIn) geometry->getOrCreateStateSet()->setTextureAttributeAndModes(unit, texIn);
        geometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        projection = osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0);
        modelView = osg::Matrix::identity();
    }

    void DeferredRenderCallback::RttGeometryRunner::drawInner(
        DeferredRenderCallback* cb, osg::RenderInfo& renderInfo) const
    {
        if (!geometry) { OSG_WARN << "[RttRunner] No geometry for " << name << std::endl; return; }
        if (geometry->getStateSet()) renderInfo.getState()->apply(geometry->getStateSet());
        if (viewport.valid()) viewport->apply(*renderInfo.getState());
        geometry->draw(renderInfo);
    }
}

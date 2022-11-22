#include <osg/io_utils>
#include <osg/Version>
#include <osg/ValueObject>
#include <osg/Depth>
#include <osgDB/ReadFile>
#include <osgUtil/RenderStage>
#include <osgViewer/Renderer>
#if VERSE_WINDOWS
    #include <osgViewer/api/Win32/GraphicsWindowWin32>
    #include <imm.h>
#endif
#include <iostream>
#include <sstream>
#include <stdarg.h>
#include "Pipeline.h"
#include "ShadowModule.h"
#include "Utilities.h"

#define DEBUG_CULL_VISITOR 1
#if OSG_VERSION_LESS_THAN(3, 6, 0)
#   define DEBUG_CULL_VISITOR 0
#endif

static osg::Camera::ComputeNearFarMode g_nearFarMode =
        osg::Camera::COMPUTE_NEAR_FAR_USING_BOUNDING_VOLUMES;

class DebugDrawCallback : public osg::Camera::DrawCallback
{
public:
    virtual void operator()(osg::RenderInfo& renderInfo) const
    {
        double fov, ratio, zn, zf;
        osg::Camera* cam = renderInfo.getCurrentCamera();
        renderInfo.getState()->getProjectionMatrix().getPerspective(fov, ratio, zn, zf);
        std::cout << _name << ": " << cam->getName() << " = "
                  << fov << ", " << ratio << ", " << zn << ", " << zf << "\n";
    }

    DebugDrawCallback(const std::string& n) : _name(n) {}
    std::string _name;
};

struct MyClampProjectionCallback : public osg::CullSettings::ClampProjectionMatrixCallback
{
    template<class MatrixType>
    bool _clampProjectionMatrix(MatrixType& proj, double& znear, double& zfar) const
    {
        static double epsilon = 1e-6;
        osg::Vec2d nearFar = _callback->getCalculatedNearFar();
        if (nearFar[0] > 0.0 && nearFar[1] > 0.0)
        {
            if (fabs(proj(0, 3)) < epsilon  && fabs(proj(1, 3)) < epsilon  && fabs(proj(2, 3)) < epsilon)
            {   // Orthographic matrix
                proj(2, 2) = -2.0f / (nearFar[1] - nearFar[0]);
                proj(3, 2) = -(nearFar[1] + nearFar[0]) / (nearFar[1] - nearFar[0]);
            }
            else
            {   // Persepective matrix
                double tNear = (-nearFar[0] * proj(2, 2) + proj(3, 2))
                             / (-nearFar[0] * proj(2, 3) + proj(3, 3));
                double tFar = (-nearFar[1] * proj(2, 2) + proj(3, 2))
                            / (-nearFar[1] * proj(2, 3) + proj(3, 3));
                double ratio = fabs(2.0 / (tNear - tFar)), center = -(tNear + tFar) / 2.0;
                proj.postMult(osg::Matrix(1.0f, 0.0f, 0.0f, 0.0f,
                                          0.0f, 1.0f, 0.0f, 0.0f,
                                          0.0f, 0.0f, ratio, 0.0f,
                                          0.0f, 0.0f, center * ratio, 1.0f));
            }
        }
        znear = nearFar[0]; zfar = nearFar[1];
        return true;
    }

    virtual bool clampProjectionMatrixImplementation(osg::Matrixf& p, double& znear, double& zfar) const
    { return _clampProjectionMatrix(p, znear, zfar); }

    virtual bool clampProjectionMatrixImplementation(osg::Matrixd& p, double& znear, double& zfar) const
    { return _clampProjectionMatrix(p, znear, zfar); }

    MyClampProjectionCallback(osgVerse::DeferredRenderCallback* cb) : _callback(cb) {}
    osg::observer_ptr<osgVerse::DeferredRenderCallback> _callback;
};

#if DEBUG_CULL_VISITOR
class MyCullVisitor : public osgUtil::CullVisitor
{
public:
    MyCullVisitor() : osgUtil::CullVisitor() {}
    MyCullVisitor(const MyCullVisitor& v) : osgUtil::CullVisitor(v) {}
    virtual CullVisitor* clone() const { return new MyCullVisitor(*this); }

    virtual void apply(osg::Drawable& drawable)
    {
        osg::RefMatrix& matrix = *getModelViewMatrix();
        const osg::BoundingBox& bb = drawable.getBoundingBox();
        if (drawable.getCullCallback())
        {
            osg::DrawableCullCallback* dcb = drawable.getCullCallback()->asDrawableCullCallback();
            if (dcb) { if (dcb->cull(this, &drawable, &_renderInfo) == true) return; }
            else drawable.getCullCallback()->run(&drawable, this);
        }

        if (drawable.isCullingActive() && isCulled(bb)) return;
        if (_computeNearFar && bb.valid()) { if (!updateCalculatedNearFar(matrix, drawable, false)) return; }

        // push the geoset's state on the geostate stack.
        unsigned int numPopStateSetRequired = 0;
        osg::StateSet* stateset = drawable.getStateSet();
        if (stateset) { ++numPopStateSetRequired; pushStateSet(stateset); }

        osg::CullingSet& cs = getCurrentCullingSet();
        if (!cs.getStateFrustumList().empty())
        {
            osg::CullingSet::StateFrustumList& sfl = cs.getStateFrustumList();
            for (osg::CullingSet::StateFrustumList::iterator itr = sfl.begin(); itr != sfl.end(); ++itr)
            {
                if (itr->second.contains(bb))
                { ++numPopStateSetRequired; pushStateSet(itr->first.get()); }
            }
        }

        float depth = bb.valid() ? distance(bb.center(), matrix) : 0.0f;
        if (osg::isNaN(depth))
        {
            OSG_NOTICE << drawable.getName() << " detected NaN..."
                       << " Camera: " << getCurrentCamera()->getName() << ", Center: " << bb.center().valid()
                       << ", Matrix: " << matrix.valid() << std::endl;
        }
        else
            addDrawableAndDepth(&drawable, &matrix, depth);
        for (unsigned int i = 0; i < numPopStateSetRequired; ++i) { popStateSet(); }
    }

protected:
    inline value_type distance(const osg::Vec3& coord, const osg::Matrix& matrix)
    {
        return -((value_type)coord[0] * (value_type)matrix(0, 2) +
                 (value_type)coord[1] * (value_type)matrix(1, 2) +
                 (value_type)coord[2] * (value_type)matrix(2, 2) + matrix(3, 2));
    }
};
#endif

class MySceneView : public osgUtil::SceneView
{
public:
    MySceneView(osgVerse::DeferredRenderCallback* cb, osg::DisplaySettings* ds = NULL)
        : osgUtil::SceneView(ds), _callback(cb) {}
    MySceneView(const MySceneView& sv, const osg::CopyOp& copyop = osg::CopyOp())
        : osgUtil::SceneView(sv, copyop), _callback(sv._callback) {}

    virtual void cull()
    {
        // Cameras that need calculate near/far globally should do the calculation here
        // Note that cullWithNearFarCalculation() will only compute whole near/far once per frame
        bool calcNearFar = false; getCamera()->getUserValue("NeedNearFarCalculation", calcNearFar);
        if (calcNearFar && _callback.valid()) _callback->cullWithNearFarCalculation(this);

        // Do regular culling and apply every input camera's inverse(ViewProj) uniform to all sceneViews
        // This uniform is helpful for deferred passes to rebuild world vertex and normals
        osgUtil::SceneView::cull();
        if (_callback.valid()) _callback->applyAndUpdateCameraUniforms(this);

        // Register RTT camera with depth buffer for later blitting with forward pass
        osg::FrameBufferObject* fbo = (getRenderStage() != NULL)
                                    ? getRenderStage()->getFrameBufferObject() : NULL;
        if (fbo && _callback.valid())
        {
            // Blit for DEPTH_BUFFER & PACKED_DEPTH_STENCIL_BUFFER
            if (fbo->hasAttachment(osg::Camera::DEPTH_BUFFER) ||
                fbo->hasAttachment(osg::Camera::PACKED_DEPTH_STENCIL_BUFFER))
                _callback->registerDepthFBO(getCamera(), fbo);
        }

#if false
        double ratio = 0.0, fovy = 0.0, znear = 0.0, zfar = 0.0;
        getProjectionMatrix().getPerspective(fovy, ratio, znear, zfar);
        if (ratio < 0.01 || znear >= zfar) return;  // invalid perspective matrix
        OSG_NOTICE << getName() << ", FrameNo = " << getFrameStamp()->getFrameNumber()
                   << ", Camera = " << getCamera()->getName() << ": Ratio = " << ratio
                   << ", NearFar = " << znear << "/" << zfar << std::endl;
#endif
    }

protected:
    osg::observer_ptr<osgVerse::DeferredRenderCallback> _callback;
};

class MyRenderer : public osgViewer::Renderer
{
public:
    MyRenderer(osg::Camera* c) : osgViewer::Renderer(c) {}

    void useCustomSceneViews(osgVerse::DeferredRenderCallback* cb)
    {
        unsigned int opt = osgUtil::SceneView::HEADLIGHT;
        osgViewer::View* view = dynamic_cast<osgViewer::View*>(_camera->getView());
        if (view)
        {
            switch (view->getLightingMode())
            {
            case(osg::View::NO_LIGHT): opt = 0; break;
            case(osg::View::SKY_LIGHT): opt = osgUtil::SceneView::SKY_LIGHT; break;
            case(osg::View::HEADLIGHT): opt = osgUtil::SceneView::HEADLIGHT; break;
            }
        }

        osg::ref_ptr<osgUtil::SceneView> sceneView0 = useCustomSceneView(0, opt, cb);
        osg::ref_ptr<osgUtil::SceneView> sceneView1 = useCustomSceneView(1, opt, cb);
        _sceneView[0] = sceneView0; sceneView0->setName("SceneView0");
        _sceneView[1] = sceneView1; sceneView1->setName("SceneView1");
        _availableQueue._queue.clear();
        _availableQueue.add(_sceneView[0]);
        _availableQueue.add(_sceneView[1]);
    }

protected:
    osgUtil::SceneView* useCustomSceneView(unsigned int i, unsigned int flags,
                                           osgVerse::DeferredRenderCallback* cb)
    {
        osg::ref_ptr<osgUtil::SceneView> newSceneView = new MySceneView(cb);
        newSceneView->setFrameStamp(const_cast<osg::FrameStamp*>(_sceneView[i]->getFrameStamp()));
        newSceneView->setAutomaticFlush(_sceneView[i]->getAutomaticFlush());
        newSceneView->setGlobalStateSet(_sceneView[i]->getGlobalStateSet());
        newSceneView->setSecondaryStateSet(_sceneView[i]->getSecondaryStateSet());
        newSceneView->setDefaults(flags);

        if (_sceneView[i]->getDisplaySettings())
            newSceneView->setDisplaySettings(_sceneView[i]->getDisplaySettings());
#if OSG_VERSION_GREATER_THAN(3, 3, 2)
        else
            newSceneView->setResetColorMaskToAllOn(false);
#endif
        newSceneView->setCamera(_camera.get(), false);

#if DEBUG_CULL_VISITOR
        MyCullVisitor* cullVisitor = new MyCullVisitor;
        cullVisitor->setStateGraph(_sceneView[i]->getStateGraph());
        cullVisitor->setRenderStage(_sceneView[i]->getRenderStage());
        newSceneView->setCullVisitor(cullVisitor);
#else
        newSceneView->setCullVisitor(_sceneView[i]->getCullVisitor());
        newSceneView->setCullVisitorLeft(_sceneView[i]->getCullVisitorLeft());
        newSceneView->setCullVisitorRight(_sceneView[i]->getCullVisitorRight());
#endif
        return newSceneView.release();
    }
};

struct MyResizedCallback : public osg::GraphicsContext::ResizedCallback
{
    MyResizedCallback(osgVerse::Pipeline* p) : _pipeline(p) {}
    osg::observer_ptr<osgVerse::Pipeline> _pipeline;

    virtual void resizedImplementation(osg::GraphicsContext* gc, int x, int y, int w, int h)
    {
        std::set<osg::Viewport*> processedViewports;
        const osg::GraphicsContext::Traits* traits = gc->getTraits();
        if (!traits) return;

        double widthChangeRatio = double(w) / double(traits->width);
        double heightChangeRatio = double(h) / double(traits->height);
        double aspectRatioChange = widthChangeRatio / heightChangeRatio;
        if (_pipeline.valid())
            _pipeline->getInvScreenResolution()->set(osg::Vec2(1.0f / (float)w, 1.0f / (float)h));

        osg::GraphicsContext::Cameras cameras = gc->getCameras();
        for (osg::GraphicsContext::Cameras::iterator itr = cameras.begin(); itr != cameras.end(); ++itr)
        {
            osg::Camera* camera = (*itr);
            osg::View* view = camera->getView();
            osg::View::Slave* slave = view ? view->findSlaveForCamera(camera) : 0;
            bool rtt = (camera->getRenderTargetImplementation() == osg::Camera::FRAME_BUFFER_OBJECT);
            bool inputCam = (slave ? slave->_useMastersSceneData : false);

            // Check if camera is for shadowing
            osgVerse::ShadowModule::ShadowData* sData =
                static_cast<osgVerse::ShadowModule::ShadowData*>(camera->getUserData());
            bool isShadowCam = (sData != NULL);

            osg::Viewport* viewport = camera->getViewport();
            if (viewport && (!rtt || inputCam) && !isShadowCam)
            {   // avoid processing a shared viewport twice
                if (processedViewports.count(viewport) == 0)
                {
                    processedViewports.insert(viewport);
                    if (viewport->x() == 0 && viewport->y() == 0 &&
                        viewport->width() >= traits->width && viewport->height() >= traits->height)
                    { viewport->setViewport(0, 0, w, h); }
                    else
                    {
                        viewport->x() = double(viewport->x() * widthChangeRatio);
                        viewport->y() = double(viewport->y() * heightChangeRatio);
                        viewport->width() = double(viewport->width() * widthChangeRatio);
                        viewport->height() = double(viewport->height() * heightChangeRatio);
                    }
                }
            }

            // if aspect ratio adjusted change the project matrix to suit.
            if (aspectRatioChange == 1.0) continue;
            if (slave)
            {
                if (camera->getReferenceFrame() == osg::Transform::RELATIVE_RF)
                {
#if OSG_VERSION_GREATER_THAN(3, 3, 2)
                    if (rtt) camera->resizeAttachments(w, h);
#endif
                    switch (view->getCamera()->getProjectionResizePolicy())
                    {
                    case (osg::Camera::HORIZONTAL):
                        slave->_projectionOffset *= osg::Matrix::scale(1.0 / aspectRatioChange, 1.0, 1.0); break;
                    case (osg::Camera::VERTICAL):
                        slave->_projectionOffset *= osg::Matrix::scale(1.0, aspectRatioChange, 1.0); break;
                    default: break;
                    }
                }
                else
                {
                    continue;  // FIXME: ignore all absolute slaves such as RTT & display quads?
                    /*switch (camera->getProjectionResizePolicy())
                    {
                    case (osg::Camera::HORIZONTAL):
                        camera->getProjectionMatrix() *= osg::Matrix::scale(1.0 / aspectRatioChange, 1.0, 1.0); break;
                    case (osg::Camera::VERTICAL):
                        camera->getProjectionMatrix() *= osg::Matrix::scale(1.0, aspectRatioChange, 1.0); break;
                    default: break;
                    }*/
                }
            }
            else
            {
                if (rtt) continue;
                osg::Camera::ProjectionResizePolicy policy = view ?
                    view->getCamera()->getProjectionResizePolicy() : camera->getProjectionResizePolicy();
                switch (policy)
                {
                case (osg::Camera::HORIZONTAL):
                    camera->getProjectionMatrix() *= osg::Matrix::scale(1.0 / aspectRatioChange, 1.0, 1.0); break;
                case (osg::Camera::VERTICAL):
                    camera->getProjectionMatrix() *= osg::Matrix::scale(1.0, aspectRatioChange, 1.0); break;
                default: break;
                }

                osg::Camera* master = view ? view->getCamera() : 0;
                if (!view || (view && camera != master)) continue;
                for (unsigned int i = 0; i < view->getNumSlaves(); ++i)
                {
                    osg::View::Slave& child = view->getSlave(i);
                    if (child._camera.valid() && child._camera->getReferenceFrame() == osg::Transform::RELATIVE_RF)
                    {
                        // scale the slaves by the inverse of the change that has been applied to master, to avoid them
                        // be scaled twice (such as when both master and slave are on the same GraphicsContexts)
                        // or by the wrong scale when master and slave are on different GraphicsContexts.
                        switch (policy)
                        {
                        case (osg::Camera::HORIZONTAL):
                            child._projectionOffset *= osg::Matrix::scale(aspectRatioChange, 1.0, 1.0); break;
                        case (osg::Camera::VERTICAL):
                            child._projectionOffset *= osg::Matrix::scale(1.0, 1.0 / aspectRatioChange, 1.0); break;
                        default: break;
                        }
                    }
                }
            }
        }

        osg::GraphicsContext::Traits* ncTraits = const_cast<osg::GraphicsContext::Traits*>(traits);
        ncTraits->x = x; ncTraits->y = y; ncTraits->width = w; ncTraits->height = h;
    }
};

namespace osgVerse
{
    static osg::GraphicsContext* createGraphicsContext(int w, int h, const std::string& glContext,
                                                       osg::GraphicsContext* shared = NULL)
    {
        osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
        traits->x = 0; traits->y = 0; traits->width = w; traits->height = h;
        traits->windowDecoration = false; traits->doubleBuffer = true;
        traits->sharedContext = shared; traits->vsync = true;
        traits->glContextVersion = glContext;
        return osg::GraphicsContext::createGraphicsContext(traits.get());
    }

    Pipeline::Pipeline(int glContextVer, int glslVer)
    {
        _deferredCallback = new osgVerse::DeferredRenderCallback(true);
        _deferredDepth = new osg::Depth(osg::Depth::LESS, 0.0, 1.0, false);
        _invScreenResolution = new osg::Uniform(
            "InvScreenResolution", osg::Vec2(1.0f / 1920.0f, 1.0f / 1080.0f));
        _glTargetVersion = glContextVer; _glslTargetVersion = glslVer;
    }

    void Pipeline::Stage::applyUniform(osg::Uniform* u)
    {
        osg::StateSet* ss = deferred ?
            runner->geometry->getOrCreateStateSet() : camera->getOrCreateStateSet();
        if (ss->getUniform(u->getName()) == NULL) ss->addUniform(u);
    }

    void Pipeline::Stage::applyBuffer(Stage& src, const std::string& buffer, int unit, osg::Texture::WrapMode wp)
    { applyBuffer(src, buffer, buffer, unit, wp); }

    void Pipeline::Stage::applyBuffer(Stage& src, const std::string& buffer, const std::string& n,
                                      int unit, osg::Texture::WrapMode wp)
    {
        if (src.outputs.find(buffer) != src.outputs.end())
        {
            osg::Texture* tex = src.outputs[buffer].get();
            if (wp != 0)
            {
                tex->setWrap(osg::Texture::WRAP_S, wp);
                tex->setWrap(osg::Texture::WRAP_T, wp);
            }

            osg::StateSet* ss = deferred ?
                runner->geometry->getOrCreateStateSet() : camera->getOrCreateStateSet();
            ss->setTextureAttributeAndModes(unit, tex);
            ss->addUniform(new osg::Uniform(n.data(), unit));
        }
        else
            std::cout << buffer << " is undefined at stage " << name
            << ", which sources from stage " << src.name << "\n";
    }

    void Pipeline::Stage::applyTexture(osg::Texture* tex, const std::string& buffer, int u)
    {
        osg::StateSet* ss = deferred ?
            runner->geometry->getOrCreateStateSet() : camera->getOrCreateStateSet();
        ss->setTextureAttributeAndModes(u, tex);
        ss->addUniform(new osg::Uniform(buffer.data(), u));
    }

    void Pipeline::Stage::applyDefaultTexture(const osg::Vec4& color, const std::string& buffer, int u)
    {
        osg::StateSet* ss = deferred ?
            runner->geometry->getOrCreateStateSet() : camera->getOrCreateStateSet();
        ss->setTextureAttributeAndModes(u, createDefaultTexture(color));
        ss->addUniform(new osg::Uniform(buffer.data(), u));
    }

    osg::StateSet::UniformList Pipeline::Stage::getUniforms() const
    {
        osg::StateSet* ss = deferred ?
            runner->geometry->getOrCreateStateSet() : camera->getOrCreateStateSet();
        return ss->getUniformList();
    }

    osg::Uniform* Pipeline::Stage::getUniform(const std::string& name) const
    {
        osg::StateSet* ss = deferred ?
            runner->geometry->getOrCreateStateSet() : camera->getOrCreateStateSet();
        return ss->getUniform(name);
    }

    osg::Texture* Pipeline::Stage::getTexture(const std::string& name) const
    {
        osg::StateSet* ss = deferred ?
            runner->geometry->getOrCreateStateSet() : camera->getOrCreateStateSet();
        osg::Uniform* samplerU = ss->getUniform(name); if (!samplerU) return NULL;

        int u = -1; if (!samplerU->get(u)) return NULL;
        return static_cast<osg::Texture*>(
            ss->getTextureAttribute(u, osg::StateAttribute::TEXTURE));
    }

    Pipeline::Stage* Pipeline::getStage(const std::string& name)
    {
        for (size_t i = 0; i < _stages.size(); ++i)
        { if (_stages[i]->name == name) return _stages[i].get(); }
        return NULL;
    }

    void Pipeline::startStages(int w, int h, osg::GraphicsContext* gc)
    {
        if (_glVersionData.valid())
        {
            if (_glVersionData->glVersion < _glTargetVersion)
                _glTargetVersion = _glVersionData->glVersion;
            if (_glVersionData->glslVersion < _glslTargetVersion)
                _glslTargetVersion = _glVersionData->glslVersion;
        }

        if (gc)
        {
            osgViewer::GraphicsWindow* gw = dynamic_cast<osgViewer::GraphicsWindow*>(gc);
            int x, y; if (gw) gw->getWindowRectangle(x, y, w, h);
            _stageContext = gc;  // FIXME: share or replace GC?
        }
        else
        {
            int m0 = _glTargetVersion / 100; int m1 = (_glTargetVersion - m0 * 100) / 10;
            std::string glContext = std::to_string(m0) + "." + std::to_string(m1);
            _stageContext = createGraphicsContext(w, h, glContext, NULL);
        }
        _stageSize = osg::Vec2s(w, h);
        _stageContext->setResizedCallback(new MyResizedCallback(this));

        // Enable the osg_* uniforms that the shaders will use in GL3/GL4 and GLES2
        if (_glTargetVersion >= 300 || _glslTargetVersion >= 140)
        {
            _stageContext->getState()->setUseModelViewAndProjectionUniforms(true);
            _stageContext->getState()->setUseVertexAttributeAliasing(true);
        }
    }

    void Pipeline::clearStagesFromView(osgViewer::View* view)
    {
        view->getCamera()->setGraphicsContext(_stageContext.get());
        for (unsigned int i = 0; i < view->getNumSlaves(); ++i)
            view->getSlave(i)._camera->setStats(NULL);

        while (view->getNumSlaves() > 0) view->removeSlave(0);
        _stages.clear(); _modules.clear(); _forwardCamera = NULL;
        if (_deferredCallback.valid())
        {
            _deferredCallback->getRunners().clear();
            _deferredCallback->setClampCallback(NULL);
        }
    }

    void Pipeline::applyStagesToView(osgViewer::View* view, unsigned int forwardMask)
    {
        if (view->getCamera() && view->getCamera()->getClampProjectionMatrixCallback())
        {
            view->getCamera()->setGraphicsContext(NULL);
            _deferredCallback->setClampCallback(view->getCamera()->getClampProjectionMatrixCallback());
        }

        for (unsigned int i = 0; i < _stages.size(); ++i)
        {
            bool useMainScene = _stages[i]->inputStage;
            if (_stages[i]->deferred || !_stages[i]->camera) continue;
            view->addSlave(_stages[i]->camera.get(), osg::Matrix(), osg::Matrix(), useMainScene);

#if false  // TEST ONLY
            _stages[i]->camera->setPreDrawCallback(new DebugDrawCallback("PRE"));
            _stages[i]->camera->setPostDrawCallback(new DebugDrawCallback("POST"));
#endif
        }

        osg::ref_ptr<osg::Camera> forwardCam = (view->getCamera() != NULL)
                                             ? new osg::Camera(*view->getCamera()) : new osg::Camera;
        forwardCam->setName("DefaultForward");
        forwardCam->setUserValue("NeedNearFarCalculation", true);
        forwardCam->setCullMask(forwardMask);
        forwardCam->setClampProjectionMatrixCallback(new MyClampProjectionCallback(_deferredCallback.get()));
        forwardCam->setComputeNearFarMode(g_nearFarMode);
        _deferredCallback->setup(forwardCam.get(), PRE_DRAW);

        forwardCam->setViewport(0, 0, _stageSize.x(), _stageSize.y());
        forwardCam->setGraphicsContext(_stageContext.get());
        forwardCam->getOrCreateStateSet()->addUniform(_deferredCallback->getNearFarUniform());
        forwardCam->getOrCreateStateSet()->addUniform(_invScreenResolution.get());

        if (!_stages.empty()) forwardCam->setClearMask(0);
        view->addSlave(forwardCam.get(), osg::Matrix(), osg::Matrix(), true);
        view->getCamera()->setViewport(0, 0, _stageSize.x(), _stageSize.y());
        view->getCamera()->setProjectionMatrixAsPerspective(
            30.0f, static_cast<double>(_stageSize.x()) / static_cast<double>(_stageSize.y()), 1.0f, 10000.0f);
        _forwardCamera = forwardCam;

#if VERSE_WINDOWS
        osgViewer::GraphicsWindowWin32* gw = static_cast<osgViewer::GraphicsWindowWin32*>(_stageContext.get());
        if (gw) ImmAssociateContext(gw->getHWND(), NULL);  // disable default IME
#endif
    }

    Pipeline::Stage* Pipeline::getStage(osg::Camera* camera)
    {
        for (size_t i = 0; i < _stages.size(); ++i)
        { if (_stages[i]->camera == camera) return _stages[i].get(); }
        return NULL;
    }

    osg::GraphicsOperation* Pipeline::createRenderer(osg::Camera* camera)
    {
        MyRenderer* render = new MyRenderer(camera);
        render->useCustomSceneViews(_deferredCallback.get());
        
        Pipeline::Stage* stage = getStage(camera);
        if (!stage || (stage && stage->inputStage))
            camera->setStats(new osg::Stats("Camera"));
        return render;
    }
    
    Pipeline::Stage* Pipeline::addInputStage(const std::string& name, unsigned int cullMask, int samples,
                                             osg::Shader* vs, osg::Shader* fs, int buffers, ...)
    {
        Stage* s = new Stage; s->deferred = false;
        va_list params; va_start(params, buffers);
        for (int i = 0; i < buffers; i ++)
        {
            std::string bufName = std::string(va_arg(params, const char*));
            BufferType type = (BufferType)va_arg(params, int); int ms = 0;
            osg::Camera::BufferComponent comp = (buffers == 1) ? osg::Camera::COLOR_BUFFER
                                              : (osg::Camera::BufferComponent)(osg::Camera::COLOR_BUFFER0 + i);
            if (type == DEPTH24_STENCIL8) comp = osg::Camera::PACKED_DEPTH_STENCIL_BUFFER;
            else if (type >= DEPTH16) comp = osg::Camera::DEPTH_BUFFER;
            else ms = samples;

            osg::ref_ptr<osg::Texture> tex = createTexture(type, _stageSize[0], _stageSize[1], _glTargetVersion);
            if (i > 0) s->camera->attach(comp, tex.get(), 0, 0, false, ms);
            else s->camera = createRTTCamera(comp, tex.get(), _stageContext.get(), false);
            s->outputs[bufName] = tex.get();
        }
        va_end(params);

        applyDefaultStageData(*s, name, vs, fs);
        applyDefaultInputStateSet(s->camera->getOrCreateStateSet());
        s->camera->setCullMask(cullMask);
        s->camera->setUserValue("NeedNearFarCalculation", true);
        s->camera->setClampProjectionMatrixCallback(new MyClampProjectionCallback(_deferredCallback.get()));
        s->camera->setComputeNearFarMode(g_nearFarMode);
        s->inputStage = true; _stages.push_back(s);
        return s;
    }

    Pipeline::Stage* Pipeline::addWorkStage(const std::string& name, float sizeScale,
                                            osg::Shader* vs, osg::Shader* fs, int buffers, ...)
    {
        Stage* s = new Stage; s->deferred = false;
        va_list params; va_start(params, buffers);
        for (int i = 0; i < buffers; i++)
        {
            std::string bufName = std::string(va_arg(params, const char*));
            BufferType type = (BufferType)va_arg(params, int);
            osg::Camera::BufferComponent comp = (buffers == 1) ? osg::Camera::COLOR_BUFFER
                                              : (osg::Camera::BufferComponent)(osg::Camera::COLOR_BUFFER0 + i);
            if (type == DEPTH24_STENCIL8) comp = osg::Camera::PACKED_DEPTH_STENCIL_BUFFER;
            else if (type >= DEPTH16) comp = osg::Camera::DEPTH_BUFFER;

            osg::ref_ptr<osg::Texture> tex = createTexture(type,
                osg::maximum((int)_stageSize[0], 1920) * sizeScale,   // deferred quad not too low
                osg::maximum((int)_stageSize[1], 1080) * sizeScale, _glTargetVersion);
            if (i > 0) s->camera->attach(comp, tex.get());
            else s->camera = createRTTCamera(comp, tex.get(), _stageContext.get(), true);
            s->outputs[bufName] = tex.get();
        }
        va_end(params);

        applyDefaultStageData(*s, name, vs, fs);
        s->camera->getOrCreateStateSet()->setAttributeAndModes(_deferredDepth.get());
        s->inputStage = false; _stages.push_back(s);
        return s;
    }

    Pipeline::Stage* Pipeline::addDeferredStage(const std::string& name, float sizeScale, bool runOnce,
                                                osg::Shader* vs, osg::Shader* fs, int buffers, ...)
    {
        Stage* s = new Stage; s->deferred = true;
        s->runner = new osgVerse::DeferredRenderCallback::RttGeometryRunner(name);
        s->runner->runOnce = runOnce; s->runner->setUseScreenQuad(0, NULL);  // quad at the beginning
        _deferredCallback->addRunner(s->runner.get());

        va_list params; va_start(params, buffers);
        for (int i = 0; i < buffers; i++)
        {
            std::string bufName = std::string(va_arg(params, const char*));
            BufferType type = (BufferType)va_arg(params, int);
            osg::Camera::BufferComponent comp = (buffers == 1) ? osg::Camera::COLOR_BUFFER
                : (osg::Camera::BufferComponent)(osg::Camera::COLOR_BUFFER0 + i);
            if (type == DEPTH24_STENCIL8) comp = osg::Camera::PACKED_DEPTH_STENCIL_BUFFER;
            else if (type >= DEPTH16) comp = osg::Camera::DEPTH_BUFFER;

            osg::ref_ptr<osg::Texture> tex = createTexture(type,
                osg::maximum((int)_stageSize[0], 1920) * sizeScale,   // deferred quad not too low
                osg::maximum((int)_stageSize[1], 1080) * sizeScale, _glTargetVersion);
            s->runner->attach(comp, tex.get());
            s->outputs[bufName] = tex.get();
        }
        va_end(params);

        applyDefaultStageData(*s, name, vs, fs);
        s->inputStage = false; _stages.push_back(s);
        return s;
    }

    Pipeline::Stage* Pipeline::addDisplayStage(const std::string& name,
                                               osg::Shader* vs, osg::Shader* fs, const osg::Vec4& geom)
    {
        Stage* s = new Stage; s->deferred = false;
        s->camera = createHUDCamera(_stageContext.get(), _stageSize[0], _stageSize[1],
                                    osg::Vec3(geom[0], geom[1], 0.0f), geom[2], geom[3], true);
        applyDefaultStageData(*s, name, vs, fs);
        //s->camera->setClearColor(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
        s->camera->getOrCreateStateSet()->setAttributeAndModes(_deferredDepth.get());
        s->inputStage = false; _stages.push_back(s);
        return s;
    }

    void Pipeline::removeModule(osg::NodeCallback* cb)
    {
        for (std::map<std::string, osg::ref_ptr<osg::NodeCallback>>::iterator itr = _modules.begin();
             itr != _modules.end(); ++itr)
        { if (itr->second == cb) { _modules.erase(itr); return; } }
    }

    void Pipeline::activateDeferredStage(const std::string& n, bool b)
    { Stage* s = getStage(n); if (s->runner.valid()) s->runner->active = b; }

    void Pipeline::applyDefaultStageData(Stage& s, const std::string& name, osg::Shader* vs, osg::Shader* fs)
    {
        if (vs || fs)
        {
            osg::ref_ptr<osg::Program> prog = new osg::Program;
            prog->setName(name + "_PROGRAM");
            if (vs)
            {
                vs->setName(name + "_SHADER_VS"); prog->addShader(vs);
                createShaderDefinitions(vs, _glTargetVersion, _glslTargetVersion);
            }

            if (fs)
            {
                fs->setName(name + "_SHADER_FS"); prog->addShader(fs);
                createShaderDefinitions(fs, _glTargetVersion, _glslTargetVersion);
            }

            osg::StateSet* ss = s.deferred ?
                s.runner->geometry->getOrCreateStateSet() : s.camera->getOrCreateStateSet();
            ss->setAttributeAndModes(prog.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            ss->addUniform(_deferredCallback->getNearFarUniform());
            ss->addUniform(_invScreenResolution.get());
        }
        s.name = name; if (!s.deferred) s.camera->setName(name);
    }

    void Pipeline::applyDefaultInputStateSet(osg::StateSet* ss)
    {
        static osg::ref_ptr<osg::Texture2D> tex0 = createDefaultTexture(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
        static osg::ref_ptr<osg::Texture2D> tex1 = createDefaultTexture(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));

        ss->setMode(GL_BLEND, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
        ss->setTextureAttributeAndModes(0, tex1.get());  // DiffuseMap
        ss->setTextureAttributeAndModes(1, tex0.get());  // NormalMap
        ss->setTextureAttributeAndModes(2, tex1.get());  // SpecularMap
        ss->setTextureAttributeAndModes(3, tex0.get());  // ShininessMap
        ss->setTextureAttributeAndModes(4, tex0.get());  // AmbientMap
        ss->setTextureAttributeAndModes(5, tex0.get());  // EmissiveMap
        ss->setTextureAttributeAndModes(6, tex0.get());  // ReflectionMap
        for (int i = 0; i < 7; ++i) ss->addUniform(new osg::Uniform(uniformNames[i].c_str(), i));
        ss->addUniform(new osg::Uniform("ModelIndicator", 0.0f));

        osg::Program* prog = static_cast<osg::Program*>(ss->getAttribute(osg::StateAttribute::PROGRAM));
        if (prog != NULL)
        {
            prog->addBindAttribLocation(attributeNames[6], 6);
            prog->addBindAttribLocation(attributeNames[7], 7);
        }
    }

    void Pipeline::setModelIndicator(osg::Node* node, IndicatorType type)
    {
        if (!node) return; osg::StateSet* ss = node->getOrCreateStateSet();
        osg::Uniform* u = ss->getOrCreateUniform("ModelIndicator", osg::Uniform::FLOAT);
        if (u) u->set((float)type);
    }

    void Pipeline::createShaderDefinitions(osg::Shader* s, int glVer, int glslVer,
                                           const std::vector<std::string>& userDefs)
    {
        std::vector<std::string> extraDefs;
        std::string source = s->getShaderSource();
        if (source.find("#version") != std::string::npos) return;

        std::string m_mvp = "gl_ModelViewProjectionMatrix", m_mv = "gl_ModelViewMatrix";
        std::string m_p = "gl_ProjectionMatrix", m_n = "gl_NormalMatrix";
        std::string tex1d = "texture", tex2d = "texture", tex3d = "texture", texCube = "texture";
        std::string vin = "in", vout = "out", fin = "in", fout = "out", finalColor = "//";
        if (glslVer <= 120)
        {
            tex1d = "texture1D"; tex2d = "texture2D"; tex3d = "texture3D"; texCube = "textureCube";
            vin = "attribute"; vout = "varying"; fin = "varying"; fout = "";
            finalColor = "gl_FragColor = ";

            extraDefs.push_back("float round(float v) { return v<0.0 ? ceil(v-0.5) : floor(v+0.5); }");
            extraDefs.push_back("vec2 round(vec2 v) { return vec2(round(v.x), round(v.y)); }");
            extraDefs.push_back("vec3 round(vec3 v) { return vec3(round(v.x), round(v.y), round(v.z)); }");
            extraDefs.push_back("vec4 textureLod(sampler2D t, vec2 uv, float l) { return texture2D(t, uv); }");
        }

        if (s->getType() == osg::Shader::VERTEX)
        {
            if (glVer >= 300 || glslVer >= 140)
            {
                m_mvp = "osg_ModelViewProjectionMatrix"; m_mv = "osg_ModelViewMatrix";
                m_p = "osg_ProjectionMatrix"; m_n = "osg_NormalMatrix";
                extraDefs.push_back("uniform mat4 osg_ModelViewProjectionMatrix, "
                                    "osg_ModelViewMatrix, osg_ProjectionMatrix;");
                extraDefs.push_back("uniform mat3 osg_NormalMatrix;");
                extraDefs.push_back("VERSE_VS_IN vec4 osg_Vertex, osg_Color, "
                                    "osg_MultiTexCoord0, osg_MultiTexCoord1;");
                extraDefs.push_back("VERSE_VS_IN vec3 osg_Normal;");
            }
            else
            {
                extraDefs.push_back("#define osg_Vertex gl_Vertex");
                extraDefs.push_back("#define osg_Color gl_Color");
                extraDefs.push_back("#define osg_MultiTexCoord0 gl_MultiTexCoord0");
                extraDefs.push_back("#define osg_MultiTexCoord1 gl_MultiTexCoord1");
                extraDefs.push_back("#define osg_Normal gl_Normal");
            }
        }

        std::stringstream ss;
        ss << "#version " << glslVer << std::endl;
        if (s->getType() == osg::Shader::VERTEX)
        {
            ss << "#define VERSE_MATRIX_MVP " << m_mvp << std::endl;
            ss << "#define VERSE_MATRIX_MV " << m_mv << std::endl;
            ss << "#define VERSE_MATRIX_P " << m_p << std::endl;
            ss << "#define VERSE_MATRIX_N " << m_n << std::endl;
            ss << "#define VERSE_VS_IN " << vin << std::endl;
            ss << "#define VERSE_VS_OUT " << vout << std::endl;
        }
        else if (s->getType() == osg::Shader::FRAGMENT)
        {
            ss << "#define VERSE_FS_IN " << fin << std::endl;
            ss << "#define VERSE_FS_OUT " << fout << std::endl;
            ss << "#define VERSE_FS_FINAL " << finalColor << std::endl;
        }
        ss << "#define VERSE_TEX1D " << tex1d << std::endl;
        ss << "#define VERSE_TEX2D " << tex2d << std::endl;
        ss << "#define VERSE_TEX3D " << tex3d << std::endl;
        ss << "#define VERSE_TEXCUBE " << texCube << std::endl;

        for (size_t i = 0; i < extraDefs.size(); ++i) ss << extraDefs[i] << std::endl;
        for (size_t i = 0; i < userDefs.size(); ++i) ss << userDefs[i] << std::endl;
        s->setShaderSource(ss.str() + source);
    }

    osg::Texture* Pipeline::createTexture(BufferType type, int w, int h, int glVer)
    {
        osg::ref_ptr<osg::Texture2D> tex = new osg::Texture2D;
        switch (type)
        {
        case RGB_INT8:
            tex->setInternalFormat(GL_RGB8);
            tex->setSourceFormat(GL_RGB);
            tex->setSourceType(GL_UNSIGNED_BYTE);
            break;
        case RGB_INT5:
            tex->setInternalFormat(GL_RGB5);
            tex->setSourceFormat(GL_RGBA);
            tex->setSourceType(GL_UNSIGNED_SHORT_5_5_5_1);
            break;
        case RGB_INT10:
            tex->setInternalFormat(GL_RGB10);
            tex->setSourceFormat(GL_RGB);
            tex->setSourceType(GL_UNSIGNED_INT_10_10_10_2);
            break;
        case RGB_FLOAT16:
            tex->setInternalFormat(GL_RGB16F_ARB);
            tex->setSourceFormat(GL_RGB);
            tex->setSourceType(GL_HALF_FLOAT);
            break;
        case RGB_FLOAT32:
            tex->setInternalFormat(GL_RGB32F_ARB);
            tex->setSourceFormat(GL_RGB);
            tex->setSourceType(GL_FLOAT);
            break;
        case SRGB_INT8:
            tex->setInternalFormat(GL_SRGB8);
            tex->setSourceFormat(GL_RGB);
            tex->setSourceType(GL_UNSIGNED_BYTE);
            break;
        case RGBA_INT8:
            tex->setInternalFormat(GL_RGBA8);
            tex->setSourceFormat(GL_RGBA);
            tex->setSourceType(GL_UNSIGNED_BYTE);
            break;
        case RGBA_INT5_1:
            tex->setInternalFormat(GL_RGB5_A1);
            tex->setSourceFormat(GL_RGBA);
            tex->setSourceType(GL_UNSIGNED_SHORT_5_5_5_1);
            break;
        case RGBA_INT10_2:
            tex->setInternalFormat(GL_RGB10_A2);
            tex->setSourceFormat(GL_RGBA);
            tex->setSourceType(GL_UNSIGNED_INT_10_10_10_2);
            break;
        case RGBA_FLOAT16:
            tex->setInternalFormat(GL_RGBA16F_ARB);
            tex->setSourceFormat(GL_RGBA);
            tex->setSourceType(GL_HALF_FLOAT);
            break;
        case RGBA_FLOAT32:
            tex->setInternalFormat(GL_RGBA32F_ARB);
            tex->setSourceFormat(GL_RGBA);
            tex->setSourceType(GL_FLOAT);
            break;
        case SRGBA_INT8:
            tex->setInternalFormat(GL_SRGB8_ALPHA8);
            tex->setSourceFormat(GL_RGBA);
            tex->setSourceType(GL_UNSIGNED_BYTE);
            break;
        case R_INT8:
            if (glVer > 0 && glVer < 300)
            {
                tex->setInternalFormat(GL_LUMINANCE8);
                tex->setSourceFormat(GL_LUMINANCE);
            }
            else
            {
                tex->setInternalFormat(GL_R8);
                tex->setSourceFormat(GL_RED);
            }
            tex->setSourceType(GL_UNSIGNED_BYTE);
            break;
        case R_FLOAT16:
            if (glVer > 0 && glVer < 300)
            {
                tex->setInternalFormat(GL_LUMINANCE16F_ARB);
                tex->setSourceFormat(GL_LUMINANCE);
            }
            else
            {
                tex->setInternalFormat(GL_R16F);
                tex->setSourceFormat(GL_RED);
            }
            tex->setSourceType(GL_HALF_FLOAT);
            break;
        case R_FLOAT32:
            if (glVer > 0 && glVer < 300)
            {
                tex->setInternalFormat(GL_LUMINANCE32F_ARB);
                tex->setSourceFormat(GL_LUMINANCE);
            }
            else
            {
                tex->setInternalFormat(GL_R32F);
                tex->setSourceFormat(GL_RED);
            }
            tex->setSourceType(GL_FLOAT);
            break;
        case RG_INT8:
            if (glVer > 0 && glVer < 300)
            {
                tex->setInternalFormat(GL_LUMINANCE8_ALPHA8);
                tex->setSourceFormat(GL_LUMINANCE_ALPHA);
            }
            else
            {
                tex->setInternalFormat(GL_RG8);
                tex->setSourceFormat(GL_RG);
            }
            tex->setSourceType(GL_UNSIGNED_BYTE);
            break;
        case RG_FLOAT16:
            if (glVer > 0 && glVer < 300)
            {
                tex->setInternalFormat(GL_LUMINANCE_ALPHA16F_ARB);
                tex->setSourceFormat(GL_LUMINANCE_ALPHA);
            }
            else
            {
                tex->setInternalFormat(GL_RG16F);
                tex->setSourceFormat(GL_RG);
            }
            tex->setSourceType(GL_HALF_FLOAT);
            break;
        case RG_FLOAT32:
            if (glVer > 0 && glVer < 300)
            {
                tex->setInternalFormat(GL_LUMINANCE_ALPHA32F_ARB);
                tex->setSourceFormat(GL_LUMINANCE_ALPHA);
            }
            else
            {
                tex->setInternalFormat(GL_RG32F);
                tex->setSourceFormat(GL_RG);
            }
            tex->setSourceType(GL_FLOAT);
            break;
        case DEPTH16:
            tex->setInternalFormat(GL_DEPTH_COMPONENT16);
            tex->setSourceFormat(GL_DEPTH_COMPONENT);
            tex->setSourceType(GL_HALF_FLOAT);
            break;
        case DEPTH24_STENCIL8:
            tex->setInternalFormat(GL_DEPTH24_STENCIL8_EXT);
            tex->setSourceFormat(GL_DEPTH_STENCIL_EXT);
            tex->setSourceType(GL_UNSIGNED_INT_24_8_EXT);
            break;
        case DEPTH32:
            tex->setInternalFormat(GL_DEPTH_COMPONENT32);
            tex->setSourceFormat(GL_DEPTH_COMPONENT);
            tex->setSourceType(GL_FLOAT);
            break;
        }

        tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        tex->setTextureSize(w, h);
        return tex.release();
    }
}

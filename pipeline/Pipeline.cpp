#include <osg/io_utils>
#include <osg/Version>
#include <osg/ValueObject>
#include <osg/Depth>
#include <osg/Billboard>
#include <osgDB/ReadFile>
#include <osgUtil/RenderStage>
#include <osgViewer/Renderer>
#include <iostream>
#include <sstream>
#include <stdarg.h>
#include "ShaderLibrary.h"
#include "Pipeline.h"
#include "ShadowModule.h"
#include "UserInputModule.h"
#include "ImageCheck.h"
#include "Utilities.h"

#ifndef GL_DEPTH32F_STENCIL8
#   define GL_DEPTH32F_STENCIL8              0x8CAD
#   define GL_FLOAT_32_UNSIGNED_INT_24_8_REV 0x8DAD
#endif

#define VERBOSE_CREATING 0
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
            // Work with near/far values to implement depth-partition here
            if (_stage.valid() && _stage->depthPartition.x() > 0.0)
            {
                double nearData = _stage->depthPartition.y(); if (nearData <= 0.0) nearData = 0.1;
                if ((int)_stage->depthPartition.x() == 1)  // front frustum
                    nearFar.set(nearData, sqrt(nearData * nearFar[1]));
                else
                    nearFar.set(sqrt(nearData * nearFar[1]), nearFar[1]);
            }

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

    MyClampProjectionCallback(osgVerse::Pipeline::Stage* s, osgVerse::DeferredRenderCallback* cb)
        : _stage(s), _callback(cb) {}
    osg::observer_ptr<osgVerse::Pipeline::Stage> _stage;
    osg::observer_ptr<osgVerse::DeferredRenderCallback> _callback;
};

class MyCullVisitor : public osgUtil::CullVisitor
{
public:
    MyCullVisitor()
    :   osgUtil::CullVisitor(), _cullMask(0xffffffff), _defaultMask(0xffffffff), _valid(0) {}
    MyCullVisitor(const MyCullVisitor& v)
    :   osgUtil::CullVisitor(v), _callback(v._callback), _shadowData(v._shadowData),
        _shadowViewport(v._shadowViewport), _pipelineMaskPath(v._pipelineMaskPath),
        _shadowModelViews(v._shadowModelViews), _shadowProjections(v._shadowProjections),
        _pixelSizeVectorList(v._pixelSizeVectorList), _cullMask(v._cullMask),
        _defaultMask(v._defaultMask), _valid(v._valid) {}

    virtual CullVisitor* clone() const { return new MyCullVisitor(*this); }
    void setDeferredCallback(osgVerse::DeferredRenderCallback* cb) { _callback = cb; }
    osgVerse::DeferredRenderCallback* getDeferredCallback() { return _callback.get(); }

    struct PassableData
    {
        PassableData() : maskSet(0) {}
        osg::ref_ptr<osg::StateSet> stateSet;
        int maskSet;
    };

    virtual void reset()
    {
        _cullMask = 0xffffffff; _pipelineMaskPath.clear(); _shadowData = NULL;
        if (_callback.valid()) _defaultMask = _callback->getForwardMask();

        osg::Camera* cam = this->getCurrentCamera();
        if (cam && cam->getUserDataContainer() != NULL)
            cam->getUserValue("PipelineCullMask", _cullMask);
        if (cam && cam->getUserData() != NULL)
        {
            osgVerse::ShadowModule::ShadowData* sd =
                dynamic_cast<osgVerse::ShadowModule::ShadowData*>(cam->getUserData());
            if (sd && sd->smallPixels > 0)
            {
                if (!_shadowModelViews.empty()) _shadowModelViews.clear();
                if (!_shadowProjections.empty()) _shadowProjections.clear();
                _shadowModelViews.push_back(sd->viewMatrix);
                _shadowProjections.push_back(sd->projMatrix);
                _shadowViewport = sd->_viewport.get(); _shadowData = sd;

                if (!_pixelSizeVectorList.empty()) _pixelSizeVectorList.clear();
                computeShadowPixelSizeVector();
            }
        }

#if false
        OSG_NOTICE << "F-" << (getFrameStamp() != NULL ? getFrameStamp()->getFrameNumber() : -1)
                   << (getUserData() != NULL ? " (COMPUTING NEAR/FAR): " : ": ")
                   << "Stage = " << (cam != NULL ? cam->getName() : "(null)") << std::endl;
#endif
        _valid = (cam != NULL) ? 1 : 0;
        osgUtil::CullVisitor::reset();
    }

    bool passable(osg::Node& node, PassableData& pdata)
    {
        pdata.maskSet = 0; pushM(node, pdata);
        if (!_valid) return false;  // The visitor is not ready...
        if (this->getUserData() != NULL) return true;  // computing near/far mode
        if (node.getUserDataContainer() != NULL)
        {
            // Use this to replace nodemasks while checking deferred/forward graphs
            unsigned int nodePipMask = 0xffffffff, flags = 0;
            if (node.getUserValue("PipelineMask", nodePipMask))
            {
                node.getUserValue("PipelineFlags", flags);
                if (!_pipelineMaskPath.empty())
                {
                    std::pair<unsigned int, unsigned int> lastM = _pipelineMaskPath.back();
                    if (lastM.second & osg::StateAttribute::OVERRIDE)
                    {
                        if (!(flags & osg::StateAttribute::PROTECTED))
                        { nodePipMask = lastM.first; flags = lastM.second; }
                    }
                }

                if (flags & osg::StateAttribute::ON)
                {
                    pushMaskPath(nodePipMask, flags); pdata.maskSet |= 1;
                    if ((_cullMask & nodePipMask) != 0)
                        return !checkSmallPixelSizeCulling(node.getBound());
                    return false;
                }  // otherwise, treat the mask as not set
            }
        }

        if (checkSmallPixelSizeCulling(node.getBound())) return false;
        if (!_pipelineMaskPath.empty())
        {
            std::pair<unsigned int, unsigned int> maskAndFlags = _pipelineMaskPath.back();
            return (_cullMask & maskAndFlags.first) != 0;
        }
        return true;
    }

    bool passable(osg::Drawable& node, PassableData& pdata)
    {
        unsigned int nodePipMask = 0xffffffff, flags = 0;
        pdata.maskSet = 0; pushM(node, pdata);
        if (!_valid) return false;  // The visitor is not ready...
        if (this->getUserData() != NULL) return true;  // computing near/far mode
        if (node.getUserValue("PipelineMask", nodePipMask))
        {
            node.getUserValue("PipelineFlags", flags);
            if (!_pipelineMaskPath.empty())
            {
                std::pair<unsigned int, unsigned int> lastM = _pipelineMaskPath.back();
                if (lastM.second & osg::StateAttribute::OVERRIDE)
                {
                    if (!(flags & osg::StateAttribute::PROTECTED))
                    { nodePipMask = lastM.first; flags = lastM.second; }
                }
            }

            if (flags & osg::StateAttribute::ON)
            {
                if ((_cullMask & nodePipMask) != 0)
                    return !checkSmallPixelSizeCulling(node.getBound());
                return false;
            }
        }

        if (checkSmallPixelSizeCulling(node.getBound())) return false;
        if (_pipelineMaskPath.empty())
        {
            // Handle drawables which is never been set pipeline masks:
            // if pipeline mask is never set, we will treat current node as forward one
            // to avoid it being rendered multiple times.
            return (_cullMask & _defaultMask) != 0;
        }
        return (_cullMask & _pipelineMaskPath.back().first) != 0;
    }

    virtual void apply(osg::Node& node)
    { PassableData s; if (passable(node, s)) osgUtil::CullVisitor::apply(node); popM(node, s); }

    virtual void apply(osg::Group& node)
    { PassableData s; if (passable(node, s)) osgUtil::CullVisitor::apply(node); popM(node, s); }

    virtual void apply(osg::Transform& node)
    {
        PassableData s;
        if (passable(node, s))
        {
            pushModelViewMatrixInShadow(node);
            osgUtil::CullVisitor::apply(node);
            popModelViewMatrixInShadow();
        }
        popM(node, s);
    }

    virtual void apply(osg::Projection& node)
    {
        PassableData s;
        if (passable(node, s))
        {
            pushProjectionMatrixInShadow(node);
            osgUtil::CullVisitor::apply(node);
            popProjectionMatrixInShadow();
        }
        popM(node, s);
    }

    virtual void apply(osg::Switch& node)
    { PassableData s; if (passable(node, s)) osgUtil::CullVisitor::apply(node); popM(node, s); }

    virtual void apply(osg::LOD& node)
    { PassableData s; if (passable(node, s)) osgUtil::CullVisitor::apply(node); popM(node, s); }

    virtual void apply(osg::ClearNode& node)
    { PassableData s; if (passable(node, s)) osgUtil::CullVisitor::apply(node); popM(node, s); }

    virtual void apply(osg::Camera& node)
    { PassableData s; if (passable(node, s)) osgUtil::CullVisitor::apply(node); popM(node, s); }

    virtual void apply(osg::Billboard& node)
    { PassableData s; if (passable(node, s)) osgUtil::CullVisitor::apply(node); popM(node, s); }

#if OSG_VERSION_GREATER_THAN(3, 2, 3)
    virtual void apply(osg::Geode& node)
    { PassableData s; if (passable(node, s)) osgUtil::CullVisitor::apply(node); popM(node, s); }

    virtual void apply(osg::Drawable& drawable)
    {
        PassableData s;
        if (passable(drawable, s))
        {
#   if OSG_VERSION_GREATER_THAN(3, 5, 9)
            osg::RefMatrix& matrix = *getModelViewMatrix();
            const osg::BoundingBox& bb = drawable.getBoundingBox();
            if (drawable.getCullCallback())
            {
                osg::DrawableCullCallback* dcb = drawable.getCullCallback()->asDrawableCullCallback();
                if (dcb) { if (dcb->cull(this, &drawable, &_renderInfo) == true) {popM(drawable, s); return;} }
                else drawable.getCullCallback()->run(&drawable, this);
            }

            if (drawable.isCullingActive() && isCulled(bb)) { popM(drawable, s); return; }
            if (_computeNearFar && bb.valid())
                { if (!updateCalculatedNearFar(matrix, drawable, false)) {popM(drawable, s); return;} }

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
                OSG_NOTICE << "[CullVisitorEx] " << drawable.getName() << " detected NaN..."
                           << " Camera: " << getCurrentCamera()->getName() << ", ValidCenter: " << bb.center().valid()
                           << ", ValidModelView: " << matrix.valid() << std::endl;
            }
            else
                addDrawableAndDepth(&drawable, &matrix, depth);
            for (unsigned int i = 0; i < numPopStateSetRequired; ++i) { popStateSet(); }
#   else
            osgUtil::CullVisitor::apply(drawable);
#   endif
        }
        popM(drawable, s);
    }
#else
    virtual void apply(osg::Geode& node)
    {
        class DisableDrawableCallbackInternal : public osg::Drawable::CullCallback
        {
        public:
            virtual bool cull(osg::NodeVisitor*, osg::Drawable* drawable, osg::State*) const
            { return true; }
        };

        PassableData pipelineMaskSet;
        if (passable(node, pipelineMaskSet))
        {
            typedef std::pair<osg::observer_ptr<osg::Drawable>,
                              osg::ref_ptr<osg::Drawable::CullCallback>> DrawablePair;
            std::vector<DrawablePair> drawablesToHide;
            for (unsigned int i = 0; i < node.getNumDrawables(); ++i)
            {
                PassableData drawMaskSet;
                osg::Drawable* drawable = node.getDrawable(i);
                if (!passable(*drawable, drawMaskSet))
                {
                    drawablesToHide.push_back(DrawablePair(drawable, drawable->getCullCallback()));
                    drawable->setCullCallback(new DisableDrawableCallbackInternal);
                }
                popM(*drawable, drawMaskSet);
            }

            osgUtil::CullVisitor::apply(node);
            if (!drawablesToHide.empty())
            {
                for (unsigned int i = 0; i < drawablesToHide.size(); ++i)
                {
                    DrawablePair& pair = drawablesToHide[i];
                    if (pair.first.valid()) pair.first->setCullCallback(pair.second);
                }
            }
        }
        popM(node, pipelineMaskSet);
    }
#endif

protected:
    void pushModelViewMatrixInShadow(osg::Transform& t)
    {
        osg::Matrix matrix; if (!_shadowData) return;
        if (!_shadowModelViews.empty()) matrix = _shadowModelViews.back();
        t.computeLocalToWorldMatrix(matrix, this);
        _shadowModelViews.push_back(matrix);
        computeShadowPixelSizeVector();
    }

    void pushProjectionMatrixInShadow(osg::Projection& p)
    {
        if (!_shadowData) return;
        _shadowProjections.push_back(p.getMatrix());
        computeShadowPixelSizeVector();
    }

    void popModelViewMatrixInShadow()
    { if (_shadowData.valid()) {_shadowModelViews.pop_back(); _pixelSizeVectorList.pop_back();} }

    void popProjectionMatrixInShadow()
    { if (_shadowData.valid()) {_shadowProjections.pop_back(); _pixelSizeVectorList.pop_back();} }

    void computeShadowPixelSizeVector()
    {
        if (!_shadowData) return; osg::Vec4 empty;
        if (_shadowData->smallPixels < 1) _pixelSizeVectorList.push_back(empty);
        else if (!_shadowViewport)
            { OSG_WARN << "[CullVisitorEx] No valid viewport" << std::endl; _pixelSizeVectorList.push_back(empty); }
        else _pixelSizeVectorList.push_back(osg::CullingSet::computePixelSizeVector(
            *_shadowViewport, _shadowProjections.back(), _shadowModelViews.back()));
    }

    bool checkSmallPixelSizeCulling(const osg::BoundingSphere& bs) const
    {
        if (_shadowData.valid() && _shadowData->smallPixels > 0)
        {
            float ps = (bs.center() * _pixelSizeVectorList.back()) * (float)_shadowData->smallPixels;
            if (bs.radius() < ps) return true;  // small-pixels-culling of shadow camera
        }
        return false;
    }

    inline value_type distance(const osg::Vec3& coord, const osg::Matrix& matrix)
    {
        return -((value_type)coord[0] * (value_type)matrix(0, 2) +
                 (value_type)coord[1] * (value_type)matrix(1, 2) +
                 (value_type)coord[2] * (value_type)matrix(2, 2) + matrix(3, 2));
    }

    inline void pushMaskPath(unsigned int m, unsigned int f)
    { _pipelineMaskPath.push_back(std::pair<unsigned int, unsigned int>(m, f)); }

    template<typename T>
    inline void pushM(T& node, PassableData& pdata)
    {
        if (_shadowData.valid() && node.getStateSet())
        {
            if (!canDisableStateSet(*node.getStateSet())) return;
            pdata.stateSet = node.getStateSet(); node.setStateSet(NULL);
        }
    }

    template<typename T>
    inline void popM(T& node, PassableData& pdata)
    {
        if (_shadowData.valid() && pdata.stateSet.valid())
        { node.setStateSet(pdata.stateSet.get()); pdata.stateSet = NULL; }

        if (pdata.maskSet == 0) return;
        if (!_pipelineMaskPath.empty()) _pipelineMaskPath.pop_back();
    }

    bool canDisableStateSet(const osg::StateSet& ss) const
    {
        const osg::StateSet::TextureAttributeList& texAttrList = ss.getTextureAttributeList();
        for (size_t i = 0; i < texAttrList.size(); ++i)
        {
            const osg::StateSet::AttributeList& attr = texAttrList[i];
            for (osg::StateSet::AttributeList::const_iterator itr = attr.begin();
                 itr != attr.end(); ++itr)
            {
                osg::StateAttribute::Type t = itr->first.first;
                if (t != osg::StateAttribute::TEXTURE) continue;
                
                osg::Texture* tex = static_cast<osg::Texture*>(itr->second.first.get());
                if (tex && tex->getNumImages() > 0)
                {
                    for (size_t j = 0; j < tex->getNumImages(); ++j)
                    {
                        osg::Image* img = tex->getImage(j);
                        if (img && osgVerse::ImageHelper::hasAlpha(*img)) return false;
                    }
                }
            }
        }
        return true;
    }

    osg::observer_ptr<osgVerse::DeferredRenderCallback> _callback;
    osg::observer_ptr<osgVerse::ShadowModule::ShadowData> _shadowData;
    osg::observer_ptr<osg::Viewport> _shadowViewport;
    std::vector<std::pair<unsigned int, unsigned int>> _pipelineMaskPath;

    typedef std::vector<osg::Matrix> MatrixValueStack;
    MatrixValueStack _shadowModelViews, _shadowProjections;
    std::vector<osg::Vec4> _pixelSizeVectorList;
    unsigned int _cullMask, _defaultMask, _valid;
};

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

        // TODO!! add software-rasterizer for occlusion culling here
        // Drawable AABBs should be collect in culling stage above, and pixels computed here
        // Results will be check in customized CullVisitor then

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

    virtual void compile()
    {
        osgUtil::SceneView* sceneView = _sceneView[0].get();
#if defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
        osg::GLExtensions* ext = (sceneView == NULL) ? NULL : sceneView->getState()->get<osg::GLExtensions>();
        if (ext)
        {
#   if OSG_VERSION_GREATER_THAN(3, 6, 5)
            // Re-check some extensions as they may fail in GLES and other situations
            ext->isTextureLODBiasSupported = osg::isGLExtensionSupported(
                sceneView->getState()->getContextID(), "GL_EXT_texture_lod_bias");
#endif
        }
#endif
        osgViewer::Renderer::compile();
    }

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

#if true
        MyCullVisitor* cullVisitor = new MyCullVisitor;
        cullVisitor->setName("CullVisitor" + std::to_string(i));
        cullVisitor->setDeferredCallback(cb);
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

            // Check if camera is for shadowing or custom use
            osgVerse::UserInputModule::CustomData* cData =
                dynamic_cast<osgVerse::UserInputModule::CustomData*>(camera->getUserData());
            osgVerse::ShadowModule::ShadowData* sData =
                dynamic_cast<osgVerse::ShadowModule::ShadowData*>(camera->getUserData());
            bool isCustomCam = (cData != NULL), isShadowCam = (sData != NULL);

            osg::Viewport* viewport = camera->getViewport();
            if (viewport && (!rtt || inputCam) && !isShadowCam)
            {
                if (processedViewports.count(viewport) == 0)  // avoid processing a shared viewport twice
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

            if (isCustomCam && cData->bypassCamera.valid())
            {
                cData->bypassCamera->setViewport(0, 0, w, h);
#if OSG_VERSION_GREATER_THAN(3, 3, 6)
                cData->bypassCamera->dirtyAttachmentMap();
#endif
            }

            // if aspect ratio adjusted change the project matrix to suit.
            //if (aspectRatioChange == 1.0) continue;
            if (slave)
            {
                if (camera->getReferenceFrame() == osg::Transform::RELATIVE_RF)
                {
                    if (rtt) resizeAttachments(camera, w, h);
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
                    //switch (camera->getProjectionResizePolicy())
                    //{
                    //case (osg::Camera::HORIZONTAL):
                    //    camera->getProjectionMatrix() *= osg::Matrix::scale(1.0 / aspectRatioChange, 1.0, 1.0); break;
                    //case (osg::Camera::VERTICAL):
                    //    camera->getProjectionMatrix() *= osg::Matrix::scale(1.0, aspectRatioChange, 1.0); break;
                    //default: break;
                    //}
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
        }  // for (osg::GraphicsContext::Cameras::iterator itr ...

#if false
        std::cout << "Resizing graphics context to: " << w << "x" << h << "..." << std::endl;
        for (osg::GraphicsContext::Cameras::iterator itr = cameras.begin(); itr != cameras.end(); ++itr)
        {
            osg::Camera* camera = (*itr); osg::View* view = camera->getView();
            osg::View::Slave* slave = view ? view->findSlaveForCamera(camera) : 0;
            
            osg::Matrix proj = camera->getProjectionMatrix(); if (slave) proj *= slave->_projectionOffset;
            double fov, aspect, zn, zf; proj.getPerspective(fov, aspect, zn, zf);
            std::cout << "    " << camera->getName() << ": " << camera->getViewport()->width() << "x"
                      << camera->getViewport()->height() << "; FOV = " << fov << ", AspectRatio = " << aspect << std::endl;
        }
#endif
        
        osg::GraphicsContext::Traits* ncTraits = const_cast<osg::GraphicsContext::Traits*>(traits);
        ncTraits->x = x; ncTraits->y = y; ncTraits->width = w; ncTraits->height = h;
    }

    void resizeAttachments(osg::Camera* camera, int width, int height)
    {
        bool modified = false;
        osg::Camera::BufferAttachmentMap& bufferAttachments = camera->getBufferAttachmentMap();
        for (osg::Camera::BufferAttachmentMap::iterator itr = bufferAttachments.begin();
             itr != bufferAttachments.end(); ++itr)
        {
            osg::Camera::Attachment& attachment = itr->second;
            if (attachment._texture.valid())
            {
                {
                    osg::Texture1D* tex = dynamic_cast<osg::Texture1D*>(attachment._texture.get());
                    if (tex && (tex->getTextureWidth() != width))
                    { modified = true; tex->setTextureWidth(width); tex->dirtyTextureObject(); }
                }

                {
                    osg::Texture2D* tex = dynamic_cast<osg::Texture2D*>(attachment._texture.get());
                    if (tex && ((tex->getTextureWidth() != width) || (tex->getTextureHeight() != height)))
                    { modified = true; tex->setTextureSize(width, height); tex->dirtyTextureObject(); }
                }

                {
                    osg::Texture3D* tex = dynamic_cast<osg::Texture3D*>(attachment._texture.get());
                    if (tex && ((tex->getTextureWidth() != width) || (tex->getTextureHeight() != height)))
                    {
                        tex->setTextureSize(width, height, tex->getTextureDepth());
                        tex->dirtyTextureObject(); modified = true;
                    }
                }

                {
                    osg::Texture2DArray* tex = dynamic_cast<osg::Texture2DArray*>(attachment._texture.get());
                    if (tex && ((tex->getTextureWidth() != width) || (tex->getTextureHeight() != height)))
                    {
                        tex->setTextureSize(width, height, tex->getTextureDepth());
                        tex->dirtyTextureObject(); modified = true;
                    }
                }
            }

            if (attachment._image.valid() && (attachment._image->s() != width || attachment._image->s() != height))
            {
                osg::Image* image = attachment._image.get(); modified = true;
                image->allocateImage(width, height, image->r(),
                                     image->getPixelFormat(), image->getDataType(), image->getPacking());
            }
        }
#if OSG_VERSION_GREATER_THAN(3, 3, 6)
        if (modified) camera->dirtyAttachmentMap();
#endif
    }
};

namespace osgVerse
{
    Pipeline::Pipeline(int glContextVer, int glslVer)
    {
        _deferredCallback = new osgVerse::DeferredRenderCallback(true);
        _deferredDepth = new osg::Depth(osg::Depth::LESS, 0.0, 1.0, false);
        _invScreenResolution = new osg::Uniform(
            "InvScreenResolution", osg::Vec2(1.0f / 1920.0f, 1.0f / 1080.0f));
        _glContextVersion = glContextVer; _glVersion = 0;
        _glslTargetVersion = glslVer;
    }

    osg::GraphicsContext* Pipeline::createGraphicsContext(int w, int h, const std::string& glContext,
                                                          osg::GraphicsContext* shared, int flags)
    {
        osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
        traits->x = 0; traits->y = 0; traits->width = w; traits->height = h;
        traits->windowDecoration = false; traits->doubleBuffer = true;
        traits->sharedContext = shared; traits->vsync = true;
        traits->glContextVersion = glContext; traits->glContextFlags = flags;
        traits->readDISPLAY(); traits->setUndefinedScreenDetailsToDefaultScreen();
        return osg::GraphicsContext::createGraphicsContext(traits.get());
    }

    void Pipeline::Stage::applyUniform(osg::Uniform* u)
    {
        osg::StateSet* ss = deferred ?
            runner->geometry->getOrCreateStateSet() : camera->getOrCreateStateSet();
        osg::Uniform* u0 = ss->getUniform(u->getName());
        if (u0 != NULL) ss->removeUniform(u0); ss->addUniform(u);
#if VERBOSE_CREATING
        OSG_NOTICE << "  Uniform: " << u->getName() << std::endl;
#endif
    }

    void Pipeline::Stage::applyBuffer(const std::string& name, int unit, Pipeline* p,
                                      int stageID, const std::string& buffer, osg::Texture::WrapMode wp)
    {
        if (stageID < 0 && p) stageID = p->getNumStages() - 2;  // last stage except me
        Stage* stage = (stageID >= 0 && p) ? p->getStage(stageID) : NULL;
        if (!stage) { OSG_WARN << "[Pipeline] invalid pipeline or stage not found\n"; return; }

        std::string bufferName = buffer;
        if (bufferName.empty() && !stage->outputs.empty())
        {
            if (stage->outputs.find(name) != stage->outputs.end()) bufferName = name;
            else bufferName = stage->outputs.begin()->first;
        }
        applyBuffer(*stage, bufferName, name, unit, wp);
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
#if VERBOSE_CREATING
            OSG_NOTICE << "  Buffer " << unit << ": " << buffer << " (" << n << ")" << std::endl;
#endif
        }
        else
            OSG_WARN << "[Pipeline] " << buffer << " is undefined at stage " << name
                     << ", which sources from stage " << src.name << "\n";
    }

    void Pipeline::Stage::applyTexture(osg::Texture* tex, const std::string& buffer, int u)
    {
        osg::StateSet* ss = deferred ?
            runner->geometry->getOrCreateStateSet() : camera->getOrCreateStateSet();
        ss->setTextureAttributeAndModes(u, tex);
        ss->addUniform(new osg::Uniform(buffer.data(), u));
#if VERBOSE_CREATING
        OSG_NOTICE << "  Texture " << u << ": " << buffer << std::endl;
#endif
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

    osg::Texture* Pipeline::Stage::getBufferTexture(osg::Camera::BufferComponent bc)
    {
        osg::Camera::BufferAttachmentMap* attMap = NULL;
        if (camera.valid()) attMap = &(camera->getBufferAttachmentMap());
        else if (runner.valid()) attMap = &(runner->attachments);

        if (attMap == NULL) return NULL;
        if (attMap->find(bc) == attMap->end())
        {
            if (bc == osg::Camera::COLOR_BUFFER) bc = osg::Camera::COLOR_BUFFER0;
            else if (bc == osg::Camera::DEPTH_BUFFER) bc = osg::Camera::PACKED_DEPTH_STENCIL_BUFFER;
            if (attMap->find(bc) == attMap->end()) return NULL;
        }
        return attMap->at(bc)._texture.get();
    }

    ScriptableProgram* Pipeline::Stage::getProgram()
    {
        if (!camera->getStateSet()) return NULL;
        return static_cast<ScriptableProgram*>(
            camera->getStateSet()->getAttribute(osg::StateAttribute::PROGRAM));
    }

    const ScriptableProgram* Pipeline::Stage::getProgram() const
    {
        if (!camera->getStateSet()) return NULL;
        return static_cast<ScriptableProgram*>(
            camera->getStateSet()->getAttribute(osg::StateAttribute::PROGRAM));
    }

    Pipeline::Stage* Pipeline::getStage(const std::string& name)
    {
        for (size_t i = 0; i < _stages.size(); ++i)
        { if (_stages[i]->name == name) return _stages[i].get(); }
        return NULL;
    }

    void Pipeline::setVersionData(GLVersionData* d)
    {
        _glVersionData = d;
        if (_glVersionData.valid())
        {
            _glVersion = _glVersionData->glVersion;
            if (_glContextVersion < _glVersionData->glVersion)
                _glContextVersion = _glVersionData->glVersion;
            if (_glslTargetVersion < _glVersionData->glslVersion)
                _glslTargetVersion = _glVersionData->glslVersion;
        }
    }

    void Pipeline::startStages(int w, int h, osg::GraphicsContext* gc)
    {
#if OSG_VERSION_GREATER_THAN(3, 6, 0)
        int suggestedVer = (int)(atof(OSG_GL_CONTEXT_VERSION) * 100.0);
        if (_glContextVersion < suggestedVer) _glContextVersion = suggestedVer;
#endif
        if (gc && gc->getTraits())
        {
            int userVer = (int)(atof(gc->getTraits()->glContextVersion.c_str()) * 100.0);
            if (_glContextVersion < userVer) _glContextVersion = userVer;
        }

#if defined(OSG_GL3_AVAILABLE)
        if (_glContextVersion < 300) _glContextVersion = 300;
        if (_glslTargetVersion < 330) _glslTargetVersion = 330;
#elif defined(OSG_GLES3_AVAILABLE)
        if (_glslTargetVersion < 300) _glslTargetVersion = 300;
#elif defined(OSG_GLES2_AVAILABLE)
        if (_glslTargetVersion < 100) _glslTargetVersion = 100;
#endif
        if (_glVersionData.valid())
        {
            OSG_NOTICE << "[Pipeline] OpenGL Driver: " << _glVersionData->version << "; GLSL: "
                       << _glVersionData->glslVersion << "; Renderer: " << _glVersionData->renderer << std::endl;
            OSG_NOTICE << "[Pipeline] Using OpenGL Context: " << getContextTargetVersion()
                       << "; Target GLSL Version: " << getGlslTargetVersion() << std::endl;
        }

        if (gc)
        {
            osgViewer::GraphicsWindow* gw = dynamic_cast<osgViewer::GraphicsWindow*>(gc);
            int x, y; if (gw) gw->getWindowRectangle(x, y, w, h);
            _stageContext = gc;  // FIXME: share or replace GC?
        }
        else
        {
            int m0 = _glContextVersion / 100; int m1 = (_glContextVersion - m0 * 100) / 10;
            std::string glContext = std::to_string(m0) + "." + std::to_string(m1);
            _stageContext = createGraphicsContext(w, h, glContext, NULL);
        }
        _stageSize = osg::Vec2s(w, h);
        _stageContext->setResizedCallback(new MyResizedCallback(this));

        // Enable the osg_* uniforms that the shaders will use in GL3/GL4 and GLES2/3
        if (_glContextVersion >= 300 || _glslTargetVersion >= 140)
        {
            _stageContext->getState()->setUseModelViewAndProjectionUniforms(true);
            _stageContext->getState()->setUseVertexAttributeAliasing(true);
        }
    }

    void Pipeline::clearStagesFromView(osgViewer::View* view, osg::Camera* mainCam)
    {
        std::vector<osg::Camera*> slavesToRemove;
        for (unsigned int i = 0; i < view->getNumSlaves(); ++i)
        {
            Stage* s = getStage(view->getSlave(i)._camera.get());
            if (s)
            {
                view->getSlave(i)._camera->setStats(NULL);
                slavesToRemove.push_back(view->getSlave(i)._camera.get());
            }
        }

        slavesToRemove.push_back(_forwardCamera.get());
        while (!slavesToRemove.empty())
        {
            osg::Camera* cam = slavesToRemove.back();
            for (unsigned int i = 0; i < view->getNumSlaves(); ++i)
            {
                if (view->getSlave(i)._camera == cam)
                { view->removeSlave(i); break; }
            }
            slavesToRemove.pop_back();
        }

        _stages.clear(); _modules.clear(); _forwardCamera = NULL;
        if (_deferredCallback.valid())
        {
            _deferredCallback->getRunners().clear();
            _deferredCallback->setClampCallback(NULL);
        }

        if (!mainCam) mainCam = view->getCamera();
        mainCam->setGraphicsContext(_stageContext.get());
        mainCam->setCullMask(0xffffffff);   // recover original slaves' displaying

        for (std::map<std::string, osg::ref_ptr<RenderingModuleBase>>::iterator itr = _modules.begin();
             itr != _modules.end(); ++itr)
        { mainCam->removeUpdateCallback(itr->second.get()); }

#if defined(VERSE_MSVC) && !defined(VERSE_NO_NATIVE_WINDOW)
        TextInputMethodManager::instance()->unbind();
#endif
    }

    void Pipeline::applyStagesToView(osgViewer::View* view, osg::Camera* mainCam, unsigned int defForwardMask)
    {
        osg::Matrix projOffset, viewOffset;
        double mainFov = 30.0, mainAspect = 1.0, mainNear = 1.0, mainFar = 10000.0;
        if (!mainCam) mainCam = view->getCamera();

        if (mainCam)
        {
            // Check if main camera is a slave and get its offsets
            for (unsigned int i = 0; i < view->getNumSlaves(); ++i)
            {
                osg::View::Slave& slave = view->getSlave(i);
                if (slave._camera == mainCam)
                {
                    projOffset = slave._projectionOffset;
                    viewOffset = slave._viewOffset;
                    mainCam->setCullMask(0);   // disable original slaves' displaying
                    // FIXME: it also disables skybox and forward scene?
                }
            }
            mainCam->setGraphicsContext(NULL);
            mainCam->getProjectionMatrixAsPerspective(mainFov, mainAspect, mainNear, mainFar);
        }

        // Set-up projection matrix clamper
        osg::ref_ptr<MyClampProjectionCallback> customClamper =
            new MyClampProjectionCallback(NULL, _deferredCallback.get());
        if (mainCam)
        {
            // User's ClampProjectionCallback on view's main camera will be kept and reused
            if (mainCam->getClampProjectionMatrixCallback())
                _deferredCallback->setClampCallback(mainCam->getClampProjectionMatrixCallback());
        }
        _deferredCallback->setForwardMask(defForwardMask);

        // Set-up stages as to add them as slaves
        int orderStart = -100;
        for (unsigned int i = 0; i < _stages.size(); ++i)
        {
            bool useMainScene = _stages[i]->inputStage;
            osg::Camera* camera = _stages[i]->camera.get();
            if (camera) camera->setRenderOrder(camera->getRenderOrder(), orderStart + (int)i);
            if (_stages[i]->deferred || !camera) continue;

            view->addSlave(_stages[i]->camera.get(), projOffset * _stages[i]->projectionOffset,
                           viewOffset * _stages[i]->viewOffset, useMainScene);
#if false  // TEST ONLY
            _stages[i]->camera->setPreDrawCallback(new DebugDrawCallback("PRE"));
            _stages[i]->camera->setPostDrawCallback(new DebugDrawCallback("POST"));
#endif
        }

        // The forward pass is kept for fixed-pipeline compatibility only
        osg::ref_ptr<osg::Camera> forwardCam = (mainCam != NULL)
                                             ? new osg::Camera(*mainCam) : new osg::Camera;
        forwardCam->setName("DefaultFixed");
        forwardCam->setRenderOrder(forwardCam->getRenderOrder(), orderStart + (int)_stages.size());
        forwardCam->setUserValue("NeedNearFarCalculation", true);
        forwardCam->setUserValue("PipelineCullMask", defForwardMask);  // replacing setCullMask()
        forwardCam->setClampProjectionMatrixCallback(customClamper.get());
        forwardCam->setComputeNearFarMode(g_nearFarMode);
        _deferredCallback->setup(forwardCam.get(), PRE_DRAW);

        forwardCam->setViewport(0, 0, _stageSize.x(), _stageSize.y());
        forwardCam->setGraphicsContext(_stageContext.get());
        forwardCam->getOrCreateStateSet()->addUniform(_deferredCallback->getNearFarUniform());
        forwardCam->getOrCreateStateSet()->addUniform(_invScreenResolution.get());
        _forwardCamera = forwardCam;

        if (!_stages.empty()) forwardCam->setClearMask(0);
        view->addSlave(forwardCam.get(), projOffset, viewOffset, true);
        mainCam->setViewport(0, 0, _stageSize.x(), _stageSize.y());
        mainCam->setProjectionMatrixAsPerspective(
            mainFov, static_cast<double>(_stageSize.x()) / static_cast<double>(_stageSize.y()), mainNear, mainFar);

#if defined(VERSE_MSVC) && !defined(VERSE_NO_NATIVE_WINDOW)
        TextInputMethodManager::instance()->disable(_stageContext.get());
#endif
    }

    Pipeline::Stage* Pipeline::getStage(osg::Camera* camera)
    {
        for (size_t i = 0; i < _stages.size(); ++i)
        { if (_stages[i]->camera == camera) return _stages[i].get(); }
        return NULL;
    }

    const Pipeline::Stage* Pipeline::getStage(osg::Camera* camera) const
    {
        for (size_t i = 0; i < _stages.size(); ++i)
        { if (_stages[i]->camera == camera) return _stages[i].get(); }
        return NULL;
    }

    osg::GraphicsOperation* Pipeline::createRenderer(osg::Camera* camera)
    {
        Pipeline::Stage* stage = getStage(camera);
        if (!stage || (stage && stage->inputStage))
            camera->setStats(new osg::Stats("Camera"));

        if (stage != NULL || camera == _forwardCamera.get())
        {
            MyRenderer* render = new MyRenderer(camera);
            render->useCustomSceneViews(_deferredCallback.get());
            return render;
        }
        else
        {
            OSG_NOTICE << "[Pipeline] Unregistered camera " << camera->getName() << std::endl;
            return new osgViewer::Renderer(camera);
        }
    }

#define ARGS_TO_BUFFERLIST(CMD) \
    BufferDescriptions bufferList; \
    va_list params; va_start(params, buffers); \
    for (int i = 0; i < buffers; i++) { \
        std::string bufName = std::string(va_arg(params, const char*)); \
        BufferType type = (BufferType)va_arg(params, int); \
        bufferList.push_back(BufferDescription(bufName, type)); \
    } va_end(params); return CMD;

    Pipeline::Stage* Pipeline::addInputStage(const std::string& name, unsigned int cullMask, int flags,
                                             osg::Shader* vs, osg::Shader* fs, int buffers, ...)
    { ARGS_TO_BUFFERLIST(addInputStage(name, cullMask, flags, vs, fs, bufferList)); }

    Pipeline::Stage* Pipeline::addWorkStage(const std::string& name, float sizeScale,
                                            osg::Shader* vs, osg::Shader* fs, int buffers, ...)
    { ARGS_TO_BUFFERLIST(addWorkStage(name, sizeScale, vs, fs, bufferList)); }

    Pipeline::Stage* Pipeline::addDeferredStage(const std::string& name, float sizeScale, bool runOnce,
                                                osg::Shader* vs, osg::Shader* fs, int buffers, ...)
    { ARGS_TO_BUFFERLIST(addDeferredStage(name, sizeScale, runOnce, vs, fs, bufferList)); }

    int Pipeline::getNumNonDepthBuffers(const BufferDescriptions& buffers)
    {
        int numBuffers = (int)buffers.size();
        for (size_t i = 0; i < buffers.size(); i++)
        {
            BufferType type = buffers[i].type;
            if (type == DEPTH24_STENCIL8 || type >= DEPTH16) numBuffers--;
        }
        return numBuffers;
    }

    Pipeline::Stage* Pipeline::addInputStage(const std::string& name, unsigned int cullMask, int flags,
                                             osg::Shader* vs, osg::Shader* fs, const BufferDescriptions& buffers)
    {
        Stage* s = new Stage; s->deferred = false;
        bool useColorBuf = (getNumNonDepthBuffers(buffers) == 1);
        bool withDefTex = ((flags & NO_DEFAULT_TEXTURES) == 0);
        for (size_t i = 0; i < buffers.size(); i ++)
        {
            std::string bufName = buffers[i].bufferName;
            BufferType type = buffers[i].type;
            osg::Camera::BufferComponent comp = useColorBuf ? osg::Camera::COLOR_BUFFER0
                                              : (osg::Camera::BufferComponent)(osg::Camera::COLOR_BUFFER0 + i);
            if (type == DEPTH24_STENCIL8 || type == DEPTH32_STENCIL8) comp = osg::Camera::PACKED_DEPTH_STENCIL_BUFFER;
            else if (type >= DEPTH16) comp = osg::Camera::DEPTH_BUFFER;

            osg::ref_ptr<osg::Texture> tex = buffers[i].bufferToShare;
            if (!tex) tex = createTexture(type, _stageSize[0], _stageSize[1], _glVersion);
            if (i > 0) s->camera->attach(comp, tex.get(), 0, 0, false, 0);
            else s->camera = createRTTCamera(comp, tex.get(), _stageContext.get(), false);
            s->outputs[bufName] = tex.get();
        }

        if ((flags & USE_COVERAGE_SAMPLES) != 0)
        {
            int samples = (flags & 0x000F); int colorSamples = osg::minimum(samples / 2, 4);
            osg::Camera::BufferAttachmentMap& attachments = s->camera->getBufferAttachmentMap();
            for (osg::Camera::BufferAttachmentMap::iterator it = attachments.begin(); it != attachments.end(); ++it)
            {
                if (it->first < osg::Camera::COLOR_BUFFER) continue;
                it->second._multisampleSamples = samples;
                it->second._multisampleColorSamples = colorSamples;
            }
        }

        applyDefaultStageData(*s, name, vs, fs);
        applyDefaultInputStateSet(*s->camera->getOrCreateStateSet(), withDefTex, true);
        s->camera->setUserValue("PipelineCullMask", cullMask);  // replacing setCullMask()
        s->camera->setUserValue("NeedNearFarCalculation", true);
        s->camera->setClampProjectionMatrixCallback(
            new MyClampProjectionCallback(s, _deferredCallback.get()));
        s->camera->setComputeNearFarMode(g_nearFarMode);
        s->inputStage = true; _stages.push_back(s);

#if VERBOSE_CREATING
        OSG_NOTICE << "[Pipeline] Add Input Stage: " << name << " ("
                   << vs->getName() << " / " << fs->getName() << ");\n  OutBuffers: ";
        for (auto& kv : s->outputs) OSG_NOTICE << kv.first << ", "; OSG_NOTICE << std::endl;
#endif
        return s;
    }

    Pipeline::Stage* Pipeline::addWorkStage(const std::string& name, float sizeScale,
                                            osg::Shader* vs, osg::Shader* fs, const BufferDescriptions& buffers)
    {
        Stage* s = new Stage; s->deferred = false;
        bool useColorBuf = (getNumNonDepthBuffers(buffers) == 1);
        for (int i = 0; i < buffers.size(); i++)
        {
            std::string bufName = buffers[i].bufferName;
            BufferType type = buffers[i].type;
            osg::Camera::BufferComponent comp = useColorBuf ? osg::Camera::COLOR_BUFFER0
                                              : (osg::Camera::BufferComponent)(osg::Camera::COLOR_BUFFER0 + i);
            if (type == DEPTH24_STENCIL8 || type == DEPTH32_STENCIL8) comp = osg::Camera::PACKED_DEPTH_STENCIL_BUFFER;
            else if (type >= DEPTH16) comp = osg::Camera::DEPTH_BUFFER;

            float ww = _stageSize[0] * sizeScale, hh = _stageSize[1] * sizeScale;
            if (ww < 1.0f) ww = 1.0f; if (hh < 1.0f) hh = 1.0f;
            osg::ref_ptr<osg::Texture> tex = buffers[i].bufferToShare;
            if (!tex) tex = createTexture(type, (int)ww, (int)hh, _glVersion);
            if (i > 0) s->camera->attach(comp, tex.get());
            else s->camera = createRTTCamera(comp, tex.get(), _stageContext.get(), true);
            s->outputs[bufName] = tex.get();
        }

        applyDefaultStageData(*s, name, vs, fs);
        s->camera->setImplicitBufferAttachmentMask(0, 0);
        s->camera->getOrCreateStateSet()->setAttributeAndModes(_deferredDepth.get());
        s->inputStage = false; _stages.push_back(s);
#if VERBOSE_CREATING
        OSG_NOTICE << "[Pipeline] Add Working Stage: " << name << " ("
                   << vs->getName() << " / " << fs->getName() << ");\n  OutBuffers: ";
        for (auto& kv : s->outputs) OSG_NOTICE << kv.first << ", "; OSG_NOTICE << std::endl;
#endif
        return s;
    }

    Pipeline::Stage* Pipeline::addDeferredStage(const std::string& name, float sizeScale, bool runOnce,
                                                osg::Shader* vs, osg::Shader* fs, const BufferDescriptions& buffers)
    {
        Stage* s = new Stage; s->deferred = true;
        s->runner = new osgVerse::DeferredRenderCallback::RttGeometryRunner(name);
        s->runner->runOnce = runOnce; s->runner->setUseScreenQuad(0, NULL);  // quad at the beginning
        _deferredCallback->addRunner(s->runner.get());

        bool useColorBuf = (getNumNonDepthBuffers(buffers) == 1);
        for (int i = 0; i < buffers.size(); i++)
        {
            std::string bufName = buffers[i].bufferName;
            BufferType type = buffers[i].type;
            osg::Camera::BufferComponent comp = useColorBuf ? osg::Camera::COLOR_BUFFER0
                                              : (osg::Camera::BufferComponent)(osg::Camera::COLOR_BUFFER0 + i);
            if (type == DEPTH24_STENCIL8 || type == DEPTH32_STENCIL8) comp = osg::Camera::PACKED_DEPTH_STENCIL_BUFFER;
            else if (type >= DEPTH16) comp = osg::Camera::DEPTH_BUFFER;

            float ww = _stageSize[0] * sizeScale, hh = _stageSize[1] * sizeScale;
            if (ww < 1.0f) ww = 1.0f; if (hh < 1.0f) hh = 1.0f;
            osg::ref_ptr<osg::Texture> tex = buffers[i].bufferToShare;
            if (!tex) tex = createTexture(type, (int)ww, (int)hh, _glVersion);
            s->runner->attach(comp, tex.get());
            s->outputs[bufName] = tex.get();
        }

        applyDefaultStageData(*s, name, vs, fs);
        s->inputStage = false; _stages.push_back(s);
#if VERBOSE_CREATING
        OSG_NOTICE << "[Pipeline] Add Deferred Stage: " << name << " ("
                   << vs->getName() << " / " << fs->getName() << ");\n  OutBuffers: ";
        for (auto& kv : s->outputs) OSG_NOTICE << kv.first << ", "; OSG_NOTICE << std::endl;
#endif
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
#if VERBOSE_CREATING
        OSG_NOTICE << "[Pipeline] Add Display Stage: " << name << " ("
                   << vs->getName() << " / " << fs->getName() << ")" << std::endl;
#endif
        return s;
    }

    void Pipeline::removeModule(RenderingModuleBase* cb)
    {
        for (std::map<std::string, osg::ref_ptr<RenderingModuleBase>>::iterator
             itr = _modules.begin(); itr != _modules.end(); ++itr)
        { if (itr->second == cb) { _modules.erase(itr); return; } }
    }

    void Pipeline::activateDeferredStage(const std::string& n, bool b)
    { Stage* s = getStage(n); if (s->runner.valid()) s->runner->active = b; }

    void Pipeline::applyDefaultStageData(Stage& s, const std::string& name, osg::Shader* vs, osg::Shader* fs)
    {
        osg::StateSet* ss = s.deferred ?
            s.runner->geometry->getOrCreateStateSet() : s.camera->getOrCreateStateSet();
        if (vs || fs)
        {
            osg::ref_ptr<ScriptableProgram> prog = new ScriptableProgram;
            prog->setName(name + "_PROGRAM");
            if (vs)
            {
                vs->setName(name + "_SHADER_VS"); prog->addShader(vs);
                createShaderDefinitions(vs, _glContextVersion, _glslTargetVersion);
            }

            if (fs)
            {
                fs->setName(name + "_SHADER_FS"); prog->addShader(fs);
                createShaderDefinitions(fs, _glContextVersion, _glslTargetVersion);
            }

            int values = s.overridedPrograms ? osg::StateAttribute::OVERRIDE : 0;
            ss->setAttributeAndModes(prog.get(), osg::StateAttribute::ON | values);
            ss->addUniform(_deferredCallback->getNearFarUniform());
            ss->addUniform(_invScreenResolution.get());
        }
        s.name = name; if (!s.deferred) s.camera->setName(name);
    }

    int Pipeline::applyDefaultInputStateSet(osg::StateSet& ss, bool applyDefTextures, bool blendOff)
    {
        osg::Vec4 color0 = osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        osg::Vec4 color1 = osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
        osg::Vec4 colorORM = osg::Vec4(1.0f, 1.0f, 0.0f, 0.0f);
        if (blendOff) ss.setMode(GL_BLEND, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);

        if (applyDefTextures)
        {
            ss.setTextureAttributeAndModes(0, createDefaultTexture(color1));  // DiffuseMap
            ss.setTextureAttributeAndModes(1, createDefaultTexture(color0));  // NormalMap
            ss.setTextureAttributeAndModes(2, createDefaultTexture(color1));  // SpecularMap
            ss.setTextureAttributeAndModes(3, createDefaultTexture(colorORM));  // ShininessMap
            ss.setTextureAttributeAndModes(4, createDefaultTexture(color0));  // AmbientMap
            ss.setTextureAttributeAndModes(5, createDefaultTexture(color0));  // EmissiveMap
            ss.setTextureAttributeAndModes(6, createDefaultTexture(color0));  // ReflectionMap
            for (int i = 0; i < 7; ++i) ss.addUniform(new osg::Uniform(uniformNames[i].c_str(), i));
        }

        osg::Program* prog = static_cast<osg::Program*>(ss.getAttribute(osg::StateAttribute::PROGRAM));
        if (prog != NULL)
        {
            prog->addBindAttribLocation(attributeNames[6], 6);
            //prog->addBindAttribLocation(attributeNames[7], 7);
        }
        return applyDefTextures ? 7 : 0;
    }

    void Pipeline::updateStageForStereoVR(Stage* s, osg::Shader* geomShader, double eyeSep, bool useClip)
    {
        int glslVer = osg::maximum(_glslTargetVersion, 130);
        createShaderDefinitions(geomShader, _glContextVersion, glslVer);

        ScriptableProgram* stageProg = s->getProgram();
        if (stageProg && stageProg->getParameter(GL_GEOMETRY_VERTICES_OUT_EXT) <= 3)
        {
            stageProg->addDefinitions(osg::Shader::VERTEX, "#define VERSE_VRMODE 1");
            stageProg->addShader(geomShader);
            stageProg->setParameter(GL_GEOMETRY_VERTICES_OUT_EXT, 2 * 3);  // Left/Right
            stageProg->setParameter(GL_GEOMETRY_INPUT_TYPE_EXT, GL_TRIANGLES);
            stageProg->setParameter(GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLES);
        }

        osg::StateSet* stageSS = s->camera->getOrCreateStateSet();
        if (useClip)
        {
#if !defined(OSG_GL3_AVAILABLE) && !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE)
            stageSS->setMode(GL_CLIP_PLANE0, osg::StateAttribute::ON);
            stageSS->setMode(GL_CLIP_PLANE1, osg::StateAttribute::ON);
            stageSS->setMode(GL_CLIP_PLANE2, osg::StateAttribute::ON);
            stageSS->setMode(GL_CLIP_PLANE3, osg::StateAttribute::ON);
#endif
        }

        osg::Uniform* eyeUniform = stageSS->getOrCreateUniform("eyeSep", osg::Uniform::FLOAT, 2);
        eyeUniform->setElement(0, -(float)eyeSep * 0.5f);
        eyeUniform->setElement(1, (float)eyeSep * 0.5f);
    }

    osg::StateSet* Pipeline::createForwardStateSet(osg::Shader* vs, osg::Shader* fs)
    {
        osg::ref_ptr<ScriptableProgram> program = new ScriptableProgram;
        program->setName("Forward_PROGRAM");
        if (vs)
        {
            vs->setName("Forward_SHADER_VS"); program->addShader(vs);
            createShaderDefinitions(vs, _glContextVersion, _glslTargetVersion);
        }

        if (fs)
        {
            fs->setName("Forward_SHADER_FS"); program->addShader(fs);
            createShaderDefinitions(fs, _glContextVersion, _glslTargetVersion);
        }

        osg::ref_ptr<osg::StateSet> ss = new osg::StateSet;
        ss->setAttributeAndModes(program, osg::StateAttribute::ON);
        applyDefaultInputStateSet(*ss, true, false);
        return ss.release();
    }

    void Pipeline::createShaderDefinitions(osg::Shader* s, int glVer, int glslVer,
                                           const std::vector<std::string>& userDefs)
    {
        if (!s || (s && s->getShaderSource().empty())) return;
        ShaderLibrary::instance()->createShaderDefinitions(*s, glVer, glslVer, userDefs);
    }

    void Pipeline::createShaderDefinitionsFromPipeline(osg::Shader* s, const std::vector<std::string>& defs)
    { createShaderDefinitions(s, _glContextVersion, _glslTargetVersion, defs); }

    void Pipeline::setPipelineMask(osg::Object& node, unsigned int mask, unsigned int flags)
    {
        if (node.getUserDataContainer() != NULL)
        {
            osg::DefaultUserDataContainer* defUdc =
                dynamic_cast<osg::DefaultUserDataContainer*>(node.getUserDataContainer());
            if (!defUdc)
            {
                OSG_NOTICE << "The node already has a user-define data container '"
                           << node.getUserDataContainer()->className()
                           << "' before setting pipeline mask, which may cause overwriting problems. "
                           << "Consider a better way to handle user values!" << std::endl;
            }
        }
        node.setUserValue("PipelineMask", mask);  // replacing setNodeMask()
        node.setUserValue("PipelineFlags", flags);
    }

    unsigned int Pipeline::getPipelineMask(osg::Object& node)
    {
        unsigned int mask = 0xffffffff;
        if (node.getUserDataContainer() != NULL)
            node.getUserValue("PipelineMask", mask); return mask;
    }

    unsigned int Pipeline::getPipelineMaskFlags(osg::Object& node)
    {
        unsigned int flags = 0xffffffff;
        if (node.getUserDataContainer() != NULL)
            node.getUserValue("PipelineFlags", flags); return flags;
    }

    osg::Texture* Pipeline::createTexture(BufferType type, int w, int h, int glVer)
    {
        osg::ref_ptr<osg::Texture2D> tex = new osg::Texture2D;
        setTextureBuffer(tex.get(), type, glVer);
        tex->setTextureSize(w, h);
        return tex.release();
    }

    void Pipeline::setTextureBuffer(osg::Texture* tex, BufferType type, int glVer)
    {
        tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

        // WebGL texture formats:
        //   https://developer.mozilla.org/en-US/docs/Web/API/WebGLRenderingContext/texImage2D
        //   https://developer.mozilla.org/en-US/docs/Web/API/OES_texture_float
        //   https://developer.mozilla.org/en-US/docs/Web/API/OES_texture_half_float
        //   https://webgl2fundamentals.org/webgl/lessons/zh_cn/webgl-data-textures.html
        switch (type)
        {
        case RGB_INT8:
#if defined(VERSE_EMBEDDED_GLES2)
            tex->setInternalFormat(GL_RGB);
#else
            tex->setInternalFormat(GL_RGB8);
#endif
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
#if defined(VERSE_EMBEDDED_GLES2)
            tex->setInternalFormat(GL_RGB);
            tex->setSourceType(GL_HALF_FLOAT_OES);
#else
            tex->setInternalFormat(GL_RGB16F_ARB);
            tex->setSourceType(GL_HALF_FLOAT);
#endif
            tex->setSourceFormat(GL_RGB);
            break;
        case RGB_FLOAT32:
#if defined(VERSE_EMBEDDED_GLES2)
            tex->setInternalFormat(GL_RGB);
#else
            tex->setInternalFormat(GL_RGB32F_ARB);
#endif
            tex->setSourceFormat(GL_RGB);
            tex->setSourceType(GL_FLOAT);
            break;
        case SRGB_INT8:
            tex->setInternalFormat(GL_SRGB8);
            tex->setSourceFormat(GL_RGB);
            tex->setSourceType(GL_UNSIGNED_BYTE);
            break;
        case RGBA_INT8:
#if defined(VERSE_EMBEDDED_GLES2)
            tex->setInternalFormat(GL_RGBA);
#else
            tex->setInternalFormat(GL_RGBA8);
#endif
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
#if defined(VERSE_EMBEDDED_GLES2)
            tex->setInternalFormat(GL_RGBA);
            tex->setSourceType(GL_HALF_FLOAT_OES);
#else
            tex->setInternalFormat(GL_RGBA16F_ARB);
            tex->setSourceType(GL_HALF_FLOAT);
#endif
            tex->setSourceFormat(GL_RGBA);
            break;
        case RGBA_FLOAT32:
#if defined(VERSE_EMBEDDED_GLES2)
            tex->setInternalFormat(GL_RGBA);
#else
            tex->setInternalFormat(GL_RGBA32F_ARB);
#endif
            tex->setSourceFormat(GL_RGBA);
            tex->setSourceType(GL_FLOAT);
            break;
        case SRGBA_INT8:
            tex->setInternalFormat(GL_SRGB8_ALPHA8);
            tex->setSourceFormat(GL_RGBA);
            tex->setSourceType(GL_UNSIGNED_BYTE);
            break;

        // RTT to 1-component / 2-component textures:
        // GLver > 300 (NVIDIA, INTEL), MTT: GL_R8 / GL_RED
        // GLver < 300: GL_LUMINANCE8 / GL_LUMINANCE
        // WebGL 1.0: GL_LUMINANCE / GL_LUMINANCE
        case R_INT8:
#if !defined(VERSE_ENABLE_MTT) && !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE)
            if (glVer > 0 && glVer < 300)
            {
                tex->setInternalFormat(GL_LUMINANCE8);
                tex->setSourceFormat(GL_LUMINANCE);
            }
            else
#endif
            {
#if defined(VERSE_EMBEDDED_GLES2)
                tex->setInternalFormat(GL_LUMINANCE);
                tex->setSourceFormat(GL_LUMINANCE);
#else
                tex->setInternalFormat(GL_R8);
                tex->setSourceFormat(GL_RED);
#endif
            }
            tex->setSourceType(GL_UNSIGNED_BYTE);
            break;
        case R_FLOAT16:
#if !defined(VERSE_ENABLE_MTT) && !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE)
            if (glVer > 0 && glVer < 300)
            {
                tex->setInternalFormat(GL_LUMINANCE16F_ARB);
                tex->setSourceFormat(GL_LUMINANCE);
                tex->setSourceType(GL_HALF_FLOAT);
            }
            else
#endif
            {
#if defined(VERSE_EMBEDDED_GLES2)
                tex->setInternalFormat(GL_LUMINANCE);
                tex->setSourceFormat(GL_LUMINANCE);
                tex->setSourceType(GL_HALF_FLOAT_OES);
#else
                tex->setInternalFormat(GL_R16F);
                tex->setSourceFormat(GL_RED);
                tex->setSourceType(GL_HALF_FLOAT);
#endif
            }
            break;
        case R_FLOAT32:
#if !defined(VERSE_ENABLE_MTT) && !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE)
            if (glVer > 0 && glVer < 300)
            {
                tex->setInternalFormat(GL_LUMINANCE32F_ARB);
                tex->setSourceFormat(GL_LUMINANCE);
            }
            else
#endif
            {
#if defined(VERSE_EMBEDDED_GLES2)
                tex->setInternalFormat(GL_LUMINANCE);
                tex->setSourceFormat(GL_LUMINANCE);
#else
                tex->setInternalFormat(GL_R32F);
                tex->setSourceFormat(GL_RED);
#endif
            }
            tex->setSourceType(GL_FLOAT);
            break;
        case RG_INT8:
#if !defined(VERSE_ENABLE_MTT) && !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE)
            if (glVer > 0 && glVer < 300)
            {
                tex->setInternalFormat(GL_LUMINANCE8_ALPHA8);
                tex->setSourceFormat(GL_LUMINANCE_ALPHA);
            }
            else
#endif
            {
#if defined(VERSE_EMBEDDED_GLES2)
                tex->setInternalFormat(GL_LUMINANCE_ALPHA);
                tex->setSourceFormat(GL_LUMINANCE_ALPHA);
#else
                tex->setInternalFormat(GL_RG8);
                tex->setSourceFormat(GL_RG);
#endif
            }
            tex->setSourceType(GL_UNSIGNED_BYTE);
            break;
        case RG_FLOAT16:
#if !defined(VERSE_ENABLE_MTT) && !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE)
            if (glVer > 0 && glVer < 300)
            {
                tex->setInternalFormat(GL_LUMINANCE_ALPHA16F_ARB);
                tex->setSourceFormat(GL_LUMINANCE_ALPHA);
                tex->setSourceType(GL_HALF_FLOAT);
            }
            else
#endif
            {
#if defined(VERSE_EMBEDDED_GLES2)
                tex->setInternalFormat(GL_LUMINANCE_ALPHA);
                tex->setSourceFormat(GL_LUMINANCE_ALPHA);
                tex->setSourceType(GL_HALF_FLOAT_OES);
#else
                tex->setInternalFormat(GL_RG16F);
                tex->setSourceFormat(GL_RG);
                tex->setSourceType(GL_HALF_FLOAT);
#endif
            }
            break;
        case RG_FLOAT32:
#if !defined(VERSE_ENABLE_MTT) && !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE)
            if (glVer > 0 && glVer < 300)
            {
                tex->setInternalFormat(GL_LUMINANCE_ALPHA32F_ARB);
                tex->setSourceFormat(GL_LUMINANCE_ALPHA);
            }
            else
#endif
            {
#if defined(VERSE_EMBEDDED_GLES2)
                tex->setInternalFormat(GL_LUMINANCE_ALPHA);
                tex->setSourceFormat(GL_LUMINANCE_ALPHA);
#else
                tex->setInternalFormat(GL_RG32F);
                tex->setSourceFormat(GL_RG);
#endif
            }
            tex->setSourceType(GL_FLOAT);
            break;
        case DEPTH16:
#if defined(VERSE_EMBEDDED_GLES2)
            tex->setInternalFormat(GL_DEPTH_COMPONENT);
#elif defined(VERSE_EMBEDDED_GLES3)
            // https://github.com/KhronosGroup/OpenGL-API/issues/84
            tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
            tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
            tex->setInternalFormat(GL_DEPTH_COMPONENT16);
#else
            tex->setInternalFormat(GL_DEPTH_COMPONENT16);
#endif
            tex->setSourceFormat(GL_DEPTH_COMPONENT);
            tex->setSourceType(GL_UNSIGNED_SHORT);
            break;
        case DEPTH24_STENCIL8:
#if defined(VERSE_EMBEDDED_GLES2)
            tex->setInternalFormat(GL_DEPTH_STENCIL_EXT);
#elif defined(VERSE_EMBEDDED_GLES3)
            // https://github.com/KhronosGroup/OpenGL-API/issues/84
            tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
            tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
            tex->setInternalFormat(GL_DEPTH24_STENCIL8_EXT);
#else
            tex->setInternalFormat(GL_DEPTH24_STENCIL8_EXT);
#endif
            tex->setSourceFormat(GL_DEPTH_STENCIL_EXT);
            tex->setSourceType(GL_UNSIGNED_INT_24_8_EXT);
            break;
        case DEPTH32:
#if defined(VERSE_EMBEDDED_GLES2)
            tex->setInternalFormat(GL_DEPTH_COMPONENT);
            tex->setSourceType(GL_UNSIGNED_INT);
#elif defined(VERSE_EMBEDDED_GLES3)
            // https://github.com/KhronosGroup/OpenGL-API/issues/84
            tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
            tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
            tex->setInternalFormat(GL_DEPTH_COMPONENT32F);
            tex->setSourceType(GL_FLOAT);
#else
            tex->setInternalFormat(GL_DEPTH_COMPONENT32);
            tex->setSourceType(GL_UNSIGNED_INT);
#endif
            tex->setSourceFormat(GL_DEPTH_COMPONENT);
            break;
        case DEPTH32_STENCIL8:
#if defined(VERSE_EMBEDDED_GLES3)
            // https://github.com/KhronosGroup/OpenGL-API/issues/84
            tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
            tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
#endif
            tex->setInternalFormat(GL_DEPTH32F_STENCIL8);
            tex->setSourceType(GL_FLOAT_32_UNSIGNED_INT_24_8_REV);
            tex->setSourceFormat(GL_DEPTH_STENCIL_EXT);
            break;
        default: break;
        }
    }
}

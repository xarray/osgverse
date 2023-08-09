#ifndef MANA_PP_DEFERRED_CALLBACK_HPP
#define MANA_PP_DEFERRED_CALLBACK_HPP

#include <osg/TextureCubeMap>
#include "Utilities.h"

namespace osgVerse
{
    /** Lightweight render-to-texture callback (use as a pre-draw-callback)
        - Support only Texture2D & TextureCubeMap, no multisample
        - Can render a single geometry/state-set for use
        For full FBO support, use osg::Camera instead */
    class DeferredRenderCallback : public CameraDrawCallback
    {
        friend class RttRunner;

    public:
        DeferredRenderCallback(bool inPipeline);
        virtual void operator()(osg::RenderInfo& renderInfo) const;

        void setForwardStateSet(osg::StateSet* ss) { _forwardStateSet = ss; }
        osg::StateSet* getForwardStateSet() { return _forwardStateSet.get(); }

        void setForwardMasks(unsigned int m1, unsigned int m2)
        { _forwardMask = m1; _fixedShadingMask = m2; }
        unsigned int getForwardMask() const { return _forwardMask; }
        unsigned int getFixedShadingMask() const { return _fixedShadingMask; }

        void setClearMask(GLenum m) { _clearMask = m; }
        void setClearColor(const osg::Vec4& c) { _clearColor = c; }
        void setClearAccum(const osg::Vec4& c) { _clearAccum = c; }
        void setClearDepth(double d) { _clearDepth = d; }
        void setClearStencil(double d) { _clearStencil = d; }

        void registerDepthFBO(osg::Camera* cam, osg::FrameBufferObject* fbo) { _depthFboMap[cam] = fbo; }
        void requireDepthBlit(osg::Camera* cam, bool addToList);

        void applyAndUpdateCameraUniforms(osgUtil::SceneView* sv);
        osg::Vec2d cullWithNearFarCalculation(osgUtil::SceneView* sv);
        osg::Vec2d getCalculatedNearFar() const { return _calculatedNearFar; }
        osg::Uniform* getNearFarUniform() { return _nearFarUniform.get(); }

        void setClampCallback(osg::CullSettings::ClampProjectionMatrixCallback* cb)
        { _userClamperCallback = cb; }

        void setDrawBuffer(GLenum buf, bool applyMask)
        { _drawBuffer = buf; _drawBufferApplyMask = applyMask; }

        void setReadBuffer(GLenum buf, bool applyMask)
        { _readBuffer = buf; _readBufferApplyMask = applyMask; }

        struct RttRunner : public osg::Referenced
        {
            virtual bool setup(DeferredRenderCallback* cb, osg::RenderInfo& renderInfo);
            virtual void finish(DeferredRenderCallback* cb, osg::RenderInfo& renderInfo);
            virtual void start(DeferredRenderCallback* cb, osg::RenderInfo& renderInfo);
            virtual bool draw(DeferredRenderCallback* cb, osg::RenderInfo& renderInfo);
            virtual void drawInner(DeferredRenderCallback* cb, osg::RenderInfo& renderInfo) const = 0;

            void attach(osg::Camera::BufferComponent buffer, osg::Texture* texture,
                        int face = 0, bool mipmap = false)
            {
                attachments[buffer]._texture = texture;
                attachments[buffer]._level = 0;
                attachments[buffer]._face = face;
                attachments[buffer]._mipMapGeneration = mipmap;
            }

            RttRunner(const std::string& n = "") : name(n), created(false), active(true), runOnce(false) {}
            void detach(osg::Camera::BufferComponent buffer) { attachments.erase(buffer); }

            osg::Camera::BufferAttachmentMap attachments;
            osg::ref_ptr<osg::FrameBufferObject> fbo;
            osg::ref_ptr<osg::Viewport> viewport;
            osg::Matrix modelView, projection;
            std::string name; bool created, active, runOnce;
        };

        struct RttGeometryRunner : public RttRunner
        {
            virtual void drawInner(DeferredRenderCallback* cb, osg::RenderInfo& renderInfo) const;
            RttGeometryRunner(const std::string& n) : RttRunner(n) {}
            void setUseScreenQuad(unsigned int unit, osg::Texture* texIn);
            osg::ref_ptr<osg::Geometry> geometry;
        };

        void addRunner(RttRunner* r) { _runners.push_back(r); }
        std::vector<osg::ref_ptr<RttRunner>>& getRunners() { return _runners; }
        const std::vector<osg::ref_ptr<RttRunner>>& getRunners() const { return _runners; }

    protected:
        typedef std::pair<std::string, std::vector<osg::Matrixf>> MatrixListPair;
        std::map<osg::Camera*, MatrixListPair> _cameraMatrixMap;
        std::map<osg::Camera*, osg::observer_ptr<osg::FrameBufferObject>> _depthFboMap;
        std::set<osg::observer_ptr<osg::Camera>> _depthBlitList;
        std::vector<osg::ref_ptr<RttRunner>> _runners;
        osg::ref_ptr<osg::StateSet> _forwardStateSet;
        osg::ref_ptr<osg::CullSettings::ClampProjectionMatrixCallback> _userClamperCallback;
        osg::ref_ptr<osg::Uniform> _nearFarUniform;
        GLenum _drawBuffer, _readBuffer, _clearMask;
        osg::Vec4 _clearColor, _clearAccum;
        osg::Vec2d _calculatedNearFar;
        double _clearDepth, _clearStencil;
        unsigned int _cullFrameNumber, _forwardMask, _fixedShadingMask;
        bool _inPipeline, _drawBufferApplyMask, _readBufferApplyMask;
    };
}

#endif

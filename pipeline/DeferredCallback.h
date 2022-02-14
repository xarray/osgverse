#ifndef MANA_DEFERRED_CALLBACK_HPP
#define MANA_DEFERRED_CALLBACK_HPP

#include <osg/Geometry>
#include <osg/Texture2D>
#include <osg/TextureCubeMap>
#include <osg/Camera>

namespace osgVerse
{
    /** Lightweight render-to-texture callback (use as a pre-draw-callback)
        - Support only Texture2D & TextureCubeMap, no multisample
        - Can render a single geometry/state-set for use
        For full FBO support, use osg::Camera instead */
    class DeferredRenderCallback : public osg::Camera::DrawCallback
    {
        friend class RttRunner;

    public:
        DeferredRenderCallback(bool inPipeline);
        virtual void operator()(osg::RenderInfo& renderInfo) const;

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

            RttRunner(const std::string& n = "") : name(n), initialized(false), active(true) {}
            void detach(osg::Camera::BufferComponent buffer) { attachments.erase(buffer); }

            osg::Camera::BufferAttachmentMap attachments;
            osg::ref_ptr<osg::FrameBufferObject> fbo;
            osg::ref_ptr<osg::Viewport> viewport;
            osg::Matrix modelView, projection;
            std::string name; bool initialized, active;
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
        std::map<osg::Camera*, osg::observer_ptr<osg::FrameBufferObject>> _depthFboMap;
        std::map<osg::Camera*, std::pair<std::string, osg::Matrixf>> _cameraMatrixMap;
        std::set<osg::observer_ptr<osg::Camera>> _depthBlitList;
        std::vector<osg::ref_ptr<RttRunner>> _runners;
        osg::ref_ptr<osg::Uniform> _nearFarUniform;
        GLenum _drawBuffer, _readBuffer, _clearMask;
        osg::Vec4 _clearColor, _clearAccum;
        osg::Vec2d _calculatedNearFar;
        double _clearDepth, _clearStencil;
        unsigned int _cullFrameNumber;
        bool _inPipeline, _drawBufferApplyMask, _readBufferApplyMask;
    };
}

#endif

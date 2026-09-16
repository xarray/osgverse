#include <osg/TextureRectangle>
#include <osg/Texture3D>
#include "HistoryBufferCallback.h"
using namespace osgVerse;

HistoryBufferCallback::HistoryBufferCallback(unsigned int size)
: CameraDrawCallback(), _lastHistoryIndex(0) { setHistorySize(size); }

HistoryBufferCallback::~HistoryBufferCallback() {}

void HistoryBufferCallback::setup(Pipeline::Stage* stage, const std::string& buffer,
                                  DeferredRenderCallback* callback)
{
    if (!stage) return; _stage = stage; _bufferName = buffer;
    _deferredCallback = callback;
    CameraDrawCallback::setup(_stage->camera.get(), 2);  // as post draw callback
}

void HistoryBufferCallback::operator()(osg::RenderInfo& renderInfo) const
{
    unsigned int historySize = _historyTextures.size();
    if (_stage.valid() && _lastHistoryIndex < historySize)
    {
        osg::Texture* srcTexture = _stage->getBufferTexture(_bufferName);
        osg::Texture* dstTexture = _historyTextures[_lastHistoryIndex].get();
        if (srcTexture && !dstTexture)
        {
            dstTexture = const_cast<HistoryBufferCallback*>(this)->createTexture(
                srcTexture, _lastHistoryIndex, renderInfo.getContextID());

            std::map<unsigned int, std::pair<std::string, unsigned int>>::
                const_iterator it = _applyTextureMap.find(_lastHistoryIndex);
            if (dstTexture && it != _applyTextureMap.end())
                _stage->applyTexture(dstTexture, it->second.first, it->second.second);
        }

        osg::State& state = *renderInfo.getState();
        osg::Viewport* viewport = renderInfo.getCurrentCamera()->getViewport();
        if (dstTexture && viewport)
        {   // Note: osg unbinds the FBO of the stage before calling this post-draw callback
            // (see RenderStage::draw()), while copyTexSubImage2D always reads from the
            // currently bound read framebuffer: without binding the stage's FBO explicitly,
            // wrong data (usually the default framebuffer) would be recorded as history
            osg::FrameBufferObject* srcFbo = NULL;
            if (_deferredCallback.valid() && _stage.valid())
                srcFbo = _deferredCallback->getFboOfCamera(_stage->camera.get());
#if OSG_VERSION_GREATER_THAN(3, 3, 2)
            osg::GLExtensions* ext = state.get<osg::GLExtensions>();
#else
            osg::FBOExtensions* ext = osg::FBOExtensions::instance(renderInfo.getContextID(), true);
#endif
            if (srcFbo) srcFbo->apply(state, osg::FrameBufferObject::READ_FRAMEBUFFER);

            osg::Texture1D* texture1D = 0; osg::Texture2D* texture2D = 0;
            osg::TextureRectangle* textureRec = 0;
            if ((texture2D = dynamic_cast<osg::Texture2D*>(dstTexture)) != 0)
            {
                texture2D->copyTexSubImage2D(state,
                    static_cast<int>(viewport->x()), static_cast<int>(viewport->y()),
                    static_cast<int>(viewport->x()), static_cast<int>(viewport->y()),
                    static_cast<int>(viewport->width()), static_cast<int>(viewport->height()));
            }
            else if ((textureRec = dynamic_cast<osg::TextureRectangle*>(dstTexture)) != 0)
            {
                textureRec->copyTexSubImage2D(state,
                    static_cast<int>(viewport->x()), static_cast<int>(viewport->y()),
                    static_cast<int>(viewport->x()), static_cast<int>(viewport->y()),
                    static_cast<int>(viewport->width()), static_cast<int>(viewport->height()));
            }
            else if ((texture1D = dynamic_cast<osg::Texture1D*>(dstTexture)) != 0)
            {
                texture1D->copyTexSubImage1D(state,
                    static_cast<int>(viewport->x()), static_cast<int>(viewport->x()),
                    static_cast<int>(viewport->y()), static_cast<int>(viewport->width()));
            }

            // Restore the default framebuffer as the read buffer, so that this callback
            // doesn't change the FBO binding expected by the following draw calls
            if (srcFbo)
            {
                GLuint fboId = state.getGraphicsContext()
                             ? state.getGraphicsContext()->getDefaultFboId() : 0;
                ext->glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, fboId);
            }
        }
    }

    _lastHistoryIndex++;
    if (historySize <= _lastHistoryIndex) _lastHistoryIndex = 0;
}

void HistoryBufferCallback::applyStageTexture(const std::string& name, unsigned int unit, unsigned int index)
{ _applyTextureMap[index] = std::pair<std::string, unsigned int>(name, unit); }

std::string HistoryBufferCallback::getAppliedTextureData(unsigned int index, unsigned int& unit)
{
    if (_applyTextureMap.find(index) == _applyTextureMap.end()) return std::string();
    unit = _applyTextureMap[index].second; return _applyTextureMap[index].first;
}

osg::Texture* HistoryBufferCallback::createTexture(osg::Texture* src, unsigned int index,
                                                   unsigned int contextID)
{
    osg::Texture* target = static_cast<osg::Texture*>(src->clone(osg::CopyOp::SHALLOW_COPY));
    target->dirtyTextureParameters(); target->dirtyTextureObject();
    _historyTextures[index] = target; return target;
}

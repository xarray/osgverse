#ifndef MANA_PP_HISTORYBUFFER_CALLBACK_HPP
#define MANA_PP_HISTORYBUFFER_CALLBACK_HPP

#include <osg/Camera>
#include "Pipeline.h"

namespace osgVerse
{

    class HistoryBufferCallback : public CameraDrawCallback
    {
    public:
        HistoryBufferCallback(unsigned int size = 1);
        virtual void operator()(osg::RenderInfo& renderInfo) const;

        void setup(Pipeline::Stage* stage, const std::string& buffer);
        std::vector<osg::ref_ptr<osg::Texture>>& getTextures() { return _historyTextures; }

        void setHistorySize(unsigned int size) { _historyTextures.resize(size); }
        unsigned int getHistorySize() const { return _historyTextures.size(); }
        unsigned int getCurrentHistoryIndex() const { return _lastHistoryIndex; }

        void applyStageTexture(const std::string& name, unsigned int unit, unsigned int index = 0);
        std::string getAppliedTextureData(unsigned int index, unsigned int& unit);

    protected:
        virtual ~HistoryBufferCallback();
        osg::Texture* createTexture(osg::Texture* src, unsigned int index, unsigned int contextID);

        std::vector<osg::ref_ptr<osg::Texture>> _historyTextures;
        std::map<unsigned int, std::pair<std::string, unsigned int>> _applyTextureMap;
        osg::observer_ptr<Pipeline::Stage> _stage;
        std::string _bufferName;
        mutable unsigned int _lastHistoryIndex;
    };
}

#endif

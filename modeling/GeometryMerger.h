#ifndef MANA_MODELING_GEOMERY_MERGER_HPP
#define MANA_MODELING_GEOMERY_MERGER_HPP

#include <math.h>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <osg/Image>
#include <osg/Geometry>
#include <osg/PrimitiveSetIndirect>

namespace osgVerse
{
    class TexturePacker;

    class GeometryMerger
    {
    public:
        typedef std::pair<osg::Geometry*, osg::Matrix> GeometryPair;
        enum Method { COMBINED_GEOMETRY = 0, INDIRECT_COMMANDS };

        GeometryMerger(Method m = COMBINED_GEOMETRY);
        ~GeometryMerger();

        osg::Geometry* process(const std::vector<GeometryPair>& geomList, size_t offset,
                               size_t size = 0, int maxTextureSize = 4096);
        osg::Node* processAsOctree(const std::vector<GeometryPair>& geomList);

    protected:
        osg::Geometry* createCombined(const std::vector<GeometryPair>& geomList,
                                      size_t offset, size_t end);
        osg::Geometry* createIndirect(const std::vector<GeometryPair>& geomList,
                                      size_t offset, size_t end);

        osg::Image* createTextureAtlas(TexturePacker* packer, const std::string& fileName,
                                       int maxTextureSize, int& originW, int& originH);

        Method _method;
    };

    class IndirectCommandDrawElements : public osg::IndirectCommandDrawElements,
                                        public osg::MixinVector<osg::DrawElementsIndirectCommand>
    {
    public:
        IndirectCommandDrawElements()
        : osg::IndirectCommandDrawElements(), osg::MixinVector<osg::DrawElementsIndirectCommand>() {}
        IndirectCommandDrawElements(const IndirectCommandDrawElements& copy, const osg::CopyOp& copyop)
        : osg::IndirectCommandDrawElements(copy, copyop), osg::MixinVector<osg::DrawElementsIndirectCommand>() {}
        META_Object(osgVerse, IndirectCommandDrawElements)

        virtual const GLvoid* getDataPointer() const { return empty() ? 0 : &front(); }
        virtual unsigned int getNumElements() const { return static_cast<unsigned int>(size()); }
        virtual unsigned int getElementSize()const { return 20u; };
        virtual void reserveElements(const unsigned int n) { reserve(n); }
        virtual void resizeElements(const unsigned int n) { resize(n); }

        virtual unsigned int& count(const unsigned int& index) { return at(index).count; }
        virtual unsigned int& instanceCount(const unsigned int& index) { return at(index).instanceCount; }
        virtual unsigned int& firstIndex(const unsigned int& index) { return at(index).firstIndex; }
        virtual unsigned int& baseVertex(const unsigned int& index) { return at(index).baseVertex; }
        virtual unsigned int& baseInstance(const unsigned int& index) { return at(index).baseInstance; }

        void pushUserData(osg::UserDataContainer* ud) { _userDataList.push_back(ud); }
        void popUserData() { _userDataList.pop_back(); }
        void clearUserData() { _userDataList.clear(); }
        void removeUserData(size_t i) { _userDataList.erase(_userDataList.begin() + i); }

        size_t getUserDataSize() const { return _userDataList.size(); }
        osg::UserDataContainer* getUserData(size_t i) { return _userDataList[i].get(); }
        const osg::UserDataContainer* getUserData(size_t i) const { return _userDataList[i].get(); }

    protected:
        std::vector<osg::ref_ptr<osg::UserDataContainer>> _userDataList;
    };
}

#endif

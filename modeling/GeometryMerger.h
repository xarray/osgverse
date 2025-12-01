#ifndef MANA_MODELING_GEOMERY_MERGER_HPP
#define MANA_MODELING_GEOMERY_MERGER_HPP

#include <math.h>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <osg/Version>
#include <osg/Image>
#include <osg/Geometry>
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
#   include <osg/PrimitiveSetIndirect>
#endif

namespace osgVerse
{
    class TexturePacker;

    class GeometryMerger
    {
    public:
        typedef std::pair<osg::Geometry*, osg::Matrix> GeometryPair;
        enum Method { COMBINED_GEOMETRY = 0, INDIRECT_COMMANDS, GPU_BAKING };

        struct GpuBaker : public osg::Referenced
        {
            virtual osg::Image* bakeTextureImage(osg::Node* node) = 0;
            virtual osg::Geometry* bakeGeometry(osg::Node* node) = 0;
        };

        struct AtlasProcessor : public osg::Referenced
        {
            /** For each geometry-image pair, pre-process them and send result image to packer */
            virtual osg::Image* preprocess(osg::Geometry* geom, osg::Image* img, int unit) { return img; }

            /** Create new atlas image from the packer */
            virtual osg::Image* process(TexturePacker* packer) = 0;

            /** Atlas image is finished, check if we should do some post work */
            virtual osg::Image* postprocess(osg::Image* img) { return img; }
        };

        GeometryMerger(Method m = COMBINED_GEOMETRY, GpuBaker* baker = NULL);
        ~GeometryMerger();

        void setGpkBaker(GpuBaker* baker) { _baker = baker; }
        GpuBaker* getGpuBaker() { return _baker.get(); }

        void setAtlasProcessor(AtlasProcessor* proc) { _atlasProcessor = proc; }
        AtlasProcessor* getAtlasProcessor() { return _atlasProcessor.get(); }

        void setMethod(Method m) { _method = m; }
        Method getMethod() const { return _method; }

        void setSimplifierRatio(float r) { _autoSimplifierRatio = r; }
        float getSimplifierRatio() const { return _autoSimplifierRatio; }

        void setForceColorArray(bool b) { _forceColorArray = b; }
        bool getForceColorArray() const { return _forceColorArray; }

        osg::Geometry* process(const std::vector<GeometryPair>& geomList, size_t offset,
                               size_t size = 0, int maxTextureSize = 4096);
        osg::Node* processAsOctree(const std::vector<GeometryPair>& geomList, size_t offset,
                                   size_t size = 0, int maxTextureSize = 4096, osg::Geode* octRoot = NULL,
                                   int maxObjectsInCell = 8, float minSizeInCell = 1.0f);

        osg::Image* processAtlas(const std::vector<GeometryPair>& geomList, size_t offset,
                                 size_t size = 0, int maxTextureSize = 4096);

    protected:
        osg::Geometry* createCombined(const std::vector<GeometryPair>& geomList,
                                      size_t offset, size_t end);
        osg::Geometry* createIndirect(const std::vector<GeometryPair>& geomList,
                                      size_t offset, size_t end);
        osg::Geometry* createGpuBaking(const std::vector<GeometryPair>& geomList,
                                       size_t offset, size_t end);

        osg::Image* createTextureAtlas(TexturePacker* packer, const std::string& fileName,
                                       int maxTextureSize, int& originW, int& originH);

        osg::ref_ptr<GpuBaker> _baker;
        osg::ref_ptr<AtlasProcessor> _atlasProcessor;
        Method _method;
        float _autoSimplifierRatio;
        bool _forceColorArray;
    };

#if OSG_VERSION_GREATER_THAN(3, 4, 1)
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

#define MULTI_DRAW_ELEMENTS_INDIRECT(t, glType) \
    class MultiDrawElementsIndirect##t : public osg::MultiDrawElementsIndirect##t { \
    public: \
        MultiDrawElementsIndirect##t (GLenum mode = 0) : osg::MultiDrawElementsIndirect##t (mode) {} \
        MultiDrawElementsIndirect##t (const MultiDrawElementsIndirect##t & mdi, \
            const osg::CopyOp& op=osg::CopyOp::SHALLOW_COPY) : osg::MultiDrawElementsIndirect##t (mdi, op) {} \
        MultiDrawElementsIndirect##t (GLenum m, unsigned int no, const glType * p) : osg::MultiDrawElementsIndirect##t (m, no, p) {} \
        MultiDrawElementsIndirect##t (GLenum m, unsigned int no) : osg::MultiDrawElementsIndirect##t (m, no) {} \
        template <class IT> MultiDrawElementsIndirect##t (GLenum m, IT f, IT l) : osg::MultiDrawElementsIndirect##t (m, f, l) {} \
        virtual osg::Object* cloneType() const { return new MultiDrawElementsIndirect##t (); } \
        virtual osg::Object* clone(const osg::CopyOp& copyop) const { \
            return new MultiDrawElementsIndirect##t (*this, copyop); } \
        virtual bool isSameKindAs(const osg::Object* obj) const { \
            return dynamic_cast<const MultiDrawElementsIndirect##t *>(obj) != NULL; } \
        virtual void accept(osg::PrimitiveFunctor& functor) const; \
        virtual void accept(osg::PrimitiveIndexFunctor& functor) const; };

    MULTI_DRAW_ELEMENTS_INDIRECT(UByte, GLubyte)
    MULTI_DRAW_ELEMENTS_INDIRECT(UShort, GLushort)
    MULTI_DRAW_ELEMENTS_INDIRECT(UInt, GLuint)
#endif
}

#endif

#ifndef MANA_READERWRITER_DATABASEPAGER_HPP
#define MANA_READERWRITER_DATABASEPAGER_HPP

#include <osg/ProxyNode>
#include <osg/PagedLOD>
#include <osgDB/Registry>
#include <osgDB/DatabasePager>
#include "Export.h"

namespace osgVerse
{

    class OSGVERSE_RW_EXPORT DatabasePager : public osgDB::DatabasePager
    {
    public:
        DatabasePager(bool compressImages = false)
        :   osgDB::DatabasePager(), _compressingImages(compressImages)
        { setDrawablePolicy(osgDB::DatabasePager::USE_VERTEX_BUFFER_OBJECTS); }

        struct DataMergeCallback : public osg::Referenced
        {
            enum FilterResult
            { MERGE_NOW = 0, MERGE_LATER, DISCARDED };

            virtual FilterResult filter(osg::PagedLOD* parent, const std::string& name, osg::Node*)
            { return MERGE_NOW; }

            virtual FilterResult filter(osg::Group* parent, const std::string& name, osg::Node*)
            { return MERGE_NOW; }

            virtual void merge(osg::Group* parent, std::vector<osg::ref_ptr<osg::Node>>& nodes)
            { for (size_t i = 0; i < nodes.size(); ++i) parent->addChild(nodes[i].get()); nodes.clear(); }
        };
        void setDataMergeCallback(DataMergeCallback* cb) { _mergeCallback = cb; }
        DataMergeCallback* getDataMergeCallback() const { return _mergeCallback.get(); }

        virtual void updateSceneGraph(const osg::FrameStamp& fs)
        {
            removeExpiredSubgraphs(fs);
            addLoadedDataToSceneGraph_Verse(fs);
        }

        void addLoadedDataToSceneGraph_Verse(const osg::FrameStamp& frameStamp);
        virtual void removeExpiredSubgraphs(const osg::FrameStamp& frameStamp);

        virtual void requestNodeFile(const std::string& fileName, osg::NodePath& nodePath,
                                     float priority, const osg::FrameStamp* framestamp,
                                     osg::ref_ptr<osg::Referenced>& databaseRequest,
                                     const osg::Referenced* options);

    protected:
        virtual ~DatabasePager() {}

        typedef std::map<osg::ref_ptr<osg::Group>, std::vector<osg::ref_ptr<osg::Node>>> LoadedNodeMap;
        LoadedNodeMap _loadedNodes;
        osg::ref_ptr<DataMergeCallback> _mergeCallback;
        bool _compressingImages;
    };

}

#endif

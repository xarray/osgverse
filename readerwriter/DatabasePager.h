#ifndef MANA_READERWRITER_DATABASEPAGER_HPP
#define MANA_READERWRITER_DATABASEPAGER_HPP

#include <osg/ProxyNode>
#include <osg/PagedLOD>
#include <osgDB/DatabasePager>
#include "Export.h"

namespace osgVerse
{

    class DatabasePager : public osgDB::DatabasePager
    {
    public:
        DatabasePager() : osgDB::DatabasePager()
        {
            setDrawablePolicy(osgDB::DatabasePager::USE_VERTEX_BUFFER_OBJECTS);
        }

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

        void addLoadedDataToSceneGraph_Verse(const osg::FrameStamp& frameStamp)
        {
            double timeStamp = frameStamp.getReferenceTime();
            unsigned int frameNumber = frameStamp.getFrameNumber();
            std::string maxFileName;

            // get the data from the _dataToMergeList, leaving it empty via a std::vector<>.swap.
            RequestQueue::RequestList localFileLoadedList;
            _dataToMergeList->swap(localFileLoadedList);

            // add the loaded data into the scene graph.
            for (RequestQueue::RequestList::iterator itr = localFileLoadedList.begin();
                 itr != localFileLoadedList.end(); ++itr)
            {
                DatabaseRequest* databaseRequest = itr->get();

                // No need to take _dr_mutex. The pager threads are done with
                // the request; the cull traversal -- which might redispatch
                // the request -- can't run at the sametime as this update traversal.
                osg::ref_ptr<osg::Group> group;
                if (!databaseRequest->_groupExpired && databaseRequest->_group.lock(group))
                {
                    if (osgDB::Registry::instance()->getSharedStateManager())
                        osgDB::Registry::instance()->getSharedStateManager()->share(databaseRequest->_loadedModel.get());

                    osg::PagedLOD* plod = dynamic_cast<osg::PagedLOD*>(group.get());
                    if (plod)
                    {
                        plod->setTimeStamp(plod->getNumChildren(), timeStamp);
                        plod->setFrameNumber(plod->getNumChildren(), frameNumber);
                        plod->getDatabaseRequest(plod->getNumChildren()) = 0;
                    }
                    else
                    {
                        osg::ProxyNode* proxyNode = dynamic_cast<osg::ProxyNode*>(group.get());
                        if (proxyNode) proxyNode->getDatabaseRequest(proxyNode->getNumChildren()) = 0;
                    }

                    DataMergeCallback::FilterResult filterResult = DataMergeCallback::MERGE_NOW;
                    if (_mergeCallback.valid())
                    {
                        if (plod) filterResult = _mergeCallback->filter(
                            plod, databaseRequest->_fileName, databaseRequest->_loadedModel.get());
                        else filterResult = _mergeCallback->filter(
                            group.get(), databaseRequest->_fileName, databaseRequest->_loadedModel.get());
                    }

                    if (filterResult == DataMergeCallback::MERGE_NOW)
                        group->addChild(databaseRequest->_loadedModel.get());
                    else if (filterResult == DataMergeCallback::MERGE_LATER)
                        _loadedNodes[group].push_back(databaseRequest->_loadedModel);

                    // Check if parent plod was already registered if not start visitor from parent
                    if (plod && !_activePagedLODList->containsPagedLOD(plod))
                        registerPagedLODs(plod, frameNumber);
                    else
                        registerPagedLODs(databaseRequest->_loadedModel.get(), frameNumber);

                    double timeToMerge = timeStamp - databaseRequest->_timestampFirstRequest;
                    if (timeToMerge < _minimumTimeToMergeTile) _minimumTimeToMergeTile = timeToMerge;
                    if (timeToMerge > _maximumTimeToMergeTile)
                    {
                        _maximumTimeToMergeTile = timeToMerge;
                        maxFileName = databaseRequest->_fileName;
                    }
                    _totalTimeToMergeTiles += timeToMerge;
                    ++_numTilesMerges;
                }
                else
                {
                    OSG_WARN << "DatabasePager::addLoadedDataToSceneGraph() node in parental chain deleted, "
                             << "discarding subgaph." << std::endl;
                }

                // reset the loadedModel pointer
                databaseRequest->_loadedModel = 0;
            }
            _maximumTimeToMergeTile = 0;

            //std::cout << "Merged " << localFileLoadedList.size() << " nodes" << std::endl;
            std::map<osg::ref_ptr<osg::Group>, std::vector<osg::ref_ptr<osg::Node>>>::iterator itr;
            for (itr = _loadedNodes.begin(); itr != _loadedNodes.end();)
            {
                if (_mergeCallback.valid()) _mergeCallback->merge(itr->first.get(), itr->second);
                if (!itr->second.empty()) itr++; else itr = _loadedNodes.erase(itr);
            }
        }

    protected:
        virtual ~DatabasePager() {}

        typedef std::map<osg::ref_ptr<osg::Group>, std::vector<osg::ref_ptr<osg::Node>>> LoadedNodeMap;
        LoadedNodeMap _loadedNodes;
        osg::ref_ptr<DataMergeCallback> _mergeCallback;
    };

}

#endif

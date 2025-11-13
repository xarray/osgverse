#include <osg/PagedLOD>
#include <modeling/Utilities.h>
#include "Utilities.h"
#include "DatabasePager.h"
using namespace osgVerse;

class CompressTextureVisitor : public osgVerse::NodeVisitorEx
{
public:
    CompressTextureVisitor() : NodeVisitorEx() {}
    std::map<osg::Image*, osg::observer_ptr<osg::Image>> _imageMap;
    std::set<osg::PagedLOD*> _plods; std::set<osg::Node*> _geodes;
    std::set<osg::Drawable*> _drawables; std::set<osg::Image*> _images;
    std::vector<float> _texDensities;

    virtual void apply(osg::PagedLOD& node)
    {
        _plods.insert(&node);
        osgVerse::NodeVisitorEx::apply(node);
    }

    virtual void apply(osg::Node* n, osg::Drawable* d, osg::Texture* tex, int u)
    {
        for (unsigned int i = 0; i < tex->getNumImages(); ++i)
        {
            osg::Image* img = tex->getImage(i);
            if (_imageMap.find(img) == _imageMap.end())
            {
                if (img && !img->isCompressed())
                {
                    osg::Image* img1 = osgVerse::compressImage(*img);
                    if (img1) { tex->setImage(i, img1); _imageMap[img] = img1; }
                }
            }
            else
                tex->setImage(i, _imageMap[img].get());

            osg::Vec2 areas = osgVerse::computeTotalAreas(d->asGeometry());
            _images.insert(img); _texDensities.push_back(areas[1] * img->s() * img->t() / areas[0]);
        }
        _drawables.insert(d); _geodes.insert(d->getParent(0));
    }
};

void DatabasePager::addLoadedDataToSceneGraph_Verse(const osg::FrameStamp& frameStamp)
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

            if (_compressingImages)
            {
                CompressTextureVisitor ctv;
                databaseRequest->_loadedModel->accept(ctv);

                osg::Node* loaded = databaseRequest->_loadedModel.get();
                std::cout << loaded->getName() << ": TILES = " << loaded->asGroup()->getNumChildren()
                          << ", PLODS = " << ctv._plods.size() << ", GEODES = " << ctv._geodes.size()
                          << ", GEOMS = " << ctv._drawables.size() << ", IMAGES = " << ctv._images.size() << "; ";
                for (size_t i = 0; i < ctv._texDensities.size(); ++i)
                    std::cout << ctv._texDensities[i] << " "; std::cout << "\n";
            }

            // Update plod / proxynode properties
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

            // Use callback to check data-merge time
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
        databaseRequest->_loadedModel = 0;  // reset the loadedModel pointer
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

void DatabasePager::removeExpiredSubgraphs(const osg::FrameStamp& frameStamp)
{
    // No need to remove anything on first frame.
    if (frameStamp.getFrameNumber() == 0) return;

    // numPagedLODs >= actual number of PagedLODs. There can be
    // invalid observer pointers in _activePagedLODList.
    unsigned int numPagedLODs = _activePagedLODList->size();
    if (numPagedLODs <= _targetMaximumNumberOfPageLOD)
    {
        // nothing to do
        return;
    }

    double expiryTime = frameStamp.getReferenceTime() - 0.1;
    unsigned int expiryFrame = frameStamp.getFrameNumber() - 1;
    ObjectList childrenRemoved;

    // First traverse inactive PagedLODs, as their children will certainly have expired.
    int numToPrune = numPagedLODs - _targetMaximumNumberOfPageLOD;
    if (numToPrune > 0)
        _activePagedLODList->removeExpiredChildren(
            numToPrune, expiryTime, expiryFrame, childrenRemoved, false);

    // Then traverse active nodes if we still need to prune.
    numToPrune = _activePagedLODList->size() - _targetMaximumNumberOfPageLOD;
    if (numToPrune > 0)
        _activePagedLODList->removeExpiredChildren(
            numToPrune, expiryTime, expiryFrame, childrenRemoved, true);

    if (!childrenRemoved.empty())
    {
        // pass the objects across to the database pager delete list
        if (_deleteRemovedSubgraphsInDatabaseThread)
        {
            // splice transfers the entire list in constant time.
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_fileRequestQueue->_requestMutex);
            _fileRequestQueue->_childrenToDeleteList.splice(
                _fileRequestQueue->_childrenToDeleteList.end(), childrenRemoved);
            _fileRequestQueue->updateBlock();
        }
        else
            childrenRemoved.clear();
    }
}

void DatabasePager::requestNodeFile(const std::string& fileName, osg::NodePath& nodePath,
                                    float priority, const osg::FrameStamp* framestamp,
                                    osg::ref_ptr<osg::Referenced>& databaseRequestRef,
                                    const osg::Referenced* options)
{
    osgDB::Options* loadOptions = dynamic_cast<osgDB::Options*>(const_cast<osg::Referenced*>(options));
    if (!loadOptions) loadOptions = osgDB::Registry::instance()->getOptions();

    if (!_acceptNewRequests) return;
    if (nodePath.empty())
    {
        OSG_NOTICE << "Warning: DatabasePager::requestNodeFile(..) passed empty NodePath,"
                   << " so nowhere to attach new subgraph to." << std::endl; return;
    }

    osg::Group* group = nodePath.back()->asGroup();
    if (!group)
    {
        OSG_NOTICE << "Warning: DatabasePager::requestNodeFile(..) passed NodePath without group as last node in path,"
                   << " so nowhere to attach new subgraph to." << std::endl; return;
    }

    osg::Node* terrain = 0;
    for (osg::NodePath::reverse_iterator itr = nodePath.rbegin(); itr != nodePath.rend(); ++itr)
    { if ((*itr)->asTerrain()) terrain = *itr; }

    double timestamp = framestamp ? framestamp->getReferenceTime() : 0.0;
    unsigned int frameNumber = framestamp ? framestamp->getFrameNumber() : static_cast<unsigned int>(_frameNumber);
    bool foundEntry = false;  // search to see if filename already exist in the file loaded list.
    if (databaseRequestRef.valid())
    {
        bool requeue = false;
        DatabaseRequest* databaseRequest = dynamic_cast<DatabaseRequest*>(databaseRequestRef.get());
        if (databaseRequest)
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> drLock(_dr_mutex);
            if (!(databaseRequest->valid()))
            {
                OSG_INFO << "DatabaseRequest has been previously invalidated whilst still "
                         << "attached to scene graph." << std::endl; databaseRequest = 0;
            }
            else
            {
                OSG_INFO << "DatabasePager::requestNodeFile(" << fileName << ") "
                         << "updating already assigned." << std::endl;
                databaseRequest->_valid = true;
                databaseRequest->_frameNumberLastRequest = frameNumber;
                databaseRequest->_timestampLastRequest = timestamp;
                databaseRequest->_priorityLastRequest = priority;
                ++(databaseRequest->_numOfRequests);

                foundEntry = true;
                if (databaseRequestRef->referenceCount() == 1)
                {
                    OSG_INFO << "DatabasePager::requestNodeFile(" << fileName << ") "
                             << "orphaned, resubmitting." << std::endl;
                    databaseRequest->_frameNumberLastRequest = frameNumber;
                    databaseRequest->_timestampLastRequest = timestamp;
                    databaseRequest->_priorityLastRequest = priority;
                    databaseRequest->_group = group;
                    databaseRequest->_terrain = terrain;
                    databaseRequest->_loadOptions = loadOptions;
                    databaseRequest->_objectCache = 0;
                    requeue = true;
                }
            }
        }

        if (requeue)
            _fileRequestQueue->add(databaseRequest);
    }  // if (databaseRequestRef.valid())

    if (!foundEntry)
    {
        OSG_INFO << "In DatabasePager::requestNodeFile(" << fileName << ")" << std::endl;
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_fileRequestQueue->_requestMutex);
        if (!databaseRequestRef.valid() || databaseRequestRef->referenceCount() == 1)
        {
            osg::ref_ptr<DatabaseRequest> databaseRequest = new DatabaseRequest;
            databaseRequestRef = databaseRequest.get();
            databaseRequest->_valid = true;
            databaseRequest->_fileName = fileName;
            databaseRequest->_frameNumberFirstRequest = frameNumber;
            databaseRequest->_timestampFirstRequest = timestamp;
            databaseRequest->_priorityFirstRequest = priority;
            databaseRequest->_frameNumberLastRequest = frameNumber;
            databaseRequest->_timestampLastRequest = timestamp;
            databaseRequest->_priorityLastRequest = priority;
            databaseRequest->_group = group;
            databaseRequest->_terrain = terrain;
            databaseRequest->_loadOptions = loadOptions;
            databaseRequest->_objectCache = 0;
            _fileRequestQueue->addNoLock(databaseRequest.get());
        }
    }

    if (!_startThreadCalled)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_run_mutex);
        if (!_startThreadCalled)
        {
            OSG_INFO << "DatabasePager::startThread()" << std::endl;
            if (_databaseThreads.empty())
            {
                setUpThreads(osg::DisplaySettings::instance()->getNumOfDatabaseThreadsHint(),
                             osg::DisplaySettings::instance()->getNumOfHttpDatabaseThreadsHint());
            }

            _startThreadCalled = true; _done = false;
            for (DatabaseThreadList::const_iterator dt_itr = _databaseThreads.begin();
                 dt_itr != _databaseThreads.end(); ++dt_itr) { (*dt_itr)->startThread(); }
        }
    }
}
